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
ConsoleProcessHandle::ConsoleProcessHandle(_In_ const CLIENT_ID* const pClientId,
                                                 _In_ ULONG const ulProcessGroupId) :
    ClientId(*pClientId),
    ProcessGroupId(ulProcessGroupId),
    pWaitBlockQueue(std::make_unique<ConsoleWaitQueue>()),
    ProcessHandle(LOG_IF_HANDLE_NULL(OpenProcess(MAXIMUM_ALLOWED,
                                                 FALSE,
                                                 (DWORD)ClientId.UniqueProcess)))
{
    if (nullptr != ProcessHandle.get())
    {
        Telemetry::Instance().LogProcessConnected(ProcessHandle.get());
    }
}
#pragma warning(pop)

ConsoleProcessHandle::~ConsoleProcessHandle()
{
    if (InputHandle != nullptr)
    {
        InputHandle->CloseHandle();
    }

    if (OutputHandle != nullptr)
    {
        OutputHandle->CloseHandle();
    }
}
