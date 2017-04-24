/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "WindowMetrics.hpp"
#include "InputServer.hpp"

#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Interactivity::OneCore;

RECT WindowMetrics::GetMinClientRectInPixels()
{
    // TODO: MSFT: 10916072 This requires switching to kernel mode and calling
    //       BgkGetConsoleState. The call's result can be cached, though that
    //       might be a problem for plugging/unplugging monitors or perhaps
    //       across KVM sessions.

    NTSTATUS Status;

    CD_IO_DISPLAY_SIZE DisplaySize = { 0 };
    Status = ServiceLocator::LocateInputServices<InputServer>()->RequestGetDisplaySize(&DisplaySize);

    RECT rct = { 0 };
    rct.top = 0;
    rct.left = 0;
    rct.bottom = DisplaySize.Height;
    rct.right = DisplaySize.Width;

    return rct;
}

RECT WindowMetrics::GetMaxClientRectInPixels()
{
    // TODO: MSFT: 10916072 Same as above.

    NTSTATUS Status;

    CD_IO_DISPLAY_SIZE DisplaySize = { 0 };
    Status = ServiceLocator::LocateInputServices<InputServer>()->RequestGetDisplaySize(&DisplaySize);

    RECT rct = { 0 };
    rct.top = 0;
    rct.left = 0;
    rct.bottom = DisplaySize.Height;
    rct.right = DisplaySize.Width;

    return rct;
}
