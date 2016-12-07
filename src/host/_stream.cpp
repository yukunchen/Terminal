/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_stream.h"
#include "stream.h"

#include "_output.h"
#include "output.h"
#include "cursor.h"
#include "dbcs.h"
#include "misc.h"
#include "window.hpp"

#include "outputStream.hpp"

#include "utf8ToWidecharParser.hpp"

#pragma hdrstop

// this is probably dead too.
#define IS_GLYPH_CHAR(wch)   (((wch) < L' ') || ((wch) == 0x007F))

// Routine Description:
// - This routine updates the cursor position.  Its input is the non-special
//   cased new location of the cursor.  For example, if the cursor were being
//   moved one space backwards from the left edge of the screen, the X
//   coordinate would be -1.  This routine would set the X coordinate to
//   the right edge of the screen and decrement the Y coordinate by one.
// Arguments:
// - pScreenInfo - Pointer to screen buffer information structure.
// - coordCursor - New location of cursor.
// - fKeepCursorVisible - TRUE if changing window origin desirable when hit right edge
// Return Value:
NTSTATUS AdjustCursorPosition(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ COORD coordCursor, _In_ const BOOL fKeepCursorVisible, _Inout_opt_ PSHORT psScrollY)
{
    if (coordCursor.X < 0)
    {
        if (coordCursor.Y > 0)
        {
            coordCursor.X = (SHORT)(pScreenInfo->ScreenBufferSize.X + coordCursor.X);
            coordCursor.Y = (SHORT)(coordCursor.Y - 1);
        }
        else
        {
            coordCursor.X = 0;
        }
    }
    else if (coordCursor.X >= pScreenInfo->ScreenBufferSize.X)
    {
        // at end of line. if wrap mode, wrap cursor.  otherwise leave it where it is.
        if (pScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT)
        {
            coordCursor.Y += coordCursor.X / pScreenInfo->ScreenBufferSize.X;
            coordCursor.X = coordCursor.X % pScreenInfo->ScreenBufferSize.X;
        }
        else
        {
            coordCursor.X = pScreenInfo->TextInfo->GetCursor()->GetPosition().X;
        }
    }
    SMALL_RECT srMargins = pScreenInfo->GetScrollMargins();
    const bool fMarginsSet = srMargins.Bottom > srMargins.Top;
    const int iCurrentCursorY = pScreenInfo->TextInfo->GetCursor()->GetPosition().Y;

    SMALL_RECT srBufferViewport = pScreenInfo->GetBufferViewport();
    // The margins are in viewport relative coordinates. Adjust for that.
    srMargins.Top += srBufferViewport.Top;
    srMargins.Bottom += srBufferViewport.Top;
    srMargins.Left += srBufferViewport.Left;
    srMargins.Right += srBufferViewport.Left;
    
    const bool fCursorInMargins = iCurrentCursorY <= srMargins.Bottom && iCurrentCursorY >= srMargins.Top;
    const bool fScrollDown = fMarginsSet && fCursorInMargins && (coordCursor.Y > srMargins.Bottom);
    bool fScrollUp = fMarginsSet && fCursorInMargins && (coordCursor.Y < srMargins.Top);

    const bool fScrollUpWithoutMargins = (!fMarginsSet) && (IsFlagSet(pScreenInfo->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) && coordCursor.Y < 0);
    // if we're in VT mode, AND MARGINS AREN'T SET and a Reverse Line Feed took the cursor up past the top of the viewport,
    //   VT style scroll the contents of the screen.
    // This can happen in applications like `less`, that don't set margins, because they're going to
    //   scroll the entire screen anyways, so no need for them to ever set the margins.
    if (fScrollUpWithoutMargins)
    {
        fScrollUp = true;
        srMargins.Top = 0;
        srMargins.Bottom = pScreenInfo->GetBufferViewport().Bottom;
    }

    if (fScrollUp || fScrollDown)
    {
        SHORT diff = coordCursor.Y - (fScrollUp? srMargins.Top : srMargins.Bottom);

        SMALL_RECT scrollRect = {0};
        scrollRect.Top = srMargins.Top;
        scrollRect.Bottom = srMargins.Bottom;
        scrollRect.Left = pScreenInfo->GetBufferViewport().Left;  // NOTE: Left/Right Scroll margins don't do anything currently.
        scrollRect.Right = pScreenInfo->GetBufferViewport().Right - pScreenInfo->GetBufferViewport().Left; // hmm? Not sure. Might just be .Right

        COORD dest;
        dest.X = scrollRect.Left;
        dest.Y = scrollRect.Top - diff;

        CHAR_INFO ciFill;
        ciFill.Attributes = pScreenInfo->GetAttributes().GetLegacyAttributes();
        ciFill.Char.UnicodeChar = L' ';

        ScrollRegion(pScreenInfo, &scrollRect, &scrollRect, dest, ciFill);

        coordCursor.Y-=diff;
    }

    NTSTATUS Status = STATUS_SUCCESS;

    if (coordCursor.Y >= pScreenInfo->ScreenBufferSize.Y)
    {
        // At the end of the buffer. Scroll contents of screen buffer so new position is visible.
        ASSERT(coordCursor.Y == pScreenInfo->ScreenBufferSize.Y);
        if (!StreamScrollRegion(pScreenInfo))
        {
            Status = STATUS_NO_MEMORY;
        }

        if (nullptr != psScrollY)
        {
            *psScrollY += (SHORT)(pScreenInfo->ScreenBufferSize.Y - coordCursor.Y - 1);
        }
        coordCursor.Y += (SHORT)(pScreenInfo->ScreenBufferSize.Y - coordCursor.Y - 1);
    }


    if (NT_SUCCESS(Status))
    {
        // if at right or bottom edge of window, scroll right or down one char.
        if (coordCursor.Y > pScreenInfo->GetBufferViewport().Bottom)
        {
            COORD WindowOrigin;
            WindowOrigin.X = 0;
            WindowOrigin.Y = coordCursor.Y - pScreenInfo->GetBufferViewport().Bottom;
            Status = pScreenInfo->SetViewportOrigin(FALSE, WindowOrigin);
        }
    }
    if (NT_SUCCESS(Status))
    {
        if (fKeepCursorVisible)
        {
            pScreenInfo->MakeCursorVisible(coordCursor);
        }
        Status = pScreenInfo->SetCursorPosition(coordCursor, fKeepCursorVisible);
    }

    return Status;
}

// Routine Description:
// - This routine writes a string to the screen, processing any embedded
//   unicode characters.  The string is also copied to the input buffer, if
//   the output mode is line mode.
// Arguments:
// - pScreenInfo - Pointer to screen buffer information structure.
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
NTSTATUS WriteCharsLegacy(_In_ PSCREEN_INFORMATION pScreenInfo,
                          _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                          _In_ PWCHAR pwchBuffer,
                          _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                          _Inout_ PDWORD const pcb,
                          _Out_opt_ PULONG const pcSpaces,
                          _In_ const SHORT sOriginalXPosition,
                          _In_ const DWORD dwFlags,
                          _Inout_opt_ PSHORT const psScrollY)
{
    PTEXT_BUFFER_INFO pTextBuffer = pScreenInfo->TextInfo;
    Cursor* const pCursor = pTextBuffer->GetCursor();
    COORD CursorPosition = pCursor->GetPosition();
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
    BOOL fUnprocessed = ((pScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT) == 0);
    CHAR LocalBufferA[LOCAL_BUFFER_SIZE];
    PCHAR LocalBufPtrA;

    // Must not adjust cursor here. It has to stay on for many write scenarios. Consumers should call for the cursor to be turned off if they want that.

    WORD const Attributes = pScreenInfo->GetAttributes().GetLegacyAttributes();
    DWORD const BufferSize = *pcb;
    *pcb = 0;
    TempNumSpaces = 0;

    PWCHAR lpString = pwchRealUnicode;

    while (*pcb < BufferSize)
    {
        // correct for delayed EOL
        if (pCursor->IsDelayedEOLWrap())
        {
            COORD coordDelayedAt = pCursor->GetDelayedAtPosition();
            pCursor->ResetDelayEOLWrap();
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

                    Status = AdjustCursorPosition(pScreenInfo, CursorPosition, IsFlagSet(dwFlags, WC_KEEP_CURSOR_VISIBLE), psScrollY);

                    CursorPosition = pCursor->GetPosition();
                }
            }
        }

        // As an optimization, collect characters in buffer and print out all at once.
        XPosition = pCursor->GetPosition().X;
        i = 0;
        LocalBufPtr = LocalBuffer;
        LocalBufPtrA = LocalBufferA;
        while (*pcb < BufferSize && i < LOCAL_BUFFER_SIZE && XPosition < pScreenInfo->ScreenBufferSize.X)
        {
#pragma prefast(suppress:26019, "Buffer is taken in multiples of 2. Validation is ok.")
            Char = *lpString;
            RealUnicodeChar = *pwchRealUnicode;
            if (!IS_GLYPH_CHAR(RealUnicodeChar) || fUnprocessed)
            {
                if (IsCharFullWidth(Char))
                {
                    if (i < (LOCAL_BUFFER_SIZE - 1) && XPosition < (pScreenInfo->ScreenBufferSize.X - 1))
                    {
                        *LocalBufPtr++ = Char;
                        *LocalBufPtrA++ = CHAR_ROW::ATTR_LEADING_BYTE;
                        *LocalBufPtr++ = Char;
                        *LocalBufPtrA++ = CHAR_ROW::ATTR_TRAILING_BYTE;
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
                    *LocalBufPtrA++ = 0;
                }
            }
            else
            {
                ASSERT(pScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);
                switch (RealUnicodeChar)
                {
                case UNICODE_BELL:
                    if (dwFlags & WC_ECHO)
                    {
                        goto CtrlChar;
                    }
                    else
                    {
                        pScreenInfo->SendNotifyBeep();
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
                    if (pScreenInfo->AreTabsSet())
                    {
                        goto EndWhile;
                    }
                    else
                    {
                        TabSize = NUMBER_OF_SPACES_IN_TAB(XPosition);
                        XPosition = (SHORT)(XPosition + TabSize);
                        if (XPosition >= pScreenInfo->ScreenBufferSize.X || IsFlagSet(dwFlags, WC_NONDESTRUCTIVE_TAB))
                        {
                            goto EndWhile;
                        }

                        for (j = 0; j < TabSize && i < LOCAL_BUFFER_SIZE; j++, i++)
                        {
                            *LocalBufPtr = (WCHAR)' ';
                            LocalBufPtr++;
                            *LocalBufPtrA++ = 0;
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
                        *LocalBufPtrA++ = 0;
                        *LocalBufPtrA++ = 0;
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
                            ConvertOutputToUnicode(g_ciConsoleInformation.OutputCP, (LPSTR)& RealUnicodeChar, 1, LocalBufPtr, 1);
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
                        *LocalBufPtrA++ = 0;
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
            if (i > (ULONG)pScreenInfo->ScreenBufferSize.X - pCursor->GetPosition().X)
            {
                i = (ULONG)pScreenInfo->ScreenBufferSize.X - pCursor->GetPosition().X;
            }

            // line was wrapped if we're writing up to the end of the current row
            bool fWasLineWrapped = XPosition >= pScreenInfo->ScreenBufferSize.X;

            StreamWriteToScreenBuffer(LocalBuffer, (SHORT)i, pScreenInfo, LocalBufferA, fWasLineWrapped);
            Region.Left = pCursor->GetPosition().X;
            Region.Right = (SHORT)(pCursor->GetPosition().X + i - 1);
            Region.Top = pCursor->GetPosition().Y;
            Region.Bottom = pCursor->GetPosition().Y;
            WriteToScreen(pScreenInfo, Region);
            TempNumSpaces += i;
            CursorPosition.X = (SHORT)(pCursor->GetPosition().X + i);
            CursorPosition.Y = pCursor->GetPosition().Y;

            // enforce a delayed newline if we're about to pass the end and the WC_DELAY_EOL_WRAP flag is set.
            if (IsFlagSet(dwFlags, WC_DELAY_EOL_WRAP) && CursorPosition.X >= pScreenInfo->ScreenBufferSize.X)
            {
                // Our cursor position as of this time is going to remain on the last position in this column.
                CursorPosition.X = pScreenInfo->ScreenBufferSize.X - 1;

                // Update in the structures that we're still pointing to the last character in the row
                pCursor->SetPosition(CursorPosition);

                // Record for the delay comparison that we're delaying on the last character in the row
                pCursor->DelayEOLWrap(CursorPosition);
            }
            else
            {
                Status = AdjustCursorPosition(pScreenInfo, CursorPosition, IsFlagSet(dwFlags, WC_KEEP_CURSOR_VISIBLE), psScrollY);
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
            ASSERT(pScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);

            // this catches the case where the number of backspaces == the number of characters.
            if (nullptr != pcSpaces)
            {
                *pcSpaces = TempNumSpaces;
            }
            Status = STATUS_SUCCESS;
            goto ExitWriteChars;
        }

        ASSERT(pScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);
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
                    pBuffer = new WCHAR[pwchBuffer - pwchBufferBackupLimit];
                    if (pBuffer == nullptr)
                    {
                        Status = STATUS_NO_MEMORY;
                        goto ExitWriteChars;
                    }
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
                        CursorPosition.X = (pScreenInfo->ScreenBufferSize.X - 1) / TAB_SIZE;
                        CursorPosition.X *= TAB_SIZE;
                        CursorPosition.X += 1;
                        CursorPosition.Y -= 1;

                        // since you just backspaced yourself back up into the previous row, unset the wrap flag on the prev row if it was set
                        ROW* pRow = pTextBuffer->GetRowByOffset(CursorPosition.Y);
                        pRow->CharRow.SetWrapStatus(false);
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
                        WriteOutputString(pScreenInfo, Blanks, CursorPosition, CONSOLE_FALSE_UNICODE,   // faster than real unicode
                            &NumChars, nullptr);
                        Status = FillOutput(pScreenInfo, Attributes, CursorPosition, CONSOLE_ATTRIBUTE, &NumChars);
                    }

                    CursorPosition.X -= 1;
                }
                else if (IsCharFullWidth(LastChar))
                {
                    CursorPosition.X -= 1;
                    TempNumSpaces -= 1;

                    Status = AdjustCursorPosition(pScreenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
                    if (dwFlags & WC_DESTRUCTIVE_BACKSPACE)
                    {
                        NumChars = 1;
                        Status = WriteOutputString(pScreenInfo, Blanks, pCursor->GetPosition(), CONSOLE_FALSE_UNICODE, // faster than real unicode
                            &NumChars, nullptr);
                        Status = FillOutput(pScreenInfo, Attributes, pCursor->GetPosition(), CONSOLE_ATTRIBUTE, &NumChars);
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
            Status = AdjustCursorPosition(pScreenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            if (dwFlags & WC_DESTRUCTIVE_BACKSPACE)
            {
                NumChars = 1;
                WriteOutputString(pScreenInfo, Blanks, pCursor->GetPosition(), CONSOLE_FALSE_UNICODE,  //faster than real unicode
                    &NumChars, nullptr);
                Status = FillOutput(pScreenInfo, Attributes, pCursor->GetPosition(), CONSOLE_ATTRIBUTE, &NumChars);
            }
            if (pCursor->GetPosition().X == 0 && (pScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT) && pwchBuffer > pwchBufferBackupLimit)
            {
                if (CheckBisectProcessW(pScreenInfo,
                                        pwchBufferBackupLimit,
                                        (ULONG)(pwchBuffer + 1 - pwchBufferBackupLimit),
                                        pScreenInfo->ScreenBufferSize.X - sOriginalXPosition,
                                        sOriginalXPosition,
                                        dwFlags & WC_ECHO))
                {
                    CursorPosition.X = pScreenInfo->ScreenBufferSize.X - 1;
                    CursorPosition.Y = (SHORT)(pCursor->GetPosition().Y - 1);

                    // since you just backspaced yourself back up into the previous row, unset the wrap flag on the prev row if it was set
                    ROW* pRow = pTextBuffer->GetRowByOffset(CursorPosition.Y);
                    ASSERT(pRow != nullptr);
                    __analysis_assume(pRow != nullptr);
                    pRow->CharRow.SetWrapStatus(false);

                    Status = AdjustCursorPosition(pScreenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
                }
            }
            break;
        }
        case UNICODE_TAB:
        {
            // if VT-style tabs are set, then handle them the VT way, including not inserting spaces.
            // just move the cursor to the next tab stop.
            if (pScreenInfo->AreTabsSet())
            {
                COORD cCursorOld = pCursor->GetPosition();
                // Get Forward tab handles tabbing past the end of the buffer
                CursorPosition = pScreenInfo->GetForwardTab(cCursorOld);
            }
            else
            {
                TabSize = NUMBER_OF_SPACES_IN_TAB(pCursor->GetPosition().X);
                CursorPosition.X = (SHORT)(pCursor->GetPosition().X + TabSize);

                // move cursor forward to next tab stop.  fill space with blanks.
                // we get here when the tab extends beyond the right edge of the
                // window.  if the tab goes wraps the line, set the cursor to the first
                // position in the next line.
                pwchBuffer++;

                TempNumSpaces += TabSize;
                if (CursorPosition.X >= pScreenInfo->ScreenBufferSize.X)
                {
                    NumChars = pScreenInfo->ScreenBufferSize.X - pCursor->GetPosition().X;
                    CursorPosition.X = 0;
                    CursorPosition.Y = pCursor->GetPosition().Y + 1;

                    // since you just tabbed yourself past the end of the row, set the wrap
                    ROW* pRow = pTextBuffer->GetRowByOffset(pCursor->GetPosition().Y);
                    pRow->CharRow.SetWrapStatus(true);
                }
                else
                {
                    NumChars = CursorPosition.X - pCursor->GetPosition().X;
                    CursorPosition.Y =pCursor->GetPosition().Y;
                }

                if (!IsFlagSet(dwFlags, WC_NONDESTRUCTIVE_TAB))
                {
                    WriteOutputString(pScreenInfo, Blanks, pCursor->GetPosition(), CONSOLE_FALSE_UNICODE,  // faster than real unicode
                        &NumChars, nullptr);
                    FillOutput(pScreenInfo, Attributes, pCursor->GetPosition(), CONSOLE_ATTRIBUTE, &NumChars);
                }

            }
            Status = AdjustCursorPosition(pScreenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        case UNICODE_CARRIAGERETURN:
        {
            // Carriage return moves the cursor to the beginning of the line.
            // We don't need to worry about handling cr or lf for
            // backspace because input is sent to the user on cr or lf.
            pwchBuffer++;
            CursorPosition.X = 0;
            CursorPosition.Y = pCursor->GetPosition().Y;
            Status = AdjustCursorPosition(pScreenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        case UNICODE_LINEFEED:
        {
            // move cursor to the next line.
            pwchBuffer++;

            if (g_ciConsoleInformation.IsReturnOnNewlineAutomatic())
            {
                // Traditionally, we reset the X position to 0 with a newline automatically.
                // Some things might not want this automatic "ONLCR line discipline" (for example, things that are expecting a *NIX behavior.)
                // They will turn it off with an output mode flag.
                CursorPosition.X = 0;
            }

            CursorPosition.Y = (SHORT)(pCursor->GetPosition().Y + 1);

            {
                // since we explicitly just moved down a row, clear the wrap status on the row we just came from
                ROW* pRow = pTextBuffer->GetRowByOffset(pCursor->GetPosition().Y);
                pRow->CharRow.SetWrapStatus(false);
            }

            Status = AdjustCursorPosition(pScreenInfo, CursorPosition, (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0, psScrollY);
            break;
        }
        default:
        {
            Char = *lpString;
            if (Char >= (WCHAR)' ' &&
                IsCharFullWidth(Char) &&
                XPosition >= (pScreenInfo->ScreenBufferSize.X - 1) &&
                (pScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT))
            {
                COORD const TargetPoint = pCursor->GetPosition();
                ROW* const pRow = pTextBuffer->GetRowByOffset(TargetPoint.Y);
                ASSERT(pRow != nullptr);

                PWCHAR const CharTmp = &pRow->CharRow.Chars[TargetPoint.X];
                PCHAR const AttrP = (PCHAR)&pRow->CharRow.KAttrs[TargetPoint.X];

                if (*AttrP & CHAR_ROW::ATTR_TRAILING_BYTE)
                {
                    *(CharTmp - 1) = UNICODE_SPACE;
                    *CharTmp = UNICODE_SPACE;
                    *AttrP = 0;
                    *(AttrP - 1) = 0;

                    Region.Left = TargetPoint.X - 1;
                    Region.Right = (SHORT)(TargetPoint.X);
                    Region.Top = TargetPoint.Y;
                    Region.Bottom = TargetPoint.Y;
                    WriteToScreen(pScreenInfo, Region);
                }

                CursorPosition.X = 0;
                CursorPosition.Y = (SHORT)(TargetPoint.Y + 1);

                // since you just moved yourself down onto the next row with 1 character, that sounds like a forced wrap so set the flag
                pRow->CharRow.SetWrapStatus(true);

                // Additionally, this padding is only called for IsConsoleFullWidth (a.k.a. when a character is too wide to fit on the current line).
                pRow->CharRow.SetDoubleBytePadded(true);

                Status = AdjustCursorPosition(pScreenInfo, CursorPosition, dwFlags & WC_KEEP_CURSOR_VISIBLE, psScrollY);
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
// - pScreenInfo - Pointer to screen buffer information structure.
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
NTSTATUS WriteChars(_In_ PSCREEN_INFORMATION pScreenInfo,
                    _In_range_(<= , pwchBuffer) PWCHAR const pwchBufferBackupLimit,
                    _In_ PWCHAR pwchBuffer,
                    _In_reads_bytes_(*pcb) PWCHAR pwchRealUnicode,
                    _Inout_ PDWORD const pcb,
                    _Out_opt_ PULONG const pcSpaces,
                    _In_ const SHORT sOriginalXPosition,
                    _In_ const DWORD dwFlags,
                    _Inout_opt_ PSHORT const psScrollY)
{
    if (!IsFlagSet(pScreenInfo->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) || !IsFlagSet(pScreenInfo->OutputMode, ENABLE_PROCESSED_OUTPUT))
    {
        return WriteCharsLegacy(pScreenInfo, pwchBufferBackupLimit, pwchBuffer, pwchRealUnicode, pcb, pcSpaces, sOriginalXPosition, dwFlags, psScrollY);
    }

    NTSTATUS Status = STATUS_SUCCESS;

    DWORD const BufferSize = *pcb;
    *pcb = 0;

    {
        DWORD TempNumSpaces = 0;

        {
            if (NT_SUCCESS(Status))
            {
                ASSERT(IsFlagSet(pScreenInfo->OutputMode, ENABLE_PROCESSED_OUTPUT));
                ASSERT(IsFlagSet(pScreenInfo->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING));

                // defined down in the WriteBuffer default case hiding on the other end of the state machine. See outputStream.cpp
                // This is the only mode used by DoWriteConsole.
                ASSERT(IsFlagSet(dwFlags, WC_LIMIT_BACKSPACE));

                StateMachine* const pMachine = pScreenInfo->GetStateMachine();
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

// Note:
// - Console lock must be held when calling this routine
// - String has been translated to unicode at this point.
NTSTATUS DoWriteConsole(_In_ PCONSOLE_API_MSG m, _In_ PSCREEN_INFORMATION pScreenInfo)
{
    PCONSOLE_WRITECONSOLE_MSG const a = &m->u.consoleMsgL1.WriteConsole;

    if (g_ciConsoleInformation.Flags & (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING))
    {
        PWCHAR TransBuffer;

        TransBuffer = (PWCHAR)new BYTE[a->NumBytes];
        if (TransBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }

        memmove(TransBuffer, m->State.TransBuffer, a->NumBytes);
        m->State.TransBuffer = TransBuffer;
        m->State.StackBuffer = FALSE;

        HRESULT hr = ConsoleWaitQueue::s_CreateWait(m, WriteConsoleWaitRoutine, pScreenInfo);
        if (FAILED(hr))
        {
            delete[] TransBuffer;
            m->State.TransBuffer = nullptr;
            m->State.StackBuffer = TRUE;
            return NTSTATUS_FROM_HRESULT(hr);
        }

        return CONSOLE_STATUS_WAIT;
    }

    PTEXT_BUFFER_INFO const pTextBuffer = pScreenInfo->TextInfo;
    return WriteChars(pScreenInfo,
                      m->State.TransBuffer,
                      m->State.TransBuffer,
                      m->State.TransBuffer,
                      &a->NumBytes,
                      nullptr,
                      pTextBuffer->GetCursor()->GetPosition().X,
                      WC_LIMIT_BACKSPACE,
                      nullptr);
}

NTSTATUS DoSrvWriteConsole(_Inout_ PCONSOLE_API_MSG m,
                           _Inout_ PBOOL ReplyPending,
                           _Inout_ PVOID BufPtr,
                           _In_ SCREEN_INFORMATION* const pScreenInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_WRITECONSOLE_MSG const a = &m->u.consoleMsgL1.WriteConsole;
    PSCREEN_INFORMATION const ScreenInfo = pScreenInfo->GetActiveBuffer();
    std::unique_ptr<wchar_t[]> wideCharBuffer {nullptr};
    static Utf8ToWideCharParser parser { g_ciConsoleInformation.OutputCP };
    // update current codepage in case it was changed from last time
    // this was called.
    parser.SetCodePage(g_ciConsoleInformation.OutputCP);

    if (a->Unicode)
    {
        m->State.TransBuffer = (PWCHAR)BufPtr;
    }
    else if (g_ciConsoleInformation.OutputCP == CP_UTF8)
    {
        wideCharBuffer.release();
        unsigned int charCount = a->NumBytes;
        HRESULT hresult = parser.Parse(reinterpret_cast<const byte*>(BufPtr), charCount, wideCharBuffer);
        if (wideCharBuffer.get() == nullptr || FAILED(hresult))
        {
            m->SetReplyStatus(Status);
            if (NT_SUCCESS(Status))
            {
                m->SetReplyInformation(a->NumBytes);
            }
            return Status;
        }
        else
        {
            m->State.TransBuffer = reinterpret_cast<wchar_t*>(wideCharBuffer.get());
            m->State.StackBuffer = FALSE;
            g_ciConsoleInformation.WriteConOutNumBytesTemp = a->NumBytes;
            a->NumBytes = g_ciConsoleInformation.WriteConOutNumBytesUnicode = charCount * sizeof(WCHAR); // WriteCharsLegacy() counts up by sizeof(WCHAR)
        }
    }
    else
    {
        PWCHAR TransBuffer;
        PWCHAR TransBufferOriginalLocation;
        DWORD Length;
        ULONG dbcsNumBytes = 0;
        ULONG BufPtrNumBytes = 0;

        // (a->NumBytes + 2) I think because we might be shoving another unicode char
        // from ScreenInfo->WriteConsoleDbcsLeadByte in front
        TransBuffer = new WCHAR[a->NumBytes + 2];
        if (TransBuffer == nullptr)
        {
            m->SetReplyStatus(STATUS_NO_MEMORY);
            return STATUS_NO_MEMORY;
        }
        m->State.StackBuffer = FALSE;
        TransBufferOriginalLocation = TransBuffer;

        if (!ScreenInfo->WriteConsoleDbcsLeadByte[0] || *(PUCHAR) BufPtr < (UCHAR) ' ')
        {
            dbcsNumBytes = 0;
            BufPtrNumBytes = a->NumBytes;
        }
        else if (a->NumBytes)
        {
            // there was a portion of a dbcs character stored from a previous
            // call so we take the 2nd half from BufPtr[0], put them together
            // and write the wide char to TransBuffer[0]
            ScreenInfo->WriteConsoleDbcsLeadByte[1] = *(PCHAR) BufPtr;
            dbcsNumBytes = sizeof(ScreenInfo->WriteConsoleDbcsLeadByte);
            /*
            * Convert the OEM characters to real Unicode according to
            * OutputCP. First find out if any special chars are involved.
            */
            dbcsNumBytes = sizeof(WCHAR) * MultiByteToWideChar(g_ciConsoleInformation.OutputCP,
                                                               0,
                                                               (LPCCH) ScreenInfo->WriteConsoleDbcsLeadByte,
                                                               dbcsNumBytes,
                                                               TransBuffer,
                                                               dbcsNumBytes);

            if (dbcsNumBytes == 0)
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            if (g_ciConsoleInformation.OutputCP == g_uiOEMCP)
            {
                dbcsNumBytes *= sizeof(WCHAR);
            }
            TransBuffer++;
            BufPtr = ((PBYTE) BufPtr) + (dbcsNumBytes / sizeof(WCHAR));
            BufPtrNumBytes = a->NumBytes - 1;
        }
        else
        {
            // nothing in ScreenInfo->WriteConsoleDbcsLeadByte and nothing in BufPtr
            BufPtrNumBytes = 0;
        }

        ScreenInfo->WriteConsoleDbcsLeadByte[0] = 0;

        // if the last byte in BufPtr is a lead byte for the current code page,
        // save it for the next time this function is called and we can piece it
        // back together then
        __analysis_assume(BufPtrNumBytes <= a->NumBytes);
        if (BufPtrNumBytes && CheckBisectStringA((PCHAR) BufPtr, BufPtrNumBytes, &g_ciConsoleInformation.OutputCPInfo))
        {
            ScreenInfo->WriteConsoleDbcsLeadByte[0] = *((PCHAR) BufPtr + BufPtrNumBytes - 1);
            BufPtrNumBytes--;
        }

        if (BufPtrNumBytes != 0)
        {
            // convert the remaining bytes in BufPtr to wide chars
            Length = sizeof(WCHAR) * MultiByteToWideChar(g_ciConsoleInformation.OutputCP,
                                                         0,
                                                         (LPCCH) BufPtr,
                                                         BufPtrNumBytes,
                                                         TransBuffer,
                                                         BufPtrNumBytes);

            if (Length == 0)
            {
                Status = STATUS_UNSUCCESSFUL;
            }
            BufPtrNumBytes = Length;
        }

        // there's nothing to write (though we might've started with just a lead
        // byte) so just exit
        if ((dbcsNumBytes + BufPtrNumBytes) == 0)
        {
            delete[] TransBuffer;

            m->SetReplyStatus(Status);
            if (NT_SUCCESS(Status))
            {
                m->SetReplyInformation(a->NumBytes);
            }
            return Status;
        }

        g_ciConsoleInformation.WriteConOutNumBytesTemp = a->NumBytes;
        a->NumBytes = g_ciConsoleInformation.WriteConOutNumBytesUnicode = dbcsNumBytes + BufPtrNumBytes;
        m->State.TransBuffer = TransBufferOriginalLocation;
    }

    Status = DoWriteConsole(m, ScreenInfo);
    if (Status == CONSOLE_STATUS_WAIT)
    {
        *ReplyPending = TRUE;
        return STATUS_SUCCESS;
    }
    else
    {
        if (!a->Unicode)
        {
            if (a->NumBytes == g_ciConsoleInformation.WriteConOutNumBytesUnicode)
            {
                a->NumBytes = g_ciConsoleInformation.WriteConOutNumBytesTemp;
            }
            else
            {
                a->NumBytes /= sizeof(WCHAR);
            }

            if (g_ciConsoleInformation.OutputCP != CP_UTF8)
            {
                delete[] m->State.TransBuffer;
            }
        }

        m->SetReplyStatus(Status);
        m->SetReplyInformation(a->NumBytes);
    }

    return Status;
}
