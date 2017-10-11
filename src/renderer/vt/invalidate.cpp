/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"
#include "..\..\inc\Viewport.hpp"
#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Notifies us that the system has requested a particular pixel area of the 
//      client rectangle should be redrawn. (On WM_PAINT)
//  For VT, this doesn't mean anything. So do nothing.
// Arguments:
// - prcDirtyClient - Pointer to pixel area (RECT) of client region the system 
//      believes is dirty
// Return Value:
// - S_OK
HRESULT VtEngine::InvalidateSystem(_In_ const RECT* const /*prcDirtyClient*/)
{
    return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the selection region and would
//      like it updated
// Arguments:
// - rgsrSelection - Array of character region rectangles (one per line) that
//      represent the selected area
// - cRectangles - Length of the array above.
// Return Value:
// - S_OK
HRESULT VtEngine::InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const /*rgsrSelection*/,
                                      _In_ UINT const /*cRectangles*/)
{
    // Selection shouldn't be handled bt the VT Renderer Host, it should be 
    //      handled by the client.

    return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the character region specified.
// - NOTE: This typically triggers on cursor or text buffer changes
// Arguments:
// - psrRegion - Character region (SMALL_RECT) that has been changed
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    return this->_InvalidCombine(psrRegion);
}

// Routine Description:
// - Notifies to repaint everything.
// - NOTE: Use sparingly. Only use when something that could affect the entire 
//      frame simultaneously occurs.
// Arguments:
// - <none>
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::InvalidateAll()
{
    Viewport view(_srLastViewport);
    SMALL_RECT rc = _srLastViewport;
    view.ConvertToOrigin(&rc);

    return this->_InvalidCombine(&rc);
}

// Routine Description:
// - Helper to combine the given rectangle into the invalid region to be 
//      updated on the next paint
// Arguments:
// - prc - Pixel region (RECT) that should be repainted on the next frame
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_InvalidCombine(_In_ const SMALL_RECT* const prc)
{
    if (!_fInvalidRectUsed)
    {
        _srcInvalid = *prc;
        _fInvalidRectUsed = true;
    }
    else
    {
        _OrRect(&_srcInvalid, prc);
    }

    // Ensure invalid areas remain within bounds of window.
    // RETURN_IF_FAILED(_InvalidRestrict());

    return S_OK;
}

// Routine Description:
// - Helper to adjust the invalid region by the given offset such as when a
//      scroll operation occurs.
// Arguments:
// - ppt - Distances by which we should move the invalid region in response to a scroll
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_InvalidOffset(_In_ const COORD* const pCoord)
{
    if (_fInvalidRectUsed)
    {
        SMALL_RECT rcInvalidNew;

        RETURN_IF_FAILED(ShortAdd(_srcInvalid.Left, pCoord->X, &rcInvalidNew.Left));
        RETURN_IF_FAILED(ShortAdd(_srcInvalid.Right, pCoord->X, &rcInvalidNew.Right));
        RETURN_IF_FAILED(ShortAdd(_srcInvalid.Top, pCoord->Y, &rcInvalidNew.Top));
        RETURN_IF_FAILED(ShortAdd(_srcInvalid.Bottom, pCoord->Y, &rcInvalidNew.Bottom));

        // Add the scrolled invalid rectangle to what was left behind to get the new invalid area.
        // This is the equivalent of adding in the "update rectangle" that we would get out of ScrollWindowEx/ScrollDC.
        _OrRect(&_srcInvalid, &rcInvalidNew);

        // Ensure invalid areas remain within bounds of window.
        // RETURN_IF_FAILED(_InvalidRestrict());
    }

    return S_OK;
}

// Routine Description:
// - Helper to ensure the invalid region remains within the bounds of the viewport.
// Arguments:
// - <none>
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or safemath failure.
HRESULT VtEngine::_InvalidRestrict()
{
    Viewport view(_srLastViewport);
    SMALL_RECT rc = _srLastViewport;
    view.ConvertToOrigin(&rc);

    _srcInvalid.Left = clamp(_srcInvalid.Left, rc.Left, rc.Right);
    _srcInvalid.Right = clamp(_srcInvalid.Right, rc.Left, rc.Right);
    _srcInvalid.Top = clamp(_srcInvalid.Top, rc.Top, rc.Bottom);
    _srcInvalid.Bottom = clamp(_srcInvalid.Bottom, rc.Top, rc.Bottom);

    return S_OK;
}