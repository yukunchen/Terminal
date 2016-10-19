/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ProcessList.h"

#include "..\host\globals.h"
#include "..\host\telemetry.hpp"

PCONSOLE_PROCESS_HANDLE ConsoleProcessList::s_FindProcessByGroupId(_In_ ULONG ProcessGroupId)
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ProcessHandleList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCONSOLE_PROCESS_HANDLE const ProcessHandleRecord = CONTAINING_RECORD(ListNext, CONSOLE_PROCESS_HANDLE, ListLink);
        ListNext = ListNext->Flink;
        if (ProcessHandleRecord->ProcessGroupId == ProcessGroupId)
        {
            return ProcessHandleRecord;
        }
    }

    return nullptr;
}

PCONSOLE_PROCESS_HANDLE ConsoleProcessList::s_AllocProcessData(_In_ CLIENT_ID const * const ClientId,
                                         _In_ ULONG const ulProcessGroupId,
                                         _In_opt_ PCONSOLE_PROCESS_HANDLE pParentProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    PCONSOLE_PROCESS_HANDLE ProcessData = s_FindProcessInList(ClientId->UniqueProcess);
    if (ProcessData != nullptr)
    {
        // In the GenerateConsoleCtrlEvent it's OK for this process to already have a ProcessData object. However, the other case is someone
        // connecting to our LPC port and they should only do that once, so we fail subsequent connection attempts.
        if (pParentProcessData == nullptr)
        {
            return nullptr;
        }
        else
        {
            return ProcessData;
        }
    }

    ProcessData = new CONSOLE_PROCESS_HANDLE();
    if (ProcessData != nullptr)
    {
        ProcessData->pWaitBlockQueue = new ConsoleWaitQueue();

        if (ProcessData->pWaitBlockQueue != nullptr)
        {
            ProcessData->ClientId = *ClientId;
            ProcessData->ProcessGroupId = ulProcessGroupId;

#pragma warning(push)
            // pointer truncation due to using the HANDLE type to store a DWORD process ID.
            // We're using the HANDLE type in the public ClientId field to store the process ID when we should
            // be using a more appropriate type. This should be collected and replaced with the server refactor.
            // TODO - MSFT:9115192
#pragma warning(disable:4311 4302) 
            ProcessData->ProcessHandle = OpenProcess(MAXIMUM_ALLOWED,
                                                     FALSE,
                                                     (DWORD)ProcessData->ClientId.UniqueProcess);
#pragma warning(pop)

            // Link this ProcessData ptr into the global list.
            InsertHeadList(&g_ciConsoleInformation.ProcessHandleList, &ProcessData->ListLink);

            if (ProcessData->ProcessHandle != nullptr)
            {
                Telemetry::Instance().LogProcessConnected(ProcessData->ProcessHandle);
            }
        }
        else
        {
            delete ProcessData;
        }
    }

    return ProcessData;
}

// Routine Description:
// - This routine frees any per-process data allocated by the console.
// Arguments:
// - ProcessData - Pointer to the per-process data structure.
// Return Value:
void ConsoleProcessList::s_FreeProcessData(_In_ PCONSOLE_PROCESS_HANDLE pProcessData)
{
    assert(g_ciConsoleInformation.IsConsoleLocked());

    delete pProcessData->pWaitBlockQueue;

    if (pProcessData->InputHandle != nullptr)
    {
        pProcessData->InputHandle->CloseHandle();
    }

    if (pProcessData->OutputHandle != nullptr)
    {
        pProcessData->OutputHandle->CloseHandle();
    }

    if (pProcessData->ProcessHandle != nullptr)
    {
        CloseHandle(pProcessData->ProcessHandle);
    }

    RemoveEntryList(&pProcessData->ListLink);
    delete pProcessData;
}

// Calling FindProcessInList(nullptr) means you want the root process.
PCONSOLE_PROCESS_HANDLE ConsoleProcessList::s_FindProcessInList(_In_opt_ HANDLE hProcess)
{
    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ProcessHandleList;
    PLIST_ENTRY ListNext = ListHead->Flink;

    while (ListNext != ListHead)
    {
        PCONSOLE_PROCESS_HANDLE const ProcessHandleRecord = CONTAINING_RECORD(ListNext, CONSOLE_PROCESS_HANDLE, ListLink);
        if (0 != hProcess)
        {
            if (ProcessHandleRecord->ClientId.UniqueProcess == hProcess)
            {
                return ProcessHandleRecord;
            }
        }
        else
        {
            if (ProcessHandleRecord->RootProcess)
            {
                return ProcessHandleRecord;
            }
        }

        ListNext = ListNext->Flink;
    }

    return nullptr;
}
