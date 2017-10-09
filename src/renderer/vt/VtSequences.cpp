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
    return _Write("\x1b[?12l");
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
    return _Write("\x1b[?12h");
}

// Method Description:
// - Formats and writes a sequence to hide the cursor.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_HideCursor()
{
    return _Write("\x1b[?25l");
}

// Method Description:
// - Formats and writes a sequence to show the cursor.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_ShowCursor()
{
    return _Write("\x1b[?25h");
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
    return _Write("\x1b[0K");
}

// Method Description:
// - Formats and writes a sequence to either insert or delete a number of lines 
//      into the buffer at the current cursor location.
// Arguments:
// - sLines: a number of lines to insert or delete
// - fInsertLine: true iff we should insert the lines, false to delete them.
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_InsertDeleteLine(_In_ const short sLines, _In_ const bool fInsertLine)
{
    if (sLines <= 0)
    {
        return S_OK;
    }
    if (sLines == 1)
    {
        return _Write(fInsertLine ? "\x1b[L" : "\x1b[M");
    }
    const PCSTR pszFormat = fInsertLine ? "\x1b[%dL" : "\x1b[%dM";

    return _WriteFormattedString(pszFormat, sLines);
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
    return _InsertDeleteLine(sLines, false);
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
    return _InsertDeleteLine(sLines, true);
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
    static const PCSTR pszCursorFormat = "\x1b[%d;%dH";

    // VT coords start at 1,1
    COORD coordVt = coord;
    coordVt.X++;
    coordVt.Y++;

    return _WriteFormattedString(pszCursorFormat, coordVt.Y, coordVt.X);
}

// Method Description:
// - Formats and writes a sequence to move the cursor to the origin.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_CursorHome()
{
    return _Write("\x1b[H");
}


// Method Description:
// - Formats and writes a sequence to change the current text attributes.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_SetGraphicsRendition16Color(_In_ const WORD wAttr,
                                               _In_ const bool fIsForeground)
{
    // Different formats for:
    //      Foreground, Bright = "\x1b[1m\x1b[%dm"
    //      Foreground, Dark = "\x1b[22m\x1b[%dm"
    //      Any Background = "\x1b[%dm"
    const char* const pszFmt = fIsForeground ? 
        (IsFlagSet(wAttr, FOREGROUND_INTENSITY) ? "\x1b[1m\x1b[%dm" : "\x1b[22m\x1b[%dm" ) :
        ("\x1b[%dm");

    // Always check using the foreground flags, because the bg flags constants 
    //  are a higher byte
    // Foreground sequences are in [30,37]
    // Background sequences are in [40,47]
    const int vtIndex = 30 
                        + (fIsForeground? 10 : 0)
                        + (IsFlagSet(wAttr, FOREGROUND_RED) ? 1 : 0)
                        + (IsFlagSet(wAttr, FOREGROUND_GREEN) ? 2 : 0)
                        + (IsFlagSet(wAttr, FOREGROUND_BLUE) ? 4 : 0);

    return _WriteFormattedString(pszFmt, vtIndex);
}

// Method Description:
// - Formats and writes a sequence to change the current text attributes to an 
//      RGB color.
// Arguments:
// - <none>
// Return Value:
// - S_OK if we succeeded, else an appropriate HRESULT for failing to allocate or write.
HRESULT VtEngine::_SetGraphicsRenditionRGBColor(_In_ const COLORREF color,
                                                _In_ const bool fIsForeground)
{
    const char* const pszFmt = fIsForeground ? 
        "\x1b[38;2;%d;%d;%dm" :
        "\x1b[48;2;%d;%d;%dm";

    DWORD const r = (color & 0xff);
    DWORD const g = (color >> 8) & 0xff;
    DWORD const b = (color >> 16) & 0xff;    

    return _WriteFormattedString(pszFmt, r, g, b);
}
