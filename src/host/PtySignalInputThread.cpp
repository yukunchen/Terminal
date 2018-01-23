/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "PtySignalInputThread.hpp"

#include "output.h"

using namespace Microsoft::Console;

// Constructor Description:
// - Creates the PTY Signal Input Thread.
// Arguments:
// - hPipe - a handle to the file representing the read end of the VT pipe. 
PtySignalInputThread::PtySignalInputThread(_In_ wil::unique_hfile hPipe)
    : _hFile(std::move(hPipe))
{
    THROW_IF_HANDLE_INVALID(_hFile.get());
}

// Function Description:
// - Static function used for initializing an instance's ThreadProc.
// Arguments:
// - lpParameter - A pointer to the PtySignalInputTHread instance that should be called.
// Return Value:
// - The return value of the underlying instance's _InputThread
DWORD PtySignalInputThread::StaticThreadProc(_In_ LPVOID lpParameter)
{
    PtySignalInputThread* const pInstance = reinterpret_cast<PtySignalInputThread*>(lpParameter);
    return pInstance->_InputThread();
}

// Method Description:
// - The ThreadProc for the PTY Signal Input Thread. 
// Return Value:
// - <none>
DWORD PtySignalInputThread::_InputThread()
{
    char buffer[256];
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
        
        // TODO MSFT:15536287 migrie : Insert communication and dispatch here.
    }
}

// Method Description:
// - Starts the PTY Signal input thread.
HRESULT PtySignalInputThread::Start()
{
    RETURN_IF_HANDLE_INVALID(_hFile.get());
    
    HANDLE hThread = nullptr;
    // 0 is the right value, https://blogs.msdn.microsoft.com/oldnewthing/20040223-00/?p=40503
    DWORD dwThreadId = 0;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)PtySignalInputThread::StaticThreadProc,
                           this,
                           0,
                           &dwThreadId);

    RETURN_IF_HANDLE_INVALID(hThread);
    _hThread.reset(hThread);
    _dwThreadId = dwThreadId;

    return S_OK;
}
