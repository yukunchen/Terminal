/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"
#include "..\..\inc\Viewport.hpp"

#include <sstream>


#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Prepares internal structures for a painting operation.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we started to paint. S_FALSE if we didn't need to paint. HRESULT error code if painting didn't start successfully.
HRESULT VtEngine::StartPaint()
{
 /*   std::string foo("A");
    return _Write(foo);*/

    // Turn off cursor
    std::string seq = "\x1b[?25l";
    _Write(seq);

    return S_OK;
}

// Routine Description:
// - Scrolls the existing data on the in-memory frame by the scroll region deltas we have collectively received
//   through the Invalidate methods since the last time this was called.
// Arguments:
// - <none>
// Return Value:
// - S_OK, suitable GDI HRESULT error, error from Win32 windowing, or safemath error.
HRESULT VtEngine::ScrollFrame()
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
        // DL - Delete Line
        std::string seq = "\x1b[M";
        for (int i = 0; i < absDy; ++i)
        {
            _Write(seq);
        }

        SMALL_RECT b = v;
        b.Top = v.Bottom - absDy;
        RETURN_IF_FAILED(_InvalidCombine(&b));
    }
    else
    {
        _MoveCursor({0,0});
        // IL - Insert Line
        std::string seq = "\x1b[L";
        for (int i = 0; i < absDy; ++i)
        {
            _Write(seq);
        }

        SMALL_RECT a = v;
        a.Bottom = absDy;
        RETURN_IF_FAILED(_InvalidCombine(&a));
    }

    // if (_scrollDelta.Y > 0)
    // {
    //     SHORT 
    // }

    return S_OK;
}

// Routine Description:
// - EndPaint helper to perform the final BitBlt copy from the memory bitmap onto the final window bitmap (double-buffering.) Also cleans up structures used while painting.
// Arguments:
// - <none>
// Return Value:
// - S_OK or suitable GDI HRESULT error.
HRESULT VtEngine::EndPaint()
{
    _srcInvalid = { 0 };
    _fInvalidRectUsed = false;

    if (_lastText.X != _lastRealCursor.X || _lastText.Y != _lastRealCursor.Y )
    {
        _MoveCursor(_lastRealCursor);
    }

    _scrollDelta = {0};
    
    // Turn on cursor
    std::string seq = "\x1b[?25h";
    _Write(seq);
    return S_OK;
}

// Routine Description:
// - Paints the background of the invalid area of the frame.
// Arguments:
// - <none>
// Return Value:
// - S_OK or suitable GDI HRESULT error.
HRESULT VtEngine::PaintBackground()
{
    return S_OK;
}

#define _CRT_SECURE_NO_WARNINGS 1

// Routine Description:
// - Draws one line of the buffer to the screen.
// - This will now be cached in a PolyText buffer and flushed periodically instead of drawing every individual segment. Note this means that the PolyText buffer must be flushed before some operations (changing the brush color, drawing lines on top of the characters, inverting for cursor/selection, etc.)
// Arguments:
// - pwsLine - string of text to be written
// - rgWidths - array specifying how many column widths that the console is expecting each character to take
// - cchLine - length of line to be read
// - coord - character coordinate target to render within viewport
// - fTrimLeft - This specifies whether to trim one character width off the left side of the output. Used for drawing the right-half only of a double-wide character.
// Return Value:
// - S_OK or suitable GDI HRESULT error.
// - HISTORICAL NOTES:
// ETO_OPAQUE will paint the background color before painting the text.
// ETO_CLIPPED required for ClearType fonts. Cleartype rendering can escape bounding rectangle unless clipped.
//   Unclipped rectangles results in ClearType cutting off the right edge of the previous character when adding chars
//   and in leaving behind artifacts when backspace/removing chars.
//   This mainly applies to ClearType fonts like Lucida Console at small font sizes (10pt) or bolded.
// See: Win7: 390673, 447839 and then superseded by http://osgvsowi/638274 when FE/non-FE rendering condensed.
//#define CONSOLE_EXTTEXTOUT_FLAGS ETO_OPAQUE | ETO_CLIPPED
//#define MAX_POLY_LINES 80
HRESULT VtEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                  _In_reads_(cchLine) const unsigned char* const rgWidths,
                                  _In_ size_t const cchLine,
                                  _In_ COORD const coord,
                                  _In_ bool const fTrimLeft)
{
    RETURN_IF_FAILED(_MoveCursor(coord));
    try
    {
        DWORD dwNeeded = WideCharToMultiByte(CP_ACP, 0, pwsLine, (int)cchLine, nullptr, 0, nullptr, nullptr);
        wistd::unique_ptr<char[]> rgchNeeded = wil::make_unique_nothrow<char[]>(dwNeeded + 1);
        RETURN_IF_NULL_ALLOC(rgchNeeded);
        RETURN_LAST_ERROR_IF_FALSE(WideCharToMultiByte(CP_ACP, 0, pwsLine, (int)cchLine, rgchNeeded.get(), dwNeeded, nullptr, nullptr));
        rgchNeeded[dwNeeded] = '\0';

        _Write(rgchNeeded.get(), dwNeeded);
        
        // Update our internal tracker of the cursor's position
        short totalWidth = 0;
        for (int i=0; i < cchLine; i++)
        {
            totalWidth+=(short)rgWidths[i];
        }
        _lastText.X += totalWidth;

        pwsLine;
        rgWidths;
        cchLine;
        coord;
        fTrimLeft;
    }
    CATCH_RETURN();

    return S_OK;
}

// Routine Description:
// - Draws up to one line worth of grid lines on top of characters.
// Arguments:
// - lines - Enum defining which edges of the rectangle to draw
// - color - The color to use for drawing the edges.
// - cchLine - How many characters we should draw the grid lines along (left to right in a row)
// - coordTarget - The starting X/Y position of the first character to draw on.
// Return Value:
// - S_OK or suitable GDI HRESULT error or E_FAIL for GDI errors in functions that don't reliably return a specific error code.
HRESULT VtEngine::PaintBufferGridLines(_In_ GridLines const lines, _In_ COLORREF const color, _In_ size_t const cchLine, _In_ COORD const coordTarget)
{
    lines;
    color;
    cchLine;
    coordTarget;
    return S_OK;
}

// Routine Description:
// - Draws the cursor on the screen
// Arguments:
// - coord - Coordinate position where the cursor should be drawn
// - ulHeightPercent - The cursor will be drawn at this percentage of the current font height.
// - fIsDoubleWidth - The cursor should be drawn twice as wide as usual.
// Return Value:
// - S_OK, suitable GDI HRESULT error, or safemath error, or E_FAIL in a GDI error where a specific error isn't set.
HRESULT VtEngine::PaintCursor(_In_ COORD const coord, _In_ ULONG const ulHeightPercent, _In_ bool const fIsDoubleWidth)
{
    coord;
    ulHeightPercent;
    fIsDoubleWidth;

    _MoveCursor(coord);

    return S_OK;
}

// Routine Description:
// - Clears out the cursor that was set in the previous PaintCursor call.
// Arguments:
// - <none>
// Return Value:
// - S_OK, suitable GDI HRESULT error, or E_FAIL in a GDI error where a specific error isn't set.
HRESULT VtEngine::ClearCursor()
{
    return S_OK;
}

// Routine Description:
//  - Inverts the selected region on the current screen buffer.
//  - Reads the selected area, selection mode, and active screen buffer
//    from the global properties and dispatches a GDI invert on the selected text area.
// Arguments:
//  - rgsrSelection - Array of rectangles, one per line, that should be inverted to make the selection area
// - cRectangles - Count of rectangle array length
// Return Value:
// - S_OK or suitable GDI HRESULT error.
HRESULT VtEngine::PaintSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    rgsrSelection;
    cRectangles;
    return S_OK;
}


HRESULT VtEngine::_MoveCursor(COORD const coord)
{
    // DebugBreak();
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
            _lastRealCursor = coord;
        }
        CATCH_RETURN();
    }
    return S_OK;
}