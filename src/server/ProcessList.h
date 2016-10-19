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

class ConsoleProcessList
{
public:
    static PCONSOLE_PROCESS_HANDLE s_FindProcessByGroupId(_In_ ULONG ProcessGroupId);
    static PCONSOLE_PROCESS_HANDLE s_AllocProcessData(_In_ CLIENT_ID const * const ClientId,
                                                      _In_ ULONG const ulProcessGroupId,
                                                      _In_opt_ PCONSOLE_PROCESS_HANDLE pParentProcessData);
    static void s_FreeProcessData(_In_ PCONSOLE_PROCESS_HANDLE ProcessData);
    static PCONSOLE_PROCESS_HANDLE s_FindProcessInList(_In_opt_ const HANDLE hProcess);
};
