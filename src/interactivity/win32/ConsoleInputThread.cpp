/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConsoleInputThread.hpp"
#include "VtInputThread.hpp"

#include "WindowIo.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

// Routine Description:
// - Starts the Win32-specific console input thread.
HANDLE ConsoleInputThread::Start()
{
    
    VtInputThread vtInput;
    vtInput.Start();

    HANDLE hThread = nullptr;
    DWORD dwThreadId = (DWORD) -1;

    hThread = CreateThread(nullptr,
                           0,
                           (LPTHREAD_START_ROUTINE)ConsoleInputThreadProcWin32,
                           nullptr,
                           0,
                           &dwThreadId);

    if (hThread)
    {
        _hThread = hThread;
        _dwThreadId = dwThreadId;
    }

    return hThread;
}