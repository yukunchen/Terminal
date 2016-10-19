/*++
Copyright (c) Microsoft Corporation

Module Name:
- ProcessLIst.h

Abstract:
- This file defines a list of process handles maintained by an instance of a console server

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#include "ProcessHandle.h"

// this structure is used to store relevant information from the console for ctrl processing so we can do it without
// holding the console lock.
typedef struct _CONSOLE_PROCESS_TERMINATION_RECORD
{
    HANDLE ProcessHandle;
    HANDLE ProcessID;
    ULONG TerminateCount;
} CONSOLE_PROCESS_TERMINATION_RECORD, *PCONSOLE_PROCESS_TERMINATION_RECORD;

class ConsoleProcessList
{
public:
    
    HRESULT AllocProcessData(_In_ CLIENT_ID const * const ClientId,
                             _In_ ULONG const ulProcessGroupId,
                             _In_opt_ CONSOLE_PROCESS_HANDLE* const pParentProcessData,
                             _Outptr_opt_ CONSOLE_PROCESS_HANDLE** const ppProcessData);

    void FreeProcessData(_In_ CONSOLE_PROCESS_HANDLE* const ProcessData);


    CONSOLE_PROCESS_HANDLE* FindProcessInList(_In_opt_ const HANDLE hProcess) const;
    CONSOLE_PROCESS_HANDLE* FindProcessByGroupId(_In_ ULONG ProcessGroupId) const;

    HRESULT GetTerminationRecordsByGroupId(_In_ DWORD const LimitingProcessId,
                                           _In_ bool const fCtrlClose,
                                           _Outptr_result_buffer_all_(*pcRecords) CONSOLE_PROCESS_TERMINATION_RECORD** prgRecords,
                                           _Out_ size_t* const pcRecords) const;

    CONSOLE_PROCESS_HANDLE* GetRootProcess() const;

    HRESULT GetProcessList(_Inout_updates_(*pcProcessList) DWORD* const pProcessList,
                           _Inout_ DWORD* const pcProcessList) const;

    void ModifyConsoleProcessFocus(_In_ const BOOL fForeground);

    bool IsEmpty() const;

private:
    std::list<CONSOLE_PROCESS_HANDLE*> _processes;
};
