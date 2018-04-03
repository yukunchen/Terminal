/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "scrolling.hpp"

#include "selection.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

ULONG Scrolling::s_ucWheelScrollLines = 0;
ULONG Scrolling::s_ucWheelScrollChars = 0;

void Scrolling::s_UpdateSystemMetrics()
{
    s_ucWheelScrollLines = ServiceLocator::LocateSystemConfigurationProvider()->GetNumberOfWheelScrollLines();
    s_ucWheelScrollChars = ServiceLocator::LocateSystemConfigurationProvider()->GetNumberOfWheelScrollCharacters();
}

bool Scrolling::s_IsInScrollMode()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return IsFlagSet(gci.Flags, CONSOLE_SCROLLING);
}

void Scrolling::s_DoScroll()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    if (!s_IsInScrollMode())
    {
        // clear any selection we may have -- can't scroll and select at the same time
        Selection::Instance().ClearSelection();

        SetFlag(gci.Flags, CONSOLE_SCROLLING);

        if (pWindow != nullptr)
        {
            pWindow->UpdateWindowText();
        }
    }
}

void Scrolling::s_ClearScroll()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    ClearFlag(gci.Flags, CONSOLE_SCROLLING);
    if (pWindow != nullptr)
    {
        pWindow->UpdateWindowText();
    }
}

void Scrolling::s_ScrollIfNecessary(_In_ const SCREEN_INFORMATION * const pScreenInfo)
{
    IConsoleWindow *pWindow = ServiceLocator::LocateConsoleWindow();
    ASSERT(pWindow);

    Selection* const pSelection = &Selection::Instance();

    if (pSelection->IsInSelectingState() && pSelection->IsMouseButtonDown())
    {
        POINT CursorPos;
        if (!pWindow->GetCursorPosition(&CursorPos))
        {
            return;
        }

        RECT ClientRect;
        if (!pWindow->GetClientRectangle(&ClientRect))
        {
            return;
        }

        pWindow->MapPoints((LPPOINT) & ClientRect, 2);
        if (!(s_IsPointInRectangle(&ClientRect, CursorPos)))
        {
            pWindow->ConvertScreenToClient(&CursorPos);

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

void Scrolling::s_HandleMouseWheel(_In_ bool isMouseWheel, _In_ bool isMouseHWheel, _In_ short wheelDelta, _In_ bool hasShift, _In_ PSCREEN_INFORMATION pScreenInfo)
{
    COORD NewOrigin;
    NewOrigin.X = pScreenInfo->GetBufferViewport().Left;
    NewOrigin.Y = pScreenInfo->GetBufferViewport().Top;

    // s_ucWheelScrollLines == 0 means that it is turned off.
    if (isMouseWheel && s_ucWheelScrollLines > 0)
    {
        // Rounding could cause this to be zero if gucWSL is bigger than 240 or so.
        ULONG const ulActualDelta = std::max(WHEEL_DELTA / s_ucWheelScrollLines, 1ul);

        // If we change direction we need to throw away any remainder we may have in the other direction.
        if ((pScreenInfo->WheelDelta > 0) == (wheelDelta > 0))
        {
            pScreenInfo->WheelDelta += wheelDelta;
        }
        else
        {
            pScreenInfo->WheelDelta = wheelDelta;
        }

        if ((ULONG)abs(pScreenInfo->WheelDelta) >= ulActualDelta)
        {
            /*
            * By default, SHIFT + WM_MOUSEWHEEL will scroll 1/2 the
            * screen size. A ScrollScale of 1 indicates 1/2 the screen
            * size. This value can be modified in the registry.
            */
            SHORT delta = 1;
            if (hasShift)
            {
                delta = gsl::narrow<SHORT>(std::max((pScreenInfo->GetScreenWindowSizeY() * pScreenInfo->ScrollScale) / 2, 1u));

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
            const COORD coordBufferSize = pScreenInfo->GetScreenBufferSize();
            if (NewOrigin.Y < 0)
            {
                NewOrigin.Y = 0;
            }
            else if (NewOrigin.Y + pScreenInfo->GetScreenWindowSizeY() > coordBufferSize.Y)
            {
                NewOrigin.Y = coordBufferSize.Y - pScreenInfo->GetScreenWindowSizeY();
            }
            LOG_IF_FAILED(pScreenInfo->SetViewportOrigin(TRUE, NewOrigin));
        }
    }
    else if (isMouseHWheel && s_ucWheelScrollChars > 0)
    {
        ULONG const ulActualDelta = std::max(WHEEL_DELTA / s_ucWheelScrollChars, 1ul);

        if ((pScreenInfo->HWheelDelta > 0) == (wheelDelta > 0))
        {
            pScreenInfo->HWheelDelta += wheelDelta;
        }
        else
        {
            pScreenInfo->HWheelDelta = wheelDelta;
        }

        if ((ULONG)abs(pScreenInfo->HWheelDelta) >= ulActualDelta)
        {
            SHORT delta = 1;

            if (hasShift)
            {
                delta = gsl::narrow<SHORT>(std::max(pScreenInfo->GetScreenWindowSizeX() - 1, 1));
            }

            delta *= (pScreenInfo->HWheelDelta / (short)ulActualDelta);
            pScreenInfo->HWheelDelta %= ulActualDelta;

            NewOrigin.X += delta;
            const COORD coordBufferSize = pScreenInfo->GetScreenBufferSize();
            if (NewOrigin.X < 0)
            {
                NewOrigin.X = 0;
            }
            else if (NewOrigin.X + pScreenInfo->GetScreenWindowSizeX() > coordBufferSize.X)
            {
                NewOrigin.X = coordBufferSize.X - pScreenInfo->GetScreenWindowSizeX();
            }

            LOG_IF_FAILED(pScreenInfo->SetViewportOrigin(TRUE, NewOrigin));
        }
    }
}

bool Scrolling::s_HandleKeyScrollingEvent(_In_ const INPUT_KEY_INFO* const pKeyInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    IConsoleWindow *pWindow = ServiceLocator::LocateConsoleWindow();
    ASSERT(pWindow);

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
                    gci.CurrentScreenBuffer->MakeCurrentCursorVisible();
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

BOOL Scrolling::s_IsPointInRectangle(_In_ CONST RECT * prc, _In_ POINT pt)
{
    return ((pt.x >= prc->left) && (pt.x < prc->right) &&
            (pt.y >= prc->top) && (pt.y < prc->bottom));
}
