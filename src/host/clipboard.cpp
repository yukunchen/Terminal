/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "clipboard.hpp"
#include "dbcs.h"
#include "selection.hpp"
#include "scrolling.hpp"

#include "globals.h"
#include "misc.h"
#include "output.h"

#pragma hdrstop

_Check_return_
NTSTATUS Clipboard::s_RetrieveTextFromBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                             _In_ bool const fLineSelection,
                                             _In_ UINT const cRectsSelected,
                                             _In_reads_(cRectsSelected) const SMALL_RECT* const rgsrSelection,
                                             _Out_writes_(cRectsSelected) PWCHAR* const rgpwszTempText,
                                             _Out_writes_(cRectsSelected) size_t* const rgTempTextLengths)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (rgpwszTempText == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_5;
    }
    else if (rgTempTextLengths == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_6;
    }

    if (NT_SUCCESS(status))
    {
        UINT iRect = 0;

        // for each row in the selection
        for (iRect = 0; iRect < cRectsSelected; iRect++)
        {
            SMALL_RECT srHighlightRow = rgsrSelection[iRect];

            SMALL_RECT srSelection;
            Selection::Instance().GetSelectionRectangle(&srSelection);

            const UINT iRow = srSelection.Top + iRect;

            // recalculate string length again as the width of the highlight row might have been reduced in the bisect call
            short sStringLength = srHighlightRow.Right - srHighlightRow.Left + 1;

            // this is the source location X/Y coordinates within the active screen buffer to start copying from
            COORD coordSourcePoint;
            coordSourcePoint.X = srHighlightRow.Left;
            coordSourcePoint.Y = srHighlightRow.Top;

            // this is how much to select from the source location. we want one row at a time and the width highlighted.
            COORD coordSelectionSize;
            coordSelectionSize.X = sStringLength;
            coordSelectionSize.Y = 1;

            // allocate our output buffer to copy into for further manipulation
            CHAR_INFO* rgSelection = new CHAR_INFO[sStringLength];
            status = NT_TESTNULL(rgSelection);

            if (NT_SUCCESS(status))
            {
                // our output buffer is 1 dimensional and is just as long as the string, so the "rectangle" should specify just a line.
                SMALL_RECT srTargetRect;
                srTargetRect.Left = 0;
                srTargetRect.Right = sStringLength - 1; // length of 80 runs from left 0 to right 79. therefore -1.
                srTargetRect.Top = 0;
                srTargetRect.Bottom = 0;

                // retrieve the data from the screen buffer
                #pragma prefast(suppress: 6001, "rgSelection's initial state doesn't matter and will be filled appropriately by the called function.")
                status = ReadRectFromScreenBuffer(pScreenInfo, coordSourcePoint, rgSelection, coordSelectionSize, &srTargetRect, nullptr);
                if (NT_SUCCESS(status))
                {
                    // allocate a string buffer
                    size_t cSelectionLength = sStringLength + 1; // add one for null trailing character
                    PWCHAR pwszSelection = new WCHAR[cSelectionLength + 2]; // add 2 to leave space for \r\n if we munged it.
                    status = NT_TESTNULL(rgSelection);

                    if (NT_SUCCESS(status))
                    {
                        // position in the selection length string as we right.
                        // this may not align with the source if we skip characters (trailing bytes)
                        UINT cSelPos = 0;

                        // copy character into the string buffer
                        for (short iCol = 0; iCol < sStringLength; iCol++)
                        {
                            // strip any characters marked "trailing byte" as they're a duplicate
                            // e.g. only copy characters that are NOT trailing bytes
                            if ((rgSelection[iCol].Attributes & COMMON_LVB_TRAILING_BYTE) == 0)
                            {
                                pwszSelection[cSelPos] = rgSelection[iCol].Char.UnicodeChar;
                                cSelPos++;
                            }
                        }

                        // replace any null characters with spaces
                        //for (int iCol = 0; iCol < sStringLength; iCol++)
                        //{
                        //    if (pwszSelection[iCol] == UNICODE_NULL)
                        //    {
                        //        pwszSelection[iCol] = UNICODE_SPACE;
                        //    }
                        //}

                        sStringLength = (short)cSelPos;

                        // trim trailing spaces
                        BOOL bMungeData = (GetKeyState(VK_SHIFT) & KEY_PRESSED) == 0;
                        if (bMungeData)
                        {
                            ROW* pRow = pScreenInfo->TextInfo->GetRowByOffset(iRow);

                            if (pRow == nullptr)
                            {
                                status = STATUS_UNSUCCESSFUL;
                                break;
                            }

                            // FOR LINE SELECTION ONLY: if the row was wrapped, don't remove the spaces at the end.
                            if (!fLineSelection
                                || !pRow->CharRow.WasWrapForced())
                            {
                                for (int iCol = (int)(sStringLength - 1); iCol >= 0; iCol--)
                                {
                                    if (pwszSelection[iCol] == UNICODE_SPACE)
                                    {
                                        pwszSelection[iCol] = UNICODE_NULL;
                                    }
                                    else
                                    {
                                        break;
                                    }

                                }
                            }
                        }

                        // ensure the last character is a null
                        pwszSelection[cSelectionLength - 1] = UNICODE_NULL;

                        // remeasure string
                        if (FAILED(StringCchLengthW(pwszSelection, cSelectionLength, &cSelectionLength)))
                        {
                            status = STATUS_UNSUCCESSFUL;
                        }

                        if (NT_SUCCESS(status))
                        {
                            // if we munged (trimmed spaces), apply CR/LF to the end of the final string
                            if (bMungeData)
                            {
                                // if we're the final line, do not apply CR/LF.
                                // a.k.a if we're earlier than the bottom, then apply CR/LF.
                                if (iRect < cRectsSelected - 1)
                                {
                                    // FOR LINE SELECTION ONLY: if the row was wrapped, do not apply CR/LF.
                                    // a.k.a. if the row was NOT wrapped, then we can assume a CR/LF is proper
                                    // always apply \r\n for box selection
                                    if (!fLineSelection
                                        || !pScreenInfo->TextInfo->GetRowByOffset(iRow)->CharRow.WasWrapForced())
                                    {
                                        pwszSelection[cSelectionLength++] = UNICODE_CARRIAGERETURN;
                                        pwszSelection[cSelectionLength++] = UNICODE_LINEFEED;
                                    }

                                    // and ensure the string is null terminated.
                                    pwszSelection[cSelectionLength] = UNICODE_NULL;
                                }
                            }

                            // save the formatted string and its length for later
                            rgpwszTempText[iRect] = pwszSelection;
                            rgTempTextLengths[iRect] = cSelectionLength;
                        }
                    }

                }

                delete[] rgSelection;
            }

            if (!NT_SUCCESS(status))
            {
                break;
            }
        }

        if (!NT_SUCCESS(status))
        {
            for (UINT iDelRect = 0; iDelRect < iRect; iDelRect++)
            {
                if (rgpwszTempText[iDelRect] != nullptr)
                {
                    delete[] rgpwszTempText[iDelRect];
                }
            }
        }
    }

    return status;
}


// Routine Description:
// - Copies the text given onto the global system clipboard.
// Arguments:
// - cTotalRows
// Return Value:
//  <none>
NTSTATUS Clipboard::s_CopyTextToSystemClipboard(_In_ const UINT cTotalRows,
                                                _In_reads_(cTotalRows) const PWCHAR* const rgTempRows,
                                                _In_reads_(cTotalRows) const size_t* const rgTempRowLengths)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (rgTempRows == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_2;
    }
    else if (rgTempRowLengths == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_3;
    }

    if (NT_SUCCESS(status))
    {

        // calculate number of characters in the rows we created
        size_t cRowCharCount = 0;

        for (UINT iRow = 0; iRow < cTotalRows; iRow++)
        {
            size_t cLength;

            if (FAILED(StringCchLengthW(rgTempRows[iRow], rgTempRowLengths[iRow] + 1, &cLength)))
            {
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            cRowCharCount += cLength;
        }

        if (NT_SUCCESS(status))
        {
            // +1 for null terminator at end of clipboard data
            cRowCharCount++;

            ASSERT(cRowCharCount > 0);

            // allocate the final clipboard data
            HANDLE hClipboardDataHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cRowCharCount * sizeof(WCHAR));

            if (hClipboardDataHandle == nullptr)
            {
                status = NTSTATUS_FROM_WIN32(GetLastError());

                // if for some reason Get Last Error returned successful, set generic memory error.
                if (NT_SUCCESS(status))
                {
                    status = STATUS_NO_MEMORY;
                }
            }

            if (NT_SUCCESS(status))
            {
                PWCHAR pwszClipboardPos = (PWCHAR)GlobalLock(hClipboardDataHandle);

                if (pwszClipboardPos == nullptr)
                {
                    status = NTSTATUS_FROM_WIN32(GetLastError());

                    // if Get Last Error was successful for some reason, set generic memory error
                    if (NT_SUCCESS(status))
                    {
                        status = STATUS_NO_MEMORY;
                    }
                }

                if (NT_SUCCESS(status))
                {
                    // copy all text into the final clipboard data handle. There should be no nulls between rows of
                    // characters, but there should be a \0 at the end.
                    for (UINT iRow = 0; iRow < cTotalRows; iRow++)
                    {
                        if (FAILED(StringCchCopyW(pwszClipboardPos, rgTempRowLengths[iRow] + 1, rgTempRows[iRow])))
                        {
                            status = STATUS_UNSUCCESSFUL;
                            break;
                        }

                        pwszClipboardPos += rgTempRowLengths[iRow];
                    }

                    *pwszClipboardPos = UNICODE_NULL;

                    GlobalUnlock(hClipboardDataHandle);

                    if (NT_SUCCESS(status))
                    {
                        BOOL fSuccess = OpenClipboard(g_ciConsoleInformation.hWnd);
                        if (!fSuccess)
                        {
                            status = NTSTATUS_FROM_WIN32(GetLastError());
                        }

                        if (NT_SUCCESS(status))
                        {
                            fSuccess = EmptyClipboard();
                            if (!fSuccess)
                            {
                                status = NTSTATUS_FROM_WIN32(GetLastError());
                            }

                            if (NT_SUCCESS(status))
                            {
                                if (SetClipboardData(CF_UNICODETEXT, hClipboardDataHandle) == nullptr)
                                {
                                    status = NTSTATUS_FROM_WIN32(GetLastError());
                                }
                            }

                            if (!CloseClipboard() && NT_SUCCESS(status))
                            {
                                status = NTSTATUS_FROM_WIN32(GetLastError());
                            }
                        }
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    // only free if we failed.
                    // the memory has to remain allocated if we successfully placed it on the clipboard.
                    GlobalFree(hClipboardDataHandle);
                }
            }
        }
    }

    return status;
}

// Routine Description:
// - Copies the selected area onto the global system clipboard.
// Arguments:
//  <none>
// Return Value:
//  <none>
void Clipboard::s_StoreSelectionToClipboard()
{
    Selection* pSelection = &Selection::Instance();

    // See if there is a selection to get
    if (!pSelection->IsAreaSelected())
    {
        return;
    }

    // read selection area.
    SCREEN_INFORMATION* const pScreenInfo = g_ciConsoleInformation.CurrentScreenBuffer;

    SMALL_RECT* rgsrSelection;
    UINT cRectsSelected;

    NTSTATUS status = pSelection->GetSelectionRects(&rgsrSelection, &cRectsSelected);

    if (NT_SUCCESS(status))
    {
        PWCHAR* rgTempRows = new PWCHAR[cRectsSelected];
        status = NT_TESTNULL(rgTempRows);

        if (NT_SUCCESS(status))
        {
            size_t* rgTempRowLengths = new size_t[cRectsSelected];
            status = NT_TESTNULL(rgTempRowLengths);

            if (NT_SUCCESS(status))
            {
                const bool fLineSelection = Selection::Instance().IsLineSelection();

                status = s_RetrieveTextFromBuffer(pScreenInfo,
                                                  fLineSelection,
                                                  cRectsSelected,
                                                  rgsrSelection,
                                                  rgTempRows,
                                                  rgTempRowLengths);

                if (NT_SUCCESS(status))
                {
                    status = s_CopyTextToSystemClipboard(cRectsSelected, rgTempRows, rgTempRowLengths);

                    for (UINT iRow = 0; iRow < cRectsSelected; iRow++)
                    {
                        if (rgTempRows[iRow] != nullptr)
                        {
                            delete rgTempRows[iRow];
                        }
                    }
                }

                delete[] rgTempRowLengths;
            }

            delete[] rgTempRows;
        }

        delete[] rgsrSelection;
    }
}

void Clipboard::s_DoCopy()
{
    s_StoreSelectionToClipboard();   // store selection in clipboard
    Selection::Instance().ClearSelection();   // clear selection in console
}

// Returns true if the character should be emitted to the paste stream
// -- in some cases, we will change what character should be emitted, as in the case of "smart quotes"
// Returns false if the character should not be emitted (e.g. <TAB>)
bool Clipboard::s_FilterCharacterOnPaste(_Inout_ WCHAR * const pwch)
{
    bool fAllowChar = true;
    if (g_ciConsoleInformation.GetFilterOnPaste() && (g_ciConsoleInformation.pInputBuffer->InputMode & ENABLE_PROCESSED_INPUT) != 0 )
    {
        switch (*pwch)
        {
            // swallow tabs to prevent inadvertant tab expansion
            case UNICODE_TAB:
            {
                fAllowChar = false;
                break;
            }

            // Replace "smart quotes" with "dumb ones"
            case UNICODE_LEFT_SMARTQUOTE:
            case UNICODE_RIGHT_SMARTQUOTE:
            {
                *pwch = UNICODE_QUOTE;
                break;
            }

            // Replace our full-extended hypen with a normal-extended one
            case UNICODE_LONG_HYPHEN:
            {
                *pwch = UNICODE_HYPHEN;
                break;
            }
        }
    }

    return fAllowChar;
}

/*++

Routine Description:

This routine pastes given Unicode string into the console window.

Arguments:
Console  -   Pointer to CONSOLE_INFORMATION structure
pwStr    -   Unicode string that is pasted to the console window
DataSize -   Size of the Unicode String in characters


Return Value:
None


--*/
#define DATA_CHUNK_SIZE 8192

void Clipboard::s_DoStringPaste(_In_reads_(cchData) PCWCHAR pwchData, _In_ const size_t cchData)
{
    if (pwchData == nullptr)
    {
        return;
    }

    size_t ChunkSize;
    if (cchData > DATA_CHUNK_SIZE)
    {
        ChunkSize = DATA_CHUNK_SIZE;
    }
    else
    {
        ChunkSize = cchData;
    }

    // allocate space to copy data.
    // 8 is maximum number of events per char
    PINPUT_RECORD const StringData = (PINPUT_RECORD)ConsoleHeapAlloc(TMP_TAG, ChunkSize * sizeof(INPUT_RECORD)* 8);
    if (StringData == nullptr)
    {
        return;
    }

    // transfer data to the input buffer in chunks
    PCWCHAR CurChar = pwchData ;    // LATER remove this
    for (size_t j = 0; j < cchData; j += ChunkSize)
    {
        if (ChunkSize > cchData - j)
        {
            ChunkSize = cchData - j;
        }
        PINPUT_RECORD CurRecord = StringData;
        ULONG EventsWritten = 0;
        for (DWORD i = 0; i < ChunkSize; i++)
        {
            // filter out LF if not first char and preceded by CR
            WCHAR Char = *CurChar;
            if (s_FilterCharacterOnPaste(&Char) && // note: s_FilterCharacterOnPaste might change the value of Char
                (Char != UNICODE_LINEFEED ||
                 (i == 0 && j == 0) ||
                 (*(CurChar - 1)) != UNICODE_CARRIAGERETURN))
            {
                SHORT KeyState;
                BYTE KeyFlags;
                BOOL AltGr = FALSE, Shift = FALSE;

                if (Char == 0)
                {
                    j = cchData;
                    break;
                }

                KeyState = VkKeyScanW(Char);
                if (KeyState == -1)
                {
                    WORD CharType;

                    // Determine DBCS character because these character does not know by VkKeyScan.
                    // GetStringTypeW(CT_CTYPE3) & C3_ALPHA can determine all linguistic characters. However, this is
                    // not include symbolic character for DBCS.
                    //
                    // IsCharFullWidth can help for DBCS symbolic character.
                    GetStringTypeW(CT_CTYPE3, &Char, 1, &CharType);
                    if ((CharType & C3_ALPHA) ||
                        IsCharFullWidth(Char))
                    {
                        KeyState = 0;
                    }
                }

                // if VkKeyScanW fails (char is not in kbd layout), we must
                // emulate the key being input through the numpad
                if (KeyState == -1)
                {
                    CHAR CharString[4];
                    UCHAR OemChar;
                    PCHAR pCharString;

                    ConvertToOem(g_ciConsoleInformation.OutputCP, &Char, 1, (LPSTR)& OemChar, 1);

                    (void)_itoa_s(OemChar, CharString, 4, 10);

                    EventsWritten++;
                    LoadKeyEvent(CurRecord, TRUE, 0, VK_MENU, 0x38, LEFT_ALT_PRESSED);
                    CurRecord++;

                    for (pCharString = CharString; *pCharString; pCharString++)
                    {
                        WORD wVirtualKey, wScancode;
                        EventsWritten++;
                        wVirtualKey = *pCharString - '0' + VK_NUMPAD0;
                        wScancode = (WORD)MapVirtualKeyW(wVirtualKey, 0);
                        LoadKeyEvent(CurRecord, TRUE, 0, wVirtualKey, wScancode, LEFT_ALT_PRESSED);
                        CurRecord++;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord, FALSE, 0, wVirtualKey, wScancode, LEFT_ALT_PRESSED);
                        CurRecord++;
                    }

                    EventsWritten++;
                    LoadKeyEvent(CurRecord, FALSE, Char, VK_MENU, 0x38, 0);
                    CurRecord++;
                }
                else
                {
                    KeyFlags = HIBYTE(KeyState);

                    // handle yucky alt-gr keys
                    if ((KeyFlags & 6) == 6)
                    {
                        AltGr = TRUE;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord, TRUE, 0, VK_MENU, 0x38, ENHANCED_KEY | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
                        CurRecord++;
                    }
                    else if (KeyFlags & 1)
                    {
                        Shift = TRUE;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord, TRUE, 0, VK_SHIFT, 0x2A, SHIFT_PRESSED);
                        CurRecord++;
                    }

                    EventsWritten++;
                    LoadKeyEvent(CurRecord,
                                 TRUE,
                                 Char,
                                 LOBYTE(KeyState),
                                 (WORD)MapVirtualKeyW(CurRecord->Event.KeyEvent.wVirtualKeyCode, 0),
                                 0);
                    if (KeyFlags & 1)
                    {
                        CurRecord->Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
                    }

                    if (KeyFlags & 2)
                    {
                        CurRecord->Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
                    }

                    if (KeyFlags & 4)
                    {
                        CurRecord->Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
                    }

                    CurRecord++;

                    EventsWritten++;
                    *CurRecord = *(CurRecord - 1);
                    CurRecord->Event.KeyEvent.bKeyDown = FALSE;
                    CurRecord++;

                    // handle yucky alt-gr keys
                    if (AltGr)
                    {
                        EventsWritten++;
                        LoadKeyEvent(CurRecord, FALSE, 0, VK_MENU, 0x38, ENHANCED_KEY);
                        CurRecord++;
                    }
                    else if (Shift)
                    {
                        EventsWritten++;
                        LoadKeyEvent(CurRecord, FALSE, 0, VK_SHIFT, 0x2A, 0);
                        CurRecord++;
                    }
                }
            }
            CurChar++;
        }

        EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, StringData, EventsWritten);
    }

    ConsoleHeapFree(StringData);
}

/*++

Perform paste request into old app by pulling out clipboard
contents and writing them to the console's input buffer

--*/
void Clipboard::s_DoPaste()
{
    HANDLE ClipboardDataHandle;

    // Clear any selection or scrolling that may be active.
    Selection::Instance().ClearSelection();
    Scrolling::s_ClearScroll();

    // Get paste data from clipboard
    if (!OpenClipboard(g_ciConsoleInformation.hWnd))
    {
        return;
    }

    ClipboardDataHandle = GetClipboardData(CF_UNICODETEXT);
    if (ClipboardDataHandle == nullptr)
    {
        CloseClipboard();
        return;
    }

    PWCHAR pwstr = (PWCHAR)GlobalLock(ClipboardDataHandle);
    s_DoStringPaste(pwstr, (ULONG)GlobalSize(ClipboardDataHandle) / sizeof(WCHAR));
    GlobalUnlock(ClipboardDataHandle);

    CloseClipboard();
}

void Clipboard::s_DoMark()
{
    Selection::Instance().InitializeMarkSelection();
}
