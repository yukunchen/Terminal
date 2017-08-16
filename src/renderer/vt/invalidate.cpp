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
// - Notifies us that the system has requested a particular pixel area of the client rectangle should be redrawn. (On WM_PAINT)
// Arguments:
// - prcDirtyClient - Pointer to pixel area (RECT) of client region the system believes is dirty
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT VtEngine::InvalidateSystem(_In_ const RECT* const prcDirtyClient)
{
    prcDirtyClient;
    return S_OK;
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the console would like us to move while scrolling.
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT VtEngine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    pcoordDelta;
    // return this->InvalidateAll();
    short dx = pcoordDelta->X;
    short dy = pcoordDelta->Y;

    if (dx != 0 || dy != 0)
    {
        // POINT ptDelta = { 0 };
        // RETURN_IF_FAILED(_ScaleByFont(pcoordDelta, &ptDelta));

        RETURN_IF_FAILED(_InvalidOffset(pcoordDelta));

        COORD invalidScrollNew;
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.X, dx, &invalidScrollNew.X));
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.Y, dy, &invalidScrollNew.Y));

        // Store if safemath succeeded
        _scrollDelta = invalidScrollNew;
        

        Viewport view(_srLastViewport);
        SMALL_RECT v = _srLastViewport;
        view.ConvertToOrigin(&v);

        SMALL_RECT b = v;
        SMALL_RECT c = v;
        short dy2 = (dy>0)? dy : -dy;
        // if (dy > 0)
        // {
            b.Top = v.Bottom - dy2;
            // I think v.Left is already 0 here
            
            c.Top = c.Left = 0;
            c.Bottom = dy2;
            RETURN_IF_FAILED(_InvalidCombine(&b));
            RETURN_IF_FAILED(_InvalidCombine(&c));
        // }
        // else
        // {
        //     b.Top = v.Bottom + dy;
        //     c.Bottom = dy;
        // }
    }

    return S_OK;
    
    // return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the selection region and would like it updated
// Arguments:
// - rgsrSelection - Array of character region rectangles (one per line) that represent the selected area
// - cRectangles - Length of the array above.
// Return Value:
// - HRESULT S_OK or GDI-based error code
HRESULT VtEngine::InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    rgsrSelection;
    cRectangles;
    // Selection shouldn't be handled bt the VT Renderer Host, it should be 
    //      handled by the client.

    // for(unsigned int i = 0; i < cRectangles; i++)
    // {
    //     this->_InvalidCombine(&rgsrSelection[i]);
    // }
    return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the character region specified.
// - NOTE: This typically triggers on cursor or text buffer changes
// Arguments:
// - psrRegion - Character region (SMALL_RECT) that has been changed
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT VtEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    psrRegion;
    this->_InvalidCombine(psrRegion);
    return S_OK;
}

// Routine Description:
// - Notifies to repaint everything.
// - NOTE: Use sparingly. Only use when something that could affect the entire frame simultaneously occurs.
// Arguments:
// - <none>
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT VtEngine::InvalidateAll()
{
    Viewport view(_srLastViewport);
    SMALL_RECT rc = _srLastViewport;
    view.ConvertToOrigin(&rc);
    // this->_InvalidCombine(&_srLastViewport);
    this->_InvalidCombine(&rc);
    return S_OK;
}




// Routine Description:
// - Helper to combine the given rectangle into the invalid region to be updated on the next paint
// Arguments:
// - prc - Pixel region (RECT) that should be repainted on the next frame
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
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
// - Helper to adjust the invalid region by the given offset such as when a scroll operation occurs.
// Arguments:
// - ppt - Distances by which we should move the invalid region in response to a scroll
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
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