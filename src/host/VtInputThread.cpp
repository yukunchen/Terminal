/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtInputThread.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"
#include "input.h"
#include "..\terminal\parser\InputStateMachineEngine.hpp"
#include <string>
#include <sstream>

using namespace Microsoft::Console;

// This is copied from ConsolInformation.cpp
// I've never thought that was a particularily good place for it...
void _HandleTerminalKeyEventCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    // FIXME
    // The prototype fix moves the VT translation to WriteInputBuffer. 
    //   This currently causes WriteInputBuffer to get called twice for every 
    //   key - not ideal. There needs to be a WriteInputBuffer that sidesteps this problem.

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

    byte buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        THROW_LAST_ERROR_IF_FALSE(ReadFile(_hFile.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));

        _HandleRunInput((char*)buffer, dwRead);
        
        // For debugging, zero mem
        ZeroMemory(buffer, ARRAYSIZE(buffer)*sizeof(*buffer));

    }
}

// Routine Description:
// - Starts the VT input thread.
HANDLE VtInputThread::Start()
{
    // Shouldn't even need to worry about this. If the ctor throws, we shouldn't even call this.
    THROW_IF_HANDLE_INVALID(_hFile.get());

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

VtInputThread::VtInputThread(HANDLE hPipe)
{
    _hFile.reset(hPipe);
    THROW_IF_HANDLE_INVALID(_hFile.get());

    InputStateMachineEngine* pEngine = new InputStateMachineEngine(_HandleTerminalKeyEventCallback);
    THROW_IF_NULL_ALLOC(pEngine);

    _pInputStateMachine = new StateMachine(pEngine);
    THROW_IF_NULL_ALLOC(_pInputStateMachine);

}
