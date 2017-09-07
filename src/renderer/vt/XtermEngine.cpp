/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "XtermEngine.hpp"
#include "..\..\inc\Viewport.hpp"
#pragma hdrstop
using namespace Microsoft::Console::Render;

XtermEngine::XtermEngine(HANDLE hPipe, _In_reads_(cColorTable) const COLORREF* const ColorTable, _In_ const WORD cColorTable)
    : VtEngine(hPipe),
    _ColorTable(ColorTable),
    _cColorTable(cColorTable)
{
}

// Routine Description:
// - Prepares internal structures for a painting operation.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we started to paint. S_FALSE if we didn't need to paint. HRESULT error code if painting didn't start successfully.
HRESULT XtermEngine::StartPaint()
{    
    HRESULT hr = VtEngine::StartPaint();
    if (SUCCEEDED(hr))
    {
        // if (!_quickReturn)
        // {
            // Turn off cursor
            std::string seq = "\x1b[?25l";
            _Write(seq);
        // }
    }

    return hr;
}

// Routine Description:
// - EndPaint helper to perform the final BitBlt copy from the memory bitmap onto the final window bitmap (double-buffering.) Also cleans up structures used while painting.
// Arguments:
// - <none>
// Return Value:
// - S_OK or suitable GDI HRESULT error.
HRESULT XtermEngine::EndPaint()
{
    HRESULT hr = VtEngine::EndPaint();
    if (SUCCEEDED(hr))
    {
        // if (!_quickReturn)
        // {
            // Turn on cursor
            std::string seq = "\x1b[?25h";
            _Write(seq);
        // }        
    }


    return hr;
}

// Routine Description:
// - This method will set the GDI brushes in the drawing context (and update the hung-window background color)
// Arguments:
// - wTextAttributes - A console attributes bit field specifying the brush colors we should use.
// Return Value:
// - S_OK if set successfully or relevant GDI error via HRESULT.
HRESULT XtermEngine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds)
{
    UNREFERENCED_PARAMETER(legacyColorAttribute);
    UNREFERENCED_PARAMETER(fIncludeBackgrounds);
    // The base xterm mode only knows about 16 colors
    return VtEngine::_16ColorUpdateDrawingBrushes(colorForeground, colorBackground, _ColorTable, _cColorTable);

}

HRESULT XtermEngine::_MoveCursor(COORD const coord)
{
    if (coord.X != _lastText.X || coord.Y != _lastText.Y)
    {
        try
        {
            if (coord.X == 0 && coord.Y == 0)
            {
                std::string seq = "\x1b[H";
                _Write(seq);
            }
            else if (coord.X == 0 && coord.Y == (_lastText.Y+1))
            {
                std::string seq = "\r\n";
                _Write(seq);
            }
            else
            {
                PCSTR pszCursorFormat = "\x1b[%d;%dH";
                COORD coordVt = coord;
                coordVt.X++;
                coordVt.Y++;

                int cchNeeded = _scprintf(pszCursorFormat, coordVt.Y, coordVt.X);
                wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
                RETURN_IF_NULL_ALLOC(psz);

                int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszCursorFormat, coordVt.Y, coordVt.X);
                _Write(psz.get(), cchWritten);
                
            }

            _lastText = coord;
        }
        CATCH_RETURN();
    }
    return S_OK;
}

HRESULT XtermEngine::ScrollFrame()
{
    if (_scrollDelta.X != 0)
    {
        // No easy way to shift left-right
        return InvalidateAll();
    }

    short dy = _scrollDelta.Y;
    short absDy = (dy>0)? dy : -dy;

    Viewport view(_srLastViewport);
    SMALL_RECT v = _srLastViewport;
    view.ConvertToOrigin(&v);

    if (dy < 0)
    {
        _MoveCursor({0,0});
        _DeleteLine(absDy);
        SMALL_RECT b = v;
        b.Top = v.Bottom - absDy;
        // RETURN_IF_FAILED(_InvalidCombine(&b));
    }
    else if (dy > 0)
    {
        _MoveCursor({0,0});
        _InsertLine(absDy);
        SMALL_RECT a = v;
        a.Bottom = absDy;
        // RETURN_IF_FAILED(_InvalidCombine(&a));
    }

    return S_OK;
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the console would like us to move while scrolling.
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT XtermEngine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    pcoordDelta;
    // return this->InvalidateAll();
    short dx = pcoordDelta->X;
    short dy = pcoordDelta->Y;
    // short absDy = (dy>0)? dy : -dy;

    if (dx != 0 || dy != 0)
    {
        // POINT ptDelta = { 0 };
        // RETURN_IF_FAILED(_ScaleByFont(pcoordDelta, &ptDelta));

        // Scroll the current offset
        RETURN_IF_FAILED(_InvalidOffset(pcoordDelta));

        // Add the top/bottom of the window to the invalid area
        Viewport view(_srLastViewport);
        SMALL_RECT v = _srLastViewport;
        view.ConvertToOrigin(&v);
        SMALL_RECT invalid = v;
        if (dy > 0)
        {
            invalid.Bottom = dy;
        }
        else if (dy < 0)
        {
            invalid.Top = v.Bottom + dy;
        }
        _InvalidCombine(&invalid);

        COORD invalidScrollNew;
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.X, dx, &invalidScrollNew.X));
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.Y, dy, &invalidScrollNew.Y));

        // Store if safemath succeeded
        _scrollDelta = invalidScrollNew;

    }

    return S_OK;
}
