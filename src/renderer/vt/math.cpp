/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Gets the size in characters of the current dirty portion of the frame.
// Arguments:
// - <none>
// Return Value:
// - The character dimensions of the current dirty area of the frame.
SMALL_RECT VtEngine::GetDirtyRectInChars()
{
    return{ 0 };
}

// Routine Description:
// - Uses the currently selected font to determine how wide the given character will be when renderered.
// - NOTE: Only supports determining half-width/full-width status for CJK-type languages (e.g. is it 1 character wide or 2. a.k.a. is it a rectangle or square.)
// Arguments:
// - wch - Character to check
// Return Value:
// - True if it is full-width (2 wide). False if it is half-width (1 wide).
bool VtEngine::IsCharFullWidthByFont(_In_ WCHAR const wch)
{
    wch;
    return false;
}
