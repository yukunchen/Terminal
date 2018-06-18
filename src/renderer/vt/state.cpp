/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "vtrenderer.hpp"
#include "../../inc/conattrs.hpp"
#include "../../types/inc/convert.hpp"

// For _vcprintf
#include <conio.h>
#include <stdarg.h>

#pragma hdrstop

using namespace Microsoft::Console;
using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;

const COORD VtEngine::INVALID_COORDS = {-1, -1};

// Routine Description:
// - Creates a new VT-based rendering engine
// - NOTE: Will throw if initialization failure. Caller must catch.
// Arguments:
// - <none>
// Return Value:
// - An instance of a Renderer.
VtEngine::VtEngine(_In_ wil::unique_hfile pipe,
                   const IDefaultColorProvider& colorProvider,
                   const Viewport initialViewport) :
    _hFile(std::move(pipe)),
    _colorProvider(colorProvider),
    _LastFG(INVALID_COLOR),
    _LastBG(INVALID_COLOR),
    _lastViewport(initialViewport),
    _invalidRect({0}),
    _fInvalidRectUsed(false),
    _lastRealCursor({0}),
    _lastText({0}),
    _scrollDelta({0}),
    _quickReturn(false),
    _clearedAllThisFrame(false),
    _cursorMoved(false),
    _suppressResizeRepaint(true),
    _virtualTop(0),
    _circled(false),
    _firstPaint(false),
    _skipCursor(false),
    _pipeBroken(false),
    _exitResult{ S_OK },
    _terminalOwner{ nullptr },
    _trace {}
{
#ifndef UNIT_TESTING
    // When unit testing, we can instantiate a VtEngine without a pipe.
    THROW_IF_HANDLE_INVALID(_hFile.get());
#else
    // member is only defined when UNIT_TESTING is.
    _usingTestCallback = false;
#endif
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
[[nodiscard]]
HRESULT VtEngine::_Write(_In_reads_(cch) const char* const psz, const size_t cch)
{
    _trace.TraceString(std::string_view(psz, cch));
#ifdef UNIT_TESTING
    if (_usingTestCallback)
    {
        RETURN_LAST_ERROR_IF_FALSE(_pfnTestCallback(psz, cch));
        return S_OK;
    }
#endif
    if (!_pipeBroken)
    {
        bool fSuccess = !!WriteFile(_hFile.get(), psz, static_cast<DWORD>(cch), nullptr, nullptr);
        if (!fSuccess)
        {
            _exitResult = HRESULT_FROM_WIN32(GetLastError());
            _pipeBroken = true;
            if (_terminalOwner)
            {
                _terminalOwner->CloseOutput();
            }
            return _exitResult;
        }
    }

    return S_OK;
}

// Method Description:
// - Helper for calling _Write with a std::string
// Arguments:
// - str: the string of characters to write to the pipe.
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT VtEngine::_Write(const std::string& str)
{
    return _Write(str.c_str(), str.length());
}

// Method Description:
// - Wrapper for ITerminalOutputConnection. See _Write.
[[nodiscard]]
HRESULT VtEngine::WriteTerminalUtf8(const std::string& str)
{
    return _Write(str);
}

// Method Description:
// - Writes a wstring to the tty, encoded as full utf-8. This is one
//      implementation of the WriteTerminalW method.
// Arguments:
// - wstr - wstring of text to be written
// Return Value:
// - S_OK or suitable HRESULT error from either conversion or writing pipe.
[[nodiscard]]
HRESULT VtEngine::_WriteTerminalUtf8(const std::wstring& wstr)
{
    wistd::unique_ptr<char[]> rgchNeeded;
    size_t needed = 0;
    RETURN_IF_FAILED(ConvertToA(CP_UTF8, wstr.c_str(), wstr.length(), rgchNeeded, needed));
    return _Write(rgchNeeded.get(), needed);
}

// Method Description:
// - Writes a wstring to the tty, encoded as "utf-8" where characters that are
//      outside the ASCII range are encoded as '?'
//   This mainly exists to maintain compatability with the inbox telnet client.
//   This is one implementation of the WriteTerminalW method.
// Arguments:
// - wstr - wstring of text to be written
// Return Value:
// - S_OK or suitable HRESULT error from writing pipe.
[[nodiscard]]
HRESULT VtEngine::_WriteTerminalAscii(const std::wstring& wstr)
{
    const size_t cchActual = wstr.length();

    wistd::unique_ptr<char[]> rgchNeeded = wil::make_unique_nothrow<char[]>(cchActual + 1);
    RETURN_IF_NULL_ALLOC(rgchNeeded);

    char* nextChar = &rgchNeeded[0];
    for (size_t i = 0; i < cchActual; i++)
    {
        // We're explicitly replacing characters outside ASCII with a ? because
        //      that's what telnet wants.
        *nextChar = (wstr[i] > L'\x7f') ? '?' : static_cast<char>(wstr[i]);
        nextChar++;
    }

    rgchNeeded[cchActual] = '\0';

    return _Write(rgchNeeded.get(), cchActual);
}

// Method Description:
// - Helper for calling _Write with a string for formatting a sequence. Used
//      extensively by VtSequences.cpp
// Arguments:
// - pFormat: the pointer to the string to write to the pipe.
// - ...: a va_list of args to format the string with.
// Return Value:
// - S_OK, E_INVALIDARG for a invalid format string, or suitable HRESULT error
//      from writing pipe.
[[nodiscard]]
HRESULT VtEngine::_WriteFormattedString(const std::string* const pFormat, ...)
{

    HRESULT hr = E_FAIL;
    va_list argList;
    va_start(argList, pFormat);

    int cchNeeded = _scprintf(pFormat->c_str(), argList);
    // -1 is the _scprintf error case https://msdn.microsoft.com/en-us/library/t32cf9tb.aspx
    if (cchNeeded > -1)
    {
        wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
        RETURN_IF_NULL_ALLOC(psz);

        int cchWritten = _vsnprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pFormat->c_str(), argList);
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
// - FontDesired - reference to font information we should use while instantiating a font.
// - Font - reference to font information where the chosen font information will be populated.
// Return Value:
// - HRESULT S_OK
[[nodiscard]]
HRESULT VtEngine::UpdateFont(const FontInfoDesired& /*pfiFontDesired*/,
                             _Out_ FontInfo& /*pfiFont*/)
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
[[nodiscard]]
HRESULT VtEngine::UpdateDpi(const int /*iDpi*/)
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
[[nodiscard]]
HRESULT VtEngine::UpdateViewport(const SMALL_RECT srNewViewport)
{
    HRESULT hr = S_OK;
    const Viewport oldView = _lastViewport;
    const Viewport newView = Viewport::FromInclusive(srNewViewport);

    _lastViewport = newView;

    if ((oldView.Height() != newView.Height()) || (oldView.Width() != newView.Width()))
    {
        // Don't emit a resize event if we've requested it be suppressed
        if (_suppressResizeRepaint)
        {
            _suppressResizeRepaint = false;
        }
        else
        {
            hr = _ResizeWindow(newView.Width(), newView.Height());
        }
    }

    if (SUCCEEDED(hr))
    {
        // Viewport is smaller now - just update it all.
        if ( oldView.Height() > newView.Height() || oldView.Width() > newView.Width() )
        {
            hr = InvalidateAll();
        }
        else
        {
            // At least one of the directions grew.
            // First try and add everything to the right of the old viewport,
            //      then everything below where the old viewport ended.
            if (oldView.Width() < newView.Width())
            {
                short left = oldView.RightExclusive();
                short top = 0;
                short right = newView.RightInclusive();
                short bottom = oldView.BottomInclusive();
                Viewport rightOfOldViewport = Viewport::FromInclusive({left, top, right, bottom});
                hr = _InvalidCombine(rightOfOldViewport);
            }
            if (SUCCEEDED(hr) && oldView.Height() < newView.Height())
            {
                short left = 0;
                short top = oldView.BottomExclusive();
                short right = newView.RightInclusive();
                short bottom = newView.BottomInclusive();
                Viewport belowOldViewport = Viewport::FromInclusive({left, top, right, bottom});
                hr = _InvalidCombine(belowOldViewport);

            }
        }
    }

    return hr;
}

// Method Description:
// - This method will figure out what the new font should be given the starting font information and a DPI.
// - When the final font is determined, the FontInfo structure given will be updated with the actual resulting font chosen as the nearest match.
// - NOTE: It is left up to the underling rendering system to choose the nearest font. Please ask for the font dimensions if they are required using the interface. Do not use the size you requested with this structure.
// - If the intent is to immediately turn around and use this font, pass the optional handle parameter and use it immediately.
//      Does nothing for vt, the font is handed by the terminal.
// Arguments:
// - FontDesired - reference to font information we should use while instantiating a font.
// - Font - reference to font information where the chosen font information will be populated.
// - iDpi - The DPI we will have when rendering
// Return Value:
// - S_FALSE: This is unsupported by the VT Renderer and should use another engine's value.
[[nodiscard]]
HRESULT VtEngine::GetProposedFont(const FontInfoDesired& /*pfiFontDesired*/,
                                  _Out_ FontInfo& /*pfiFont*/,
                                  const int /*iDpi*/)
{
    return S_FALSE;
}

// Method Description:
// - Retrieves the current pixel size of the font we have selected for drawing.
// Arguments:
// - pFontSize - recieves the current X by Y size of the font.
// Return Value:
// - S_FALSE: This is unsupported by the VT Renderer and should use another engine's value.
[[nodiscard]]
HRESULT VtEngine::GetFontSize(_Out_ COORD* const pFontSize)
{
    *pFontSize = COORD({1, 1});
    return S_FALSE;
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

// Method Description:
// - Returns true if the entire viewport has been invalidated. That signals we
//      should use a VT Clear Screen sequence as an optimization.
// Arguments:
// - <none>
// Return Value:
// - true if the entire viewport has been invalidated
bool VtEngine::_AllIsInvalid() const
{
    return _lastViewport == _invalidRect;
}

// Method Description:
// - Prevent the renderer from emitting output on the next resize. This prevents
//      the host from echoing a resize to the terminal that requested it.
// Arguments:
// - <none>
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::SuppressResizeRepaint()
{
    _suppressResizeRepaint = true;
    return S_OK;
}

// Method Description:
// - "Inherit" the cursor at the given position. We won't need to move it
//      anywhere, so update where we last thought the cursor was.
//  Also update our "virtual top", indicating where should clip all updates to
//      (we don't want to paint the empty region above the inherited cursor).
//  Also ignore the next InvalidateCursor call.
// Arguments:
// - coordCursor: The cursor position to inherit from.
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT VtEngine::InheritCursor(const COORD coordCursor)
{
    _virtualTop = coordCursor.Y;
    _lastText = coordCursor;
    _skipCursor = true;
    return S_OK;
}

void VtEngine::SetTerminalOwner(Microsoft::Console::ITerminalOwner* const terminalOwner)
{
    _terminalOwner = terminalOwner;
}
