/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

// Routine Description:
// - Gets the size in characters of the current dirty portion of the frame.
// Arguments:
// - <none>
// Return Value:
// - The character dimensions of the current dirty area of the frame. 
//      This is an Inclusive rect.
SMALL_RECT VtEngine::GetDirtyRectInChars()
{
    return Viewport::FromExclusive(_srcInvalid).ToInclusive();
}

// Routine Description:
// - Uses the currently selected font to determine how wide the given character will be when renderered.
// - NOTE: Only supports determining half-width/full-width status for CJK-type languages (e.g. is it 1 character wide or 2. a.k.a. is it a rectangle or square.)
// Arguments:
// - wch - Character to check
// Return Value:
// - True if it is full-width (2 wide). False if it is half-width (1 wide).
bool VtEngine::IsCharFullWidthByFont(_In_ WCHAR const /*wch*/)
{
    return false;
}

// Routine Description:
// - Performs a "CombineRect" with the "OR" operation.
// - Basically extends the existing rect outward to also encompass the passed-in region.
// Arguments:
// - pRectExisting - Expand this rectangle to encompass the add rect.
// - pRectToOr - Add this rectangle to the existing one.
// Return Value:
// - <none>
void VtEngine::_OrRect(_Inout_ SMALL_RECT* const pRectExisting, _In_ const SMALL_RECT* const pRectToOr) const
{
    pRectExisting->Left = min(pRectExisting->Left, pRectToOr->Left);
    pRectExisting->Top = min(pRectExisting->Top, pRectToOr->Top);
    pRectExisting->Right = max(pRectExisting->Right, pRectToOr->Right);
    pRectExisting->Bottom = max(pRectExisting->Bottom, pRectToOr->Bottom);
}

// Method Description:
// - Returns true if the invalidated region indicates that we only need to 
//      simply print text from the current cursor position. This will prevent us
//      from sending extra VT set-up/tear down sequences (?12h/l) when all we 
//      need to do is print more text at the current cursor position.
// Arguments:
// - <none>
// Return Value:
// - true iff only the next character is invalid
bool VtEngine::_WillWriteSingleChar() const
{
    COORD currentCursor = _lastText;

    bool noScrollDelta = (_scrollDelta.X == 0 && _scrollDelta.Y == 0);

    bool invalidIsOneChar = (_srcInvalid.Bottom ==_srcInvalid.Top+1) &&
                            (_srcInvalid.Right == (_srcInvalid.Left+1));

    // Either the next character to the right or the immediately previous 
    //      character should follow this code path
    //      (The immediate previous character would suggest a backspace)
    bool invalidIsNext = (_srcInvalid.Top == _lastText.Y)
                         && (_srcInvalid.Left == _lastText.X);
    bool invalidIsLast = (_srcInvalid.Top == _lastText.Y)
                         && (_srcInvalid.Left == (_lastText.X-1));

    return noScrollDelta && invalidIsOneChar && (invalidIsNext || invalidIsLast);
}
