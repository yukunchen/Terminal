/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"
#include "..\..\inc\Viewport.hpp"
#include "..\..\inc\conattrs.hpp"

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
    // If there's nothing to do, quick return
    _quickReturn = false;
    _quickReturn |= (_fInvalidRectUsed);
    _quickReturn |= (_scrollDelta.X != 0 || _scrollDelta.Y != 0);

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
// - Draws one line of the buffer to the screen. Writes the characters to the 
//      pipe, encoded in UTF-8.
// Arguments:
// - pwsLine - string of text to be written
// - rgWidths - array specifying how many column widths that the console is 
//      expecting each character to take
// - cchLine - length of line to be read
// - coord - character coordinate target to render within viewport
// - fTrimLeft - This specifies whether to trim one character width off the left
//      side of the output. Used for drawing the right-half only of a 
//      double-wide character.
// Return Value:
// - S_OK or suitable HRESULT error frow writing pipe.
HRESULT VtEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                  _In_reads_(cchLine) const unsigned char* const rgWidths,
                                  _In_ size_t const cchLine,
                                  _In_ COORD const coord,
                                  _In_ bool const fTrimLeft)
{
    UNREFERENCED_PARAMETER(fTrimLeft);

    RETURN_IF_FAILED(_MoveCursor(coord));
    try
    {
        // TODO: MSFT:14099536 Try and optimize the spaces.
        size_t actualCch = cchLine;

        // Question for reviewers:
        // What's the right way to render the buffer text to utf-8 here?
        // Will this work?
        DWORD dwNeeded = WideCharToMultiByte(CP_ACP, 0, pwsLine, (int)actualCch, nullptr, 0, nullptr, nullptr);
        wistd::unique_ptr<char[]> rgchNeeded = wil::make_unique_nothrow<char[]>(dwNeeded + 1);
        RETURN_IF_NULL_ALLOC(rgchNeeded);
        RETURN_LAST_ERROR_IF_FALSE(WideCharToMultiByte(CP_ACP, 0, pwsLine, (int)actualCch, rgchNeeded.get(), dwNeeded, nullptr, nullptr));
        rgchNeeded[dwNeeded] = '\0';

        RETURN_IF_FAILED(_Write(rgchNeeded.get(), dwNeeded));
        
        // Update our internal tracker of the cursor's position
        short totalWidth = 0;
        for (size_t i=0; i < actualCch; i++)
        {
            totalWidth+=(short)rgWidths[i];
        }
        _lastText.X += totalWidth;

    }
    CATCH_RETURN();

    return S_OK;
}

// Method Description:
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
    UNREFERENCED_PARAMETER(lines);
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(cchLine);
    UNREFERENCED_PARAMETER(coordTarget);
    return S_OK;
}

// Routine Description:
// - Draws the cursor on the screen
// Arguments:
// - coord - Coordinate position where the cursor should be drawn
// - ulHeightPercent - The cursor will be drawn at this percentage of the 
//      current font height.
// - fIsDoubleWidth - The cursor should be drawn twice as wide as usual.
// Return Value:
// - S_OK, suitable GDI HRESULT error, or safemath error, or E_FAIL in a GDI
//       error where a specific error isn't set.
HRESULT VtEngine::PaintCursor(_In_ COORD const coord, _In_ ULONG const ulHeightPercent, _In_ bool const fIsDoubleWidth)
{
    coord;
    ulHeightPercent;
    fIsDoubleWidth;

    // TODO: MSFT 13310327
    // The cursor needs some help. It invalidates itself, and really that should 
    //      be the renderer's responsibility. We don't want to keep repainting 
    //      the character under the cursor.
    // if (_lastRealCursor.X != coord.X || _lastRealCursor.Y != coord.Y)
    // {
    //     _MoveCursor(coord);
    // }

    _lastRealCursor = coord;

    return S_OK;
}

// Routine Description:
// - Clears out the cursor that was set in the previous PaintCursor call.
//      VT doesn't need to do anything to "unpaint" the old cursor location.
// Arguments:
// - <none>
// Return Value:
// - S_OK
HRESULT VtEngine::ClearCursor()
{
    return S_OK;
}

// Routine Description:
//  - Inverts the selected region on the current screen buffer.
//  - Reads the selected area, selection mode, and active screen buffer
//    from the global properties and dispatches a GDI invert on the selected text area.
//  Because the selection is the responsibility of the terminal, and not the 
//      host, render nothing.
// Arguments:
//  - rgsrSelection - Array of rectangles, one per line, that should be inverted to make the selection area
// - cRectangles - Count of rectangle array length
// Return Value:
// - S_OK
HRESULT VtEngine::PaintSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    UNREFERENCED_PARAMETER(rgsrSelection);
    UNREFERENCED_PARAMETER(cRectangles);
    return S_OK;
}

// Routine Description:
// - Write a VT sequence to change the current colors of text. Writes true RGB 
//      color sequences.
// Arguments:
// - colorForeground: The RGB Color to use to paint the foreground text.
// - colorBackground: The RGB Color to use to paint the background of the text.
// - legacyColorAttribute: A console attributes bit field specifying the brush
//      colors we should use.
// - fIncludeBackgrounds: indicates if we should change the background color of 
//      the window. Unused for VT
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_RgbUpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground)
{
    try
    {
        if (colorForeground != _LastFG)
        {
            PCSTR pszFgFormat = "\x1b[38;2;%d;%d;%dm";
            DWORD const fgRed = (colorForeground & 0xff);
            DWORD const fgGreen = (colorForeground >> 8) & 0xff;
            DWORD const fgBlue = (colorForeground >> 16) & 0xff;
            
            int cchNeeded = _scprintf(pszFgFormat, fgRed, fgGreen, fgBlue);
            wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
            RETURN_IF_NULL_ALLOC(psz);

            int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFgFormat, fgRed, fgGreen, fgBlue);
            _Write(psz.get(), cchWritten);
            _LastFG = colorForeground;
            
        }
        if (colorBackground != _LastBG) 
        {
            PCSTR pszBgFormat = "\x1b[48;2;%d;%d;%dm";
            DWORD const bgRed = (colorBackground & 0xff);
            DWORD const bgGreen = (colorBackground >> 8) & 0xff;
            DWORD const bgBlue = (colorBackground >> 16) & 0xff;

            int cchNeeded = _scprintf(pszBgFormat, bgRed, bgGreen, bgBlue);
            wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
            RETURN_IF_NULL_ALLOC(psz);

            int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszBgFormat, bgRed, bgGreen, bgBlue);
            _Write(psz.get(), cchWritten);
            _LastBG = colorBackground;
        }
    }
    CATCH_RETURN();
    
    return S_OK;
}

// Routine Description:
// - Write a VT sequence to change the current colors of text. It will try to 
//      find the colors in the color table that are nearest to the input colors,
//       and write those inidicies to the pipe.
// Arguments:
// - colorForeground: The RGB Color to use to paint the foreground text.
// - colorBackground: The RGB Color to use to paint the background of the text.
// - legacyColorAttribute: A console attributes bit field specifying the brush
//      colors we should use.
// - fIncludeBackgrounds: indicates if we should change the background color of 
//      the window. Unused for VT
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_16ColorUpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_reads_(cColorTable) const COLORREF* const ColorTable, _In_ const WORD cColorTable)
{
    if (colorForeground != _LastFG)
    {
        WORD wNearestFg = ::FindNearestTableIndex(colorForeground, ColorTable, cColorTable);

        char* fmt = IsFlagSet(wNearestFg, FOREGROUND_INTENSITY)? 
            "\x1b[1m\x1b[%dm" : "\x1b[22m\x1b[%dm";

        int fg = 30
                 + (IsFlagSet(wNearestFg,FOREGROUND_RED)? 1 : 0)
                 + (IsFlagSet(wNearestFg,FOREGROUND_GREEN)? 2 : 0)
                 + (IsFlagSet(wNearestFg,FOREGROUND_BLUE)? 4 : 0)
                 ;
        int cchNeeded = _scprintf(fmt, fg);
        wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
        RETURN_IF_NULL_ALLOC(psz);

        int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, fmt, fg);
        RETURN_IF_FAILED(_Write(psz.get(), cchWritten));

        _LastFG = colorForeground;
    }

    if (colorBackground != _LastBG) 
    {
        WORD wNearestBg = ::FindNearestTableIndex(colorBackground, ColorTable, cColorTable);

        char* fmt = "\x1b[%dm";

        // Check using the foreground flags, because the bg flags are a higher byte
        int bg = 40
                 + (IsFlagSet(wNearestBg,FOREGROUND_RED)? 1 : 0)
                 + (IsFlagSet(wNearestBg,FOREGROUND_GREEN)? 2 : 0)
                 + (IsFlagSet(wNearestBg,FOREGROUND_BLUE)? 4 : 0)
                 ;

        int cchNeeded = _scprintf(fmt, bg);
        wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
        RETURN_IF_NULL_ALLOC(psz);

        int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, fmt, bg);
        RETURN_IF_FAILED(_Write(psz.get(), cchWritten));

        _LastBG = colorBackground;
    }

    return S_OK;
}

