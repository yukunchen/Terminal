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

void _HandleTerminalKeyEventCallback(_In_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->pInputBuffer->Write(inEvents);
}

VtInputThread::~VtInputThread()
{
    if(_pInputStateMachine != nullptr)
    {
        delete _pInputStateMachine;
    }
}

HRESULT VtInputThread::_HandleRunInput(_In_reads_(cch) const char* const charBuffer, _In_ const int cch)
{
    try
    {
        std::string inputSequence = std::string(charBuffer, cch);

        size_t cch1 = inputSequence.length() + 1;
        wchar_t* wc = new wchar_t[cch1];
        THROW_IF_NULL_ALLOC(wc);

        size_t numConverted = 0;
        mbstowcs_s(&numConverted, wc, cch1, inputSequence.c_str(), cch1);

        _pInputStateMachine->ProcessString(wc, cch);
    }
    CATCH_RETURN();

    return S_OK;
}

DWORD VtInputThread::StaticVtInputThreadProc(_In_ LPVOID lpParameter)
{
    VtInputThread* const pInstance = reinterpret_cast<VtInputThread*>(lpParameter);
    return pInstance->_InputThread();
}

DWORD VtInputThread::_InputThread()
{
    char buffer[256];
    DWORD dwRead;
    while (true)
    {
        dwRead = 0;
        THROW_LAST_ERROR_IF_FALSE(ReadFile(_hFile.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr));
        THROW_IF_FAILED(_HandleRunInput(buffer, dwRead));
    }
}

// Routine Description:
// - Starts the VT input thread.
HRESULT VtInputThread::Start()
{
    RETURN_IF_HANDLE_INVALID(_hFile.get());

    HANDLE hThread = nullptr;
    DWORD dwThreadId = static_cast<DWORD>(-1);

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)VtInputThread::StaticVtInputThreadProc,
                           this,
                           0,
                           &dwThreadId);

    RETURN_IF_HANDLE_INVALID(hThread);
    _hThread.reset(hThread);
    _dwThreadId = dwThreadId;

    return S_OK;
}

VtInputThread::VtInputThread(_In_ HANDLE hPipe)
{
    _hFile.reset(hPipe);
    THROW_IF_HANDLE_INVALID(_hFile.get());

    InputStateMachineEngine* pEngine = new InputStateMachineEngine(_HandleTerminalKeyEventCallback);
    THROW_IF_NULL_ALLOC(pEngine);

    _pInputStateMachine = new StateMachine(pEngine);
    THROW_IF_NULL_ALLOC(_pInputStateMachine);

}
