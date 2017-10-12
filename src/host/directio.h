/*++
Copyright (c) Microsoft Corporation

Module Name:
- directio.h

Abstract:
- This file implements the NT console direct I/O API (read/write STDIO streams)

Author:
- KazuM Apr.19.1996

Revision History:
--*/

#pragma once

#include "conapi.h"
#include "inputBuffer.hpp"

class SCREEN_INFORMATION;

NTSTATUS TranslateOutputToUnicode(_Inout_ PCHAR_INFO OutputBuffer, _In_ COORD Size);
NTSTATUS TranslateOutputToPaddingUnicode(_Inout_ PCHAR_INFO OutputBuffer,
                                         _In_ COORD Size,
                                         _Inout_ PCHAR_INFO OutputBufferR);

HRESULT DoSrvWriteConsoleInput(_Inout_ InputBuffer* const pInputBuffer,
                               _Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                               _Out_ size_t& eventsWritten,
                               _In_ const bool unicode,
                               _In_ const bool append);

NTSTATUS SrvReadConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvWriteConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvReadConsoleOutputString(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvWriteConsoleOutputString(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

NTSTATUS SrvFillConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvFillConsoleOutput(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_FILLCONSOLEOUTPUT_MSG* pMsg);

NTSTATUS ConsoleCreateScreenBuffer(_Out_ ConsoleHandleData** ppHandle,
                                   _In_ PCONSOLE_API_MSG Message,
                                   _In_ PCD_CREATE_OBJECT_INFORMATION Information,
                                   _In_ PCONSOLE_CREATESCREENBUFFER_MSG a);
