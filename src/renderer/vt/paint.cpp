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
// - Prepares internal structures for a painting operation.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we started to paint. S_FALSE if we didn't need to paint. HRESULT error code if painting didn't start successfully.
HRESULT VtEngine::StartPaint()
{
    std::string foo("A");
    return _Write(foo);
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
    pwsLine;
    rgWidths;
    cchLine;
    coord;
    fTrimLeft;
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
