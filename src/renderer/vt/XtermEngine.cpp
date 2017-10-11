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

XtermEngine::XtermEngine(_In_ wil::unique_hfile hPipe,
                         _In_reads_(cColorTable) const COLORREF* const ColorTable,
                         _In_ const WORD cColorTable) :
    VtEngine(std::move(hPipe)),
    _ColorTable(ColorTable),
    _cColorTable(cColorTable)
{
}

// Method Description:
// - Prepares internal structures for a painting operation. Turns the cursor 
//      off, so we don't see it flashing all over the client's screen as we 
//      paint the new contents.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we started to paint. S_FALSE if we didn't need to paint. HRESULT
//      error code if painting didn't start successfully, or we failed to write 
//      the pipe.
HRESULT XtermEngine::StartPaint()
{    
    HRESULT hr = VtEngine::StartPaint();
    if (SUCCEEDED(hr))
    {
        // todo come back to this before the PR is finished.
        if (!_quickReturn)
        {
            // Turn off cursor
            hr = _HideCursor();
        }
    }

    return hr;
}

// Routine Description:
// - EndPaint helper to perform the final rendering steps. Turn the cursor back 
//      on.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT XtermEngine::EndPaint()
{
    HRESULT hr = VtEngine::EndPaint();
    if (SUCCEEDED(hr))
    {
        // todo come back to this before the PR is finished.
        if (!_quickReturn)
        {
            // Turn on cursor
            hr = _ShowCursor();
        }        
    }


    return hr;
}

// Routine Description:
// - Write a VT sequence to change the current colors of text. Only writes 
//      16-color attributes.
// Arguments:
// - colorForeground: The RGB Color to use to paint the foreground text.
// - colorBackground: The RGB Color to use to paint the background of the text.
// - legacyColorAttribute: A console attributes bit field specifying the brush
//      colors we should use.
// - fIncludeBackgrounds: indicates if we should change the background color of 
//      the window. Unused for VT
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT XtermEngine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                          _In_ COLORREF const colorBackground,
                                          _In_ WORD const /*legacyColorAttribute*/,
                                          _In_ bool const /*fIncludeBackgrounds*/)
{
    // The base xterm mode only knows about 16 colors
    return VtEngine::_16ColorUpdateDrawingBrushes(colorForeground, colorBackground, _ColorTable, _cColorTable);
}

// Routine Description:
// - Write a VT sequence to move the cursor to the specified coordinates. We 
//      also store the last place we left the cursor for future optimizations.
//  If the cursor only needs to go to the origin, only write the home sequence.
//  If the new cursor is only down one line from the current, only write a newline
//  If the new cursor is only down one line and at the start of the line, write 
//      a carriage return.
//  Otherwise just write the whole sequence for moving it.
// Arguments:
// - coord: location to move the cursor to.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT XtermEngine::_MoveCursor(COORD const coord)
{
    HRESULT hr = S_OK;
    if (coord.X != _lastText.X || coord.Y != _lastText.Y)
    {
        if (coord.X == 0 && coord.Y == 0)
        {
            hr = _CursorHome();
        }
        else if (coord.X == 0 && coord.Y == (_lastText.Y+1))
        {
            // Down one line, at the start of the line.
            std::string seq = "\r\n";
            hr = _Write(seq);
        }
        else if (coord.X == 0 && coord.Y == _lastText.Y)
        {
            // Start of this line
            std::string seq = "\r";
            hr = _Write(seq);
        }
        else if (coord.X == _lastText.X && coord.Y == (_lastText.Y+1))
        {
            // Down one line, same X position
            std::string seq = "\n";
            hr = _Write(seq);
        }
        else
        {
            hr = _CursorPosition(coord);
        }

        if (SUCCEEDED(hr))
        {
            _lastText = coord;
        }
    }
    return hr;
}

// Routine Description:
// - Scrolls the existing data on the in-memory frame by the scroll region 
//      deltas we have collectively received through the Invalidate methods 
//      since the last time this was called.
//  Move the cursor to the origin, and insert or delete rows as appropriate.
//      The inserted rows will be blank, but marked invalid by InvalidateScroll,
//      so they will later be written by PaintBufferLine.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT XtermEngine::ScrollFrame()
{
    if (_scrollDelta.X != 0)
    {
        // No easy way to shift left-right. Everything needs repainting.
        return InvalidateAll();
    }
    if (_scrollDelta.Y == 0)
    {
        // There's nothing to do here. Do nothing.
        return S_OK;
    }

    const short dy = _scrollDelta.Y;
    const short absDy = static_cast<short>(abs(dy));

    Viewport view(_srLastViewport);
    SMALL_RECT v = _srLastViewport;
    view.ConvertToOrigin(&v);

    HRESULT hr = _MoveCursor({0,0});
    if (SUCCEEDED(hr))
    {
        if (dy < 0)
        {
            hr = _DeleteLine(absDy);
        }
        else if (dy > 0)
        {
            hr = _InsertLine(absDy);
        }
    }

    return hr;
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen 
//      area. Add the top or bottom rows to the invalid region, and update the
//      total scroll delta accumulated this frame.
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the 
//      console would like us to move while scrolling.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for safemath failure
HRESULT XtermEngine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    const short dx = pcoordDelta->X;
    const short dy = pcoordDelta->Y;

    if (dx != 0 || dy != 0)
    {
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
