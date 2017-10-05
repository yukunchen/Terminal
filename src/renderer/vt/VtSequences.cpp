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

HRESULT VtEngine::_StopCursorBlinking()
{
    std::string seq = "\x1b[?12l";
    return _Write(seq);
}

HRESULT VtEngine::_StartCursorBlinking()
{
    std::string seq = "\x1b[?12h";
    return _Write(seq);
}

HRESULT VtEngine::_HideCursor()
{
    std::string seq = "\x1b[?25l";
    return _Write(seq);
}

HRESULT VtEngine::_ShowCursor()
{
    std::string seq = "\x1b[?25h";
    return _Write(seq);
}

HRESULT VtEngine::_EraseLine()
{
    std::string seq = "\x1b[0K";
    return _Write(seq);
}

HRESULT VtEngine::_DeleteLine(_In_ const short sLines)
{
    if (sLines <= 0) return S_OK;

    PCSTR pszFormat = "\x1b[%dM";

    int cchNeeded = _scprintf(pszFormat, sLines);
    wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
    RETURN_IF_NULL_ALLOC(psz);

    int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFormat, sLines);
    return _Write(psz.get(), cchWritten);

}

HRESULT VtEngine::_InsertLine(_In_ const short sLines)
{
    if (sLines <= 0) return S_OK;
    PCSTR pszFormat = "\x1b[%dL";

    int cchNeeded = _scprintf(pszFormat, sLines);
    wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
    RETURN_IF_NULL_ALLOC(psz);

    int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFormat, sLines);
    return _Write(psz.get(), cchWritten);
}

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

HRESULT VtEngine::_CursorHome()
{
    std::string seq = "\x1b[H";
    return _Write(seq);
}
