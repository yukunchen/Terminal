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

typedef struct _CONSOLE_PROCESS_HANDLE
{
    LIST_ENTRY ListLink;
    HANDLE ProcessHandle;
    ULONG TerminateCount;
    ULONG ProcessGroupId;
    CLIENT_ID ClientId;
    BOOL RootProcess;
    ConsoleWaitQueue* pWaitBlockQueue;
    ConsoleHandleData* InputHandle;
    ConsoleHandleData* OutputHandle;
} CONSOLE_PROCESS_HANDLE, *PCONSOLE_PROCESS_HANDLE;
