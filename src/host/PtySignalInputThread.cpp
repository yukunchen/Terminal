/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "PtySignalInputThread.hpp"

#include "output.h"
#include "handle.h"
#include "..\interactivity\inc\ServiceLocator.hpp"
#include "..\terminal\adapter\DispatchCommon.hpp"

#define PTY_SIGNAL_RESIZE_WINDOW 8u
#define PTY_SIGNAL_FLUSH_BUFFER 7u

struct PTY_SIGNAL_RESIZE
{
    unsigned short sx;
    unsigned short sy;
};
// For reference, the Flush message is 0 bytes long / has no payload
// struct PTY_SIGNAL_FLUSH {};

using namespace Microsoft::Console;

// Constructor Description:
// - Creates the PTY Signal Input Thread.
// Arguments:
// - hPipe - a handle to the file representing the read end of the VT pipe.
PtySignalInputThread::PtySignalInputThread(_In_ wil::unique_hfile hPipe) :
    _hFile(std::move(hPipe)),
    _pConApi(new ConhostInternalGetSet(Microsoft::Console::Interactivity::ServiceLocator::LocateGlobals()->getConsoleInformation()))
{
    THROW_IF_HANDLE_INVALID(_hFile.get());
    THROW_IF_NULL_ALLOC(_pConApi.get());
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
// - S_OK if the thread runs to completion.
// - Otherwise it may cause an application termination another route and never return.
HRESULT PtySignalInputThread::_InputThread()
{
    // DebugBreak();
    unsigned short signalId;
    while (_GetData(&signalId, sizeof(signalId)))
    {
        // DebugBreak();
        switch (signalId)
        {
        case PTY_SIGNAL_RESIZE_WINDOW:
        {
            PTY_SIGNAL_RESIZE resizeMsg = { 0 };
            _GetData(&resizeMsg, sizeof(resizeMsg));

            LockConsole();
            auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

            if (DispatchCommon::s_ResizeWindow(_pConApi.get(), resizeMsg.sx, resizeMsg.sy))
            {
                DispatchCommon::s_SuppressResizeRepaint(_pConApi.get());
            }

            break;
        }
        case PTY_SIGNAL_FLUSH_BUFFER:
        {
            // DebugBreak();
            LockConsole();
            auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

            if (ServiceLocator::LocateGlobals()->pRender)
            {
                ServiceLocator::LocateGlobals()->pRender->TriggerTeardown();
            }
            unsigned short buffer = 7u;
            DWORD cbBuffer = sizeof(buffer);
            WriteFile(_hFile.get(), &buffer, cbBuffer, nullptr, nullptr);
            break;
        }
        default:
        {
            THROW_HR(E_UNEXPECTED);
        }
        }
    }
    return S_OK;
}

// Method Description:
// - Retrieves bytes from the file stream and exits or throws errors should the pipe state
//   be compromised.
// Arguments:
// - pBuffer - Buffer to fill with data.
// - cbBuffer - Count of bytes in the given buffer.
// Return Value:
// - True if data was retrieved successfully. False otherwise.
bool PtySignalInputThread::_GetData(_Out_writes_bytes_(cbBuffer) void* const pBuffer,
                                    _In_ const DWORD cbBuffer)
{
    DWORD dwRead = 0;
    // If we failed to read because the terminal broke our pipe (usually due
    //      to dying itself), close gracefully with ERROR_BROKEN_PIPE.
    // Otherwise throw an exception. ERROR_BROKEN_PIPE is the only case that
    //       we want to gracefully close in.
    if (FALSE == ReadFile(_hFile.get(), pBuffer, cbBuffer, &dwRead, nullptr))
    {
            // DebugBreak();
        DWORD lastError = GetLastError();
        if (lastError == ERROR_BROKEN_PIPE)
        {
            // Trigger process shutdown.
            CloseConsoleProcessState();
            return false;
        }
        else
        {
            THROW_WIN32(lastError);
        }
    }
    else if (dwRead != cbBuffer)
    {
        // DebugBreak();
        // Trigger process shutdown.
        CloseConsoleProcessState();
        return false;
    }
    // DebugBreak();
    return true;
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

// HRESULT PtySignalInputThread::TriggerTeardown()
// {
//     unsigned short buffer = 7u;
//     DWORD cbBuffer = sizeof(buffer);
//     WriteFile(_hFile.get(), &buffer, cbBuffer, nullptr, nullptr);
//     return S_OK;
// }
