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
NTSTATUS TranslateOutputToPaddingUnicode(_Inout_ PCHAR_INFO OutputBuffer, _In_ COORD Size, _Inout_ PCHAR_INFO OutputBufferR);

NTSTATUS SrvGetConsoleInput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

NTSTATUS SrvWriteConsoleInput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvWriteConsoleInput(_In_ InputBuffer* pInputInfo, _Inout_ CONSOLE_WRITECONSOLEINPUT_MSG* pMsg, _In_ INPUT_RECORD* const rgInputRecords);

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
