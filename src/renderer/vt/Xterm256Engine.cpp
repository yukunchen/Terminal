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

Xterm256Engine::Xterm256Engine(HANDLE hPipe, VtIoMode IoMode)
    : VtEngine(hPipe, IoMode)
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
    try
    {
        if (colorForeground != _LastFG)
        {
            PCSTR pszFgFormat = "\x1b[38;2;%d;%d;%dm";
            DWORD const fgRed = (colorForeground & 0xff);
            DWORD const fgGreen = (colorForeground >> 8) & 0xff;
            DWORD const fgBlue = (colorForeground >> 16) & 0xff;
            
            int cchNeeded = _scprintf(pszFgFormat, fgRed, fgGreen, fgBlue);
            wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
            RETURN_IF_NULL_ALLOC(psz);

            int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszFgFormat, fgRed, fgGreen, fgBlue);
            _Write(psz.get(), cchWritten);
            _LastFG = colorForeground;
            
        }
        if (colorBackground != _LastBG) 
        {
            PCSTR pszBgFormat = "\x1b[48;2;%d;%d;%dm";
            DWORD const bgRed = (colorBackground & 0xff);
            DWORD const bgGreen = (colorBackground >> 8) & 0xff;
            DWORD const bgBlue = (colorBackground >> 16) & 0xff;

            int cchNeeded = _scprintf(pszBgFormat, bgRed, bgGreen, bgBlue);
            wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
            RETURN_IF_NULL_ALLOC(psz);

            int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszBgFormat, bgRed, bgGreen, bgBlue);
            _Write(psz.get(), cchWritten);
            _LastBG = colorBackground;
        }
    }
    CATCH_RETURN();

    colorForeground;
    colorBackground;
    legacyColorAttribute;
    fIncludeBackgrounds;
    
    return S_OK;
}

HRESULT Xterm256Engine::_MoveCursor(COORD const coord)
{
    // DebugBreak();
    if (coord.X != _lastText.X || coord.Y != _lastText.Y)
    {
        try
        {
            if (coord.X == 0 && coord.Y == 0)
            {
                std::string seq = "\x1b[H";
                _Write(seq);
            }
            else if (coord.X == 0 && coord.Y == (_lastText.Y+1))
            {
                std::string seq = "\r\n";
                _Write(seq);
            }
            else
            {
                PCSTR pszCursorFormat = "\x1b[%d;%dH";
                COORD coordVt = coord;
                coordVt.X++;
                coordVt.Y++;

                int cchNeeded = _scprintf(pszCursorFormat, coordVt.Y, coordVt.X);
                wistd::unique_ptr<char[]> psz = wil::make_unique_nothrow<char[]>(cchNeeded + 1);
                RETURN_IF_NULL_ALLOC(psz);

                int cchWritten = _snprintf_s(psz.get(), cchNeeded + 1, cchNeeded, pszCursorFormat, coordVt.Y, coordVt.X);
                _Write(psz.get(), cchWritten);
                
            }

            _lastText = coord;
            // _lastRealCursor = coord;
        }
        CATCH_RETURN();
    }
    return S_OK;
}

HRESULT Xterm256Engine::ScrollFrame()
{
    if (_scrollDelta.X != 0)
    {
        // No easy way to shift left-right
        return InvalidateAll();
    }

    short dy = _scrollDelta.Y;
    short absDy = (dy>0)? dy : -dy;

    Viewport view(_srLastViewport);
    SMALL_RECT v = _srLastViewport;
    view.ConvertToOrigin(&v);

    if (dy < 0)
    {
        _MoveCursor({0,0});
        // DL - Delete Line
        std::string seq = "\x1b[M";
        for (int i = 0; i < absDy; ++i)
        {
            _Write(seq);
        }

        SMALL_RECT b = v;
        b.Top = v.Bottom - absDy;
        // RETURN_IF_FAILED(_InvalidCombine(&b));
    }
    else if (dy > 0)
    {
        _MoveCursor({0,0});
        // IL - Insert Line
        std::string seq = "\x1b[L";
        for (int i = 0; i < absDy; ++i)
        {
            _Write(seq);
        }

        SMALL_RECT a = v;
        a.Bottom = absDy;
        // RETURN_IF_FAILED(_InvalidCombine(&a));
    }

    return S_OK;
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the console would like us to move while scrolling.
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT Xterm256Engine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    pcoordDelta;
    // return this->InvalidateAll();
    short dx = pcoordDelta->X;
    short dy = pcoordDelta->Y;
    // short absDy = (dy>0)? dy : -dy;

    if (dx != 0 || dy != 0)
    {
        // POINT ptDelta = { 0 };
        // RETURN_IF_FAILED(_ScaleByFont(pcoordDelta, &ptDelta));

        // Scroll the current offset
        RETURN_IF_FAILED(_InvalidOffset(pcoordDelta));

        // Add the top/bottom of the window to the invalid area
        Viewport view(_srLastViewport);
        SMALL_RECT v = _srLastViewport;
        view.ConvertToOrigin(&v);
        SMALL_RECT invalid = v;
        if (dy > 0)
        {
            invalid.Bottom = dy;
        }
        else if (dy < 0)
        {
            invalid.Top = v.Bottom + dy;
        }
        _InvalidCombine(&invalid);

        COORD invalidScrollNew;
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.X, dx, &invalidScrollNew.X));
        RETURN_IF_FAILED(ShortAdd(_scrollDelta.Y, dy, &invalidScrollNew.Y));

        // Store if safemath succeeded
        _scrollDelta = invalidScrollNew;

    }

    return S_OK;
}
