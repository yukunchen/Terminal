/*++
Copyright (c) Microsoft Corporation

Module Name:
- srvinit.h

Abstract:
- This is the main initialization file for the console Server.

Author:
- Therese Stowell (ThereseS) 11-Nov-1990

Revision History:
--*/

#pragma once

NTSTATUS GetConsoleLangId(_In_ const UINT uiOutputCP, _Out_ LANGID * const pLangId);

PWSTR TranslateConsoleTitle(_In_ PCWSTR pwszConsoleTitle, _In_ const BOOL fUnexpand, _In_ const BOOL fSubstitute);

// TODO: reconcile into server

#include "consrv.h"
NTSTATUS ConsoleInitializeConnectInfo(_In_ PCONSOLE_API_MSG Message, _Out_ PCONSOLE_API_CONNECTINFO Cac);
NTSTATUS ConsoleAllocateConsole(PCONSOLE_API_CONNECTINFO p);
NTSTATUS RemoveConsole(_In_ ConsoleProcessHandle* ProcessData);
