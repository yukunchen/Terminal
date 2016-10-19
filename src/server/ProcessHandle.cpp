/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessHandle.h"

#include "..\host\globals.h"
#include "..\host\telemetry.hpp"

// pointer truncation due to using the HANDLE type to store a DWORD process ID.
// We're using the HANDLE type in the public ClientId field to store the process ID when we should
// be using a more appropriate type. This should be collected and replaced with the server refactor.
// TODO - MSFT:9115192
#pragma warning(push)
#pragma warning(disable:4311 4302) 
ConsoleProcessHandle::ConsoleProcessHandle(_In_ DWORD const dwProcessId,
                                           _In_ DWORD const dwThreadId,
                                           _In_ ULONG const ulProcessGroupId) :
    dwProcessId(dwProcessId),
    dwThreadId(dwThreadId),
    _ulProcessGroupId(ulProcessGroupId),
    pWaitBlockQueue(std::make_unique<ConsoleWaitQueue>()),
    _hProcess(LOG_IF_HANDLE_NULL(OpenProcess(MAXIMUM_ALLOWED,
                                                 FALSE,
                                                 dwProcessId)))
{
    if (nullptr != _hProcess.get())
    {
        Telemetry::Instance().LogProcessConnected(_hProcess.get());
    }
}
#pragma warning(pop)

ConsoleProcessHandle::~ConsoleProcessHandle()
{
    if (pInputHandle != nullptr)
    {
        pInputHandle->CloseHandle();
    }

    if (pOutputHandle != nullptr)
    {
        pOutputHandle->CloseHandle();
    }
}
