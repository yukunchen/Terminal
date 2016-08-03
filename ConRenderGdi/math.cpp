/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "gdirenderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Gets the size in characters of the current dirty portion of the frame.
// Arguments:
// - <none>
// Return Value:
// - The character dimensions of the current dirty area of the frame.
SMALL_RECT GdiEngine::GetDirtyRectInChars()
{
    RECT rc = _psInvalidData.rcPaint;

    SMALL_RECT sr = _ScaleByFont(&rc);

    return sr;
}

// Routine Description:
// - Uses the currently selected font to determine how wide the given character will be when renderered.
// - NOTE: Only supports determining half-width/full-width status for CJK-type languages (e.g. is it 1 character wide or 2. a.k.a. is it a rectangle or square.)
// Arguments:
// - wch - Character to check
// Return Value:
// - True if it is full-width (2 wide). False if it is half-width (1 wide).
bool GdiEngine::IsCharFullWidthByFont(_In_ WCHAR const wch)
{
    bool isFullWidth = false;

    
    if (_IsFontTrueType())
    {
        ABC abc;
        if (GetCharABCWidthsW(_hdcMemoryContext, wch, wch, &abc))
        {
            int const totalWidth = abc.abcB;

            isFullWidth = totalWidth > _GetFontSize().X;
        }
    }
    else
    {
        INT cpxWidth = 0;
        if (GetCharWidth32W(_hdcMemoryContext, wch, wch, &cpxWidth))
        {
            isFullWidth = cpxWidth > _GetFontSize().X;
        }
    }

    return isFullWidth;
}

// Routine Description:
// - Scales a character region (SMALL_RECT) into a pixel region (RECT) by the current font size.
// Arguments:
// - Character region (SMALL_RECT) from the console text buffer.
// Return Value:
// - Pixel region (RECT) for drawing to the client surface.
RECT GdiEngine::_ScaleByFont(const SMALL_RECT* const psr) const
{
    COORD const coordFontSize = _GetFontSize();
    RECT rc;

    rc.left = psr->Left * coordFontSize.X;
    rc.right = psr->Right * coordFontSize.X;
    rc.top = psr->Top * coordFontSize.Y;
    rc.bottom = psr->Bottom * coordFontSize.Y;

    return rc;
}

// Routine Description:
// - Scales a character coordinate (COORD) into a pixel coordinate (POINT) by the current font size.
// Arguments:
// - Character coordinate (COORD) from the console text buffer.
// Return Value:
// - Pixel coordinate (POINT) for drawing to the client surface.
POINT GdiEngine::_ScaleByFont(const COORD* const pcoord) const
{
    COORD const coordFontSize = _GetFontSize();
    POINT pt;

    pt.x = pcoord->X * coordFontSize.X;
    pt.y = pcoord->Y * coordFontSize.Y;

    return pt;
}

// Routine Description:
// - Scales a pixel region (RECT) into a character region (SMALL_RECT) by the current font size.
// Arguments:
// - Pixel region (RECT) from drawing to the client surface.
// Return Value:
// - Character region (SMALL_RECT) from the console text buffer.
SMALL_RECT GdiEngine::_ScaleByFont(const RECT* const prc) const
{
    COORD const coordFontSize = _GetFontSize();

    SMALL_RECT sr;
    sr.Left = static_cast<SHORT>(prc->left / coordFontSize.X);
    sr.Top = static_cast<SHORT>(prc->top / coordFontSize.Y);
    sr.Right = static_cast<SHORT>(prc->right / coordFontSize.X);
    sr.Bottom = static_cast<SHORT>(prc->bottom / coordFontSize.Y);

    // Pixels are exclusive and character rects are inclusive.
    sr.Right--;
    sr.Bottom--;

    return sr;
}

// Routine Description:
// - Scales the given pixel measurement up from the typical system DPI (generally 96) to whatever the given DPI is.
// Arguments:
// - iPx - Pixel length measurement.
// - iDpi - Given DPI scalar value
// Return Value:
// - Pixel measurement scaled against the given DPI scalar.
int GdiEngine::s_ScaleByDpi(_In_ const int iPx, _In_ const int iDpi)
{
    return MulDiv(iPx, iDpi, s_iBaseDpi);
}

// Routine Description:
// - Shrinks the given pixel measurement down from whatever the given DPI is to the typical system DPI (generally 96).
// Arguments:
// - iPx - Pixel measurement scaled against the given DPI.
// - iDpi - Given DPI for pixel scaling
// Return Value:
// - Pixel length measurement.
int GdiEngine::s_ShrinkByDpi(_In_ const int iPx, _In_ const int iDpi)
{
    return MulDiv(iPx, s_iBaseDpi, iDpi);
}

// Routine Description:
// - Uses internal invalid structure to determine the top left pixel point of the invalid frame to be painted.
// Arguments:
// - <none>
// Return Value:
// - Top left corner in pixels of where to start repainting the frame.
POINT GdiEngine::_GetInvalidRectPoint() const
{
    POINT pt;
    pt.x = _psInvalidData.rcPaint.left;
    pt.y = _psInvalidData.rcPaint.top;

    return pt;
}

// Routine Description:
// - Uses internal invalid structure to determine the size of the invalid area of the frame to be painted.
// Arguments:
// - <none>
// Return Value:
// - Width and height in pixels of the invalid area of the frame.
SIZE GdiEngine::_GetInvalidRectSize() const
{
    return _GetRectSize(&_psInvalidData.rcPaint);
}

// Routine Description:
// - Converts a pixel region (RECT) into its width/height (SIZE)
// Arguments:
// - Pixel region (RECT)
// Return Value:
// - Pixel dimensions (SIZE)
SIZE GdiEngine::_GetRectSize(_In_ const RECT* const pRect) const
{
    SIZE sz;
    sz.cx = pRect->right - pRect->left;
    sz.cy = pRect->bottom - pRect->top;

    return sz;
}

// Routine Description:
// - Performs a "CombineRect" with the "OR" operation.
// - Basically extends the existing rect outward to also encompass the passed-in region.
// Arguments:
// - pRectExisting - Expand this rectangle to encompass the add rect.
// - pRectToOr - Add this rectangle to the existing one.
// Return Value:
// - <none>
void GdiEngine::_OrRect(_In_ RECT* const pRectExisting, _In_ const RECT* const pRectToOr) const
{
    pRectExisting->left = min(pRectExisting->left, pRectToOr->left);
    pRectExisting->top = min(pRectExisting->top, pRectToOr->top);
    pRectExisting->right = max(pRectExisting->right, pRectToOr->right);
    pRectExisting->bottom = max(pRectExisting->bottom, pRectToOr->bottom);
}

