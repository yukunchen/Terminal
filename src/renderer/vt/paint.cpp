/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"
#include "../../inc/conattrs.hpp"
#include "../../types/inc/convert.hpp"

#pragma hdrstop
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

// Routine Description:
// - Prepares internal structures for a painting operation.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we started to paint. S_FALSE if we didn't need to paint.
//      HRESULT error code if painting didn't start successfully.
[[nodiscard]]
HRESULT VtEngine::StartPaint()
{
    if (_pipeBroken)
    {
        return S_FALSE;
    }

    // If there's nothing to do, quick return
    bool somethingToDo = _fInvalidRectUsed ||
        (_scrollDelta.X != 0 || _scrollDelta.Y != 0) ||
        _cursorMoved;

    _quickReturn = !somethingToDo;

    return _quickReturn ? S_FALSE : S_OK;
}

// Routine Description:
// - EndPaint helper to perform the final cleanup after painting. If we
//      returned S_FALSE from StartPaint, there's no guarantee this was called.
//      That's okay however, EndPaint only zeros structs that would be zero if
//      StartPaint returns S_FALSE.
// Arguments:
// - <none>
// Return Value:
// - S_OK, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT VtEngine::EndPaint()
{
    _invalidRect = Viewport({ 0 });
    _fInvalidRectUsed = false;
    _scrollDelta = {0};
    _clearedAllThisFrame = false;
    _cursorMoved = false;
    _firstPaint = false;
    _skipCursor = false;
    // If we've circled the buffer this frame, move our virtual top upwards.
    // We do this at the END of the frame, so that during the paint, we still
    //      use the original virtual top.
    if (_circled)
    {
        if (_virtualTop > 0)
        {
            _virtualTop--;
        }
    }
    _circled = false;
    return S_OK;
}

// Routine Description:
// - Paints the background of the invalid area of the frame.
// Arguments:
// - <none>
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::PaintBackground()
{
    return S_OK;
}

// Routine Description:
// - Draws one line of the buffer to the screen. Writes the characters to the
//      pipe. If the characters are outside the ASCII range (0-0x7f), then
//      instead writes a '?'
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
HRESULT VtEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                  _In_reads_(cchLine) const unsigned char* const rgWidths,
                                  const size_t cchLine,
                                  const COORD coord,
                                  const bool /*fTrimLeft*/,
                                  const bool /*lineWrapped*/)
{
    return VtEngine::_PaintAsciiBufferLine(pwsLine, rgWidths, cchLine, coord);
}

// Method Description:
// - Draws up to one line worth of grid lines on top of characters.
// Arguments:
// - lines - Enum defining which edges of the rectangle to draw
// - color - The color to use for drawing the edges.
// - cchLine - How many characters we should draw the grid lines along (left to right in a row)
// - coordTarget - The starting X/Y position of the first character to draw on.
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::PaintBufferGridLines(const GridLines /*lines*/,
                                       const COLORREF /*color*/,
                                       const size_t /*cchLine*/,
                                       const COORD /*coordTarget*/)
{
    return S_OK;
}

// Routine Description:
// - Draws the cursor on the screen
// Arguments:
// - ulHeightPercent - The cursor will be drawn at this percentage of the
//      current font height.
// - fIsDoubleWidth - The cursor should be drawn twice as wide as usual.
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT VtEngine::PaintCursor(const COORD coordCursor,
                              const ULONG /*ulCursorHeightPercent*/,
                              const bool /*fIsDoubleWidth*/,
                              const CursorType /*cursorType*/,
                              const bool /*fUseColor*/,
                              const COLORREF /*cursorColor*/)
{
    // MSFT:15933349 - Send the terminal the updated cursor information, if it's changed.
    LOG_IF_FAILED(_MoveCursor(coordCursor));

    return S_OK;
}

// Routine Description:
// - Clears out the cursor that was set in the previous PaintCursor call.
//      VT doesn't need to do anything to "unpaint" the old cursor location.
// Arguments:
// - <none>
// Return Value:
// - S_OK
[[nodiscard]]
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
//  - rectangles - Vector of rectangles, one per line, that should be inverted to make the selection area
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::PaintSelection(const std::vector<SMALL_RECT>& /*rectangles*/)
{
    return S_OK;
}

// Routine Description:
// - Write a VT sequence to change the current colors of text. Writes true RGB
//      color sequences.
// Arguments:
// - colorForeground: The RGB Color to use to paint the foreground text.
// - colorBackground: The RGB Color to use to paint the background of the text.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT VtEngine::_RgbUpdateDrawingBrushes(const COLORREF colorForeground,
                                           const COLORREF colorBackground,
                                           _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                           const WORD cColorTable)
{
    WORD wFoundColor = 0;
    if (colorForeground != _LastFG)
    {
        if (colorForeground == _colorProvider.GetDefaultForeground())
        {
            RETURN_IF_FAILED(_SetGraphicsRenditionDefaultColor(true));
        }
        else if (::FindTableIndex(colorForeground, ColorTable, cColorTable, &wFoundColor))
        {
            RETURN_IF_FAILED(_SetGraphicsRendition16Color(wFoundColor, true));
        }
        else
        {
            RETURN_IF_FAILED(_SetGraphicsRenditionRGBColor(colorForeground, true));
        }
        _LastFG = colorForeground;

    }

    if (colorBackground != _LastBG)
    {
        if (colorBackground == _colorProvider.GetDefaultBackground())
        {
            RETURN_IF_FAILED(_SetGraphicsRenditionDefaultColor(false));
        }
        else if (::FindTableIndex(colorBackground, ColorTable, cColorTable, &wFoundColor))
        {
            RETURN_IF_FAILED(_SetGraphicsRendition16Color(wFoundColor, false));
        }
        else
        {
            RETURN_IF_FAILED(_SetGraphicsRenditionRGBColor(colorBackground, false));
        }
        _LastBG = colorBackground;
    }

    return S_OK;
}

// Routine Description:
// - Write a VT sequence to change the current colors of text. It will try to
//      find the colors in the color table that are nearest to the input colors,
//       and write those indicies to the pipe.
// Arguments:
// - colorForeground: The RGB Color to use to paint the foreground text.
// - colorBackground: The RGB Color to use to paint the background of the text.
// - ColorTable: An array of colors to find the closest match to.
// - cColorTable: size of the color table.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT VtEngine::_16ColorUpdateDrawingBrushes(const COLORREF colorForeground,
                                               const COLORREF colorBackground,
                                               _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                               const WORD cColorTable)
{
    if (colorForeground != _LastFG)
    {
        const WORD wNearestFg = ::FindNearestTableIndex(colorForeground, ColorTable, cColorTable);
        RETURN_IF_FAILED(_SetGraphicsRendition16Color(wNearestFg, true));

        _LastFG = colorForeground;
    }

    if (colorBackground != _LastBG)
    {
        const WORD wNearestBg = ::FindNearestTableIndex(colorBackground, ColorTable, cColorTable);
        RETURN_IF_FAILED(_SetGraphicsRendition16Color(wNearestBg, false));

        _LastBG = colorBackground;
    }

    return S_OK;
}

// Routine Description:
// - Draws one line of the buffer to the screen. Writes the characters to the
//      pipe. If the characters are outside the ASCII range (0-0x7f), then
//      instead writes a '?'.
//   This is needed because the Windows internal telnet client implementation
//      doesn't know how to handle >ASCII characters. The old telnetd would
//      just replace them with '?' characters. If we render the >ASCII
//      characters to telnet, it will likely end up drawing them wrong, which
//      will make the client appear buggy and broken.
// Arguments:
// - pwsLine - string of text to be written
// - rgWidths - array specifying how many column widths that the console is
//      expecting each character to take
// - cchLine - length of line to be read
// - coord - character coordinate target to render within viewport
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT VtEngine::_PaintAsciiBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                        _In_reads_(cchLine) const unsigned char* const rgWidths,
                                        const size_t cchLine,
                                        const COORD coord)
{
    RETURN_IF_FAILED(_MoveCursor(coord));

    short totalWidth = 0;
    for (size_t i = 0; i < cchLine; i++)
    {
        RETURN_IF_FAILED(ShortAdd(totalWidth, static_cast<short>(rgWidths[i]), &totalWidth));
    }

    std::wstring wstr = std::wstring(pwsLine, cchLine);
    RETURN_IF_FAILED(VtEngine::_WriteTerminalAscii(wstr));

    // Update our internal tracker of the cursor's position
    _lastText.X += totalWidth;

    return S_OK;
}

// Routine Description:
// - Draws one line of the buffer to the screen. Writes the characters to the
//      pipe, encoded in UTF-8.
// Arguments:
// - pwsLine - string of text to be written
// - rgWidths - array specifying how many column widths that the console is
//      expecting each character to take
// - cchLine - length of line to be read
// - coord - character coordinate target to render within viewport
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT VtEngine::_PaintUtf8BufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                       _In_reads_(cchLine) const unsigned char* const rgWidths,
                                       const size_t cchLine,
                                       const COORD coord)
{
    if (coord.Y < _virtualTop)
    {
        return S_OK;
    }

    RETURN_IF_FAILED(_MoveCursor(coord));

    short totalWidth = 0;
    for (size_t i = 0; i < cchLine; i++)
    {
        RETURN_IF_FAILED(ShortAdd(totalWidth, static_cast<short>(rgWidths[i]), &totalWidth));
    }

    bool foundNonspace = false;
    size_t lastNonSpace = 0;
    for (size_t i = 0; i < cchLine; i++)
    {
        if (pwsLine[i] != L'\x20')
        {
            lastNonSpace = i;
            foundNonspace = true;
        }
    }
    // Examples:
    // - "  ":
    //      cch = 2, lastNonSpace = 0, foundNonSpace = false
    //      cch-lastNonSpace = 2 -> good
    //      cch-lastNonSpace-(0) = 2 -> good
    // - "A "
    //      cch = 2, lastNonSpace = 0, foundNonSpace = true
    //      cch-lastNonSpace = 2 -> bad
    //      cch-lastNonSpace-(1) = 1 -> good
    // - "AA"
    //      cch = 2, lastNonSpace = 1, foundNonSpace = true
    //      cch-lastNonSpace = 1 -> bad
    //      cch-lastNonSpace-(1) = 0 -> good
    const size_t numSpaces = cchLine - lastNonSpace - (foundNonspace ? 1 : 0);

    // Optimizations:
    // If there are lots of spaces at the end of the line, we can try to Erase
    //      Character that number of spaces, then move the cursor forward (to
    //      where it would be if we had written the spaces)
    // An erase character and move right sequence is 8 chars, and possibly 10
    //      (if there are at least 10 spaces, 2 digits to print)
    // ESC [ %d X ESC [ %d C
    // ESC [ %d %d X ESC [ %d %d C
    // So we need at least 9 spaces for the optimized sequence to make sense.
    // Also, if we already erased the entire display this frame, then
    //    don't do ANYTHING with erasing at all.

    // Note: We're only doing these optimizations along the UTF-8 path, because
    // the inbox telnet client doesn't understand the Erase Character sequence,
    // and it uses xterm-ascii. This ensures that xterm and -256color consumers
    // get the enhancements, and telnet isn't broken.

    const bool useEraseChar = (numSpaces > ERASE_CHARACTER_STRING_LENGTH) &&
                              (!_clearedAllThisFrame);
    // If we're not using erase char, but we did erase all at the start of the
    //      frame, don't add spaces at the end.
    const size_t cchActual = (useEraseChar || _clearedAllThisFrame) ?
                                (cchLine - numSpaces) :
                                cchLine;

    // Write the actual text string
    std::wstring wstr = std::wstring(pwsLine, cchActual);
    RETURN_IF_FAILED(VtEngine::_WriteTerminalUtf8(wstr));

    if (useEraseChar)
    {
        RETURN_IF_FAILED(_EraseCharacter(static_cast<short>(numSpaces)));
        RETURN_IF_FAILED(_CursorForward(static_cast<short>(numSpaces)));
    }

    // Update our internal tracker of the cursor's position
    _lastText.X += totalWidth;

    return S_OK;
}

// Method Description:
// - Updates the window's title string. Emits the VT sequence to SetWindowTitle.
//      Because wintelnet does not understand these sequences by default, we
//      don't do anything by default. Other modes can implement if they support
//      the sequence.
// Arguments:
// - newTitle: the new string to use for the title of the window
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::UpdateTitle(const std::wstring& /*newTitle*/)
{
    return S_OK;
}
