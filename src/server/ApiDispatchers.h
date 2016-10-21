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
    HRESULT ServeDeprecatedApi(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);

#pragma region L1
    HRESULT ServeGetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeReadConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeWriteConsole(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleLangId(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L2
    HRESULT ServeFillConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleCP(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeReadConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeWriteConsoleInput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeWriteConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeReadConsoleOutput(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleTitle(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion

#pragma region L3
    HRESULT ServeGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleFontSize(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeAddConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleAlias(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleAliases(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);

    // CMDEXT functions
    HRESULT ServeExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    // end CMDEXT functions
    
    HRESULT ServeGetConsoleWindow(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleProcessList(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeGetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleHistory(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
    HRESULT ServeSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG* const m, _Inout_ BOOL* const pbReplyPending);
#pragma endregion
};

