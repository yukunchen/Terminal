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

NTSTATUS SrvGetConsoleProcessList(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleHistory(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvSetConsoleHistory(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleLangId(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS GetConsoleLangId(_In_ const UINT uiOutputCP, _Out_ LANGID * const pLangId);

PWSTR TranslateConsoleTitle(_In_ PCWSTR pwszConsoleTitle, _In_ const BOOL fUnexpand, _In_ const BOOL fSubstitute);

// TODO: temp for now. going to ApiSorter and IoDispatchers
PCONSOLE_API_MSG ConsoleHandleConnectionRequest(_Inout_ PCONSOLE_API_MSG ReceiveMsg);
PCONSOLE_API_MSG ConsoleDispatchRequest(_Inout_ PCONSOLE_API_MSG Message);
PCONSOLE_API_MSG ConsoleCreateObject(_In_ PCONSOLE_API_MSG Message, _Inout_ CONSOLE_INFORMATION *Console);
VOID ConsoleClientDisconnectRoutine(ConsoleProcessHandle* ProcessData);
