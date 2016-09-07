/*++
Copyright (c) Microsoft Corporation

Module Name:
- _stream.h

Abstract:
- Process stream written content into the text buffer

Author:
- KazuM Jun.09.1997

Revision History:
- Remove FE/Non-FE separation in preparation for refactoring. (MiNiksa, 2014)
--*/

#pragma once

/*++
Routine Description:
    This routine updates the cursor position.  Its input is the non-special
    cased new location of the cursor.  For example, if the cursor were being
    moved one space backwards from the left edge of the screen, the X
    coordinate would be -1.  This routine would set the X coordinate to
    the right edge of the screen and decrement the Y coordinate by one.

Arguments:
    pScreenInfo - Pointer to screen buffer information structure.
    coordCursor - New location of cursor.
    fKeepCursorVisible - TRUE if changing window origin desirable when hit right edge

Return Value:
--*/
NTSTATUS AdjustCursorPosition(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ COORD coordCursor, _In_ const BOOL fKeepCursorVisible, _Inout_opt_ PSHORT psScrollY);

#define LOCAL_BUFFER_SIZE 100


/*++
Routine Description:
    This routine writes a string to the screen, processing any embedded
    unicode characters.  The string is also copied to the input buffer, if
    the output mode is line mode.

Arguments:
    ScreenInfo - Pointer to screen buffer information structure.
    lpBufferBackupLimit - Pointer to beginning of buffer.
    lpBuffer - Pointer to buffer to copy string to.  assumed to be at least
        as long as lpRealUnicodeString.  This pointer is updated to point to the
        next position in the buffer.
    lpRealUnicodeString - Pointer to string to write.
    NumBytes - On input, number of bytes to write.  On output, number of
        bytes written.
    NumSpaces - On output, the number of spaces consumed by the written characters.
    dwFlags -
      WC_DESTRUCTIVE_BACKSPACE backspace overwrites characters.
      WC_KEEP_CURSOR_VISIBLE   change window origin desirable when hit rt. edge
      WC_ECHO                  if called by Read (echoing characters)

Return Value:

Note:
    This routine does not process tabs and backspace properly.  That code
    will be implemented as part of the line editing services.
--*/
NTSTATUS WriteCharsLegacy(_In_ PSCREEN_INFORMATION pScreenInfo,
                          _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                          _In_ PWCHAR pwchBuffer,
                          _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                          _Inout_ PDWORD const pcb,
                          _Out_opt_ PULONG const pcSpaces,
                          _In_ const SHORT sOriginalXPosition,
                          _In_ const DWORD dwFlags,
                          _Inout_opt_ PSHORT const psScrollY);

// The new entry point for WriteChars to act as an intercept in case we place a Virtual Terminal processor in the way.
NTSTATUS WriteChars(_In_ PSCREEN_INFORMATION pScreenInfo,
                    _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                    _In_ PWCHAR pwchBuffer,
                    _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                    _Inout_ PDWORD const pcb,
                    _Out_opt_ PULONG const pcSpaces,
                    _In_ const SHORT sOriginalXPosition,
                    _In_ const DWORD dwFlags,
                    _Inout_opt_ PSHORT const psScrollY);

// NOTE: console lock must be held when calling this routine
// String has been translated to unicode at this point.
NTSTATUS DoWriteConsole(_In_ PCONSOLE_API_MSG m, _In_ PSCREEN_INFORMATION pScreenInfo);

NTSTATUS DoSrvWriteConsole(_Inout_ PCONSOLE_API_MSG m,
                           _Inout_ PBOOL ReplyPending,
                           _Inout_ PVOID BufPtr,
                           _In_ PCONSOLE_HANDLE_DATA HandleData);
