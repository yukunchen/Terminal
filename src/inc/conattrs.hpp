/*++
Copyright (c) Microsoft Corporation
--*/
#pragma once

#define FG_ATTRS (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)
#define BG_ATTRS (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)
#define META_ATTRS (COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE | COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_LVERTICAL | COMMON_LVB_GRID_RVERTICAL | COMMON_LVB_REVERSE_VIDEO | COMMON_LVB_UNDERSCORE )

WORD FindNearestTableIndex(_In_ COLORREF const Color,
                           _In_reads_(cColorTable) const COLORREF* const ColorTable,
                           _In_ const WORD cColorTable);

bool FindTableIndex(_In_ COLORREF const Color,
                    _In_reads_(cColorTable) const COLORREF* const ColorTable,
                    _In_ const WORD cColorTable,
                    _Out_ WORD* const pFoundIndex);

WORD XtermToWindowsIndex(_In_ const size_t index);

const WORD WINDOWS_RED_ATTR     = FOREGROUND_RED;
const WORD WINDOWS_GREEN_ATTR   = FOREGROUND_GREEN;
const WORD WINDOWS_BLUE_ATTR    = FOREGROUND_BLUE;
const WORD WINDOWS_BRIGHT_ATTR  = FOREGROUND_INTENSITY;

const WORD XTERM_RED_ATTR       = 0x01;
const WORD XTERM_GREEN_ATTR     = 0x02;
const WORD XTERM_BLUE_ATTR      = 0x04;
const WORD XTERM_BRIGHT_ATTR    = 0x08;
