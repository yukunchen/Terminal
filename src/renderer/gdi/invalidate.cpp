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
// - Notifies us that the system has requested a particular pixel area of the client rectangle should be redrawn. (On WM_PAINT)
// Arguments:
// - prcDirtyClient - Pointer to pixel area (RECT) of client region the system believes is dirty
// Return Value:
// - <none>
void GdiEngine::InvalidateSystem(_In_ const RECT* const prcDirtyClient)
{
    _InvalidCombine(prcDirtyClient);
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the console would like us to move while scrolling.
// Return Value:
// - <none>
void GdiEngine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    if (pcoordDelta->X != 0 ||
        pcoordDelta->Y != 0)
    {
        POINT const ptDelta = _ScaleByFont(pcoordDelta);

        _InvalidOffset(&ptDelta);
        _szInvalidScroll.cx += ptDelta.x;
        _szInvalidScroll.cy += ptDelta.y;
    }
}

// Routine Description:
// - Notifies us that the console has changed the selection region and would like it updated
// Arguments:
// - rgsrSelection - Array of character region rectangles (one per line) that represent the selected area
// - cRectangles - Length of the array above.
// Return Value:
// - <none>
void GdiEngine::InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    // Get the currently selected area as a GDI region
    HRGN hrgnSelection = CreateRectRgn(0, 0, 0, 0);
    NTSTATUS status = NT_TESTNULL_GLE(hrgnSelection);
    if (NT_SUCCESS(status))
    {
        status = _PaintSelectionCalculateRegion(rgsrSelection, cRectangles, hrgnSelection);

        // XOR against the region we saved from the last time we rendered to find out what to invalidate
        // This is the space that needs to be inverted to either select or deselect the existing region into the new one.
        HRGN hrgnInvalid = CreateRectRgn(0, 0, 0, 0);
        status = NT_TESTNULL_GLE(hrgnInvalid);
        if (NT_SUCCESS(status))
        {
            int const iCombineResult = CombineRgn(hrgnInvalid, _hrgnGdiPaintedSelection, hrgnSelection, RGN_XOR);

            if (iCombineResult != NULLREGION && iCombineResult != ERROR)
            {
                // Invalidate that.
                _InvalidateRgn(hrgnInvalid);
            }

            DeleteObject(hrgnInvalid);
        }

        DeleteObject(hrgnSelection);
    }
}

// Routine Description:
// - Notifies us that the console has changed the character region specified.
// - NOTE: This typically triggers on cursor or text buffer changes
// Arguments:
// - psrRegion - Character region (SMALL_RECT) that has been changed
// Return Value:
// - <none>
void GdiEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    RECT const rcRegion = _ScaleByFont(psrRegion);
    _InvalidateRect(&rcRegion);
}

// Routine Description:
// - Notifies to repaint everything.
// - NOTE: Use sparingly. Only use when something that could affect the entire frame simultaneously occurs.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::InvalidateAll()
{
    RECT rc;
    if (FALSE != GetClientRect(_hwndTargetWindow, &rc))
    {
        InvalidateSystem(&rc);
    }
}

// Routine Description:
// - Helper to combine the given rectangle into the invalid region to be updated on the next paint
// Arguments:
// - prc - Pixel region (RECT) that should be repainted on the next frame
// Return Value:
// - <none>
void GdiEngine::_InvalidCombine(_In_ const RECT* const prc)
{
    if (!_fInvalidRectUsed)
    {
        _rcInvalid = *prc;
        _fInvalidRectUsed = true;
    }
    else
    {
        _OrRect(&_rcInvalid, prc);
    }

    // Ensure invalid areas remain within bounds of window.
    _InvalidRestrict();
}

// Routine Description:
// - Helper to adjust the invalid region by the given offset such as when a scroll operation occurs.
// Arguments:
// - ppt - Distances by which we should move the invalid region in response to a scroll
// Return Value:
// - <none>
void GdiEngine::_InvalidOffset(_In_ const POINT* const ppt)
{
    if (_fInvalidRectUsed)
    {
        _rcInvalid.left += ppt->x;
        _rcInvalid.right += ppt->x;
        _rcInvalid.top += ppt->y;
        _rcInvalid.bottom += ppt->y;

        // Ensure invalid areas remain within bounds of window.
        _InvalidRestrict();
    }
}

// Routine Description:
// - Helper to ensure the invalid region remains within the bounds of the window.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::_InvalidRestrict()
{
    // Ensure that the invalid area remains within the bounds of the client area
    RECT rcClient;
    GetClientRect(_hwndTargetWindow, &rcClient);

    _rcInvalid.left = max(_rcInvalid.left, rcClient.left);
    _rcInvalid.right = min(_rcInvalid.right, rcClient.right);
    _rcInvalid.top = max(_rcInvalid.top, rcClient.top);
    _rcInvalid.bottom = min(_rcInvalid.bottom, rcClient.bottom);
}

// Routine Description:
// - Helper to add a pixel rectangle to the invalid area
// Arguments:
// - prc - Pointer to pixel rectangle representing invalid area to add to next paint frame
// Return Value:
// - <none>
void GdiEngine::_InvalidateRect(_In_ const RECT* const prc)
{
    _InvalidCombine(prc);
}

// Routine Description:
// - Helper to add a pixel region to the invalid area
// Arguments:
// - hrgn - Handle to pixel region representing invalid area to add to next paint frame
// Return Value:
// - <none>
void GdiEngine::_InvalidateRgn(_In_ HRGN hrgn)
{
    RECT rcInvalid;
    GetRgnBox(hrgn, &rcInvalid);
    _InvalidateRect(&rcInvalid);
}
