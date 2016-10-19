/*++
Copyright (c) Microsoft Corporation

Module Name:
- ProcessHandle.h

Abstract:
- This file defines the handles that were given to a particular client process ID when it connected.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#include "ObjectHandle.h"
#include "WaitQueue.h"

#include <memory>
#include <wil\resource.h>

class ConsoleProcessHandle
{
public:
    wil::unique_handle const ProcessHandle;
    ULONG TerminateCount;
    ULONG const ProcessGroupId;
    CLIENT_ID const ClientId;
    std::unique_ptr<ConsoleWaitQueue> const pWaitBlockQueue;
    ConsoleHandleData* InputHandle;
    ConsoleHandleData* OutputHandle;

    bool RootProcess;

private:
    ConsoleProcessHandle(_In_ const CLIENT_ID* const pClientId,
                         _In_ ULONG const ulProcessGroupId);
    ~ConsoleProcessHandle();

    friend class ConsoleProcessList; // ensure List manages lifetimes and not other classes.
};
