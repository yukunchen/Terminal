/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConsoleWindow.hpp"

#include "..\inc\ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity::OneCore;

BOOL ConsoleWindow::EnableBothScrollBars()
{
    return FALSE;
}

int ConsoleWindow::UpdateScrollBar(bool isVertical, bool isAltBuffer, UINT pageSize, int maxSize, int viewportPosition)
{
    UNREFERENCED_PARAMETER(isVertical);
    UNREFERENCED_PARAMETER(isAltBuffer);
    UNREFERENCED_PARAMETER(pageSize);
    UNREFERENCED_PARAMETER(maxSize);
    UNREFERENCED_PARAMETER(viewportPosition);

    return 0;
}

bool ConsoleWindow::IsInFullscreen() const
{
    return true;
}

void ConsoleWindow::SetIsFullscreen(bool const fFullscreenEnabled)
{
    UNREFERENCED_PARAMETER(fFullscreenEnabled);
}

NTSTATUS ConsoleWindow::SetViewportOrigin(SMALL_RECT NewWindow)
{
    UNREFERENCED_PARAMETER(NewWindow);

    SCREEN_INFORMATION* const ScreenInfo = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
    COORD const FontSize = ScreenInfo->GetScreenFontSize();

    Selection* pSelection = &Selection::Instance();
    pSelection->HideSelection();

    ScreenInfo->SetBufferViewport(NewWindow);

    if (ServiceLocator::LocateGlobals()->pRender != nullptr)
    {
        ServiceLocator::LocateGlobals()->pRender->TriggerScroll();
    }

    pSelection->ShowSelection();

    ScreenInfo->UpdateScrollBars();
    return STATUS_SUCCESS;
}

void ConsoleWindow::SetWindowHasMoved(BOOL const fHasMoved)
{
    UNREFERENCED_PARAMETER(fHasMoved);
}

void ConsoleWindow::CaptureMouse()
{
}

BOOL ConsoleWindow::ReleaseMouse()
{
    return TRUE;
}

HWND ConsoleWindow::GetWindowHandle() const
{
    return nullptr;
}

void ConsoleWindow::SetOwner()
{
}

BOOL ConsoleWindow::GetCursorPosition(LPPOINT lpPoint)
{
    UNREFERENCED_PARAMETER(lpPoint);

    return FALSE;
}

BOOL ConsoleWindow::GetClientRectangle(LPRECT lpRect)
{
    UNREFERENCED_PARAMETER(lpRect);

    return FALSE;
}

int ConsoleWindow::MapPoints(LPPOINT lpPoints, UINT cPoints)
{
    UNREFERENCED_PARAMETER(lpPoints);
    UNREFERENCED_PARAMETER(cPoints);

    return 0;
}

BOOL ConsoleWindow::ConvertScreenToClient(LPPOINT lpPoint)
{
    UNREFERENCED_PARAMETER(lpPoint);

    return 0;
}

BOOL ConsoleWindow::SendNotifyBeep() const
{
    return Beep(800, 200);
}

BOOL ConsoleWindow::PostUpdateScrollBars() const
{
    return FALSE;
}

BOOL ConsoleWindow::PostUpdateTitleWithCopy(const PCWSTR pwszNewTitle) const
{
    UNREFERENCED_PARAMETER(pwszNewTitle);

    return TRUE;
}

BOOL ConsoleWindow::PostUpdateWindowSize() const
{
    return FALSE;
}

void ConsoleWindow::UpdateWindowSize(COORD const coordSizeInChars) const
{
    UNREFERENCED_PARAMETER(coordSizeInChars);
}

void ConsoleWindow::UpdateWindowText()
{
}

void ConsoleWindow::HorizontalScroll(const WORD wScrollCommand, const WORD wAbsoluteChange) const
{
    UNREFERENCED_PARAMETER(wScrollCommand);
    UNREFERENCED_PARAMETER(wAbsoluteChange);
}

void ConsoleWindow::VerticalScroll(const WORD wScrollCommand, const WORD wAbsoluteChange) const
{
    UNREFERENCED_PARAMETER(wScrollCommand);
    UNREFERENCED_PARAMETER(wAbsoluteChange);
}

HRESULT ConsoleWindow::SignalUia(_In_ EVENTID id)
{
    UNREFERENCED_PARAMETER(id);
    return E_NOTIMPL;
}

RECT ConsoleWindow::GetWindowRect() const
{
    RECT rc = {0};
    return rc;
}
