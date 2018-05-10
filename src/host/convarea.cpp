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

bool InsertConvertedString(_In_ LPCWSTR lpStr)
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

        while (*lpStr)
        {
            keyEvent.SetCharData(*lpStr);
            inEvents.push_back(std::make_unique<KeyEvent>(keyEvent));

            ++lpStr;
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
NTSTATUS ConsoleImeCompStr(_In_ LPCONIME_UICOMPMESSAGE CompStr)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const Cursor& cursor = gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    if (CompStr->dwCompStrLen == 0 || CompStr->dwResultStrLen != 0)
    {
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
        
        if (CompStr->dwResultStrLen != 0)
        {
            #pragma prefast(suppress:26035, "CONIME_UICOMPMESSAGE structure impossible for PREfast to trace due to its structure.")
            if (!InsertConvertedString((LPCWSTR) ((PBYTE) CompStr + CompStr->dwResultStrOffset)))
            {
                return STATUS_INVALID_HANDLE;
            }
        }

        if (pIme->CompStrData)
        {
            delete[] pIme->CompStrData;
            pIme->CompStrData = nullptr;
        }
    }
    else
    {
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

        try
        {
            pIme->WriteCompMessage(CompStr);
        }
        CATCH_LOG();
    }

    return STATUS_SUCCESS;
}

[[nodiscard]]
NTSTATUS ConsoleImeResizeCompStrView()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    // Compositon string
    LPCONIME_UICOMPMESSAGE const CompStr = pIme->CompStrData;
    if (CompStr)
    {
        try
        {
            pIme->WriteCompMessage(CompStr);
        }
        CATCH_LOG();
    }

    return STATUS_SUCCESS;
}

[[nodiscard]]
HRESULT ConsoleImeResizeCompStrScreenBuffer(const COORD coordNewScreenSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ConsoleImeInfo* const pIme = &gci.ConsoleIme;

    return pIme->ResizeAllAreas(coordNewScreenSize);
}

// Routine Description:
// - This routine handle WM_COPYDATA message.
// Arguments:
// - Console - Pointer to console information structure.
// - wParam -
// - lParam -
// Return Value:
[[nodiscard]]
NTSTATUS ImeControl(_In_ PCOPYDATASTRUCT pCopyDataStruct)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (pCopyDataStruct == nullptr)
    {
        // fail safe.
        return STATUS_SUCCESS;
    }

    switch ((LONG) pCopyDataStruct->dwData)
    {
        case CI_CONIMECOMPOSITION:
            if (pCopyDataStruct->cbData >= sizeof(CONIME_UICOMPMESSAGE))
            {
                LPCONIME_UICOMPMESSAGE CompStr;

                CompStr = (LPCONIME_UICOMPMESSAGE) pCopyDataStruct->lpData;
                if (CompStr && CompStr->dwSize == pCopyDataStruct->cbData)
                {
                    if (gci.ConsoleIme.CompStrData)
                    {
                        delete[] gci.ConsoleIme.CompStrData;
                    }

                    gci.ConsoleIme.CompStrData = (LPCONIME_UICOMPMESSAGE) new(std::nothrow) BYTE[CompStr->dwSize];
                    if (gci.ConsoleIme.CompStrData == nullptr)
                    {
                        break;
                    }

                    memmove(gci.ConsoleIme.CompStrData, CompStr, CompStr->dwSize);
                    LOG_IF_FAILED(ConsoleImeCompStr(gci.ConsoleIme.CompStrData));
                }
            }
            break;
        case CI_ONSTARTCOMPOSITION:
            gci.pInputBuffer->fInComposition = true;
            break;
        case CI_ONENDCOMPOSITION:
            gci.pInputBuffer->fInComposition = false;
            break;
    }

    return STATUS_SUCCESS;
}
