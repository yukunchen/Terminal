/*++
Copyright (c) Microsoft Corporation

Module Name:
- scrolling.hpp

Abstract:
- This module is used for managing the scrolling state and process

Author(s):
- Michael Niksa (MiNiksa) 4-Jun-2014
- Paul Campbell (PaulCam) 4-Jun-2014

Revision History:
- From components of clipbrd.h/.c and input.h/.c
--*/

#pragma once

#include "input.h"

// TODO: Static methods generally mean they're getting their state globally and not from this object _yet_
class Scrolling
{
public:
    static void s_UpdateSystemMetrics();

    // legacy methods (not refactored yet)
    static bool s_IsInScrollMode();
    static void s_DoScroll();
    static void s_ClearScroll();
    static void s_ScrollIfNecessary(_In_ const SCREEN_INFORMATION * const pScreenInfo);
    static void s_HandleMouseWheel(_In_ bool isMouseWheel, _In_ bool isMouseHWheel, _In_ short wheelDelta, _In_ bool hasShift, _In_ PSCREEN_INFORMATION pScreenInfo);
    static bool s_HandleKeyScrollingEvent(_In_ const INPUT_KEY_INFO* const pKeyInfo);

private:
    static BOOL s_IsPointInRectangle(_In_ CONST RECT *prc, _In_ POINT pt);

    static ULONG s_ucWheelScrollLines;
    static ULONG s_ucWheelScrollChars;
};
