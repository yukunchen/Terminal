/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <cpl.h>

#include "menu.h"

#include "cursor.h"
#include "dbcs.h"
#include "getset.h"
#include "handle.h"
#include "icon.hpp"
#include "misc.h"
#include "server.h"
#include "scrolling.hpp"
#include "telemetry.hpp"
#include "window.hpp"

CONST WCHAR gwszPropertiesDll[] = L"\\console.dll";

#pragma hdrstop

// Routine Description:
// - This routine edits the indicated control to one word.
// - This is used to trim the Accelerator key text off of the end of the standard menu items because we don't support the accelerators.
void MyModifyMenuItem(_In_ UINT const ItemId)
{
    WCHAR ItemString[30];
    int const ItemLength = LoadStringW(g_hInstance, ItemId, ItemString, ARRAYSIZE(ItemString));
    if (ItemLength == 0)
    {
        return;
    }

    MENUITEMINFO mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = ItemString;

    if (ItemId == SC_CLOSE)
    {
        mii.fMask |= MIIM_BITMAP;
        mii.hbmpItem = HBMMENU_POPUP_CLOSE;
    }

    SetMenuItemInfoW(g_ciConsoleInformation.hMenu, ItemId, FALSE, &mii);
}

void InitSystemMenu()
{
    WCHAR ItemString[32];
    HMENU const hHeirMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCE(ID_CONSOLE_SYSTEMMENU));

    int ItemLength;
    g_ciConsoleInformation.hHeirMenu = hHeirMenu;
    if (g_ciConsoleInformation.hHeirMenu)
    {
        ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_EDIT, ItemString, ARRAYSIZE(ItemString));
        if (ItemLength)
        {
            // Append the clipboard menu to system menu.
            AppendMenuW(g_ciConsoleInformation.hMenu, MF_POPUP | MF_STRING, (UINT_PTR)g_ciConsoleInformation.hHeirMenu, ItemString);
        }
    }

    // Edit the accelerators off of the standard items.
    MyModifyMenuItem(SC_CLOSE);

    // Add other items to system menu.
    ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_DEFAULTS, ItemString, ARRAYSIZE(ItemString));
    if (ItemLength)
    {
        AppendMenuW(g_ciConsoleInformation.hMenu, MF_STRING, ID_CONSOLE_DEFAULTS, ItemString);
    }

    ItemLength = LoadStringW(g_hInstance, ID_CONSOLE_CONTROL, ItemString, ARRAYSIZE(ItemString));
    if (ItemLength)
    {
        AppendMenuW(g_ciConsoleInformation.hMenu, MF_STRING, ID_CONSOLE_CONTROL, ItemString);
    }
}

// Routine Description:
// - this initializes the system menu when a WM_INITMENU message is read.
void InitializeMenu()
{
    HMENU const hMenu = g_ciConsoleInformation.hMenu;
    HMENU const hHeirMenu = g_ciConsoleInformation.hHeirMenu;

    // If the console is iconic, disable Mark and Scroll.
    if (g_ciConsoleInformation.Flags & CONSOLE_IS_ICONIC)
    {
        EnableMenuItem(hHeirMenu, ID_CONSOLE_MARK, MF_GRAYED);
        EnableMenuItem(hHeirMenu, ID_CONSOLE_SCROLL, MF_GRAYED);
    }
    else
    {
        // if the console is not iconic
        //   if there are no scroll bars
        //       or we're in mark mode
        //       disable scroll
        //   else
        //       enable scroll
        //
        //   if we're in scroll mode
        //       disable mark
        //   else
        //       enable mark

        if (g_ciConsoleInformation.CurrentScreenBuffer->IsMaximizedBoth() || g_ciConsoleInformation.Flags & CONSOLE_SELECTING)
        {
            EnableMenuItem(hHeirMenu, ID_CONSOLE_SCROLL, MF_GRAYED);
        }
        else
        {
            EnableMenuItem(hHeirMenu, ID_CONSOLE_SCROLL, MF_ENABLED);
        }

        if (Scrolling::s_IsInScrollMode())
        {
            EnableMenuItem(hHeirMenu, ID_CONSOLE_SCROLL, MF_GRAYED);
        }
        else
        {
            EnableMenuItem(hHeirMenu, ID_CONSOLE_SCROLL, MF_ENABLED);
        }
    }

    // If we're selecting or scrolling, disable paste. Otherwise, enable it.
    if (g_ciConsoleInformation.Flags & (CONSOLE_SELECTING | CONSOLE_SCROLLING))
    {
        EnableMenuItem(hHeirMenu, ID_CONSOLE_PASTE, MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hHeirMenu, ID_CONSOLE_PASTE, MF_ENABLED);
    }

    // If app has active selection, enable copy. Otherwise, disable it.
    if (g_ciConsoleInformation.Flags & CONSOLE_SELECTING && Selection::Instance().IsAreaSelected())
    {
        EnableMenuItem(hHeirMenu, ID_CONSOLE_COPY, MF_ENABLED);
    }
    else
    {
        EnableMenuItem(hHeirMenu, ID_CONSOLE_COPY, MF_GRAYED);
    }

    // Enable move if not iconic.
    if (g_ciConsoleInformation.Flags & CONSOLE_IS_ICONIC)
    {
        EnableMenuItem(hMenu, SC_MOVE, MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hMenu, SC_MOVE, MF_ENABLED);
    }

    // Enable settings.
    EnableMenuItem(hMenu, ID_CONSOLE_CONTROL, MF_ENABLED);
}

// This routine adds or removes the name to or from the beginning of the window title. The possible names are "Scroll", "Mark", and "Select"
void UpdateWinText()
{
    const bool fInScrollMode = Scrolling::s_IsInScrollMode();

    Selection *pSelection = &Selection::Instance();
    const bool fInKeyboardMarkMode = pSelection->IsInSelectingState() && pSelection->IsKeyboardMarkSelection();
    const bool fInMouseSelectMode = pSelection->IsInSelectingState() && pSelection->IsMouseInitiatedSelection();

    // should have at most one active mode
    ASSERT((fInKeyboardMarkMode && !fInMouseSelectMode && !fInScrollMode) ||
        (!fInKeyboardMarkMode && fInMouseSelectMode && !fInScrollMode) ||
           (!fInKeyboardMarkMode && !fInMouseSelectMode && fInScrollMode) ||
           (!fInKeyboardMarkMode && !fInMouseSelectMode && !fInScrollMode));

    // determine which message, if any, we want to use
    DWORD dwMsgId = 0;
    if (fInKeyboardMarkMode)
    {
        dwMsgId = ID_CONSOLE_MSGMARKMODE;
    }
    else if (fInMouseSelectMode)
    {
        dwMsgId = ID_CONSOLE_MSGSELECTMODE;
    }
    else if (fInScrollMode)
    {
        dwMsgId = ID_CONSOLE_MSGSCROLLMODE;
    }

    // if we have a message, use it
    if (dwMsgId != 0)
    {
        // load format string
        WCHAR szFmt[64];
        if (LoadStringW(g_hInstance, ID_CONSOLE_FMT_WINDOWTITLE, szFmt, ARRAYSIZE(szFmt)) > 0)
        {
            // load mode string
            WCHAR szMsg[64];
            if (LoadStringW(g_hInstance, dwMsgId, szMsg, ARRAYSIZE(szMsg)) > 0)
            {
                // construct new title string
                const size_t TITLE_LENGTH = 256; //dirty arbitrary constant from original conhostv1
                PWSTR pwszTitle = new WCHAR[TITLE_LENGTH]; //
                if (pwszTitle != nullptr) {

                    if (SUCCEEDED(StringCchPrintfW(pwszTitle,
                                                   TITLE_LENGTH, //dirty arbitrary constant, I'm assuming from original conhostv1
                                                   szFmt,
                                                   szMsg,
                                                   g_ciConsoleInformation.Title)))
                    {
                        g_ciConsoleInformation.pWindow->PostUpdateTitle(pwszTitle);
                    }
                    else
                    {
                        delete[] pwszTitle;
                    }
                }
            }
        }
    }
    else
    {
        // no mode-based message. set title back to original state.
        //Copy the title into a new buffer, because consuming the update_title HeapFree's the pwsz.
        g_ciConsoleInformation.pWindow->PostUpdateTitleWithCopy(g_ciConsoleInformation.Title);
    }
}

void GetConsoleState(_Out_ CONSOLE_STATE_INFO * const pStateInfo)
{
    PSCREEN_INFORMATION const ScreenInfo = g_ciConsoleInformation.CurrentScreenBuffer;
    pStateInfo->ScreenBufferSize = ScreenInfo->ScreenBufferSize;
    pStateInfo->WindowSize.X = ScreenInfo->GetScreenWindowSizeX();
    pStateInfo->WindowSize.Y = ScreenInfo->GetScreenWindowSizeY();

    RECT const rcWindow = g_ciConsoleInformation.pWindow->GetWindowRect();
    pStateInfo->WindowPosX = rcWindow.left;
    pStateInfo->WindowPosY = rcWindow.top;

    const FontInfo* const pfiCurrentFont = ScreenInfo->TextInfo->GetCurrentFont();
    pStateInfo->FontFamily = pfiCurrentFont->GetFamily();
    pStateInfo->FontSize = pfiCurrentFont->GetUnscaledSize();
    pStateInfo->FontWeight = pfiCurrentFont->GetWeight();
    StringCchCopyW(pStateInfo->FaceName, ARRAYSIZE(pStateInfo->FaceName), pfiCurrentFont->GetFaceName());

    pStateInfo->CursorSize = ScreenInfo->TextInfo->GetCursor()->GetSize();

    // Retrieve small icon for use in displaying the dialog
    Icon::Instance().GetIcons(nullptr, &pStateInfo->hIcon);

    pStateInfo->QuickEdit = !!(g_ciConsoleInformation.Flags & CONSOLE_QUICK_EDIT_MODE);
    pStateInfo->AutoPosition = !!(g_ciConsoleInformation.Flags & CONSOLE_AUTO_POSITION);
    pStateInfo->InsertMode = g_ciConsoleInformation.GetInsertMode();
    pStateInfo->ScreenAttributes = ScreenInfo->GetAttributes();
    pStateInfo->PopupAttributes = ScreenInfo->GetPopupAttributes();
    pStateInfo->HistoryBufferSize = g_ciConsoleInformation.GetHistoryBufferSize();
    pStateInfo->NumberOfHistoryBuffers = g_ciConsoleInformation.GetNumberOfHistoryBuffers();
    pStateInfo->HistoryNoDup = !!(g_ciConsoleInformation.Flags & CONSOLE_HISTORY_NODUP);

    CopyMemory(pStateInfo->ColorTable, g_ciConsoleInformation.GetColorTable(), g_ciConsoleInformation.GetColorTableSize() * sizeof(COLORREF));

    pStateInfo->OriginalTitle = g_ciConsoleInformation.OriginalTitle;
    pStateInfo->LinkTitle = g_ciConsoleInformation.LinkTitle;

    pStateInfo->CodePage = g_ciConsoleInformation.OutputCP;

    // begin console v2 properties
    pStateInfo->fIsV2Console = TRUE;
    pStateInfo->fWrapText = g_ciConsoleInformation.GetWrapText();
    pStateInfo->fFilterOnPaste = g_ciConsoleInformation.GetFilterOnPaste();
    pStateInfo->fCtrlKeyShortcutsDisabled = g_ciConsoleInformation.GetCtrlKeyShortcutsDisabled();
    pStateInfo->fLineSelection = g_ciConsoleInformation.GetLineSelection();
    pStateInfo->bWindowTransparency = g_ciConsoleInformation.pWindow->GetWindowOpacity();
    // end console v2 properties
}

// Updates the console state from information sent by the properties dialog box.
void PropertiesUpdate(_In_ PCONSOLE_STATE_INFO pStateInfo)
{
    PSCREEN_INFORMATION const ScreenInfo = g_ciConsoleInformation.CurrentScreenBuffer;

    if (g_ciConsoleInformation.OutputCP != pStateInfo->CodePage)
    {
        g_ciConsoleInformation.OutputCP = pStateInfo->CodePage;

        SetConsoleCPInfo(TRUE);
    }

    if (g_ciConsoleInformation.CP != pStateInfo->CodePage)
    {
        g_ciConsoleInformation.CP = pStateInfo->CodePage;

        SetConsoleCPInfo(FALSE);
    }

    // begin V2 console properties

    // NOTE: We must set the wrap state before further manipulating the buffer/window.
    // If we do not, the user will get a different result than the preview (e.g. we'll resize without scroll bars first then turn off wrapping.)
    g_ciConsoleInformation.SetWrapText(!!pStateInfo->fWrapText);
    g_ciConsoleInformation.SetFilterOnPaste(!!pStateInfo->fFilterOnPaste);
    g_ciConsoleInformation.SetCtrlKeyShortcutsDisabled(!!pStateInfo->fCtrlKeyShortcutsDisabled);
    g_ciConsoleInformation.SetLineSelection(!!pStateInfo->fLineSelection);
    Selection::Instance().SetLineSelection(!!g_ciConsoleInformation.GetLineSelection());

    g_ciConsoleInformation.pWindow->SetWindowOpacity(pStateInfo->bWindowTransparency);
    g_ciConsoleInformation.pWindow->ApplyWindowOpacity();
    // end V2 console properties

    // Apply font information (must come before all character calculations for window/buffer size).
    FontInfo fiNewFont(pStateInfo->FaceName, static_cast<BYTE>(pStateInfo->FontFamily), pStateInfo->FontWeight, pStateInfo->FontSize, pStateInfo->CodePage);

    ScreenInfo->UpdateFont(&fiNewFont);

    const FontInfo* const pfiFontApplied = ScreenInfo->TextInfo->GetCurrentFont();

    // Now make sure internal font state reflects the font chosen
    g_ciConsoleInformation.SetFontFamily(pfiFontApplied->GetFamily());
    g_ciConsoleInformation.SetFontSize(pfiFontApplied->GetUnscaledSize());
    g_ciConsoleInformation.SetFontWeight(pfiFontApplied->GetWeight());
    g_ciConsoleInformation.SetFaceName(pfiFontApplied->GetFaceName(), LF_FACESIZE);

    ScreenInfo->SetCursorInformation(pStateInfo->CursorSize, ScreenInfo->TextInfo->GetCursor()->IsVisible());

    {
        // Requested window in characters
        COORD coordWindow = pStateInfo->WindowSize;

        // Requested buffer in characters.
        COORD coordBuffer = pStateInfo->ScreenBufferSize;

        // First limit the window so it cannot be bigger than the monitor.
        // Maximum number of characters we could fit on the given monitor.
        COORD const coordLargest = ScreenInfo->GetLargestWindowSizeInCharacters();

        coordWindow.X = min(coordLargest.X, coordWindow.X);
        coordWindow.Y = min(coordLargest.Y, coordWindow.Y);

        if (g_ciConsoleInformation.GetWrapText())
        {
            // Then if wrap text is on, the buffer size gets fixed to the window size value.
            coordBuffer.X = coordWindow.X;

            // However, we're not done. The "max window size" is if we had no scroll bar. 
            // We need to adjust slightly more if there's space reserved for a vertical scroll bar
            // which happens when the buffer Y is taller than the window Y.
            if (coordBuffer.Y > coordWindow.Y)
            {
                // Since we need a scroll bar in the Y direction, clamp the buffer width to make sure that
                // it is leaving appropriate space for a scroll bar.
                COORD const coordScrollBars = ScreenInfo->GetScrollBarSizesInCharacters();
                SHORT const sMaxBufferWidthWithScroll = coordLargest.X - coordScrollBars.X;

                coordBuffer.X = min(coordBuffer.X, sMaxBufferWidthWithScroll);
            }
        }

        // Now adjust the buffer size first to whatever we want it to be if it's different than before.
        if (coordBuffer.X != ScreenInfo->ScreenBufferSize.X ||
            coordBuffer.Y != ScreenInfo->ScreenBufferSize.Y)
        {
            CommandLine* const pCommandLine = &CommandLine::Instance();

            pCommandLine->Hide(FALSE);

            ScreenInfo->ResizeScreenBuffer(coordBuffer, TRUE);

            pCommandLine->Show();
        }

        // Finally, restrict window size to the maximum possible size for the given buffer now that it's processed.
        COORD const coordMaxForBuffer = ScreenInfo->GetMaxWindowSizeInCharacters();

        coordWindow.X = min(coordWindow.X, coordMaxForBuffer.X);
        coordWindow.Y = min(coordWindow.Y, coordMaxForBuffer.Y);

        // Then finish by updating the window.
        g_ciConsoleInformation.pWindow->UpdateWindowSize(coordWindow);
    }

    if (pStateInfo->QuickEdit)
    {
        g_ciConsoleInformation.Flags |= CONSOLE_QUICK_EDIT_MODE;
    }
    else
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_QUICK_EDIT_MODE;
    }

    if (pStateInfo->AutoPosition)
    {
        g_ciConsoleInformation.Flags |= CONSOLE_AUTO_POSITION;
    }
    else
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_AUTO_POSITION;

        POINT pt;
        pt.x = pStateInfo->WindowPosX;
        pt.y = pStateInfo->WindowPosY;

        g_ciConsoleInformation.pWindow->UpdateWindowPosition(pt);
    }

    if (g_ciConsoleInformation.GetInsertMode() != !!pStateInfo->InsertMode)
    {
        ScreenInfo->SetCursorDBMode(FALSE);
        g_ciConsoleInformation.SetInsertMode(pStateInfo->InsertMode != FALSE);
        if (g_ciConsoleInformation.lpCookedReadData)
        {
            g_ciConsoleInformation.lpCookedReadData->InsertMode = !!g_ciConsoleInformation.GetInsertMode();
        }
    }

    g_ciConsoleInformation.SetColorTable(pStateInfo->ColorTable, g_ciConsoleInformation.GetColorTableSize());

    SetScreenColors(ScreenInfo, pStateInfo->ScreenAttributes, pStateInfo->PopupAttributes, TRUE);
    ScreenInfo->GetAdapterDispatch()->UpdateDefaultColor(pStateInfo->ScreenAttributes);

    ResizeCommandHistoryBuffers(pStateInfo->HistoryBufferSize);
    g_ciConsoleInformation.SetNumberOfHistoryBuffers(pStateInfo->NumberOfHistoryBuffers);
    if (pStateInfo->HistoryNoDup)
    {
        g_ciConsoleInformation.Flags |= CONSOLE_HISTORY_NODUP;
    }
    else
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_HISTORY_NODUP;
    }

    // Since edit keys are global state only stored once in the registry, post the message to the queue to reload
    // those properties specifically from the registry in case they were changed.
    g_ciConsoleInformation.pWindow->PostUpdateExtendedEditKeys();

    SetUndetermineAttribute();
}

// Displays the properties dialog and updates the window state as necessary.
void PropertiesDlgShow(_In_ HWND const hwnd, _In_ BOOL const Defaults)
{
    CONSOLE_STATE_INFO StateInfo = { 0 };
    if (!Defaults)
    {
        GetConsoleState(&StateInfo);
        StateInfo.UpdateValues = FALSE;
    }
    StateInfo.hWnd = hwnd;
    StateInfo.Defaults = Defaults;
    StateInfo.fIsV2Console = TRUE;

    UnlockConsole();

    WCHAR wszFilePath[MAX_PATH + 1] = { 0 };
    UINT const len = GetSystemDirectoryW(wszFilePath, ARRAYSIZE(wszFilePath));
    if (len < ARRAYSIZE(wszFilePath))
    {
        if (SUCCEEDED(StringCchCatW(wszFilePath, ARRAYSIZE(wszFilePath) - len, gwszPropertiesDll)))
        {
            wszFilePath[ARRAYSIZE(wszFilePath) - 1] = UNICODE_NULL;


            HANDLE const hLibrary = LoadLibraryExW(wszFilePath, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (hLibrary != nullptr)
            {
                APPLET_PROC const pfnCplApplet = (APPLET_PROC)GetProcAddress((HMODULE)hLibrary, "CPlApplet");
                if (pfnCplApplet != nullptr)
                {
                    (*pfnCplApplet) (hwnd, CPL_INIT, 0, 0);
                    (*pfnCplApplet) (hwnd, CPL_DBLCLK, (LPARAM)& StateInfo, 0);
                    (*pfnCplApplet) (hwnd, CPL_EXIT, 0, 0);
                }

                FreeLibrary((HMODULE)hLibrary);
            }
        }
    }

    LockConsole();

    if (!Defaults && StateInfo.UpdateValues)
    {
        PropertiesUpdate(&StateInfo);
    }
}
