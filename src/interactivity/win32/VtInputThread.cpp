/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtInputThread.hpp"

#include "..\inc\ServiceLocator.hpp"
#include "..\..\host\input.h"
#include "..\..\terminal\parser\InputStateMachineEngine.hpp"
#include <string>
#include <sstream>

using namespace Microsoft::Console::Interactivity::Win32;

// This is copied from ConsolInformation.cpp
// I've never thought that was a particularily good place for it...
void _HandleTerminalKeyEventCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    // auto _cIn = cInput;
    // // ServiceLocator::LocateGlobals()->getConsoleInformation()->
    // //     pInputBuffer->PrependInputBuffer(rgInput, &_cIn);

    // // FIXME
    // // The prototype fix moves the VT translation to WriteInputBuffer. 
    // //   This currently causes WriteInputBuffer to get called twice for every 
    // //   key - not ideal. There needs to be a WriteInputBuffer that sidesteps this problem.
    // ServiceLocator::LocateGlobals()->getConsoleInformation()->
    //     pInputBuffer->WriteInputBuffer(rgInput, cInput);

    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> inEvents = IInputEvent::Create(rgInput, cInput);
        gci->pInputBuffer->WriteInputBuffer(inEvents);
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}

VtInputThread::VtInputThread()
{

}

VtInputThread::~VtInputThread()
{
    if(_pInputStateMachine != nullptr)
    {
        delete _pInputStateMachine;
    }
}

void VtInputThread::_HandleRunInput(char* charBuffer, int cch)
{
    std::string inputSequence = std::string(charBuffer, cch);
    
    auto cch1 = inputSequence.length() + 1;
    wchar_t* wc = new wchar_t[cch1];
    size_t numConverted = 0;
    mbstowcs_s(&numConverted, wc, cch1, inputSequence.c_str(), cch1);

    _pInputStateMachine->ProcessString(wc, cch);
}

DWORD VtInputThread::StaticVtInputThreadProc(LPVOID lpParameter)
{
    VtInputThread* const pInstance = (VtInputThread*)lpParameter;
    return pInstance->_InputThread();
}

DWORD VtInputThread::_InputThread()
{
    auto g = ServiceLocator::LocateGlobals();
    HANDLE hPipe = g->hVtInPipe;
    // HANDLE hPipe = g->hVtPipe;
    if (hPipe == INVALID_HANDLE_VALUE || hPipe == nullptr)
    {
        return 0;
    }

    // _hFile.reset(CreateFileW(L"\\\\.\\pipe\\convtinpipe", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    _hFile = hPipe;

    InputStateMachineEngine* pEngine = new InputStateMachineEngine(_HandleTerminalKeyEventCallback);
    RETURN_IF_NULL_ALLOC(pEngine);

    _pInputStateMachine = new StateMachine(pEngine);

    byte buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        // THROW_LAST_ERROR_IF_FALSE(ReadFile(_hFile.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_LAST_ERROR_IF_FALSE(
            ReadFile(_hFile, buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        // DebugBreak();
        _HandleRunInput((char*)buffer, dwRead);
        
        // For debugging, zero mem
        ZeroMemory(buffer, ARRAYSIZE(buffer)*sizeof(*buffer));

    }
}

// Routine Description:
// - Starts the Win32-specific console input thread.
HANDLE VtInputThread::Start()
{
    HANDLE hThread = nullptr;
    DWORD dwThreadId = (DWORD) -1;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)VtInputThread::StaticVtInputThreadProc,
                           this,
                           0,
                           &dwThreadId);

    if (hThread)
    {
        _hThread = hThread;
        _dwThreadId = dwThreadId;
    }

    return hThread;
}