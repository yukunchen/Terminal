/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessHandle.h"

#include "..\host\globals.h"
#include "..\host\telemetry.hpp"

// Routine Description:
// - Constructs an instance of the ConsoleProcessHandle Class
// - NOTE: Can throw if allocation fails or if there is a console policy we do not understand.
// - NOTE: Not being able to open the process by ID isn't a failure. It will be logged and continued.
ConsoleProcessHandle::ConsoleProcessHandle(const DWORD dwProcessId,
                                           const DWORD dwThreadId,
                                           const ULONG ulProcessGroupId) :
    pWaitBlockQueue(std::make_unique<ConsoleWaitQueue>()),
    pInputHandle(nullptr),
    pOutputHandle(nullptr),
    fRootProcess(false),
    dwProcessId(dwProcessId),
    dwThreadId(dwThreadId),
    _ulTerminateCount(0),
    _ulProcessGroupId(ulProcessGroupId),
    _hProcess(LOG_IF_HANDLE_NULL(OpenProcess(MAXIMUM_ALLOWED,
                                             FALSE,
                                             dwProcessId))),
    _policy(ConsoleProcessPolicy::s_CreateInstance(_hProcess.get()))
{
    if (nullptr != _hProcess.get())
    {
        Telemetry::Instance().LogProcessConnected(_hProcess.get());
    }
}

// Routine Description:
// - Destroys an instance of the ConsoleProcessHandle class.
// - NOTE: Will close ConsoleHandleData handles if attached.
ConsoleProcessHandle::~ConsoleProcessHandle()
{
    // TODO: MSFT: 9358923 Convert to deleters and put CloseHandle in respective destructors? Then use smart pointers? http://osgvsowi/9358923
    if (pInputHandle != nullptr)
    {
        LOG_IF_FAILED(pInputHandle->CloseHandle());
    }

    if (pOutputHandle != nullptr)
    {
        LOG_IF_FAILED(pOutputHandle->CloseHandle());
    }
}

// Routine Description:
// - Retrieves the policies set on this particular process handle
// - This specifies restrictions that may apply to the calling console client application
const ConsoleProcessPolicy ConsoleProcessHandle::GetPolicy() const
{
    return _policy;
}
