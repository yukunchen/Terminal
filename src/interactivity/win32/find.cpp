/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "find.h"
#include "resource.h"
#include "window.hpp"

#include "..\..\host\dbcs.h"
#include "..\..\host\handle.h"
#include "..\..\host\search.h"

#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

INT_PTR FindDialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    const CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
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
                    PSCREEN_INFORMATION const ScreenInfo = gci->CurrentScreenBuffer;
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

                        if (srSelection.Left < ScreenInfo->GetBufferViewport().Left)
                        {
                            Position.X = srSelection.Left;
                        }
                        else if (srSelection.Right > ScreenInfo->GetBufferViewport().Right)
                        {
                            Position.X = srSelection.Right - ScreenInfo->GetScreenWindowSizeX() + 1;
                        }
                        else
                        {
                            Position.X = ScreenInfo->GetBufferViewport().Left;
                        }

                        if (srSelection.Top < ScreenInfo->GetBufferViewport().Top)
                        {
                            Position.Y = srSelection.Top;
                        }
                        else if (srSelection.Bottom > ScreenInfo->GetBufferViewport().Bottom)
                        {
                            Position.Y = srSelection.Bottom - ScreenInfo->GetScreenWindowSizeY() + 1;
                        }
                        else
                        {
                            Position.Y = ScreenInfo->GetBufferViewport().Top;
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
    Globals* const g = ServiceLocator::LocateGlobals();
    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    
    UnlockConsole();
    if (pWindow != nullptr)
    {
        HWND const hwnd = pWindow->GetWindowHandle();
        
        ++g->uiDialogBoxCount;
        DialogBoxParamW(g->hInstance, MAKEINTRESOURCE(ID_CONSOLE_FINDDLG), hwnd, (DLGPROC)FindDialogProc, (LPARAM) nullptr);
        --g->uiDialogBoxCount;
    }
}
