/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "Xterm256Engine.hpp"
#pragma hdrstop
using namespace Microsoft::Console;
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

Xterm256Engine::Xterm256Engine(_In_ wil::unique_hfile hPipe,
                               _In_ const IDefaultColorProvider& colorProvider,
                               _In_ const Viewport initialViewport,
                               _In_reads_(cColorTable) const COLORREF* const ColorTable,
                               _In_ const WORD cColorTable) :
    XtermEngine(std::move(hPipe), colorProvider, initialViewport, ColorTable, cColorTable, false)
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
    return VtEngine::_RgbUpdateDrawingBrushes(colorForeground,
                                              colorBackground,
                                              _ColorTable,
                                              _cColorTable);
}
