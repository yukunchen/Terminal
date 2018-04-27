/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiRoutines.h"

#include "_stream.h"
#include "stream.h"
#include "writeData.hpp"

#include "_output.h"
#include "output.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "../types/inc/convert.hpp"

#include "utf8ToWidecharParser.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// Used by WriteCharsLegacy.
#define IS_GLYPH_CHAR(wch)   (((wch) < L' ') || ((wch) == 0x007F))

// Routine Description:
// - This routine updates the cursor position.  Its input is the non-special
//   cased new location of the cursor.  For example, if the cursor were being
//   moved one space backwards from the left edge of the screen, the X
//   coordinate would be -1.  This routine would set the X coordinate to
//   the right edge of the screen and decrement the Y coordinate by one.
// Arguments:
// - screenInfo - reference to screen buffer information structure.
// - coordCursor - New location of cursor.
// - fKeepCursorVisible - TRUE if changing window origin desirable when hit right edge
// Return Value:
[[nodiscard]]
NTSTATUS AdjustCursorPosition(SCREEN_INFORMATION& screenInfo,
                              _In_ COORD coordCursor,
                              const BOOL fKeepCursorVisible,
                              _Inout_opt_ PSHORT psScrollY)
{
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (coordCursor.X < 0)
    {
        if (coordCursor.Y > 0)
        {
            coordCursor.X = (SHORT)(coordScreenBufferSize.X + coordCursor.X);
            coordCursor.Y = (SHORT)(coordCursor.Y - 1);
        }
        else
        {
            coordCursor.X = 0;
        }
    }
    else if (coordCursor.X >= coordScreenBufferSize.X)
    {
        // at end of line. if wrap mode, wrap cursor.  otherwise leave it where it is.
        if (screenInfo.OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT)
        {
            coordCursor.Y += coordCursor.X / coordScreenBufferSize.X;
            coordCursor.X = coordCursor.X % coordScreenBufferSize.X;
        }
        else
        {
            coordCursor.X = screenInfo.GetTextBuffer().GetCursor().GetPosition().X;
        }
    }
    SMALL_RECT srMargins = screenInfo.GetScrollMargins();
    const bool fMarginsSet = srMargins.Bottom > srMargins.Top;
    const int iCurrentCursorY = screenInfo.GetTextBuffer().GetCursor().GetPosition().Y;

    SMALL_RECT srBufferViewport = screenInfo.GetBufferViewport();
    // The margins are in viewport relative coordinates. Adjust for that.
    srMargins.Top += srBufferViewport.Top;
    srMargins.Bottom += srBufferViewport.Top;
    srMargins.Left += srBufferViewport.Left;
    srMargins.Right += srBufferViewport.Left;

    const bool fCursorInMargins = iCurrentCursorY <= srMargins.Bottom && iCurrentCursorY >= srMargins.Top;
    const bool fScrollDown = fMarginsSet && fCursorInMargins && (coordCursor.Y > srMargins.Bottom);
    bool fScrollUp = fMarginsSet && fCursorInMargins && (coordCursor.Y < srMargins.Top);

    const bool fScrollUpWithoutMargins = (!fMarginsSet) && (IsFlagSet(screenInfo.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) && coordCursor.Y < 0);
    // if we're in VT mode, AND MARGINS AREN'T SET and a Reverse Line Feed took the cursor up past the top of the viewport,
    //   VT style scroll the contents of the screen.
    // This can happen in applications like `less`, that don't set margins, because they're going to
    //   scroll the entire screen anyways, so no need for them to ever set the margins.
    if (fScrollUpWithoutMargins)
    {
        fScrollUp = true;
        srMargins.Top = 0;
        srMargins.Bottom = screenInfo.GetBufferViewport().Bottom;
    }

    if (fScrollUp || fScrollDown)
    {
        SHORT diff = coordCursor.Y - (fScrollUp ? srMargins.Top : srMargins.Bottom);

        SMALL_RECT scrollRect = { 0 };
        scrollRect.Top = srMargins.Top;
        scrollRect.Bottom = srMargins.Bottom;
        scrollRect.Left = screenInfo.GetBufferViewport().Left;  // NOTE: Left/Right Scroll margins don't do anything currently.
        scrollRect.Right = screenInfo.GetBufferViewport().Right - screenInfo.GetBufferViewport().Left; // hmm? Not sure. Might just be .Right

        COORD dest;
        dest.X = scrollRect.Left;
        dest.Y = scrollRect.Top - diff;

        CHAR_INFO ciFill;
        ciFill.Attributes = screenInfo.GetAttributes().GetLegacyAttributes();
        ciFill.Char.UnicodeChar = L' ';

        try
        {
            ScrollRegion(screenInfo, scrollRect, scrollRect, dest, ciFill);
        }
        CATCH_LOG();

        coordCursor.Y -= diff;
    }

    NTSTATUS Status = STATUS_SUCCESS;

    if (coordCursor.Y >= coordScreenBufferSize.Y)
    {
        // At the end of the buffer. Scroll contents of screen buffer so new position is visible.
        ASSERT(coordCursor.Y == coordScreenBufferSize.Y);
        if (!StreamScrollRegion(screenInfo))
        {
            Status = STATUS_NO_MEMORY;
        }

        if (nullptr != psScrollY)
        {
            *psScrollY += (SHORT)(coordScreenBufferSize.Y - coordCursor.Y - 1);
        }
        coordCursor.Y += (SHORT)(coordScreenBufferSize.Y - coordCursor.Y - 1);
    }


    if (NT_SUCCESS(Status))
    {
        // if at right or bottom edge of window, scroll right or down one char.
        if (coordCursor.Y > screenInfo.GetBufferViewport().Bottom)
        {
            COORD WindowOrigin;
            WindowOrigin.X = 0;
            WindowOrigin.Y = coordCursor.Y - screenInfo.GetBufferViewport().Bottom;
            Status = screenInfo.SetViewportOrigin(FALSE, WindowOrigin);
        }
    }
    if (NT_SUCCESS(Status))
    {
        if (fKeepCursorVisible)
        {
            screenInfo.MakeCursorVisible(coordCursor);
        }
        Status = screenInfo.SetCursorPosition(coordCursor, !!fKeepCursorVisible);
    }

    return Status;
}

// Routine Description:
// - This routine writes a string to the screen, processing any embedded
//   unicode characters.  The string is also copied to the input buffer, if
//   the output mode is line mode.
// Arguments:
// - screenInfo - reference to screen buffer information structure.
// - pwchBufferBackupLimit - Pointer to beginning of buffer.
// - pwchBuffer - Pointer to buffer to copy string to.  assumed to be at least as long as pwchRealUnicode.
//                This pointer is updated to point to the next position in the buffer.
// - pwchRealUnicode - Pointer to string to write.
// - pcb - On input, number of bytes to write.  On output, number of bytes written.
// - pcSpaces - On output, the number of spaces consumed by the written characters.
// - dwFlags -
//      WC_DESTRUCTIVE_BACKSPACE backspace overwrites characters.
//      WC_KEEP_CURSOR_VISIBLE   change window origin desirable when hit rt. edge
//      WC_ECHO                  if called by Read (echoing characters)
// Return Value:
// Note:
// - This routine does not process tabs and backspace properly.  That code will be implemented as part of the line editing services.
[[nodiscard]]
NTSTATUS WriteCharsLegacy(SCREEN_INFORMATION& screenInfo,
                          _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                          _In_ PWCHAR pwchBuffer,
                          _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                          _Inout_ PDWORD const pcb,
                          _Out_opt_ PULONG const pcSpaces,
                          const SHORT sOriginalXPosition,
                          const DWORD dwFlags,
                          _Inout_opt_ PSHORT const psScrollY)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    TextBuffer& textBuffer = screenInfo.GetTextBuffer();
    Cursor& cursor = textBuffer.GetCursor();
    COORD CursorPosition = cursor.GetPosition();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumChars;
    static WCHAR Blanks[TAB_SIZE] = { UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE,
        UNICODE_SPACE
    };
    SHORT XPosition;
    WCHAR LocalBuffer[LOCAL_BUFFER_SIZE];
    PWCHAR LocalBufPtr;
    ULONG i, j;
    SMALL_RECT Region;
    ULONG TabSize;
    DWORD TempNumSpaces;
    WCHAR Char;
    WCHAR RealUnicodeChar;
    BOOL fUnprocessed = ((screenInfo.OutputMode & ENABLE_PROCESSED_OUTPUT) == 0);
    DbcsAttribute dbcsAttributes[LOCAL_BUFFER_SIZE];
    DbcsAttribute* currentDbcsAttribute = nullptr;

    // Must not adjust cursor here. It has to stay on for many write scenarios. Consumers should call for the cursor to be turned off if they want that.

    WORD const Attributes = screenInfo.GetAttributes().GetLegacyAttributes();
    DWORD const BufferSize = *pcb;
    *pcb = 0;
    TempNumSpaces = 0;

    PWCHAR lpString = pwchRealUnicode;

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();

    while (*pcb < BufferSize)
    {
        // correct for delayed EOL
        if (cursor.IsDelayedEOLWrap())
        {
            COORD coordDelayedAt = cursor.GetDelayedAtPosition();
            cursor.ResetDelayEOLWrap();
            // Only act on a delayed EOL if we didn't move the cursor to a different position from where the EOL was marked.
            if (coordDelayedAt.X == CursorPosition.X && coordDelayedAt.Y == CursorPosition.Y)
            {
                bool fDoEolWrap = false;

                if (IsFlagSet(dwFlags, WC_DELAY_EOL_WRAP))
                {
                    // Correct if it's a printable character and whoever called us still understands/wants delayed EOL wrap.
                    if (*lpString >= UNICODE_SPACE)
                    {
                        fDoEolWrap = true;
                    }
                    else if (*lpString == UNICODE_BACKSPACE)
                    {
                        // if we have an active wrap and a backspace comes in, we need to just advance and go to the next character. don't process it.
                        *pcb += sizeof(WCHAR);
                        lpString++;
                        pwchRealUnicode++;
                        continue;
                    }
                }
                else
                {
                    // Uh oh, we've hit a consumer that doesn't know about delayed end of lines. To rectify this, just quickly jump
                    // forward to the next line as if we had done it earlier, then let everything else play out normally.
                    fDoEolWrap = true;
                }

                if (fDoEolWrap)
                {
                    CursorPosition.X = 0;
                    CursorPosition.Y++;

                    Status = AdjustCursorPosition(screenInfo, CursorPosition, IsFlagSet(dwFlags, WC_KEEP_CURSOR_VISIBLE), psScrollY);

                    CursorPosition = cursor.GetPosition();
                }
            }
        }

        // As an optimization, collect characters in buffer and print out all at once.
        XPosition = cursor.GetPosition().X;
        i = 0;
        LocalBufPtr = LocalBuffer;
        currentDbcsAttribute = dbcsAttributes;
        while (*pcb < BufferSize && i < LOCAL_BUFFER_SIZE && XPosition < coordScreenBufferSize.X)
        {
#pragma prefast(suppress:26019, "Buffer is taken in multiples of 2. Validation is ok.")
            Char = *lpString;
            RealUnicodeChar = *pwchRealUnicode;
            if (!IS_GLYPH_CHAR(RealUnicodeChar) || fUnprocessed)
            {
                if (IsCharFullWidth(Char))
                {
                    if (i < (LOCAL_BUFFER_SIZE - 1) && XPosition < (coordScreenBufferSize.X - 1))
                    {
                        *LocalBufPtr++ = Char;
                        currentDbcsAttribute->SetLeading();
                        ++currentDbcsAttribute;

                        *LocalBufPtr++ = Char;
                        currentDbcsAttribute->SetTrailing();
                        ++currentDbcsAttribute;

                        XPosition += 2;
                        i += 2;
                        pwchBuffer++;
                    }
                    else
                    {
                        goto EndWhile;
                    }
                }
                else
                {
                    *LocalBufPtr = Char;
                    LocalBufPtr++;
                    XPosition++;
                    i++;
                    pwchBuffer++;
                    currentDbcsAttribute->SetSingle();
                    ++currentDbcsAttribute;
                }
            }
            else
            {
                ASSERT(screenInfo.OutputMode & ENABLE_PROCESSED_OUTPUT);
                switch (RealUnicodeChar)
                {
                case UNICODE_BELL:
                    if (dwFlags & WC_ECHO)
                    {
                        goto CtrlChar;
                    }
                    else
                    {
                        screenInfo.SendNotifyBeep();
                    }
                    break;
                case UNICODE_BACKSPACE:

                    // automatically go to EndWhile.  this is because
                    // backspace is not destructive, so "aBkSp" prints
                    // a with the cursor on the "a". we could achieve
                    // this behavior staying in this loop and figuring out
                    // the string that needs to be printed, but it would
                    // be expensive and it's the exceptional case.

                    goto EndWhile;
                    break;
                case UNICODE_TAB:
                    if (screenInfo.AreTabsSet())
                    {
                        goto EndWhile;
                    }
                    else
                    {
                        TabSize = NUMBER_OF_SPACES_IN_TAB(XPosition);
                        XPosition = (SHORT)(XPosition + TabSize);
                        if (XPosition >= coordScreenBufferSize.X || IsFlagSet(dwFlags, WC_NONDESTRUCTIVE_TAB))
                        {
                            goto EndWhile;
                        }

                        for (j = 0; j < TabSize && i < LOCAL_BUFFER_SIZE; j++, i++)
                        {
                            *LocalBufPtr = (WCHAR)' ';
                            LocalBufPtr++;
                            currentDbcsAttribute->SetSingle();
                            ++currentDbcsAttribute;
                        }
                    }

                    pwchBuffer++;
                    break;
                case UNICODE_LINEFEED:
                case UNICODE_CARRIAGERETURN:
                    goto EndWhile;
                default:

                    // if char is ctrl char, write ^char.
                    if ((dwFlags & WC_ECHO) && (IS_CONTROL_CHAR(RealUnicodeChar)))
                    {

                    CtrlChar:          if (i < (LOCAL_BUFFER_SIZE - 1))
                    {
                        *LocalBufPtr = (WCHAR)'^';
                        LocalBufPtr++;
                        XPosition++;
                        i++;
                        *LocalBufPtr = (WCHAR)(RealUnicodeChar + (WCHAR)'@');
                        LocalBufPtr++;
                        XPosition++;
                        i++;
                        pwchBuffer++;
                        currentDbcsAttribute->SetSingle();
                        ++currentDbcsAttribute;
                        currentDbcsAttribute->SetSingle();
                        ++currentDbcsAttribute;
                    }
                                       else
                                       {
                                           goto EndWhile;
                                       }
                    }
                    else
                    {
                        // As a special favor to incompetent apps that attempt to display control chars, convert to corresponding OEM Glyph Chars
                        WORD CharType;

                        GetStringTypeW(CT_CTYPE1, &RealUnicodeChar, 1, &CharType);
                        if (CharType == C1_CNTRL)
                        {
                            ConvertOutputToUnicode(gci.OutputCP,
                                                   (LPSTR)& RealUnicodeChar,
                                                   1,
                                                   LocalBufPtr,
                                                   1);
                        }
                        else if (Char == UNICODE_NULL)
                        {
                            *LocalBufPtr = UNICODE_SPACE;
                        }
                        else
                        {
                            *LocalBufPtr = Char;
                        }

                        LocalBufPtr++;
                        XPosition++;
                        i++;
                        pwchBuffer++;
                        currentDbcsAttribute->SetSingle();
                        ++currentDbcsAttribute;
                    }
                }
            }
            lpString++;
            pwchRealUnicode++;
            *pcb += sizeof(WCHAR);
        }
    EndWhile:
        if (i != 0)
        {
            // Make sure we don't write past the end of the buffer.
            if (i > (ULONG)coordScreenBufferSize.X - cursor.GetPosition().X)
            {
                i = (ULONG)coordScreenBufferSize.X - cursor.GetPosition().X;
            }

            // line was wrapped if we're writing up to the end of the current row
            bool fWasLineWrapped = XPosition >= coordScreenBufferSize.X;

            StreamWriteToScreenBuffer(LocalBuffer, (SHORT)i, screenInfo, dbcsAttributes, fWasLineWrapped);
            Region.Left = cursor.GetPosition().X;
            Region.Right = (SHORT)(cursor.GetPosition().X + i - 1);
            Region.Top = cursor.GetPosition().Y;
            Region.Bottom = cursor.GetPosition().Y;
            WriteToScreen(screenInfo, Region);
            TempNumSpaces += i;
            CursorPosition.X = (SHORT)(cursor.GetPosition().X + i);
            CursorPosition.Y = cursor.GetPosition().Y;

            // enforce a delayed newline if we're about to pass the end and the WC_DELAY_EOL_WRAP flag is set.
            if (IsFlagSet(dwFlags, WC_DELAY_EOL_WRAP) && CursorPosition.X >= coordScreenBufferSize.X)
            {
                // Our cursor position as of this time is going to remain on the last position in this column.
                CursorPosition.X = coordScreenBufferSize.X - 1;

                // Update in the structures that we're still pointing to the last character in the row
                cursor.SetPosition(CursorPosition);

                // Record for the delay comparison that we're delaying on the last character in the row
                cursor.DelayEOLWrap(CursorPosition);
            }
            else
            {
                Status = AdjustCursorPosition(screenInfo, CursorPosition, IsFlagSet(dwFlags, WC_KEEP_CURSOR_VISIBLE), psScrollY);
            }

            if (*pcb == BufferSize)
            {
                if (nullptr != pcSpaces)
                {
                    *pcSpaces = TempNumSpaces;
                }
                Status = STATUS_SUCCESS;
                goto ExitWriteChars;
            }
            continue;
        }
        else if (*pcb >= BufferSize)
        {
            ASSERT(screenInfo.OutputMode & ENABLE_PROCESSED_OUTPUT);

            // this catches the case where the number of backspaces == the number of characters.
            if (nullptr != pcSpaces)
            {
                *pcSpaces = TempNumSpaces;
            }
            Status = STATUS_SUCCESS;
            goto ExitWriteChars;
        }

        ASSERT(screenInfo.OutputMode & ENABLE_PROCESSED_OUTPUT);
        switch (*lpString)
        {
        case UNICODE_BACKSPACE:
        {
            // move cursor backwards one space. overwrite current char with blank.
            // we get here because we have to backspace from the beginning of the line
            TempNumSpaces -= 1;
            if (pwchBuffer == pwchBufferBackupLimit)
            {
                CursorPosition.X -= 1;
            }
            else
            {
                PWCHAR pBuffer = nullptr;
                WCHAR TmpBuffer[LOCAL_BUFFER_SIZE];
                PWCHAR Tmp, Tmp2;
                WCHAR LastChar;

                if (pwchBuffer - pwchBufferBackupLimit > LOCAL_BUFFER_SIZE)
                {
                    pBuffer = new(std::nothrow) WCHAR[pwchBuffer - pwchBufferBackupLimit];
                    if (pBuffer == nullptr)
                    {
                        Status = STATUS_NO_MEMORY;
                        goto ExitWriteChars;
                    }
                    ZeroMemory(pBuffer, sizeof(WCHAR) * (pwchBuffer - pwchBufferBackupLimit));
                }
                else
                {
                    pBuffer = TmpBuffer;
                }

                for (i = 0, Tmp2 = pBuffer, Tmp = pwchBufferBackupLimit; i < (ULONG)(pwchBuffer - pwchBufferBackupLimit); i++, Tmp++)
                {


                    if (*Tmp == UNICODE_BACKSPACE)
                    {
                        if (Tmp2 > pBuffer)
                        {
                            Tmp2--;
                        }
                    }
                    else
                    {
                        ASSERT(Tmp2 >= pBuffer);
                        *Tmp2++ = *Tmp;
                    }
                }

                if (Tmp2 == pBuffer)
                {
                    LastChar = (WCHAR)' ';
                }
                else
                {
#pragma prefast(suppress:26001, "This is fine. Tmp2 has to have advanced or it would equal pBuffer.")
                    LastChar = *(Tmp2 - 1);
                }

                if (pBuffer != TmpBuffer)
                {
                    delete[] pBuffer;
                }

                if (LastChar == UNICODE_TAB)
                {
                    CursorPosition.X -= (SHORT)(RetrieveNumberOfSpaces(sOriginalXPosition,
                                                                       pwchBufferBackupLimit,
                                                                       (ULONG)(pwchBuffer - pwchBufferBackupLimit - 1)));
                    if (CursorPosition.X < 0)
                    {
                        CursorPosition.X = (coordScreenBufferSize.X - 1) / TAB_SIZE;
                        CursorPosition.X *= TAB_SIZE;
                        CursorPosition.X += 1;
                        CursorPosition.Y -= 1;

                        // since you just backspaced yourself back up into the previous row, unset the wrap
                        // flag on the prev row if it was set
                        textBuffer.GetRowByOffset(CursorPosition.Y).GetCharRow().SetWrapForced(false);
                    }
                }
                else if (IS_CONTROL_CHAR(LastChar))
                {
                    CursorPosition.X -= 1;
                    TempNumSpaces -= 1;

                    // overwrite second character of ^x sequence.
                    if (dwFlags & WC_DESTRUCTIVE_BACKSPACE)
                    {
                        NumChars = 1;
                        LOG_IF_FAILED(WriteOutputString(screenInfo,
                                                        Blanks,
                                                        CursorPosition,
                                                        CONSOLE_FALSE_UNICODE,   // faster than real unicode
                                                        &NumChars,
                                                        nullptr));
                        Status = FillOutput(screenInfo, Attributes, CursorPosition, CONSOLE_ATTRIBUTE, &NumChars);
                    }

                    CursorPosition.X -= 1;
                }
                else if (IsCharFullWidth(LastChar))
                {
                    CursorPosition.X -= 1;
                    TempNumSpaces -= 1;

                    Status = AdjustCursorPosition(screenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
                    if (dwFlags & WC_DESTRUCTIVE_BACKSPACE)
                    {
                        NumChars = 1;
                        Status = WriteOutputString(screenInfo, Blanks, cursor.GetPosition(), CONSOLE_FALSE_UNICODE, // faster than real unicode
                                                   &NumChars, nullptr);
                        Status = FillOutput(screenInfo, Attributes, cursor.GetPosition(), CONSOLE_ATTRIBUTE, &NumChars);
                    }
                    CursorPosition.X -= 1;
                }
                else
                {
                    CursorPosition.X--;
                }
            }
            if ((dwFlags & WC_LIMIT_BACKSPACE) && (CursorPosition.X < 0))
            {
                CursorPosition.X = 0;
                OutputDebugStringA(("CONSRV: Ignoring backspace to previous line\n"));
            }
            Status = AdjustCursorPosition(screenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            if (dwFlags & WC_DESTRUCTIVE_BACKSPACE)
            {
                NumChars = 1;
                LOG_IF_FAILED(WriteOutputString(screenInfo,
                                                Blanks,
                                                cursor.GetPosition(),
                                                CONSOLE_FALSE_UNICODE,  //faster than real unicode
                                                &NumChars,
                                                nullptr));
                Status = FillOutput(screenInfo, Attributes, cursor.GetPosition(), CONSOLE_ATTRIBUTE, &NumChars);
            }
            if (cursor.GetPosition().X == 0 && (screenInfo.OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT) && pwchBuffer > pwchBufferBackupLimit)
            {
                if (CheckBisectProcessW(screenInfo,
                                        pwchBufferBackupLimit,
                                        (ULONG)(pwchBuffer + 1 - pwchBufferBackupLimit),
                                        coordScreenBufferSize.X - sOriginalXPosition,
                                        sOriginalXPosition,
                                        dwFlags & WC_ECHO))
                {
                    CursorPosition.X = coordScreenBufferSize.X - 1;
                    CursorPosition.Y = (SHORT)(cursor.GetPosition().Y - 1);

                    // since you just backspaced yourself back up into the previous row, unset the wrap flag
                    // on the prev row if it was set
                    textBuffer.GetRowByOffset(CursorPosition.Y).GetCharRow().SetWrapForced(false);

                    Status = AdjustCursorPosition(screenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
                }
            }
            break;
        }
        case UNICODE_TAB:
        {
            // if VT-style tabs are set, then handle them the VT way, including not inserting spaces.
            // just move the cursor to the next tab stop.
            if (screenInfo.AreTabsSet())
            {
                COORD cCursorOld = cursor.GetPosition();
                // Get Forward tab handles tabbing past the end of the buffer
                CursorPosition = screenInfo.GetForwardTab(cCursorOld);
            }
            else
            {
                TabSize = NUMBER_OF_SPACES_IN_TAB(cursor.GetPosition().X);
                CursorPosition.X = (SHORT)(cursor.GetPosition().X + TabSize);

                // move cursor forward to next tab stop.  fill space with blanks.
                // we get here when the tab extends beyond the right edge of the
                // window.  if the tab goes wraps the line, set the cursor to the first
                // position in the next line.
                pwchBuffer++;

                TempNumSpaces += TabSize;
                if (CursorPosition.X >= coordScreenBufferSize.X)
                {
                    NumChars = coordScreenBufferSize.X - cursor.GetPosition().X;
                    CursorPosition.X = 0;
                    CursorPosition.Y = cursor.GetPosition().Y + 1;

                    // since you just tabbed yourself past the end of the row, set the wrap
                    textBuffer.GetRowByOffset(cursor.GetPosition().Y).GetCharRow().SetWrapForced(true);
                }
                else
                {
                    NumChars = CursorPosition.X - cursor.GetPosition().X;
                    CursorPosition.Y = cursor.GetPosition().Y;
                }

                if (!IsFlagSet(dwFlags, WC_NONDESTRUCTIVE_TAB))
                {
                    LOG_IF_FAILED(WriteOutputString(screenInfo,
                                                    Blanks,
                                                    cursor.GetPosition(),
                                                    CONSOLE_FALSE_UNICODE,  // faster than real unicode
                                                    &NumChars,
                                                    nullptr));
                    LOG_IF_FAILED(FillOutput(screenInfo, Attributes, cursor.GetPosition(), CONSOLE_ATTRIBUTE, &NumChars));
                }

            }
            Status = AdjustCursorPosition(screenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        case UNICODE_CARRIAGERETURN:
        {
            // Carriage return moves the cursor to the beginning of the line.
            // We don't need to worry about handling cr or lf for
            // backspace because input is sent to the user on cr or lf.
            pwchBuffer++;
            CursorPosition.X = 0;
            CursorPosition.Y = cursor.GetPosition().Y;
            Status = AdjustCursorPosition(screenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        case UNICODE_LINEFEED:
        {
            // move cursor to the next line.
            pwchBuffer++;

            if (gci.IsReturnOnNewlineAutomatic())
            {
                // Traditionally, we reset the X position to 0 with a newline automatically.
                // Some things might not want this automatic "ONLCR line discipline" (for example, things that are expecting a *NIX behavior.)
                // They will turn it off with an output mode flag.
                CursorPosition.X = 0;
            }

            CursorPosition.Y = (SHORT)(cursor.GetPosition().Y + 1);

            {
                // since we explicitly just moved down a row, clear the wrap status on the row we just came from
                textBuffer.GetRowByOffset(cursor.GetPosition().Y).GetCharRow().SetWrapForced(false);
            }

            Status = AdjustCursorPosition(screenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        default:
        {
            Char = *lpString;
            if (Char >= (WCHAR)' ' &&
                IsCharFullWidth(Char) &&
                XPosition >= (coordScreenBufferSize.X - 1) &&
                (screenInfo.OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT))
            {
                COORD const TargetPoint = cursor.GetPosition();
                ROW& Row = textBuffer.GetRowByOffset(TargetPoint.Y);
                ICharRow& iCharRow = Row.GetCharRow();

                try
                {
                    if (iCharRow.GetAttribute(TargetPoint.X).IsTrailing())
                    {
                        iCharRow.ClearCell(TargetPoint.X);
                        iCharRow.ClearCell(TargetPoint.X - 1);

                        Region.Left = TargetPoint.X - 1;
                        Region.Right = (SHORT)(TargetPoint.X);
                        Region.Top = TargetPoint.Y;
                        Region.Bottom = TargetPoint.Y;
                        WriteToScreen(screenInfo, Region);
                    }
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                CursorPosition.X = 0;
                CursorPosition.Y = (SHORT)(TargetPoint.Y + 1);

                // since you just moved yourself down onto the next row with 1 character, that sounds like a
                // forced wrap so set the flag
                iCharRow.SetWrapForced(true);

                // Additionally, this padding is only called for IsConsoleFullWidth (a.k.a. when a character
                // is too wide to fit on the current line).
                iCharRow.SetDoubleBytePadded(true);

                Status = AdjustCursorPosition(screenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
                continue;
            }
            break;
        }
        }
        if (!NT_SUCCESS(Status))
        {
            goto ExitWriteChars;
        }

        *pcb += sizeof(WCHAR);
        lpString++;
        pwchRealUnicode++;
    }

    if (nullptr != pcSpaces)
    {
        *pcSpaces = TempNumSpaces;
    }

    Status = STATUS_SUCCESS;

ExitWriteChars:
    return Status;
}

// Routine Description:
// - This routine writes a string to the screen, processing any embedded
//   unicode characters.  The string is also copied to the input buffer, if
//   the output mode is line mode.
// Arguments:
// - screenInfo - reference to screen buffer information structure.
// - pwchBufferBackupLimit - Pointer to beginning of buffer.
// - pwchBuffer - Pointer to buffer to copy string to.  assumed to be at least as long as pwchRealUnicode.
//              This pointer is updated to point to the next position in the buffer.
// - pwchRealUnicode - Pointer to string to write.
// - pcb - On input, number of bytes to write.  On output, number of bytes written.
// - pcSpaces - On output, the number of spaces consumed by the written characters.
// - dwFlags -
//      WC_DESTRUCTIVE_BACKSPACE backspace overwrites characters.
//      WC_KEEP_CURSOR_VISIBLE   change window origin (viewport) desirable when hit rt. edge
//      WC_ECHO                  if called by Read (echoing characters)
// Return Value:
// Note:
// - This routine does not process tabs and backspace properly.  That code will be implemented as part of the line editing services.
[[nodiscard]]
NTSTATUS WriteChars(SCREEN_INFORMATION& screenInfo,
                    _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                    _In_ PWCHAR pwchBuffer,
                    _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                    _Inout_ PDWORD const pcb,
                    _Out_opt_ PULONG const pcSpaces,
                    const SHORT sOriginalXPosition,
                    const DWORD dwFlags,
                    _Inout_opt_ PSHORT const psScrollY)
{
    if (!IsFlagSet(screenInfo.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) ||
        !IsFlagSet(screenInfo.OutputMode, ENABLE_PROCESSED_OUTPUT))
    {
        return WriteCharsLegacy(screenInfo,
                                pwchBufferBackupLimit,
                                pwchBuffer,
                                pwchRealUnicode,
                                pcb,
                                pcSpaces,
                                sOriginalXPosition,
                                dwFlags,
                                psScrollY);
    }

    NTSTATUS Status = STATUS_SUCCESS;

    DWORD const BufferSize = *pcb;
    *pcb = 0;

    {
        DWORD TempNumSpaces = 0;

        {
            if (NT_SUCCESS(Status))
            {
                ASSERT(IsFlagSet(screenInfo.OutputMode, ENABLE_PROCESSED_OUTPUT));
                ASSERT(IsFlagSet(screenInfo.OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING));

                // defined down in the WriteBuffer default case hiding on the other end of the state machine. See outputStream.cpp
                // This is the only mode used by DoWriteConsole.
                ASSERT(IsFlagSet(dwFlags, WC_LIMIT_BACKSPACE));

                StateMachine* const pMachine = screenInfo.GetStateMachine();
                size_t const cch = BufferSize / sizeof(WCHAR);

                pMachine->ProcessString(pwchRealUnicode, cch);
                *pcb += BufferSize;
            }
        }

        if (nullptr != pcSpaces)
        {
            *pcSpaces = TempNumSpaces;
        }
    }

    return Status;
}

// Routine Description:
// - Takes the given text and inserts it into the given screen buffer.
// Note:
// - Console lock must be held when calling this routine
// - String has been translated to unicode at this point.
// Arguments:
// - pwchBuffer - wide character text to be inserted into buffer
// - pcbBuffer - byte count of pwchBuffer on the way in, number of bytes consumed on the way out.
// - screenInfo - Screen Information class to write the text into at the current cursor position
// - ppWaiter - If writing to the console is blocked for whatever reason, this will be filled with a pointer to context
//              that can be used by the server to resume the call at a later time.
// Return Value:
// - STATUS_SUCCESS if OK.
// - CONSOLE_STATUS_WAIT if we couldn't finish now and need to be called back later (see ppWaiter).
// - Or a suitable NTSTATUS format error code for memory/string/math failures.
[[nodiscard]]
NTSTATUS DoWriteConsole(_In_reads_bytes_(*pcbBuffer) PWCHAR pwchBuffer,
                        _Inout_ ULONG* const pcbBuffer,
                        SCREEN_INFORMATION& screenInfo,
                        _Outptr_result_maybenull_ WriteData** const ppWaiter)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (IsAnyFlagSet(gci.Flags, (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING)))
    {
        try
        {
            *ppWaiter = new WriteData(screenInfo,
                                      (wchar_t*)pwchBuffer,
                                      *pcbBuffer,
                                      gci.OutputCP);
        }
        catch (...)
        {
            return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
        }

        return CONSOLE_STATUS_WAIT;
    }

    const auto& textBuffer = screenInfo.GetTextBuffer();
    return WriteChars(screenInfo,
                      pwchBuffer,
                      pwchBuffer,
                      pwchBuffer,
                      pcbBuffer,
                      nullptr,
                      textBuffer.GetCursor().GetPosition().X,
                      WC_LIMIT_BACKSPACE,
                      nullptr);
}

// Routine Description:
// - This method performs the actual work of attempting to write to the console, converting data types as necessary
//   to adapt from the server types to the legacy internal host types.
// - It operates on Unicode data only. It's assumed the text is translated by this point.
// Arguments:
// - OutContext - the console output object to write the new text into
// - pwsTextBuffer - wide character text buffer provided by client application to insert
// - cchTextBufferLength - text buffer counted in characters
// - pcchTextBufferRead - character count of the number of characters we were able to insert before returning
// - ppWaiter - If we are blocked from writing now and need to wait, this is filled with contextual data for the server to restore the call later
// Return Value:
// - S_OK if successful.
// - S_OK if we need to wait (check if ppWaiter is not nullptr).
// - Or a suitable HRESULT code for math/string/memory failures.
[[nodiscard]]
HRESULT WriteConsoleWImplHelper(_In_ IConsoleOutputObject& OutContext,
                                _In_reads_(cchTextBufferLength) const wchar_t* const pwsTextBuffer,
                                const size_t cchTextBufferLength,
                                _Out_ size_t* const pcchTextBufferRead,
                                _Outptr_result_maybenull_ WriteData** const ppWaiter)
{
    // Set out variables in case we exit early.
    *pcchTextBufferRead = 0;
    *ppWaiter = nullptr;

    // Convert characters to bytes to give to DoWriteConsole.
    size_t cbTextBufferLength;
    RETURN_IF_FAILED(SizeTMult(cchTextBufferLength, sizeof(wchar_t), &cbTextBufferLength));

    ULONG ulTextBufferLength;
    RETURN_IF_FAILED(SizeTToULong(cbTextBufferLength, &ulTextBufferLength));

    NTSTATUS Status = DoWriteConsole(const_cast<wchar_t*>(pwsTextBuffer), &ulTextBufferLength, OutContext, ppWaiter);

    // Convert back from bytes to characters for the resulting string length written.
    *pcchTextBufferRead = ulTextBufferLength / sizeof(wchar_t);

    if (Status == CONSOLE_STATUS_WAIT)
    {
        assert(nullptr != ppWaiter);
        Status = STATUS_SUCCESS;
    }

    RETURN_NTSTATUS(Status);
}

// Routine Description:
// - Writes non-Unicode formatted data into the given console output object.
// - This method will convert from the given input into wide characters before chain calling the wide character version of the function.
//   It uses the current Output Codepage for conversions (set via SetConsoleOutputCP).
// - NOTE: This may be blocked for various console states and will return a wait context pointer if necessary.
// Arguments:
// - pOutContext - the console output object to write the new text into
// - psTextBuffer - char/byte text buffer provided by client application to insert
// - cchTextBufferLength - text buffer counted in characters (which is equivalent to bytes because this is the A version)
// - pcchTextBufferRead - character count of the number of characters (also bytes because A version) we were able to insert before returning
// - ppWaiter - If we are blocked from writing now and need to wait, this is filled with contextual data for the server to restore the call later
// Return Value:
// - S_OK if successful.
// - S_OK if we need to wait (check if ppWaiter is not nullptr).
// - Or a suitable HRESULT code for math/string/memory failures.
[[nodiscard]]
HRESULT ApiRoutines::WriteConsoleAImpl(_In_ IConsoleOutputObject& OutContext,
                                       _In_reads_(cchTextBufferLength) const char* const psTextBuffer,
                                       const size_t cchTextBufferLength,
                                       _Out_ size_t* const pcchTextBufferRead,
                                       _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Ensure output variables are initialized.
    *pcchTextBufferRead = 0;
    *ppWaiter = nullptr;

    bool fLeadByteCaptured = false;
    bool fLeadByteConsumed = false;

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    if (cchTextBufferLength == 0)
    {
        return S_OK;
    }

    UINT const uiCodePage = gci.OutputCP;

    // Convert our input parameters to Unicode
    std::unique_ptr<wchar_t[]> wideCharBuffer{ nullptr };
    static Utf8ToWideCharParser parser{ gci.OutputCP };

    // update current codepage in case it was changed from last time
    // this was called. We do this outside the UTF-8 check because the parser drops its state
    // when the codepage changes.
    parser.SetCodePage(gci.OutputCP);

    SCREEN_INFORMATION& ScreenInfo = OutContext.GetActiveBuffer();
    wchar_t* pwchBuffer;
    size_t cchBuffer;
    if (uiCodePage == CP_UTF8)
    {
        wideCharBuffer.release();
        unsigned int charCount;
        unsigned int charsConsumed;
        unsigned int charsGenerated;
        RETURN_IF_FAILED(SizeTToUInt(cchTextBufferLength, &charCount));
        RETURN_IF_FAILED(parser.Parse(reinterpret_cast<const byte*>(psTextBuffer),
                                      charCount,
                                      charsConsumed,
                                      wideCharBuffer,
                                      charsGenerated));

        pwchBuffer = reinterpret_cast<wchar_t*>(wideCharBuffer.get());
        cchBuffer = charsGenerated;
        *pcchTextBufferRead = charsConsumed;
    }
    else
    {
        NTSTATUS Status = STATUS_SUCCESS;
        PWCHAR TransBuffer;
        PWCHAR TransBufferOriginalLocation;
        DWORD Length;
        ULONG dbcsNumBytes = 0;
        ULONG BufPtrNumBytes = 0;
        const char* BufPtr = psTextBuffer;

        // (cchTextBufferLength + 2) I think because we might be shoving another unicode char
        // from ScreenInfo->WriteConsoleDbcsLeadByte in front
        TransBuffer = new(std::nothrow) WCHAR[cchTextBufferLength + 2];
        RETURN_IF_NULL_ALLOC(TransBuffer);
        ZeroMemory(TransBuffer, sizeof(WCHAR) * (cchTextBufferLength + 2));

        TransBufferOriginalLocation = TransBuffer;

        unsigned int uiTextBufferLength;
        RETURN_IF_FAILED(SizeTToUInt(cchTextBufferLength, &uiTextBufferLength));

        if (!ScreenInfo.WriteConsoleDbcsLeadByte[0] || *(PUCHAR)BufPtr < (UCHAR) ' ')
        {
            dbcsNumBytes = 0;
            BufPtrNumBytes = uiTextBufferLength;
        }
        else if (cchTextBufferLength)
        {
            // there was a portion of a dbcs character stored from a previous
            // call so we take the 2nd half from BufPtr[0], put them together
            // and write the wide char to TransBuffer[0]
            ScreenInfo.WriteConsoleDbcsLeadByte[1] = *(PCHAR)BufPtr;

            wistd::unique_ptr<wchar_t[]> convertedChars;
            size_t cchConverted = 0;
            if (FAILED(ConvertToW(gci.OutputCP,
                                  reinterpret_cast<const char* const>(ScreenInfo.WriteConsoleDbcsLeadByte),
                                  ARRAYSIZE(ScreenInfo.WriteConsoleDbcsLeadByte),
                                  convertedChars,
                                  cchConverted)))
            {
                Status = STATUS_UNSUCCESSFUL;
                dbcsNumBytes = 0;
            }
            else
            {
                assert(cchConverted == 1);
                dbcsNumBytes = sizeof(wchar_t);
                TransBuffer[0] = convertedChars[0];
                BufPtr++;
            }

            // this looks weird to be always incrementing even if the conversion failed, but this is the
            // original behavior so it's left unchanged.
            TransBuffer++;
            BufPtrNumBytes = uiTextBufferLength - 1;

            // Note that we used a stored lead byte from a previous call in order to complete this write
            // Use this to offset the "number of bytes consumed" calculation at the end by -1 to account
            // for using a byte we had internally, not off the stream.
            fLeadByteConsumed = true;
        }
        else
        {
            // nothing in ScreenInfo->WriteConsoleDbcsLeadByte and nothing in BufPtr
            BufPtrNumBytes = 0;
        }

        ScreenInfo.WriteConsoleDbcsLeadByte[0] = 0;

        // if the last byte in BufPtr is a lead byte for the current code page,
        // save it for the next time this function is called and we can piece it
        // back together then
        __analysis_assume(BufPtrNumBytes <= uiTextBufferLength);
        if (BufPtrNumBytes && CheckBisectStringA((PCHAR)BufPtr, BufPtrNumBytes, &gci.OutputCPInfo))
        {
            ScreenInfo.WriteConsoleDbcsLeadByte[0] = *((PCHAR)BufPtr + BufPtrNumBytes - 1);
            BufPtrNumBytes--;

            // Note that we captured a lead byte during this call, but won't actually draw it until later.
            // Use this to offset the "number of bytes consumed" calculation at the end by +1 to account
            // for taking a byte off the stream.
            fLeadByteCaptured = true;
        }

        if (BufPtrNumBytes != 0)
        {
            // convert the remaining bytes in BufPtr to wide chars
            Length = sizeof(WCHAR) * MultiByteToWideChar(gci.OutputCP,
                                                         0,
                                                         (LPCCH)BufPtr,
                                                         BufPtrNumBytes,
                                                         TransBuffer,
                                                         BufPtrNumBytes);

            if (Length == 0)
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            BufPtrNumBytes = Length;
        }

        pwchBuffer = TransBufferOriginalLocation;
        cchBuffer = (dbcsNumBytes + BufPtrNumBytes) / sizeof(wchar_t);
    }

    // Make the W version of the call
    size_t cchBufferRead;

    // Hold the specific version of the waiter locally so we can tinker with it if we must to store additional context.
    WriteData* pWriteDataWaiter = nullptr;

    HRESULT const hr = WriteConsoleWImplHelper(ScreenInfo, pwchBuffer, cchBuffer, &cchBufferRead, &pWriteDataWaiter);

    // If there is no waiter, process the byte count now.
    if (nullptr == pWriteDataWaiter)
    {
        // Calculate how many bytes of the original A buffer were consumed in the W version of the call to satisfy pcchTextBufferRead.
        // For UTF-8 conversions, we've already returned this information above.
        if (CP_UTF8 != uiCodePage)
        {
            size_t cchTextBufferRead = 0;

            // Start by counting the number of A bytes we used in printing our W string to the screen.
            LOG_IF_FAILED(GetALengthFromW(uiCodePage, pwchBuffer, cchBufferRead, &cchTextBufferRead));

            // If we captured a byte off the string this time around up above, it means we didn't feed
            // it into the WriteConsoleW above, and therefore its consumption isn't accounted for
            // in the count we just made. Add +1 to compensate.
            if (fLeadByteCaptured)
            {
                cchTextBufferRead++;
            }

            // If we consumed an internally-stored lead byte this time around up above, it means that we
            // fed a byte into WriteConsoleW that wasn't a part of this particular call's request.
            // We need to -1 to compensate and tell the caller the right number of bytes consumed this request.
            if (fLeadByteConsumed)
            {
                cchTextBufferRead--;
            }

            *pcchTextBufferRead = cchTextBufferRead;
        }
    }
    else
    {
        // If there is a waiter, then we need to stow some additional information in the wait structure so
        // we can synthesize the correct byte count later when the wait routine is triggered.
        if (CP_UTF8 != uiCodePage)
        {
            // For non-UTF8 codepages, save the lead byte captured/consumed data so we can +1 or -1 the final decoded count
            // in the WaitData::Notify method later.
            pWriteDataWaiter->SetLeadByteAdjustmentStatus(fLeadByteCaptured, fLeadByteConsumed);
        }
        else
        {
            // For UTF8 codepages, just remember the consumption count from the UTF-8 parser.
            pWriteDataWaiter->SetUtf8ConsumedCharacters(*pcchTextBufferRead);
        }
    }

    // Free remaining data
    if (uiCodePage != CP_UTF8)
    {
        delete[] pwchBuffer;
    }

    // Give back the waiter now that we're done with tinkering with it.
    *ppWaiter = pWriteDataWaiter;

    return hr;
}

// Routine Description:
// - Writes Unicode formatted data into the given console output object.
// - NOTE: This may be blocked for various console states and will return a wait context pointer if necessary.
// Arguments:
// - OutContext - the console output object to write the new text into
// - pwsTextBuffer - wide character text buffer provided by client application to insert
// - cchTextBufferLength - text buffer counted in characters
// - pcchTextBufferRead - character count of the number of characters we were able to insert before returning
// - ppWaiter - If we are blocked from writing now and need to wait, this is filled with contextual data for the server to restore the call later
// Return Value:
// - S_OK if successful.
// - S_OK if we need to wait (check if ppWaiter is not nullptr).
// - Or a suitable HRESULT code for math/string/memory failures.
[[nodiscard]]
HRESULT ApiRoutines::WriteConsoleWImpl(_In_ IConsoleOutputObject& OutContext,
                                       _In_reads_(cchTextBufferLength) const wchar_t* const pwsTextBuffer,
                                       const size_t cchTextBufferLength,
                                       _Out_ size_t* const pcchTextBufferRead,
                                       _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return WriteConsoleWImplHelper(OutContext.GetActiveBuffer(),
                                   pwsTextBuffer,
                                   cchTextBufferLength,
                                   pcchTextBufferRead,
                                   reinterpret_cast<WriteData**>(ppWaiter));
}
