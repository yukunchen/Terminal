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
    std::unique_ptr<ConsoleWaitQueue> const pWaitBlockQueue;
    ConsoleHandleData* pInputHandle;
    ConsoleHandleData* pOutputHandle;

    bool fRootProcess;

    DWORD const dwProcessId;
    DWORD const dwThreadId;

private:
    ConsoleProcessHandle(_In_ DWORD const dwProcessId,
                         _In_ DWORD const dwThreadId,
                         _In_ ULONG const ulProcessGroupId);
    ~ConsoleProcessHandle();

    ULONG _ulTerminateCount;
    ULONG const _ulProcessGroupId;
    wil::unique_handle const _hProcess;

    friend class ConsoleProcessList; // ensure List manages lifetimes and not other classes.
};
