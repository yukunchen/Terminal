/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "AccessibilityNotifier.hpp"

using namespace Microsoft::Console::Interactivity::OneCore;

void AccessibilityNotifier::NotifyConsoleCaretEvent(_In_ RECT rectangle)
{
    UNREFERENCED_PARAMETER(rectangle);
}

void AccessibilityNotifier::NotifyConsoleCaretEvent(_In_ ConsoleCaretEventFlags flags, _In_ LONG position)
{
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(position);
}

void AccessibilityNotifier::NotifyConsoleUpdateScrollEvent(_In_ LONG x, _In_ LONG y)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
}

void AccessibilityNotifier::NotifyConsoleUpdateSimpleEvent(_In_ LONG start, _In_ LONG charAndAttribute)
{
    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(charAndAttribute);
}

void AccessibilityNotifier::NotifyConsoleUpdateRegionEvent(_In_ LONG startXY, _In_ LONG endXY)
{
    UNREFERENCED_PARAMETER(startXY);
    UNREFERENCED_PARAMETER(endXY);
}

void AccessibilityNotifier::NotifyConsoleLayoutEvent()
{
}

void AccessibilityNotifier::NotifyConsoleStartApplicationEvent(_In_ DWORD processId)
{
    UNREFERENCED_PARAMETER(processId);
}

void AccessibilityNotifier::NotifyConsoleEndApplicationEvent(_In_ DWORD processId)
{
    UNREFERENCED_PARAMETER(processId);
}
