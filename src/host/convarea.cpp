/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"

#include "../interactivity/inc/ServiceLocator.hpp"

#pragma hdrstop

bool IsValidSmallRect(_In_ PSMALL_RECT const Rect)
{
    return (Rect->Right >= Rect->Left && Rect->Bottom >= Rect->Top);
}

void WriteConvRegionToScreen(const SCREEN_INFORMATION& ScreenInfo,
                             const SMALL_RECT srConvRegion)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (!ScreenInfo.IsActiveScreenBuffer())
    {
        return;
    }

    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    for (unsigned int i = 0; i < pIme->ConvAreaCompStr.size(); ++i)
    {
        const auto& ConvAreaInfo = pIme->ConvAreaCompStr[i];

        if (!ConvAreaInfo.IsHidden())
        {
            const SMALL_RECT currentViewport = ScreenInfo.GetBufferViewport();
            // Do clipping region
            SMALL_RECT Region;
            Region.Left = currentViewport.Left + ConvAreaInfo.CaInfo.rcViewCaWindow.Left + ConvAreaInfo.CaInfo.coordConView.X;
            Region.Right = Region.Left + (ConvAreaInfo.CaInfo.rcViewCaWindow.Right - ConvAreaInfo.CaInfo.rcViewCaWindow.Left);
            Region.Top = currentViewport.Top + ConvAreaInfo.CaInfo.rcViewCaWindow.Top + ConvAreaInfo.CaInfo.coordConView.Y;
            Region.Bottom = Region.Top + (ConvAreaInfo.CaInfo.rcViewCaWindow.Bottom - ConvAreaInfo.CaInfo.rcViewCaWindow.Top);

            SMALL_RECT ClippedRegion;
            ClippedRegion.Left = std::max(Region.Left, currentViewport.Left);
            ClippedRegion.Top = std::max(Region.Top, currentViewport.Top);
            ClippedRegion.Right = std::min(Region.Right, currentViewport.Right);
            ClippedRegion.Bottom = std::min(Region.Bottom, currentViewport.Bottom);

            if (IsValidSmallRect(&ClippedRegion))
            {
                Region = ClippedRegion;
                ClippedRegion.Left = std::max(Region.Left, srConvRegion.Left);
                ClippedRegion.Top = std::max(Region.Top, srConvRegion.Top);
                ClippedRegion.Right = std::min(Region.Right, srConvRegion.Right);
                ClippedRegion.Bottom = std::min(Region.Bottom, srConvRegion.Bottom);
                if (IsValidSmallRect(&ClippedRegion))
                {
                    // if we have a renderer, we need to update.
                    // we've already confirmed (above with an early return) that we're on conversion areas that are a part of the active (visible/rendered) screen
                    // so send invalidates to those regions such that we're queried for data on the next frame and repainted.
                    if (ServiceLocator::LocateGlobals().pRender != nullptr)
                    {
                        // convert inclusive rectangle to exclusive rectangle
                        SMALL_RECT srExclusive = ClippedRegion;
                        srExclusive.Right++;
                        srExclusive.Bottom++;

                        ServiceLocator::LocateGlobals().pRender->TriggerRedraw(&srExclusive);
                    }
                }
            }
        }
    }
}

bool InsertConvertedString(const std::wstring_view str)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    bool fResult = false;

    auto& screenInfo = gci.GetActiveOutputBuffer();
    if (screenInfo.GetTextBuffer().GetCursor().IsOn())
    {
        screenInfo.GetTextBuffer().GetCursor().TimerRoutine(screenInfo);
    }

    const DWORD dwControlKeyState = GetControlKeyState(0);
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> inEvents;
        KeyEvent keyEvent{ TRUE, // keydown
            1, // repeatCount
            0, // virtualKeyCode
            0, // virtualScanCode
            0, // charData
            dwControlKeyState }; // activeModifierKeys

        for (const auto& ch : str)
        {
            keyEvent.SetCharData(ch);
            inEvents.push_back(std::make_unique<KeyEvent>(keyEvent));
        }

        gci.pInputBuffer->Write(inEvents);

        fResult = true;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }

    return fResult;
}

[[nodiscard]]
HRESULT ConsoleImeResizeCompStrView()
{
    try
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        ConsoleImeInfo* const pIme = &gci.ConsoleIme;
        pIme->RedrawCompMessage();
    }
    CATCH_RETURN();

    return S_OK;
}

[[nodiscard]]
HRESULT ConsoleImeResizeCompStrScreenBuffer(const COORD coordNewScreenSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    return pIme->ResizeAllAreas(coordNewScreenSize);
}

[[nodiscard]]
HRESULT ImeStartComposition()
{
    auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.LockConsole();
    auto unlock = wil::scope_exit([&] { gci.UnlockConsole(); });

    gci.pInputBuffer->fInComposition = true;
    return S_OK;
}

[[nodiscard]]
HRESULT ImeEndComposition()
{
    auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.LockConsole();
    auto unlock = wil::scope_exit([&] { gci.UnlockConsole(); });

    gci.pInputBuffer->fInComposition = false;
    return S_OK;
}

[[nodiscard]]
HRESULT ImeComposeData(std::wstring_view text,
                       std::basic_string_view<BYTE> attributes,
                       std::basic_string_view<WORD> colorArray)
{
    try
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.LockConsole();
        auto unlock = wil::scope_exit([&] { gci.UnlockConsole(); });

        const Cursor& cursor = gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor();
        ConsoleImeInfo* const pIme = &gci.ConsoleIme;

        // Cursor turn OFF.
        if (cursor.IsVisible())
        {
            pIme->SavedCursorVisible = true;

            gci.GetActiveOutputBuffer().SetCursorInformation(
                cursor.GetSize(),
                FALSE,
                cursor.GetColor(),
                cursor.GetType()
            );
        }

        pIme->WriteCompMessage(text, attributes, colorArray);
    }
    CATCH_RETURN();
    return S_OK;
}

[[nodiscard]]
HRESULT ImeClearComposeData()
{
    try
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.LockConsole();
        auto unlock = wil::scope_exit([&] { gci.UnlockConsole(); });

        ConsoleImeInfo* const pIme = &gci.ConsoleIme;
        pIme->ClearAllAreas();
    }
    CATCH_RETURN();
    return S_OK;
}

[[nodiscard]]
HRESULT ImeComposeResult(std::wstring_view text)
{
    try
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.LockConsole();
        auto unlock = wil::scope_exit([&] { gci.UnlockConsole(); });

        const Cursor& cursor = gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor();
        ConsoleImeInfo* const pIme = &gci.ConsoleIme;

        // Cursor turn ON.
        if (pIme->SavedCursorVisible)
        {
            pIme->SavedCursorVisible = false;

            gci.GetActiveOutputBuffer().SetCursorInformation(
                cursor.GetSize(),
                TRUE,
                cursor.GetColor(),
                cursor.GetType()
            );

        }

        // Determine string.
        pIme->ClearAllAreas();

        if (!InsertConvertedString(text))
        {
            return E_HANDLE;
        }

        pIme->ClearComposition();
    }
    CATCH_RETURN();
    return S_OK;
}
