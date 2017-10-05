/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "Xterm256Engine.hpp"
#include "..\..\inc\Viewport.hpp"
#pragma hdrstop
using namespace Microsoft::Console::Render;

Xterm256Engine::Xterm256Engine(wil::unique_hfile hPipe)
    : XtermEngine(hPipe, nullptr, 0)
{
}

// Routine Description:
// - This method will set the GDI brushes in the drawing context (and update the hung-window background color)
// Arguments:
// - wTextAttributes - A console attributes bit field specifying the brush colors we should use.
// Return Value:
// - S_OK if set successfully or relevant GDI error via HRESULT.
HRESULT Xterm256Engine::UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds)
{
    UNREFERENCED_PARAMETER(legacyColorAttribute);
    UNREFERENCED_PARAMETER(fIncludeBackgrounds);
    return VtEngine::_RgbUpdateDrawingBrushes(colorForeground, colorBackground);
}
