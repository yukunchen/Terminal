/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "Xterm256Engine.hpp"
#pragma hdrstop
using namespace Microsoft::Console::Render;

Xterm256Engine::Xterm256Engine(_In_ wil::unique_hfile hPipe) :
    XtermEngine(std::move(hPipe), nullptr, 0)
{
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
HRESULT Xterm256Engine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                             _In_ COLORREF const colorBackground,
                                             _In_ WORD const /*legacyColorAttribute*/,
                                             _In_ bool const /*fIncludeBackgrounds*/)
{
    return VtEngine::_RgbUpdateDrawingBrushes(colorForeground, colorBackground);
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
// - fTrimLeft - This specifies whether to trim one character width off the left
//      side of the output. Used for drawing the right-half only of a 
//      double-wide character.
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
HRESULT Xterm256Engine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                  _In_reads_(cchLine) const unsigned char* const rgWidths,
                                  _In_ size_t const cchLine,
                                  _In_ COORD const coord,
                                  _In_ bool const /*fTrimLeft*/)
{
    return VtEngine::_PaintUtf8BufferLine(pwsLine, rgWidths, cchLine, coord);
}
