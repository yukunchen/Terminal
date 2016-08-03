/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "find.h"

#include "clipboard.hpp"
#include "dbcs.h"
#include "handle.h"
#include "menu.h"
#include "output.h"
#include "window.hpp"

#pragma hdrstop


USHORT SearchForString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                       _In_reads_(cchSearch) PCWSTR pwszSearch,
                       _In_range_(1, SEARCH_STRING_LENGTH) USHORT cchSearch,
                       _In_ const BOOLEAN IgnoreCase,
                       _In_ const BOOLEAN Reverse,
                       _In_ const BOOLEAN SearchAndSetAttr,
                       _In_ const ULONG ulAttr,
                       _Out_opt_ PCOORD coordStringPosition)  // not touched for SearchAndSetAttr case.
{
    if (coordStringPosition != nullptr)
    {
        coordStringPosition->X = 0;
        coordStringPosition->Y = 0;
    }

    COORD MaxPosition;
    MaxPosition.X = pScreenInfo->ScreenBufferSize.X - cchSearch;
    MaxPosition.Y = pScreenInfo->ScreenBufferSize.Y - 1;

    Selection* const pSelection = &Selection::Instance();

    // calculate starting position
    COORD Position;
    if (pSelection->IsInSelectingState())
    {
        COORD coordSelAnchor;
        pSelection->GetSelectionAnchor(&coordSelAnchor);

        Position.X = min(coordSelAnchor.X, MaxPosition.X);
        Position.Y = coordSelAnchor.Y;
    }
    else if (Reverse)
    {
        Position.X = 0;
        Position.Y = 0;
    }
    else
    {
        Position.X = MaxPosition.X;
        Position.Y = MaxPosition.Y;
    }

    // Prepare search string.
    WCHAR SearchString2[SEARCH_STRING_LENGTH * 2 + 1];  // search string buffer
    UINT const cchSearchString2 = ARRAYSIZE(SearchString2);
    ASSERT(cchSearch == wcslen(pwszSearch) && cchSearch < cchSearchString2);

    PWSTR pStr = SearchString2;

    // boundary to prevent loop overflow. -1 to leave space for the null terminator at the end of the string.
    const PCWCHAR pwchSearchString2End = SearchString2 + cchSearchString2 - 1;

    while (*pwszSearch && pStr < pwchSearchString2End)
    {
        *pStr++ = *pwszSearch;
#if defined(CON_TB_MARK)
        // On the screen, one East Asian "FullWidth" character occupies two columns (double width),
        // so we have to share two screen buffer elements for one DBCS character.
        // For example, if the screen shows "AB[DBC]CD", the screen buffer will be,
        //   [L'A'] [L'B'] [DBC(Unicode)] [CON_TB_MARK] [L'C'] [L'D']
        //   (DBC:: Double Byte Character)
        // CON_TB_MARK is used to indicate that the column is the trainling byte.
        //
        // Before comparing the string with the screen buffer, we need to modify the search
        // string to match the format of the screen buffer.
        // If we find a FullWidth character in the search string, put CON_TB_MARK
        // right after it so that we're able to use NLS functions.
#else
        // If KAttribute is used, the above example will look like:
        // CharRow.Chars: [L'A'] [L'B'] [DBC(Unicode)] [DBC(Unicode)] [L'C'] [L'D']
        // CharRow.KAttrs:    0      0   LEADING_BYTE  TRAILING_BYTE       0      0
        //
        // We do no fixup if SearchAndSetAttr was specified.  In this case the search buffer has
        // come straight out of the console buffer,  so is already in the required format.
#endif
        if (!SearchAndSetAttr
            && IsCharFullWidth(*pwszSearch)
            && pStr < pwchSearchString2End)
        {
#if defined(CON_TB_MARK)
            *pStr++ = CON_TB_MARK;
#else
            *pStr++ = *pwszSearch;
#endif
        }
        ++pwszSearch;
    }

    *pStr = L'\0';
    
    USHORT const ColumnWidth = (USHORT) (pStr - SearchString2);
    pwszSearch = SearchString2;

    // search for the string
    BOOL RecomputeRow = TRUE;
    COORD const EndPosition = Position;
    SHORT RowIndex;
    PROW Row = nullptr;
    do
    {
#if !defined(CON_TB_MARK)
#if DBG
        int nLoop = 0;
#endif
recalc:
#endif
        if (Reverse)
        {
            if (--Position.X < 0)
            {
                Position.X = MaxPosition.X;
                if (--Position.Y < 0)
                {
                    Position.Y = MaxPosition.Y;
                }
                RecomputeRow = TRUE;
            }
        }
        else
        {
            if (++Position.X > MaxPosition.X)
            {
                Position.X = 0;
                if (++Position.Y > MaxPosition.Y)
                {
                    Position.Y = 0;
                }
                RecomputeRow = TRUE;
            }
        }
  
        if (RecomputeRow)
        {
            RowIndex = (pScreenInfo->TextInfo->GetFirstRowIndex() + Position.Y) % pScreenInfo->ScreenBufferSize.Y;
            Row = &pScreenInfo->TextInfo->Rows[RowIndex];
            RecomputeRow = FALSE;
        }
#if !defined(CON_TB_MARK)
        ASSERT(nLoop++ < 2);
        if (Row->CharRow.KAttrs && (Row->CharRow.KAttrs[Position.X] & CHAR_ROW::ATTR_TRAILING_BYTE))
        {
            goto recalc;
        }
#endif
        if (IgnoreCase ? 
            0 == _wcsnicmp(pwszSearch, &Row->CharRow.Chars[Position.X], cchSearch) :
            0 == wcsncmp(pwszSearch, &Row->CharRow.Chars[Position.X], cchSearch))
        {
            //  If this operation was a normal user find, then return now. Otherwise set the attributes of this match,
            //  and continue searching the whole buffer.
            if (!SearchAndSetAttr)
            {
                *coordStringPosition = Position;
                return ColumnWidth;
            }
            else
            {
                SMALL_RECT Rect;
                Rect.Top = Rect.Bottom = Position.Y;
                Rect.Left = Position.X;
                Rect.Right = Rect.Left + ColumnWidth - 1;

                pSelection->ColorSelection(&Rect, ulAttr);
            }
        }
    }
    while (!((Position.X == EndPosition.X) && (Position.Y == EndPosition.Y)));

    return 0;   // the string was not found
}

INT_PTR FindDialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    // This bool is used to track which option - up or down - was used to perform the last search. That way, the next time the 
    //   find dialog is opened, it will default to the last used option.
    static bool fFindSearchUp = true;
    WCHAR szBuf[SEARCH_STRING_LENGTH + 1];
    switch (Message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtrW(hWnd, DWLP_USER, lParam);
            SendDlgItemMessageW(hWnd, ID_CONSOLE_FINDSTR, EM_LIMITTEXT, ARRAYSIZE(szBuf) - 1, 0);
            CheckRadioButton(hWnd, ID_CONSOLE_FINDUP, ID_CONSOLE_FINDDOWN, (fFindSearchUp? ID_CONSOLE_FINDUP : ID_CONSOLE_FINDDOWN));
            return TRUE;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    USHORT const StringLength = (USHORT) GetDlgItemTextW(hWnd, ID_CONSOLE_FINDSTR, szBuf, ARRAYSIZE(szBuf));
                    if (StringLength == 0)
                    {
                        break;
                    }
                    BOOLEAN const IgnoreCase = IsDlgButtonChecked(hWnd, ID_CONSOLE_FINDCASE) == 0;
                    BOOLEAN const Reverse = IsDlgButtonChecked(hWnd, ID_CONSOLE_FINDDOWN) == 0;
                    fFindSearchUp = !!Reverse;
                    PSCREEN_INFORMATION const ScreenInfo = g_ciConsoleInformation.CurrentScreenBuffer;
                    COORD Position;
                    USHORT const ColumnWidth = SearchForString(ScreenInfo, szBuf, StringLength, IgnoreCase, Reverse, FALSE, 0, &Position);
                    if (ColumnWidth != 0)
                    {
                        Telemetry::Instance().LogFindDialogNextClicked(StringLength, (Reverse != 0), (IgnoreCase == 0));

                        LockConsole();

                        Selection* pSelection = &Selection::Instance();

                        if (pSelection->IsInSelectingState())
                        {
                            pSelection->ClearSelection(true);
                        }

                        // Position is the start of the selection region
                        // EndPosition is the end
                        COORD endPosition;
                        endPosition.X = Position.X + ColumnWidth - 1;
                        endPosition.Y = Position.Y;

                        pSelection->InitializeMouseSelection(Position);
                        pSelection->ShowSelection();
                        pSelection->ExtendSelection(endPosition);

                        // Make sure the highlighted text will be visible
                        // TODO: can this just be merged with the select region code?
                        SMALL_RECT srSelection;
                        pSelection->GetSelectionRectangle(&srSelection);

                        if (srSelection.Left < ScreenInfo->BufferViewport.Left)
                        {
                            Position.X = srSelection.Left;
                        }
                        else if (srSelection.Right > ScreenInfo->BufferViewport.Right)
                        {
                            Position.X = srSelection.Right - ScreenInfo->GetScreenWindowSizeX() + 1;
                        }
                        else
                        {
                            Position.X = ScreenInfo->BufferViewport.Left;
                        }

                        if (srSelection.Top < ScreenInfo->BufferViewport.Top)
                        {
                            Position.Y = srSelection.Top;
                        }
                        else if (srSelection.Bottom > ScreenInfo->BufferViewport.Bottom)
                        {
                            Position.Y = srSelection.Bottom - ScreenInfo->GetScreenWindowSizeY() + 1;
                        }
                        else
                        {
                            Position.Y = ScreenInfo->BufferViewport.Top;
                        }
                        ScreenInfo->SetViewportOrigin(TRUE, Position);

                        UnlockConsole();

                        return TRUE;
                    }
                    else
                    {
                        // The string wasn't found.
                        ScreenInfo->SendNotifyBeep();
                    }
                    break;
                }
                case IDCANCEL:
                    Telemetry::Instance().FindDialogClosed();
                    EndDialog(hWnd, 0);
                    return TRUE;
            }
            break;
        }
        default:
            break;
    }
    return FALSE;
}

void DoFind()
{
    HWND const hwnd = g_ciConsoleInformation.hWnd;

    UnlockConsole();

    ++g_uiDialogBoxCount;
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCE(ID_CONSOLE_FINDDLG), hwnd, FindDialogProc, (LPARAM) nullptr);
    --g_uiDialogBoxCount;
}
