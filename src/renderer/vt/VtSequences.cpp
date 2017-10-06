/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "vtrenderer.hpp"
#include "..\..\inc\Viewport.hpp"
#include "..\..\inc\conattrs.hpp"

#pragma hdrstop
using namespace Microsoft::Console::Render;

// Method Description:
// - Formats and writes a sequence to stop the cursor from blinking.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_StopCursorBlinking()
{
    std::string seq = "\x1b[?12l";
    return _Write(seq);
}

// Method Description:
// - Formats and writes a sequence to start the cursor blinking. If it's 
//      hidden, this won't also show it.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_StartCursorBlinking()
{
    std::string seq = "\x1b[?12h";
    return _Write(seq);
}

// Method Description:
// - Formats and writes a sequence to hide the cursor.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_HideCursor()
{
    std::string seq = "\x1b[?25l";
    return _Write(seq);
}

// Method Description:
// - Formats and writes a sequence to show the cursor.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_ShowCursor()
{
    std::string seq = "\x1b[?25h";
    return _Write(seq);
}

// Method Description:
// - Formats and writes a sequence to erase the remainer of the line starting 
//      from the cursor position.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_EraseLine()
{
    std::string seq = "\x1b[0K";
    return _Write(seq);
}

// Method Description:
// - Formats and writes a sequence to delete a number of lines into the buffer 
//      at the current cursor location.
// Arguments:
// - sLines: a number of lines to insert
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_DeleteLine(_In_ const short sLines)
{
    if (sLines <= 0)
    {
        return S_OK;
    }
    if (sLines == 1)
    {
        std::string seq = "\x1b[M";
        return _Write(seq);
    }

    PCSTR pszFormat = "\x1b[%dM";

    int cchNeeded = _scprintf(pszFormat, sLines);
    wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
    RETURN_IF_NULL_ALLOC(psz);

    int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFormat, sLines);
    return _Write(psz.get(), cchWritten);

}

// Method Description:
// - Formats and writes a sequence to insert a number of lines into the buffer 
//      at the current cursor location.
// Arguments:
// - sLines: a number of lines to insert
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_InsertLine(_In_ const short sLines)
{
    if (sLines <= 0)
    {
        return S_OK;
    }
    if (sLines == 1)
    {
        std::string seq = "\x1b[L";
        return _Write(seq);
    }

    PCSTR pszFormat = "\x1b[%dL";

    int cchNeeded = _scprintf(pszFormat, sLines);
    wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
    RETURN_IF_NULL_ALLOC(psz);

    int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFormat, sLines);
    return _Write(psz.get(), cchWritten);
}

// Method Description:
// - Formats and writes a sequence to move the cursor to the specified 
//      coordinate position. The input coord should be in console coordinates, 
//      where origin=(0,0).
// Arguments:
// - coord: Console coordinates to move the cursor to.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_CursorPosition(_In_ const COORD coord)
{
    PCSTR pszCursorFormat = "\x1b[%d;%dH";

    // VT coords start at 1,1
    COORD coordVt = coord;
    coordVt.X++;
    coordVt.Y++;

    int cchNeeded = _scprintf(pszCursorFormat, coordVt.Y, coordVt.X);
    wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
    RETURN_IF_NULL_ALLOC(psz);

    int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszCursorFormat, coordVt.Y, coordVt.X);
    return _Write(psz.get(), cchWritten);
}

// Method Description:
// - Formats and writes a sequence to move the cursor to the origin.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_CursorHome()
{
    std::string seq = "\x1b[H";
    return _Write(seq);
}
