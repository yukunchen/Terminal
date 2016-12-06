/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "scrolling.hpp"

#include "menu.h"
#include "selection.hpp"
#include "window.hpp"

ULONG Scrolling::s_ucWheelScrollLines = 0;
ULONG Scrolling::s_ucWheelScrollChars = 0;

void Scrolling::s_UpdateSystemMetrics()
{
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &s_ucWheelScrollLines, FALSE);
    SystemParametersInfoW(SPI_GETWHEELSCROLLCHARS, 0, &s_ucWheelScrollChars, FALSE);
}

bool Scrolling::s_IsInScrollMode()
{
    return IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_SCROLLING);
}

void Scrolling::s_DoScroll()
{
    if (!s_IsInScrollMode())
    {
        // clear any selection we may have -- can't scroll and select at the same time
        Selection::Instance().ClearSelection();

        g_ciConsoleInformation.Flags |= CONSOLE_SCROLLING;
        UpdateWinText();
    }
}

void Scrolling::s_ClearScroll()
{
    g_ciConsoleInformation.Flags &= ~CONSOLE_SCROLLING;
    UpdateWinText();
}

void Scrolling::s_ScrollIfNecessary(_In_ const SCREEN_INFORMATION * const pScreenInfo)
{
    Selection* const pSelection = &Selection::Instance();

    if (pSelection->IsInSelectingState() && pSelection->IsMouseButtonDown())
    {
        POINT CursorPos;
        if (!GetCursorPos(&CursorPos))
        {
            return;
        }

        RECT ClientRect;
        if (!GetClientRect(g_ciConsoleInformation.hWnd, &ClientRect))
        {
            return;
        }

        MapWindowPoints(g_ciConsoleInformation.hWnd, nullptr, (LPPOINT) & ClientRect, 2);
        if (!(PtInRect(&ClientRect, CursorPos)))
        {
            ScreenToClient(g_ciConsoleInformation.hWnd, &CursorPos);

            COORD MousePosition;
            MousePosition.X = (SHORT)CursorPos.x;
            MousePosition.Y = (SHORT)CursorPos.y;

            COORD coordFontSize = pScreenInfo->GetScreenFontSize();

            MousePosition.X /= coordFontSize.X;
            MousePosition.Y /= coordFontSize.Y;

            MousePosition.X += pScreenInfo->GetBufferViewport().Left;
            MousePosition.Y += pScreenInfo->GetBufferViewport().Top;

            pSelection->ExtendSelection(MousePosition);
        }
    }
}

void Scrolling::s_HandleMouseWheel(_In_ const UINT msg, _In_ const WPARAM wParam, PSCREEN_INFORMATION pScreenInfo)
{
    COORD NewOrigin;
    NewOrigin.X = pScreenInfo->GetBufferViewport().Left;
    NewOrigin.Y = pScreenInfo->GetBufferViewport().Top;

    // s_ucWheelScrollLines == 0 means that it is turned off.
    if ((msg == WM_MOUSEWHEEL) && (s_ucWheelScrollLines > 0))
    {
        // Rounding could cause this to be zero if gucWSL is bigger than 240 or so.
        ULONG const ulActualDelta = max(WHEEL_DELTA / s_ucWheelScrollLines, 1);

        // If we change direction we need to throw away any remainder we may have in the other direction.
        if ((pScreenInfo->WheelDelta > 0) == ((short)HIWORD(wParam) > 0))
        {
            pScreenInfo->WheelDelta += (short)HIWORD(wParam);
        }
        else
        {
            pScreenInfo->WheelDelta = (short)HIWORD(wParam);
        }

        if ((ULONG)abs(pScreenInfo->WheelDelta) >= ulActualDelta)
        {
            /*
            * By default, SHIFT + WM_MOUSEWHEEL will scroll 1/2 the
            * screen size. A ScrollScale of 1 indicates 1/2 the screen
            * size. This value can be modified in the registry.
            */
            SHORT delta = 1;
            if (wParam & MK_SHIFT)
            {
                delta = (SHORT)max((pScreenInfo->GetScreenWindowSizeY() * pScreenInfo->ScrollScale) / 2, 1);

                // Account for scroll direction changes by adjusting delta if there was a direction change.
                delta *= (pScreenInfo->WheelDelta < 0 ? -1 : 1);
                pScreenInfo->WheelDelta %= delta;
            }
            else
            {
                delta *= (pScreenInfo->WheelDelta / (short)ulActualDelta);
                pScreenInfo->WheelDelta %= ulActualDelta;
            }

            NewOrigin.Y -= delta;
            if (NewOrigin.Y < 0)
            {
                NewOrigin.Y = 0;
            }
            else if (NewOrigin.Y + pScreenInfo->GetScreenWindowSizeY() > pScreenInfo->ScreenBufferSize.Y)
            {
                NewOrigin.Y = pScreenInfo->ScreenBufferSize.Y - pScreenInfo->GetScreenWindowSizeY();
            }
            pScreenInfo->SetViewportOrigin(TRUE, NewOrigin);
        }
    }
    else if ((msg == WM_MOUSEHWHEEL) && (s_ucWheelScrollChars > 0))
    {
        ULONG const ulActualDelta = max(WHEEL_DELTA / s_ucWheelScrollChars, 1);

        if ((pScreenInfo->HWheelDelta > 0) == ((short)HIWORD(wParam) > 0))
        {
            pScreenInfo->HWheelDelta += (short)HIWORD(wParam);
        }
        else
        {
            pScreenInfo->HWheelDelta = (short)HIWORD(wParam);
        }

        if ((ULONG)abs(pScreenInfo->HWheelDelta) >= ulActualDelta)
        {
            SHORT delta = 1;

            if (wParam & MK_SHIFT)
            {
                delta = max(pScreenInfo->GetScreenWindowSizeX() - 1, 1);
            }

            delta *= (pScreenInfo->HWheelDelta / (short)ulActualDelta);
            pScreenInfo->HWheelDelta %= ulActualDelta;

            NewOrigin.X += delta;
            if (NewOrigin.X < 0)
            {
                NewOrigin.X = 0;
            }
            else if (NewOrigin.X + pScreenInfo->GetScreenWindowSizeX() > pScreenInfo->ScreenBufferSize.X)
            {
                NewOrigin.X = pScreenInfo->ScreenBufferSize.X - pScreenInfo->GetScreenWindowSizeX();
            }

            pScreenInfo->SetViewportOrigin(TRUE, NewOrigin);
        }
    }
}

bool Scrolling::s_HandleKeyScrollingEvent(_In_ const INPUT_KEY_INFO* const pKeyInfo)
{
    Window* const pWindow = g_ciConsoleInformation.pWindow;

    const WORD VirtualKeyCode = pKeyInfo->GetVirtualKey();
    const bool fIsCtrlPressed = pKeyInfo->IsCtrlPressed();
    const bool fIsEditLineEmpty = CommandLine::Instance().IsEditLineEmpty();

    // If escape, enter or ctrl-c, cancel scroll.
    if (VirtualKeyCode == VK_ESCAPE ||
        VirtualKeyCode == VK_RETURN ||
        (VirtualKeyCode == 'C' && fIsCtrlPressed))
    {
        Scrolling::s_ClearScroll();
    }
    else
    {
        WORD ScrollCommand;
        BOOL Horizontal = FALSE;
        switch (VirtualKeyCode)
        {
        case VK_UP:
        {
            ScrollCommand = SB_LINEUP;
            break;
        }
        case VK_DOWN:
        {
            ScrollCommand = SB_LINEDOWN;
            break;
        }
        case VK_LEFT:
        {
            ScrollCommand = SB_LINEUP;
            Horizontal = TRUE;
            break;
        }
        case VK_RIGHT:
        {
            ScrollCommand = SB_LINEDOWN;
            Horizontal = TRUE;
            break;
        }
        case VK_NEXT:
        {
            ScrollCommand = SB_PAGEDOWN;
            break;
        }
        case VK_PRIOR:
        {
            ScrollCommand = SB_PAGEUP;
            break;
        }
        case VK_END:
        {
            if (fIsCtrlPressed)
            {
                if (fIsEditLineEmpty)
                {
                    // Ctrl-End when edit line is empty will scroll to last line in the buffer.
                    g_ciConsoleInformation.CurrentScreenBuffer->MakeCurrentCursorVisible();
                    return true;
                }
                else
                {
                    // If edit line is non-empty, we won't handle this so it can modify the edit line (trim characters to end of line from cursor pos).
                    return false;
                }
            }
            else
            {
                ScrollCommand = SB_PAGEDOWN;
                Horizontal = TRUE;
            }
            break;
        }
        case VK_HOME:
        {
            if (fIsCtrlPressed)
            {
                if (fIsEditLineEmpty)
                {
                    // Ctrl-Home when edit line is empty will scroll to top of buffer.
                    ScrollCommand = SB_TOP;
                }
                else
                {
                    // If edit line is non-empty, we won't handle this so it can modify the edit line (trim characters to beginning of line from cursor pos).
                    return false;
                }
            }
            else
            {
                ScrollCommand = SB_PAGEUP;
                Horizontal = TRUE;
            }
            break;
        }
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        {
            return true;
        }
        default:
        {
            pWindow->SendNotifyBeep();
            return true;
        }
        }

        if (Horizontal)
        {
            pWindow->HorizontalScroll(ScrollCommand, 0);
        }
        else
        {
            pWindow->VerticalScroll(ScrollCommand, 0);
        }
    }

    return true;
}
