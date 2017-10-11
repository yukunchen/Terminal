/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"

// For _vcprintf
#include <conio.h>
#include <stdarg.h>

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Creates a new VT-based rendering engine
// - NOTE: Will throw if initialization failure. Caller must catch.
// Arguments:
// - <none>
// Return Value:
// - An instance of a Renderer.
VtEngine::VtEngine(_In_ wil::unique_hfile pipe) :
    _hFile(std::move(pipe)),
    _srLastViewport({0}),
    _srcInvalid({0}),
    _lastRealCursor({0}),
    _lastText({0}),
    _scrollDelta({0}),
    _LastFG(INVALID_COLOR),
    _LastBG(INVALID_COLOR),
    _usingTestCallback(false)

{
#ifndef UNIT_TESTING
    // When unit testing, we can instantiate a VtEngine without a pipe.
    THROW_IF_HANDLE_INVALID(_hFile.get());
#endif
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
HRESULT VtEngine::_Write(_In_reads_(cch) const char* const psz, _In_ size_t const cch)
{
#ifdef UNIT_TESTING
    if (_usingTestCallback)
    {
        RETURN_LAST_ERROR_IF_FALSE(_pfnTestCallback(psz, cch));
        return S_OK;
    }
#endif
    
    bool fSuccess = !!WriteFile(_hFile.get(), psz, static_cast<DWORD>(cch), nullptr, nullptr);
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

// Method Description:
// - Helper for calling _Write with just a PCSTR.
// Arguments:
// - str: the string of characters to write to the pipe.
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
HRESULT VtEngine::_Write(_In_ const char* const psz)
{
    try
    {
        std::string seq = std::string(psz);
        _Write(seq.c_str(), seq.length());
    }
    CATCH_RETURN();

    return S_OK;
}

// Method Description:
// - Helper for calling _Write with a PCSTR for formatting a sequence. Used 
//      extensively by VtSequences.cpp
// Arguments:
// - pszFormat: the string of characters to write to the pipe.
// - ...: a va_list of args to format the string with.
// Return Value:
// - S_OK, E_INVALIDARG for a invalid format string, or suitable HRESULT error 
//      from writing pipe.
HRESULT VtEngine::_WriteFormattedString(_In_ const char* const pszFormat, ...)
{
    HRESULT hr = E_FAIL;
    va_list argList;
    va_start(argList, pszFormat);

    int cchNeeded = _scprintf(pszFormat, argList);
    // -1 is the _scprintf error case https://msdn.microsoft.com/en-us/library/t32cf9tb.aspx
    if (cchNeeded > -1)
    {
        wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
        RETURN_IF_NULL_ALLOC(psz);

        int cchWritten = _vsnprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFormat, argList);
        hr = _Write(psz.get(), cchWritten);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    va_end(argList);
    return hr;
}

// Method Description:
// - This method will update the active font on the current device context
//      Does nothing for vt, the font is handed by the terminal. 
// Arguments:
// - pfiFontDesired - Pointer to font information we should use while instantiating a font.
// - pfiFont - Pointer to font information where the chosen font information will be populated.
// Return Value:
// - HRESULT S_OK
HRESULT VtEngine::UpdateFont(_In_ FontInfoDesired const * const /*pfiFontDesired*/,
                             _Out_ FontInfo* const /*pfiFont*/)
{
    return S_OK;
}

// Method Description:
// - This method will modify the DPI we're using for scaling calculations.
//      Does nothing for vt, the dpi is handed by the terminal. 
// Arguments:
// - iDpi - The Dots Per Inch to use for scaling. We will use this relative to 
//      the system default DPI defined in Windows headers as a constant.
// Return Value:
// - HRESULT S_OK
HRESULT VtEngine::UpdateDpi(_In_ int const /*iDpi*/)
{
    return S_OK;
}

// Method Description:
// - This method will update our internal reference for how big the viewport is. 
//      If the viewport has changed size, then we'll need to send an update to 
//      the terminal.
// Arguments:
// - srNewViewport - The bounds of the new viewport.
// Return Value:
// - HRESULT S_OK
HRESULT VtEngine::UpdateViewport(_In_ SMALL_RECT const srNewViewport)
{
    _srLastViewport = srNewViewport;

    // If the viewport has changed, then send a window update.
    // TODO: 13847317(adapter), 13271084(renderer)

    return S_OK;
}

// Method Description:
// - This method will figure out what the new font should be given the starting font information and a DPI.
// - When the final font is determined, the FontInfo structure given will be updated with the actual resulting font chosen as the nearest match.
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// - If the intent is to immediately turn around and use this font, pass the optional handle parameter and use it immediately.
//      Does nothing for vt, the font is handed by the terminal. 
// Arguments:
// - pfiFontDesired - Pointer to font information we should use while instantiating a font.
// - pfiFont - Pointer to font information where the chosen font information will be populated.
// - iDpi - The DPI we will have when rendering
// Return Value:
// - S_OK
HRESULT VtEngine::GetProposedFont(_In_ FontInfoDesired const * const /*pfiFontDesired*/,
                                  _Out_ FontInfo* const /*pfiFont*/,
                                  _In_ int const /*iDpi*/)
{
    return S_OK;
}

// Method Description:
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

#ifdef UNIT_TESTING

    _pfnTestCallback = pfn;
    _usingTestCallback = true;

#else
    THROW_HR(E_FAIL);
#endif

}


