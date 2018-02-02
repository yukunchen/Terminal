/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "VtInputThread.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"
#include "input.h"
#include "../terminal/parser/InputStateMachineEngine.hpp"
#include "outputStream.hpp" // For ConhostInternalGetSet
#include "../terminal/adapter/InteractDispatch.hpp"
#include "../types/inc/convert.hpp"
#include "server.h"
#include "output.h"

using namespace Microsoft::Console;

void _HandleTerminalKeyEventCallback(_In_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->pInputBuffer->Write(inEvents);
}

// Constructor Description:
// - Creates the VT Input Thread.
// Arguments:
// - hPipe - a handle to the file representing the read end of the VT pipe.
VtInputThread::VtInputThread(_In_ wil::unique_hfile hPipe)
    : _hFile(std::move(hPipe)),
    _utf8Parser(CP_UTF8)
{
    THROW_IF_HANDLE_INVALID(_hFile.get());
}

// Method Description:
// - Processes a buffer of input characters. The characters should be utf-8
//      encoded, and will get converted to wchar_t's to be processed by the
//      input state machine.
// Arguments:
// - charBuffer - the UTF-8 characters recieved.
// - cch - number of UTF-8 characters in charBuffer
// Return Value:
// - S_OK on success, otherwise an appropriate failure.
HRESULT VtInputThread::_HandleRunInput(_In_reads_(cch) const byte* const charBuffer, _In_ const int cch)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->LockConsole();
    auto Unlock = wil::ScopeExit([&] { gci->UnlockConsole(); });

    try
    {
        std::unique_ptr<wchar_t[]> pwsSequence;
        unsigned int cchConsumed;
        unsigned int cchSequence;

         RETURN_IF_FAILED(_utf8Parser.Parse(charBuffer, cch, cchConsumed, pwsSequence, cchSequence));
        _pInputStateMachine->ProcessString(pwsSequence.get(), cchSequence);
    }
    CATCH_RETURN();

    return S_OK;
}

// Function Description:
// - Static function used for initializing an instance's ThreadProc.
// Arguments:
// - lpParameter - A pointer to the VtInputThread instance that should be called.
// Return Value:
// - The return value of the underlying instance's _InputThread
DWORD VtInputThread::StaticVtInputThreadProc(_In_ LPVOID lpParameter)
{
    VtInputThread* const pInstance = reinterpret_cast<VtInputThread*>(lpParameter);
    return pInstance->_InputThread();
}

// Method Description:
// - The ThreadProc for the VT Input Thread. Reads input from the pipe, and
//      passes it to _HandleRunInput to be processed by the
//      InputStateMachineEngine.
// Return Value:
// - <none>
DWORD VtInputThread::_InputThread()
{
    byte buffer[256];
    DWORD dwRead;

    while (true)
    {
        dwRead = 0;
        bool fSuccess = !!ReadFile(_hFile.get(), buffer, ARRAYSIZE(buffer), &dwRead, nullptr);

        // If we failed to read because the terminal broke our pipe (usually due
        //      to dying itself), close gracefully with ERROR_BROKEN_PIPE.
        // Otherwise throw an exception. ERROR_BROKEN_PIPE is the only case that
        //       we want to gracefully close in.
        if (!fSuccess)
        {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_BROKEN_PIPE)
            {
                // This won't return. We'll be terminated.
                CloseConsoleProcessState();
            }
            else
            {
                THROW_WIN32(lastError);
            }
        }

        THROW_IF_FAILED(_HandleRunInput(buffer, dwRead));
    }
}

// Method Description:
// - Starts the VT input thread.
HRESULT VtInputThread::Start()
{
    RETURN_IF_HANDLE_INVALID(_hFile.get());

    //Initialize the state machine here, because the gci->pInputBuffer
    // hasn't been initialized when we initialize the VtInputThread object.
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();

    std::unique_ptr<ConhostInternalGetSet> pGetSet =
        std::make_unique<ConhostInternalGetSet>(gci);
    THROW_IF_NULL_ALLOC(pGetSet);

    std::unique_ptr<InteractDispatch> pDispatch = std::make_unique<InteractDispatch>(std::move(pGetSet));
    THROW_IF_NULL_ALLOC(pDispatch);

    std::unique_ptr<InputStateMachineEngine> pEngine =
        std::make_unique<InputStateMachineEngine>(std::move(pDispatch));
    THROW_IF_NULL_ALLOC(pEngine);

    _pInputStateMachine = std::make_unique<StateMachine>(std::move(pEngine));
    THROW_IF_NULL_ALLOC(_pInputStateMachine);

    HANDLE hThread = nullptr;
    // 0 is the right value, https://blogs.msdn.microsoft.com/oldnewthing/20040223-00/?p=40503
    DWORD dwThreadId = 0;

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
