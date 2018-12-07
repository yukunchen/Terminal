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
                               const IDefaultColorProvider& colorProvider,
                               const Viewport initialViewport,
                               _In_reads_(cColorTable) const COLORREF* const ColorTable,
                               const WORD cColorTable) :
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
[[nodiscard]]
HRESULT Xterm256Engine::UpdateDrawingBrushes(const COLORREF colorForeground,
                                             const COLORREF colorBackground,
                                             const WORD legacyColorAttribute,
                                             const bool isBold,
                                             const bool /*fIncludeBackgrounds*/) noexcept
{
    RETURN_IF_FAILED(_UpdateUnderline(legacyColorAttribute));

    return VtEngine::_RgbUpdateDrawingBrushes(colorForeground,
                                              colorBackground,
                                              isBold,
                                              _ColorTable,
                                              _cColorTable);
}
