/*++
Copyright (c) Microsoft Corporation

Module Name:
- Api.h

Abstract:
- Implemenations of the requests that this server accepts.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

// Standard Windows Input APIs

UINT
ApiMapVirtualKey(
    _In_ UINT wCode,
    _In_ UINT wMapType
);

SHORT
ApiVkKeyScan(
    _In_ WCHAR cChar
);

SHORT
ApiGetKeyState(
    _In_ int vk
);

// Display APIs

NTSTATUS
ApiGetDisplaySize(
    _Inout_ CD_IO_DISPLAY_SIZE* const pDisplaySize
);

NTSTATUS
ApiGetFontSize(
    _Inout_ CD_IO_FONT_SIZE* const pFontSize
);

NTSTATUS
ApiSetCursor(
    _In_ CD_IO_CURSOR_INFORMATION CursorInformation,
    _In_ PCIS_CLIENT ClientContext
);

NTSTATUS
ApiUpdateDisplay(
    _In_ PCIS_MSG Message,
    _In_ PCIS_CLIENT ClientContext
);
