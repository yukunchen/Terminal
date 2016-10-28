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
    HRESULT ServerDeprecatedApi(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);

#pragma region L1
    HRESULT ServerGetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerReadConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerWriteConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleLangId(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L2
    HRESULT ServerFillConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerReadConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerWriteConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerWriteConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerReadConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L3
    HRESULT ServerGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleFontSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerAddConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleAliases(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);

    // CMDEXT functions
    HRESULT ServerExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    // end CMDEXT functions
    
    HRESULT ServerGetConsoleWindow(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleProcessList(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerGetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServerSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion
};

