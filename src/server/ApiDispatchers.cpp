/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiDispatchers.h"

#include "..\host\directio.h"
#include "..\host\getset.h"
#include "..\host\stream.h"
#include "..\host\srvinit.h"

NTSTATUS ApiDispatchers::ServeGetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleCP(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleMode(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleMode(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleNumberOfInputEvents(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleInput(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeReadConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvReadConsole(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeWriteConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvWriteConsole(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleLangId(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleLangId(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeFillConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvFillConsoleOutput(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGenerateConsoleCtrlEvent(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleActiveScreenBuffer(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvFlushConsoleInputBuffer(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleCP(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleCursorInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleCursorInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleScreenBufferInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetScreenBufferInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleScreenBufferSize(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleCursorPosition(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetLargestConsoleWindowSize(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvScrollConsoleScreenBuffer(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleTextAttribute(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleWindowInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvReadConsoleOutputString(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeWriteConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvWriteConsoleInput(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvWriteConsoleOutput(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvWriteConsoleOutputString(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvReadConsoleOutput(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleTitle(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleTitle(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleMouseInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleFontSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleFontSize(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleCurrentFont(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleDisplayMode(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleDisplayMode(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeAddConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvAddConsoleAlias(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleAlias(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleAliasesLength(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleAliasExesLength(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliases(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleAliases(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleAliasExes(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvExpungeConsoleCommandHistory(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleNumberOfCommands(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleCommandHistoryLength(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleCommandHistory(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleWindow(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleWindow(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleSelectionInfo(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleProcessList(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleProcessList(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeGetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvGetConsoleHistory(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleHistory(m, pbReplyPending);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    return SrvSetConsoleCurrentFont(m, pbReplyPending);
}
