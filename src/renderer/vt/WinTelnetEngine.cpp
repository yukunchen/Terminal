/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WinTelnetEngine.hpp"
#include "..\..\inc\conattrs.hpp"
#pragma hdrstop
using namespace Microsoft::Console;
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

WinTelnetEngine::WinTelnetEngine(_In_ wil::unique_hfile hPipe,
                                 _In_ const IDefaultColorProvider& colorProvider,
                                 _In_ const Viewport initialViewport,
                                 _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                 _In_ const WORD cColorTable) :
    VtEngine(std::move(hPipe), colorProvider, initialViewport),
    _ColorTable(ColorTable),
    _cColorTable(cColorTable)
{

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
HRESULT WinTelnetEngine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                              _In_ COLORREF const colorBackground,
                                              _In_ WORD const /*legacyColorAttribute*/,
                                              _In_ bool const /*fIncludeBackgrounds*/)
{
    return VtEngine::_16ColorUpdateDrawingBrushes(colorForeground, colorBackground, _ColorTable, _cColorTable);
}

// Routine Description:
// - Write a VT sequence to move the cursor to the specified coordinates. We
//      also store the last place we left the cursor for future optimizations.
// Arguments:
// - coord: location to move the cursor to.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT WinTelnetEngine::_MoveCursor(COORD const coord)
{
    HRESULT hr = S_OK;
    // don't try and be clever about moving the cursor.
    // Always just use the full sequence
    if (coord.X != _lastText.X || coord.Y != _lastText.Y)
    {
        hr = _CursorPosition(coord);
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
//  Because win-telnet doesn't know how to do anything smart in response to
//      scrolling, we do nothing.
// Arguments:
// - <none>
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT WinTelnetEngine::ScrollFrame()
{
    // win-telnet doesn't know anything about scroll vt sequences
    // every frame, we're repainitng everything, always.
    return S_OK;
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen
//      area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the
//      console would like us to move while scrolling.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
[[nodiscard]]
HRESULT WinTelnetEngine::InvalidateScroll(_In_ const COORD* const /*pcoordDelta*/)
{
    // win-telnet assumes the client doesn't know anything about inserting or
    //  deleting lines.
    // So instead, just invalidate the entire viewport. Every line is going to
    //  have to move.
    return InvalidateAll();
}

// Method Description:
// - Wrapper for ITerminalOutputConnection. See _Write.
[[nodiscard]]
HRESULT WinTelnetEngine::WriteTerminalW(_In_ const std::wstring& wstr)
{
    return VtEngine::_WriteTerminalAscii(wstr);
}
