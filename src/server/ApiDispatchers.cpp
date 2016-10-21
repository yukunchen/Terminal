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

HRESULT ApiDispatchers::ServeDeprecatedApi(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    // assert if we hit a deprecated API.
    assert(TRUE);

    return E_NOTIMPL;
}

HRESULT ApiDispatchers::ServeGetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    CONSOLE_GETCP_MSG* const a = &m->u.consoleMsgL1.GetConsoleCP;

    if (a->Output)
    {
        return m->_pApiRoutines->GetConsoleOutputCodePageImpl(&a->CodePage);
    }
    else
    {
        return m->_pApiRoutines->GetConsoleInputCodePageImpl(&a->CodePage);
    }
}

HRESULT ApiDispatchers::ServeGetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleMode(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleMode(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleNumberOfInputEvents(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleInput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeReadConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvReadConsole(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeWriteConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsole(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleLangId(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleLangId(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeFillConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvFillConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGenerateConsoleCtrlEvent(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGenerateConsoleCtrlEvent(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleActiveScreenBuffer(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvFlushConsoleInputBuffer(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleCP(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleCursorInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleCursorInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleScreenBufferInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetScreenBufferInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleScreenBufferSize(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleCursorPosition(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetLargestConsoleWindowSize(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvScrollConsoleScreenBuffer(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleTextAttribute(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleWindowInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeReadConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvReadConsoleOutputString(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeWriteConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleInput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeWriteConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleOutputString(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeReadConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvReadConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleTitle(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleTitle(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleMouseInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleFontSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleFontSize(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleCurrentFont(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleDisplayMode(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleDisplayMode(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeAddConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvAddConsoleAlias(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleAlias(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleAliasesLength(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleAliasExesLength(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleAliases(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleAliases(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleAliasExes(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvExpungeConsoleCommandHistory(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleNumberOfCommands(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleCommandHistoryLength(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleCommandHistory(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleWindow(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleWindow(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleSelectionInfo(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleProcessList(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleProcessList(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeGetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleHistory(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleHistory(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServeSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvSetConsoleCurrentFont(m, pbReplyPending));
}
