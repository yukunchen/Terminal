/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "WindowMetrics.hpp"
#include "ConIoSrvComm.hpp"

#include "..\..\renderer\wddmcon\wddmconrenderer.hpp"

#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

// Default metrics for when in headless mode.
#define HEADLESS_FONT_SIZE_WIDTH      (8)
#define HEADLESS_FONT_SIZE_HEIGHT     (12)
#define HEADLESS_DISPLAY_SIZE_WIDTH   (80)
#define HEADLESS_DISPLAY_SIZE_HEIGHT  (25)

using namespace Microsoft::Console::Interactivity::OneCore;

RECT WindowMetrics::GetMinClientRectInPixels()
{
    NTSTATUS Status;

    ConIoSrvComm *Server;

    COORD FontSize = { 1 };
    RECT DisplaySize = { 0 };
    CD_IO_FONT_SIZE FontSizeIoctl = { 0 };
    CD_IO_DISPLAY_SIZE DisplaySizeIoctl = { 0 };

    USHORT DisplayMode;

    // Fetch a reference to the Console IO Server.
    Server = ServiceLocator::LocateInputServices<ConIoSrvComm>();
    
    // Figure out what kind of display we are using.
    Status = Server->RequestGetDisplayMode(&DisplayMode);

    // Note on status propagation:
    //
    // The IWindowMetrics contract was extracted from the original methods in
    // the Win32 Window class, which have no failure modes. However, in the case
    // of their OneCore implementations, because getting this information
    // requires reaching out to the Console IO Server if display output occurs
    // via BGFX, there is a possibility of failure where the server may be
    // unreachable. As a result, Get[Max|Min]ClientRectInPixels call
    // SetLastError in their OneCore implementations to reflect whether their
    // return value is accurate.

    if (NT_SUCCESS(Status))
    {
        switch (DisplayMode)
        {
            case CIS_DISPLAY_MODE_BGFX:
            {
                // TODO: MSFT: 10916072 This requires switching to kernel mode and calling
                //       BgkGetConsoleState. The call's result can be cached, though that
                //       might be a problem for plugging/unplugging monitors or perhaps
                //       across KVM sessions.

                Status = Server->RequestGetDisplaySize(&DisplaySizeIoctl);

                if (NT_SUCCESS(Status))
                {
                    Status = Server->RequestGetFontSize(&FontSizeIoctl);

                    if (NT_SUCCESS(Status))
                    {
                        DisplaySize.top = 0;
                        DisplaySize.left = 0;
                        DisplaySize.bottom = DisplaySizeIoctl.Height;
                        DisplaySize.right = DisplaySizeIoctl.Width;

                        FontSize.X = (SHORT)FontSizeIoctl.Width;
                        FontSize.Y = (SHORT)FontSizeIoctl.Height;
                    }
                }
                else
                {
                    SetLastError(Status);
                }
            }
            break;

            case CIS_DISPLAY_MODE_DIRECTX:
            {
                WddmConEngine *WddmEngine = (WddmConEngine *)ServiceLocator::LocateGlobals()->pWddmconEngine;

                FontSize = WddmEngine->GetFontSize();
                DisplaySize = WddmEngine->GetDisplaySize();
            }
            break;

            case CIS_DISPLAY_MODE_NONE:
            {
                // When in headless mode and using EMS (Emergency Management
                // Services), ensure that the buffer isn't zero-sized.
                FontSize.X = HEADLESS_FONT_SIZE_WIDTH;
                FontSize.Y = HEADLESS_FONT_SIZE_HEIGHT;

                DisplaySize.top = 0;
                DisplaySize.left = 0;
                DisplaySize.right = HEADLESS_DISPLAY_SIZE_WIDTH;
                DisplaySize.bottom = HEADLESS_DISPLAY_SIZE_HEIGHT;
            }
            break;
        }

        // The result is expected to be in pixels, not rows/columns.
        DisplaySize.right *= FontSize.X;
        DisplaySize.bottom *= FontSize.Y;
    }
    else
    {
        SetLastError(Status);
    }

    return DisplaySize;
}

RECT WindowMetrics::GetMaxClientRectInPixels()
{
    // OneCore consoles only have one size and cannot be resized.
    return GetMinClientRectInPixels();
}
