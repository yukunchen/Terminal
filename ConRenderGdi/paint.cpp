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
// - Prepares internal structures for a painting operation.
// Arguments:
// - <none>
// Return Value:
// - True if we could start to paint. False if there's nothing to paint or we have no surface to paint on.
bool GdiEngine::StartPaint()
{
    if (_hwndTargetWindow != INVALID_HANDLE_VALUE)
    {
        if (!_fPaintStarted)
        {
            _fPaintStarted = true;

            _cPolyText = 0;

            _PrepareMemoryBitmap(_hwndTargetWindow);

            // We must use Get and Release DC because BeginPaint/EndPaint can only be called in response to a WM_PAINT message (and may hang otherwise)
            // We'll still use the PAINTSTRUCT for information because it's convenient.
            _psInvalidData.hdc = GetDC(_hwndTargetWindow);
            _psInvalidData.fErase = TRUE;
            _psInvalidData.rcPaint = _rcInvalid;

            return true;
        }
    }

    return false;
}

// Routine Description:
// - Scrolls the existing data on the in-memory frame by the scroll region deltas we have collectively received
//   through the Invalidate methods since the last time this was called.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::ScrollFrame()
{
    // do any leftover scrolling
    if (_szInvalidScroll.cx != 0 ||
        _szInvalidScroll.cy != 0)
    {
        RECT rcUpdate;

        // We have to limit the region that can be scrolled to not include the gutters.
        // Gutters are defined as sub-character width pixels at the bottom or right of the screen.
        COORD const coordFontSize = _GetFontSize();
        SIZE szGutter;
        szGutter.cx = _szMemorySurface.cx % coordFontSize.X;
        szGutter.cy = _szMemorySurface.cy % coordFontSize.Y;

        RECT rcScrollLimit = { 0 };
        rcScrollLimit.right = _szMemorySurface.cx - szGutter.cx;
        rcScrollLimit.bottom = _szMemorySurface.cy - szGutter.cy;

        ScrollDC(_hdcMemoryContext, _szInvalidScroll.cx, _szInvalidScroll.cy, &rcScrollLimit, &rcScrollLimit, nullptr, &rcUpdate);

        _InvalidCombine(&rcUpdate);

        // update invalid rect for the remainder of paint functions
        _psInvalidData.rcPaint = _rcInvalid;
    }
}

// Routine Description:
// - BeginPaint helper to prepare the in-memory bitmap for double-buffering
// Arguments:
// - hwnd - Window handle to use for the DC properties when creating a memory DC and for checking the client area size.
// Return Value:
// - <none>
void GdiEngine::_PrepareMemoryBitmap(_In_ HWND const hwnd)
{
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    SIZE const szClient = _GetRectSize(&rcClient);
    
    if (_szMemorySurface.cx != szClient.cx ||
        _szMemorySurface.cy != szClient.cy) 
    {
        HDC hdcRealWindow = GetDC(_hwndTargetWindow);

        if (hdcRealWindow != nullptr)
        {
            // If we already had a bitmap, Blt the old one onto the new one and clean up the old one.
            if (_hbitmapMemorySurface != nullptr)
            {
                // Make a temporary DC for us to Blt with.
                HDC hdcTemp = CreateCompatibleDC(hdcRealWindow);

                // Make the new bitmap we'll use going forward with the new size.
                HBITMAP hbitmapNew = CreateCompatibleBitmap(hdcRealWindow, szClient.cx, szClient.cy);

                // Select it into the DC, but hold onto the junky one pixel bitmap (made by default) to give back when we need to Delete.
                HBITMAP hbitmapOnePixelJunk = static_cast<HBITMAP>(SelectObject(hdcTemp, hbitmapNew));

                // Blt from the DC/bitmap we're already holding onto into the new one.
                BitBlt(hdcTemp, 0, 0, _szMemorySurface.cx, _szMemorySurface.cy, _hdcMemoryContext, 0, 0, SRCCOPY);

                // Put the junky bitmap back into the temp DC
                SelectObject(hdcTemp, hbitmapOnePixelJunk);

                // Get rid of the temp DC.
                DeleteObject(hdcTemp);
                hdcTemp = nullptr;

                // Move our new bitmap into the long-standing DC we're holding onto.
                HBITMAP hbitmapOld = static_cast<HBITMAP>(SelectObject(_hdcMemoryContext, hbitmapNew));

                // Free the old bitmap
                DeleteObject(hbitmapOld);

                // Free the junk bitmap
                DeleteObject(hbitmapOnePixelJunk);

                // Now save a pointer to our new bitmap into the class state.
                _hbitmapMemorySurface = hbitmapNew;
            }
            else
            {
                _hbitmapMemorySurface = CreateCompatibleBitmap(hdcRealWindow, szClient.cx, szClient.cy);
                HGDIOBJ hobj = SelectObject(_hdcMemoryContext, _hbitmapMemorySurface); // DC has a default junk bitmap, take it and delete it.
                DeleteObject(hobj);
            }

            // Save the new client size.
            _szMemorySurface = szClient;

            ReleaseDC(_hwndTargetWindow, hdcRealWindow);
        }
    }
}

// Routine Description:
// - EndPaint helper to perform the final BitBlt copy from the memory bitmap onto the final window bitmap (double-buffering.) Also cleans up structures used while painting.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::EndPaint()
{
    if (_fPaintStarted)
    {
        _FlushBufferLines();

        // If we scrolled, Blt the whole thing.
        if (_szInvalidScroll.cx != 0 ||
            _szInvalidScroll.cy != 0)
        {
            RECT rcClient;
            GetClientRect(_hwndTargetWindow, &rcClient);

            SIZE const szClient = _GetRectSize(&rcClient);
            BitBlt(_psInvalidData.hdc, 0, 0, szClient.cx, szClient.cy, _hdcMemoryContext, 0, 0, SRCCOPY);
            _szInvalidScroll = { 0 };
        }
        else
        {
            POINT const pt = _GetInvalidRectPoint();
            SIZE const sz = _GetInvalidRectSize();

            BitBlt(_psInvalidData.hdc, pt.x, pt.y, sz.cx, sz.cy, _hdcMemoryContext, pt.x, pt.y, SRCCOPY);
        }

        _rcInvalid = { 0 };
        _fInvalidRectUsed = false;
        _szInvalidScroll = { 0 };

        GdiFlush();
        ReleaseDC(_hwndTargetWindow, _psInvalidData.hdc);

        _fPaintStarted = false;
    }
}

// Routine Description:
// - Fills the given rectangle with the background color on the drawing context.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::_PaintBackgroundColor(_In_ const RECT* const prc)
{
    HBRUSH hbr = (HBRUSH)GetStockObject(DC_BRUSH);
    if (hbr != nullptr)
    {
        FillRect(_hdcMemoryContext, prc, hbr);
        DeleteObject(hbr);
    }
}

// Routine Description:
// - Paints the background of the invalid area of the frame.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::PaintBackground()
{
    if (_psInvalidData.fErase)
    {
        _PaintBackgroundColor(&_psInvalidData.rcPaint);
    }
}

// Routine Description:
// - Paints in the gutter region of the drawing canvas.
// - The gutter is defined as the extraneous pixels at the bottom and right hand of the display that
//   make up less than 1 character width/height. Various other operations might accidentally paint here
//   and the final gutter paint ensures that this region stays clean and tidy.
void GdiEngine::PaintGutter()
{
    COORD const coordFontSize = _GetFontSize();

    SIZE szGutter = { 0 };
    szGutter.cx = _szMemorySurface.cx % coordFontSize.X;
    szGutter.cy = _szMemorySurface.cy % coordFontSize.Y;

    if (szGutter.cx > 0)
    {
        RECT rcGutterRight = { 0 };
        rcGutterRight.top = 0;
        rcGutterRight.bottom = rcGutterRight.top + _szMemorySurface.cy;
        rcGutterRight.right = _szMemorySurface.cx;
        rcGutterRight.left = rcGutterRight.right - szGutter.cx;

        _PaintBackgroundColor(&rcGutterRight);
    }

    if (szGutter.cy > 0)
    {
        RECT rcGutterBottom = { 0 };
        rcGutterBottom.left = 0;
        rcGutterBottom.right = rcGutterBottom.left + _szMemorySurface.cx;
        rcGutterBottom.bottom = _szMemorySurface.cy;
        rcGutterBottom.top = rcGutterBottom.bottom - szGutter.cy;

        _PaintBackgroundColor(&rcGutterBottom);
    }
}

// Routine Description:
// - Draws one line of the buffer to the screen.
// - This will now be cached in a PolyText buffer and flushed periodically instead of drawing every individual segment. Note this means that the PolyText buffer must be flushed before some operations (changing the brush color, drawing lines on top of the characters, inverting for cursor/selection, etc.)
// Arguments:
// - pwsLine - string of text to be written
// - cchLine - length of line to be read
// - coord - character coordinate target to render within viewport
// - cchCharWidths - This is the length of the string before double-wide characters are stripped. Used for determining clipping rectangle size 
//                 - The clipping rectangle is the font width * this many characters expected.
// - fTrimLeft - This specifies whether to trim one character width off the left side of the output. Used for drawing the right-half only of a double-wide character.
// Return Value:
// - <none>
// - HISTORICAL NOTES:
// ETO_OPAQUE will paint the background color before painting the text.
// ETO_CLIPPED required for ClearType fonts. Cleartype rendering can escape bounding rectangle unless clipped.
//   Unclipped rectangles results in ClearType cutting off the right edge of the previous character when adding chars
//   and in leaving behind artifacts when backspace/removing chars.
//   This mainly applies to ClearType fonts like Lucida Console at small font sizes (10pt) or bolded.
// See: Win7: 390673, 447839 and then superseded by http://osgvsowi/638274 when FE/non-FE rendering condensed.
//#define CONSOLE_EXTTEXTOUT_FLAGS ETO_OPAQUE | ETO_CLIPPED
//#define MAX_POLY_LINES 80
void GdiEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine, 
                                _In_ size_t const cchLine, 
                                _In_ COORD const coord, 
                                _In_ size_t const cchCharWidths, 
                                _In_ bool const fTrimLeft)
{
    if (cchLine > 0)
    {
        POINT ptDraw = _ScaleByFont(&coord);

        POLYTEXTW* const pPolyTextLine = &_pPolyText[_cPolyText];

        PWCHAR pwsPoly = new wchar_t[cchLine];
        if (pwsPoly != nullptr)
        {
            wmemcpy_s(pwsPoly, cchLine, pwsLine, cchLine);

            COORD const coordFontSize = _GetFontSize();

            pPolyTextLine->lpstr = pwsPoly;
            pPolyTextLine->n = (UINT)cchLine;
            pPolyTextLine->x = ptDraw.x;
            pPolyTextLine->y = ptDraw.y;
            pPolyTextLine->uiFlags = ETO_OPAQUE | ETO_CLIPPED;
            pPolyTextLine->rcl.left = pPolyTextLine->x;
            pPolyTextLine->rcl.top = pPolyTextLine->y;
            pPolyTextLine->rcl.right = pPolyTextLine->rcl.left + ((SHORT)cchCharWidths * coordFontSize.X);
            pPolyTextLine->rcl.bottom = pPolyTextLine->rcl.top + coordFontSize.Y;

            if (fTrimLeft)
            {
                pPolyTextLine->rcl.left += coordFontSize.X;
            }

            _cPolyText++;

            if (_cPolyText >= s_cPolyTextCache)
            {
                _FlushBufferLines();
            }
        }
    }
}

// Routine Description:
// - Flushes any buffer lines in the PolyTextOut cache by drawing them and freeing the strings.
// - See also: PaintBufferLine
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::_FlushBufferLines()
{
    if (_cPolyText > 0)
    {
        PolyTextOutW(_hdcMemoryContext, _pPolyText, (UINT)_cPolyText);

        for (size_t iPoly = 0; iPoly < _cPolyText; iPoly++)
        {
            if (_pPolyText[iPoly].lpstr != nullptr)
            {
                delete[] _pPolyText[iPoly].lpstr;
            }
        }

        _cPolyText = 0;
    }
}

// Routine Description:
// - Draws up to one line worth of grid lines on top of characters.
// Arguments:
// - lines - Enum defining which edges of the rectangle to draw
// - color - The color to use for drawing the edges.
// - cchLine - How many characters we should draw the grid lines along (left to right in a row)
// - coordTarget - The starting X/Y position of the first character to draw on.
// Return Value:
// - <none>
void GdiEngine::PaintBufferGridLines(_In_ GridLines const lines, _In_ COLORREF const color, _In_ size_t const cchLine, _In_ COORD const coordTarget)
{
    if (lines != GridLines::None)
    {
        _FlushBufferLines();

        // Set the brush color as requested and save the previous brush to restore at the end.
        HBRUSH const hbr = CreateSolidBrush(color);
        HBRUSH const hbrPrev = static_cast<HBRUSH>(SelectObject(_hdcMemoryContext, hbr));

        // Convert the target from characters to pixels.
        POINT ptTarget = _ScaleByFont(&coordTarget);

        // Get the font size so we know the size of the rectangle lines we'll be inscribing.
        COORD const coordFontSize = _GetFontSize();

        // For each length of the line, inscribe the various lines as specified by the enum
        for (size_t i = 0; i < cchLine; i++)
        {
            if (lines & GridLines::Top)
            {
                PatBlt(_hdcMemoryContext, ptTarget.x, ptTarget.y, coordFontSize.X, 1, PATCOPY);
            }

            if (lines & GridLines::Left)
            {
                PatBlt(_hdcMemoryContext, ptTarget.x, ptTarget.y, 1, coordFontSize.Y, PATCOPY);
            }

            // NOTE: Watch out for inclusive/exclusive rectangles here.
            // We have to remove 1 from the font size for the bottom and right lines to ensure that the
            // starting point remains within the clipping rectangle. 
            // For example, if we're drawing a letter at 0,0 and the font size is 8x16....
            // The bottom left corner inclusive is at 0,15 which is Y (0) + Font Height (16) - 1 = 15.
            // The top right corner inclusive is at 7,0 which is X (0) + Font Height (8) - 1 = 7.

            if (lines & GridLines::Bottom)
            {
                PatBlt(_hdcMemoryContext, ptTarget.x, ptTarget.y + coordFontSize.Y - 1, coordFontSize.X, 1, PATCOPY);
            }

            if (lines & GridLines::Right)
            {
                PatBlt(_hdcMemoryContext, ptTarget.x + coordFontSize.X - 1, ptTarget.y, 1, coordFontSize.Y, PATCOPY);
            }

            // Move to the next character in this run.
            ptTarget.x += coordFontSize.X;
        }

        // Restore the previous brush.
        SelectObject(_hdcMemoryContext, hbrPrev);
        DeleteObject(hbr);
    }
}

// Routine Description:
// - Draws the cursor on the screen
// Arguments:
// - coord - Coordinate position where the cursor should be drawn
// - ulHeightPercent - The cursor will be drawn at this percentage of the current font height.
// - fIsDoubleWidth - The cursor should be drawn twice as wide as usual.
// Return Value:
// - <none>
void GdiEngine::PaintCursor(_In_ COORD const coord, _In_ ULONG const ulHeightPercent, _In_ bool const fIsDoubleWidth)
{
    _FlushBufferLines();

    COORD const coordFontSize = _GetFontSize();

    // First set up a block cursor the size of the font.
    RECT rcInvert;
    rcInvert.left = coord.X * coordFontSize.X;
    rcInvert.top = coord.Y * coordFontSize.Y;
    rcInvert.right = rcInvert.left + coordFontSize.X;
    rcInvert.bottom = rcInvert.top + coordFontSize.Y;

    // If we're double-width cursor, make it an extra font wider.
    if (fIsDoubleWidth)
    {
        rcInvert.right += coordFontSize.X;
    }

    // Now adjust the cursor height
    // enforce min/max cursor height
    ULONG ulHeight = ulHeightPercent;
    ulHeight = max(ulHeight, s_ulMinCursorHeightPercent); // No smaller than 25%
    ulHeight = min(ulHeight, s_ulMaxCursorHeightPercent); // No larger than 100%

    ulHeight = MulDiv(coordFontSize.Y, ulHeight, 100); // divide by 100 because percent.

    // Reduce the height of the top to be relative to the bottom by the height we want.
    rcInvert.top = rcInvert.bottom - ulHeight;

    InvertRect(_hdcMemoryContext, &rcInvert);

    // Save inverted cursor position so we can clear it.
    _rcCursorInvert = rcInvert;
}

// Routine Description:
// - Clears out the cursor that was set in the previous PaintCursor call.
// Arguments:
// - <none>
// Return Value:
// - <none>
void GdiEngine::ClearCursor()
{
    if (!IsRectEmpty(&_rcCursorInvert))
    {
        // We inverted to set the cursor, so invert the same rect to clear it out.
        InvertRect(_hdcMemoryContext, &_rcCursorInvert);

        _rcCursorInvert = { 0 };
    }
}

// Routine Description:
//  - Inverts the selected region on the current screen buffer.
//  - Reads the selected area, selection mode, and active screen buffer
//    from the global properties and dispatches a GDI invert on the selected text area.
// Arguments:
//  - rgsrSelection - Array of rectangles, one per line, that should be inverted to make the selection area
// - cRectangles - Count of rectangle array length
// Return Value:
//  - <none>
void GdiEngine::PaintSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    _FlushBufferLines();

    // Get a region ready
    HRGN hrgnSelection = CreateRectRgn(0, 0, 0, 0);
    NTSTATUS status = NT_TESTNULL_GLE(hrgnSelection);
    if (NT_SUCCESS(status))
    {
        // Adjust the selected region to invert
        status = _PaintSelectionCalculateRegion(rgsrSelection, cRectangles, hrgnSelection);
        if (NT_SUCCESS(status))
        {
            // Save the painted region for the next paint
            int rgnType = CombineRgn(_hrgnGdiPaintedSelection, hrgnSelection, nullptr, RGN_COPY);

            // Don't paint if there was an error in the region or it's empty.
            if (rgnType != ERROR && rgnType != NULLREGION)
            {
                // Do the invert
                if (!InvertRgn(_hdcMemoryContext, hrgnSelection))
                {
                    status = NTSTATUS_FROM_WIN32(GetLastError());
                }
            }
        }

        DeleteObject(hrgnSelection);
    }
}

// Routine Description:
//  - Composes a GDI region representing the area of the buffer that
//    is currently selected based on member variable (selection rectangle) state.
// Arguments:
//  - rgsrSelection - Array of rectangles, one per line, that should be inverted to make the selection area
// - cRectangles - Count of rectangle array length
//  - hrgnSelection - Handle to empty GDI region. Will be filled with selection region information.
// Return Value:
//  - NTSTATUS. STATUS_SUCCESS or Expect GDI-based errors or memory errors.
NTSTATUS GdiEngine::_PaintSelectionCalculateRegion(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, 
                                                   _In_ UINT const cRectangles, 
                                                   _Inout_ HRGN const hrgnSelection) const
{
    NTSTATUS status = STATUS_SUCCESS;

    // for each row in the selection
    for (UINT iRect = 0; iRect < cRectangles; iRect++)
    {
        // multiply character counts by font size to obtain pixels
        RECT const rectHighlight = _ScaleByFont(&rgsrSelection[iRect]);

        // create region for selection rectangle
        HRGN hrgnLine = CreateRectRgn(rectHighlight.left, rectHighlight.top, rectHighlight.right, rectHighlight.bottom);
        status = NT_TESTNULL_GLE(hrgnLine);
        if (NT_SUCCESS(status))
        {
            // compose onto given selection region
            CombineRgn(hrgnSelection, hrgnSelection, hrgnLine, RGN_OR);

            // clean up selection region
            DeleteObject(hrgnLine);
        }
        else
        {
            break; // don't continue loop if we are having GDI errors.
        }
    }

    return status;
}
