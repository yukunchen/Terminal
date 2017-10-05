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
// - Creates a new VT-based rendering engine
// - NOTE: Will throw if initialization failure. Caller must catch.
// Arguments:
// - <none>
// Return Value:
// - An instance of a Renderer.
VtEngine::VtEngine(wil::unique_hfile pipe) :
    _hFile(std::move(pipe)),
    _srLastViewport({0}),
    _srcInvalid({0}),
    _lastRealCursor({0}),
    _lastText({0}),
    _scrollDelta({0}),
    _LastFG(0xff000000),
    _LastBG(0xff000000),
    _usingTestCallback(false)

{
    // When unit testing, we can instantiate a VtEngine without a pipe.
    // THROW_IF_HANDLE_INVALID(_hFile.get());
    
}

// Routine Description:
// - Destroys an instance of a VT-based rendering engine
// Arguments:
// - <none>
// Return Value:
// - <none>
VtEngine::~VtEngine()
{
}

// Method Description:
// - Writes the characters to our file handle. If we're building the unit tests, 
//      we can instead write to the test callback, in order to avoid needing to 
//      set up pipes and threads for unit tests.
// Arguments:
// - psz: The buffer the write to the pipe. Might have nulls in it. 
// - cch: size of psz
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
HRESULT VtEngine::_Write(_In_reads_(cch) PCSTR psz, _In_ size_t const cch)
{
    if (_usingTestCallback)
    {
        RETURN_LAST_ERROR_IF_FALSE(_pfnTestCallback(psz, cch));
        return S_OK;
    }

    bool fSuccess = !!WriteFile(_hFile.get(), psz, (DWORD)cch, nullptr, nullptr);
    RETURN_LAST_ERROR_IF_FALSE(fSuccess);

    return S_OK;
}

// Method Description:
// - Helper for calling _Write with a std::string
// Arguments:
// - str: the string of characters to write to the pipe.
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
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
    UNREFERENCED_PARAMETER(pfiFontDesired);
    UNREFERENCED_PARAMETER(pfiFont);
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
    UNREFERENCED_PARAMETER(iDpi);
    return S_OK;
}

HRESULT VtEngine::UpdateViewport(_In_ SMALL_RECT const srNewViewport)
{
    _srLastViewport = srNewViewport;

    // If the viewport has changed, then send a window update.
    // TODO: 13847317(adapter), 13271084(renderer)

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
    UNREFERENCED_PARAMETER(pfiFontDesired);
    UNREFERENCED_PARAMETER(pfiFont);
    UNREFERENCED_PARAMETER(iDpi);
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
}

// Method Description:
// - Sets the test callback for this instance. Instead of rendering to a pipe, 
//      this instance will instead render to a callback for testing.
// Arguments:
// - pfn: a callback to call instead of writing to the pipe.
// Return Value:
// - <none>
void VtEngine::SetTestCallback(_In_ std::function<bool(const char* const, size_t const)> pfn)
{
    _pfnTestCallback = pfn;
    _usingTestCallback = true;
}


