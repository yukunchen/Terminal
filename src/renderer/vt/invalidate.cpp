/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"

using namespace Microsoft::Console::Types;
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
    Viewport newInvalid = Viewport::FromExclusive(*psrRegion);
    return this->_InvalidCombine(newInvalid);
}

// Routine Description:
// - Notifies us that the console has changed the position of the cursor.
// Arguments:
// - pcoordCursor - the new position of the cursor
// Return Value:
// - S_OK
HRESULT VtEngine::InvalidateCursor(const COORD* const /*pcoordCursor*/)
{
    // COORD bad = {0};
    // if (_firstCursor == INVALID_COORDS && *pcoordCursor != bad)
    // {
    //     // DebugBreak();
    //     _firstCursor = *pcoordCursor;
    //     _virtualTop = _firstCursor.Y;
    // }

    _cursorMoved = true;
    return S_OK;
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
    return this->_InvalidCombine(_lastViewport.ToOrigin());
}

// Method Description:
// - Notifies us that we're about to circle the buffer, giving us a chance to
//      force a repaint before the buffer contents are lost. The VT renderer
//      needs to be able to render all text before it's lost, so we return true.
// Arguments:
// - Recieves a bool indicating if we should force the repaint.
// Return Value:
// - S_OK
HRESULT VtEngine::InvalidateCircling(_Out_ bool* const pForcePaint)
{
    *pForcePaint = true;
    if (_virtualTop > 0)
    {
        _virtualTop--;
    }
    return S_OK;
}

// Method Description:
// - Notifies us that we're about to be torn down. This gives us a last chance
//      to force a repaint before the buffer contents are lost. The VT renderer
//      needs to be able to render all text before it's lost, so we return true.
// Arguments:
// - Recieves a bool indicating if we should force the repaint.
// Return Value:
// - S_OK
HRESULT VtEngine::PrepareForTeardown(_Out_ bool* const pForcePaint)
{
    *pForcePaint = true;
    return S_OK;
}

// Routine Description:
// - Helper to combine the given rectangle into the invalid region to be
//      updated on the next paint
// Expects EXCLUSIVE rectangles.
// Arguments:
// - invalid - A viewport containing the character region that should be
//      repainted on the next frame
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_InvalidCombine(_In_ const Viewport invalid)
{
    if (!_fInvalidRectUsed)
    {
        _invalidRect = invalid;
        _fInvalidRectUsed = true;
    }
    else
    {
        _invalidRect = Viewport::OrViewports(_invalidRect, invalid);
    }

    // Ensure invalid areas remain within bounds of window.
    RETURN_IF_FAILED(_InvalidRestrict());

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
        Viewport newInvalid = _invalidRect;
        RETURN_IF_FAILED(Viewport::AddCoord(_invalidRect, *pCoord, newInvalid));

        // Add the scrolled invalid rectangle to what was left behind to get the new invalid area.
        // This is the equivalent of adding in the "update rectangle" that we would get out of ScrollWindowEx/ScrollDC.
        _invalidRect = Viewport::OrViewports(_invalidRect, newInvalid);

        // Ensure invalid areas remain within bounds of window.
        RETURN_IF_FAILED(_InvalidRestrict());

        if (_virtualTop > 0)
        {
            _virtualTop += pCoord->Y;
        }

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
    SMALL_RECT oldInvalid = _invalidRect.ToExclusive();

    _lastViewport.ToOrigin().TrimToViewport(&oldInvalid);

    _invalidRect = Viewport::FromExclusive(oldInvalid);

    return S_OK;
}
