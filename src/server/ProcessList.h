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
                             _In_opt_ ConsoleProcessHandle* const pParentProcessData,
                             _Outptr_opt_ ConsoleProcessHandle** const ppProcessData);

    void FreeProcessData(_In_ ConsoleProcessHandle* const ProcessData);


    ConsoleProcessHandle* FindProcessInList(_In_opt_ const HANDLE hProcess) const;
    ConsoleProcessHandle* FindProcessByGroupId(_In_ ULONG ProcessGroupId) const;

    HRESULT GetTerminationRecordsByGroupId(_In_ DWORD const LimitingProcessId,
                                           _In_ bool const fCtrlClose,
                                           _Outptr_result_buffer_all_(*pcRecords) CONSOLE_PROCESS_TERMINATION_RECORD** prgRecords,
                                           _Out_ size_t* const pcRecords) const;

    ConsoleProcessHandle* GetRootProcess() const;

    HRESULT GetProcessList(_Inout_updates_(*pcProcessList) DWORD* const pProcessList,
                           _Inout_ DWORD* const pcProcessList) const;

    void ModifyConsoleProcessFocus(_In_ const bool fForeground);

    bool IsEmpty() const;

private:
    std::list<ConsoleProcessHandle*> _processes;
    
    void _SetProcessFocus(_In_ HANDLE const hProcess, _In_ bool const fForeground) const;
    void _SetProcessForegroundRights(_In_ HANDLE const hProcess, _In_ bool const fForeground) const;

};
