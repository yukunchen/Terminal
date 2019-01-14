/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "XtermEngine.hpp"
#include "../../types/inc/convert.hpp"
#pragma hdrstop
using namespace Microsoft::Console;
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

XtermEngine::XtermEngine(_In_ wil::unique_hfile hPipe,
                         const IDefaultColorProvider& colorProvider,
                         const Viewport initialViewport,
                         _In_reads_(cColorTable) const COLORREF* const ColorTable,
                         const WORD cColorTable,
                         const bool fUseAsciiOnly) :
    VtEngine(std::move(hPipe), colorProvider, initialViewport),
    _ColorTable(ColorTable),
    _cColorTable(cColorTable),
    _fUseAsciiOnly(fUseAsciiOnly),
    _previousLineWrapped(false),
    _usingUnderLine(false)
{
    // Set out initial cursor position to -1, -1. This will force our initial
    //      paint to manually move the cursor to 0, 0, not just ignore it.
    _lastText = VtEngine::INVALID_COORDS;
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
[[nodiscard]]
HRESULT XtermEngine::StartPaint() noexcept
{
    RETURN_IF_FAILED(VtEngine::StartPaint());
    if (_firstPaint)
    {
        // MSFT:17815688
        // If the caller requested to inherit the cursor, we shouldn't
        //      clear the screen on the first paint. Otherwise, we'll clear
        //      the screen on the first paint, just to make sure that the
        //      terminal's state is consistent with what we'll be rendering.
        RETURN_IF_FAILED(_ClearScreen());
        _clearedAllThisFrame = true;
        _firstPaint = false;
    }
    else
    {
        if (!_resized && Viewport::FromInclusive(GetDirtyRectInChars()) == _lastViewport)
        {
            RETURN_IF_FAILED(_ClearScreen());
            _clearedAllThisFrame = true;
        }
    }

    if (!_quickReturn)
    {
        if (!_WillWriteSingleChar())
        {
            // Turn off cursor
            RETURN_IF_FAILED(_HideCursor());
        }
        else
        {
            // Don't re-enable the cursor.
            _quickReturn = true;
        }
    }

    return S_OK;
}

// Routine Description:
// - EndPaint helper to perform the final rendering steps. Turn the cursor back
//      on.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT XtermEngine::EndPaint() noexcept
{
    if (!_quickReturn)
    {
        // Turn on cursor
        RETURN_IF_FAILED(_ShowCursor());
    }

    RETURN_IF_FAILED(VtEngine::EndPaint());

    return S_OK;
}


// Routine Description:
// - Write a VT sequence to either start or stop underlining text.
// Arguments:
// - legacyColorAttribute: A console attributes bit field containing information
//      about the underlining state of the text.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT XtermEngine::_UpdateUnderline(const WORD legacyColorAttribute) noexcept
{
    bool textUnderlined = WI_IsFlagSet(legacyColorAttribute, COMMON_LVB_UNDERSCORE);
    if (textUnderlined != _usingUnderLine)
    {
        if (textUnderlined)
        {
            RETURN_IF_FAILED(_BeginUnderline());
        }
        else
        {
            RETURN_IF_FAILED(_EndUnderline());
        }
        _usingUnderLine = textUnderlined;
    }
    return S_OK;
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
[[nodiscard]]
HRESULT XtermEngine::UpdateDrawingBrushes(const COLORREF colorForeground,
                                          const COLORREF colorBackground,
                                          const WORD legacyColorAttribute,
                                          const bool isBold,
                                          const bool /*fIncludeBackgrounds*/) noexcept
{
    //When we update the brushes, check the wAttrs to see if the LVB_UNDERSCORE
    //      flag is there. If the state of that flag is different then our
    //      current state, change the underlining state.
    // We have to do this here, instead of in PaintBufferGridLines, because
    //      we'll have already painted the text by the time PaintBufferGridLines
    //      is called.

    RETURN_IF_FAILED(_UpdateUnderline(legacyColorAttribute));
    // The base xterm mode only knows about 16 colors
    return VtEngine::_16ColorUpdateDrawingBrushes(colorForeground, colorBackground, isBold, _ColorTable, _cColorTable);
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
[[nodiscard]]
HRESULT XtermEngine::_MoveCursor(COORD const coord) noexcept
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

            // If the previous line wrapped, then the cursor is already at this
            //      position, we just don't know it yet. Don't emit anything.
            if (_previousLineWrapped)
            {
                hr = S_OK;
            }
            else
            {
                std::string seq = "\r\n";
                hr = _Write(seq);
            }
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
        else if (coord.X == (_lastText.X-1) && coord.Y == (_lastText.Y))
        {
            // Back one char, same Y position
            std::string seq = "\b";
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
[[nodiscard]]
HRESULT XtermEngine::ScrollFrame() noexcept
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

    HRESULT hr = S_OK;
    if (dy < 0)
    {
        // Instead of deleting the first line (causing everything to move up)
        // move to the bottom of the buffer, and newline.
        //      That will cause everything to move up, by moving the viewport down.
        // This will let remote conhosts scroll up to see history like normal.
        const short bottom = _lastViewport.ToOrigin().BottomInclusive();
        hr = _MoveCursor({0, bottom});
        if (SUCCEEDED(hr))
        {
            std::string seq = std::string(absDy, '\n');
            hr = _Write(seq);
        }
        // We don't need to _MoveCursor the cursor again, because it's still
        //      at the bottom of the viewport.
    }
    else if (dy > 0)
    {
        // Move to the top of the buffer, and insert some lines of text, to
        //      cause the viewport contents to shift down.
        hr = _MoveCursor({0, 0});
        if (SUCCEEDED(hr))
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
[[nodiscard]]
HRESULT XtermEngine::InvalidateScroll(const COORD* const pcoordDelta) noexcept
{
    const short dx = pcoordDelta->X;
    const short dy = pcoordDelta->Y;

    if (dx != 0 || dy != 0)
    {
        // Scroll the current offset
        RETURN_IF_FAILED(_InvalidOffset(pcoordDelta));

        // Add the top/bottom of the window to the invalid area
        SMALL_RECT invalid = _lastViewport.ToOrigin().ToExclusive();

        if (dy > 0)
        {
            invalid.Bottom = dy;
        }
        else if (dy < 0)
        {
            invalid.Top = invalid.Bottom + dy;
        }
        LOG_IF_FAILED(_InvalidCombine(Viewport::FromExclusive(invalid)));

        COORD invalidScrollNew;
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.X, dx, &invalidScrollNew.X));
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.Y, dy, &invalidScrollNew.Y));

        // Store if safemath succeeded
        _scrollDelta = invalidScrollNew;

    }

    return S_OK;
}

// Routine Description:
// - Draws one line of the buffer to the screen. Writes the characters to the
//      pipe, encoded in UTF-8 or ASCII only, depending on the VtIoMode.
//      (See descriptions of both implementations for details.)
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
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT XtermEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                     _In_reads_(cchLine) const unsigned char* const rgWidths,
                                     const size_t cchLine,
                                     const COORD coord,
                                     const bool /*fTrimLeft*/,
                                     const bool /*lineWrapped*/) noexcept
{
    return _fUseAsciiOnly ?
        VtEngine::_PaintAsciiBufferLine(pwsLine, rgWidths, cchLine, coord) :
        VtEngine::_PaintUtf8BufferLine(pwsLine, rgWidths, cchLine, coord);
}

// Method Description:
// - Wrapper for ITerminalOutputConnection. Write either an ascii-only, or a
//      proper utf-8 string, depending on our mode.
// Arguments:
// - wstr - wstring of text to be written
// Return Value:
// - S_OK or suitable HRESULT error from either conversion or writing pipe.
[[nodiscard]]
HRESULT XtermEngine::WriteTerminalW(const std::wstring& wstr) noexcept
{
    return _fUseAsciiOnly ?
        VtEngine::_WriteTerminalAscii(wstr) :
        VtEngine::_WriteTerminalUtf8(wstr);
}

// Method Description:
// - Updates the window's title string. Emits the VT sequence to SetWindowTitle.
// Arguments:
// - newTitle: the new string to use for the title of the window
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT XtermEngine::_DoUpdateTitle(const std::wstring& newTitle) noexcept
{
    // inbox telnet uses xterm-ascii as it's mode. If we're in ascii mode, don't
    //      do anything, to maintain compatibility.
    if (_fUseAsciiOnly)
    {
        return S_OK;
    }

    try
    {
        const auto converted = ConvertToA(CP_UTF8, newTitle);
        return VtEngine::_ChangeTitle(converted);
    }
    CATCH_RETURN();
}
