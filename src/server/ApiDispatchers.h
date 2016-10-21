/*++
Copyright (c) Microsoft Corporation

Module Name:
- ApiDispatchers.h

Abstract:
- This file decodes the client's API request message and dispatches it to the appropriate defined routine in the server.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in srvinit.cpp 
--*/

#pragma once

#include "IApiRoutines.h"

#include "ApiMessage.h"

namespace ApiDispatchers
{
#pragma region L1
    NTSTATUS ServeGetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeReadConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeWriteConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleLangId(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L2
    NTSTATUS ServeFillConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeReadConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeWriteConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeWriteConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeReadConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L3
    NTSTATUS ServeGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleFontSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeAddConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleAliases(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);

    // CMDEXT functions
    NTSTATUS ServeExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    // end CMDEXT functions
    
    NTSTATUS ServeGetConsoleWindow(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleProcessList(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeGetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    NTSTATUS ServeSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion
};

