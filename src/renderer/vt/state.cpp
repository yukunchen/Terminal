/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"

#include <sstream>

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Creates a new GDI-based rendering engine
// - NOTE: Will throw if initialization failure. Caller must catch.
// Arguments:
// - <none>
// Return Value:
// - An instance of a Renderer.
VtEngine::VtEngine(wil::unique_hfile pipe) 
{
    _hFile = std::move(pipe);
    THROW_IF_HANDLE_INVALID(_hFile.get());

    _srLastViewport = {0};
    _srcInvalid = {0};
    _lastRealCursor = {0};
    _lastText = {0};
    _scrollDelta = {0};
    _LastFG = 0xff000000;
    _LastBG = 0xff000000;

}

// Routine Description:
// - Destroys an instance of a GDI-based rendering engine
// Arguments:
// - <none>
// Return Value:
// - <none>
VtEngine::~VtEngine()
{
}

HRESULT VtEngine::_Write(_In_reads_(cch) PCSTR psz, _In_ size_t const cch)
{
    bool fSuccess = !!WriteFile(_hFile.get(), psz, (DWORD)cch, nullptr, nullptr);
    RETURN_LAST_ERROR_IF_FALSE(fSuccess);

    return S_OK;
}

HRESULT VtEngine::_Write(_In_ std::string& str)
{
    return _Write(str.c_str(), str.length());
}


// Routine Description:
// - This method will update the active font on the current device context
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// Arguments:
// - pfiFontDesired - Pointer to font information we should use while instantiating a font.
// - pfiFont - Pointer to font information where the chosen font information will be populated.
// Return Value:
// - S_OK if set successfully or relevant GDI error via HRESULT.
HRESULT VtEngine::UpdateFont(_In_ FontInfoDesired const * const pfiFontDesired, _Out_ FontInfo* const pfiFont)
{
    pfiFontDesired;
    pfiFont;
    return S_OK;
}

// Routine Description:
// - This method will modify the DPI we're using for scaling calculations.
// Arguments:
// - iDpi - The Dots Per Inch to use for scaling. We will use this relative to the system default DPI defined in Windows headers as a constant.
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT VtEngine::UpdateDpi(_In_ int const iDpi)
{
    iDpi;
    return S_OK;
}

HRESULT VtEngine::UpdateViewport(_In_ SMALL_RECT const srNewViewport)
{
    _srLastViewport = srNewViewport;

    // If the w,h has changed, then send a window update.

    // UNREFERENCED_PARAMETER(srNewViewport);
    return S_OK;
}

// Routine Description:
// - This method will figure out what the new font should be given the starting font information and a DPI.
// - When the final font is determined, the FontInfo structure given will be updated with the actual resulting font chosen as the nearest match.
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// - If the intent is to immediately turn around and use this font, pass the optional handle parameter and use it immediately.
// Arguments:
// - pfiFontDesired - Pointer to font information we should use while instantiating a font.
// - pfiFont - Pointer to font information where the chosen font information will be populated.
// - iDpi - The DPI we will have when rendering
// Return Value:
// - S_OK if set successfully or relevant GDI error via HRESULT.
HRESULT VtEngine::GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired, _Out_ FontInfo* const pfiFont, _In_ int const iDpi)
{
    pfiFontDesired;
    pfiFont;
    iDpi;
    return S_OK;
}

// Routine Description:
// - Retrieves the current pixel size of the font we have selected for drawing.
// Arguments:
// - <none>
// Return Value:
// - X by Y size of the font.
COORD VtEngine::GetFontSize()
{
    return{ 1, 1 };
    // return{ 0, 0 };
}
