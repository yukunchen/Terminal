/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "BgfxEngine.hpp"

#include "InputServer.hpp"
#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Interactivity::OneCore;

// !!! ATTENTION !!!
// Palette entries must match those in condrv\display.c.

#define PALETTE_ENTRY(Red, Green, Blue) \
    ((Red << 16) | (Green << 8) | (Blue))

const ULONG CdpPalette[] = {
    PALETTE_ENTRY(0x00, 0x00, 0x00),
    PALETTE_ENTRY(0x00, 0x00, 0x80),
    PALETTE_ENTRY(0x00, 0x80, 0x00),
    PALETTE_ENTRY(0x00, 0x80, 0x80),
    PALETTE_ENTRY(0x80, 0x00, 0x00),
    PALETTE_ENTRY(0x80, 0x00, 0x80),
    PALETTE_ENTRY(0x80, 0x80, 0x00),
    PALETTE_ENTRY(0xC0, 0xC0, 0xC0),
    PALETTE_ENTRY(0x80, 0x80, 0x80),
    PALETTE_ENTRY(0x00, 0x00, 0xFF),
    PALETTE_ENTRY(0x00, 0xFF, 0x00),
    PALETTE_ENTRY(0x00, 0xFF, 0xFF),
    PALETTE_ENTRY(0xFF, 0x00, 0x00),
    PALETTE_ENTRY(0xFF, 0x00, 0xFF),
    PALETTE_ENTRY(0xFF, 0xFF, 0x00),
    PALETTE_ENTRY(0xFF, 0xFF, 0xFF),
};

BgfxEngine::BgfxEngine(PVOID SharedViewBase, LONG DisplayHeight, LONG DisplayWidth, LONG FontWidth, LONG FontHeight)
    : _sharedViewBase((ULONG_PTR)SharedViewBase),
      _displayHeight(DisplayHeight),
      _displayWidth(DisplayWidth),
      _currentForegroundColor(0),
      _currentBackgroundColor(0)
{
    _runLength = sizeof(CD_IO_CHARACTER) * DisplayWidth;
    
    _fontSize.X = FontWidth > SHORT_MAX ? SHORT_MAX : (SHORT)FontWidth;
    _fontSize.Y = FontHeight > SHORT_MAX ? SHORT_MAX : (SHORT)FontHeight;
}

HRESULT BgfxEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    UNREFERENCED_PARAMETER(psrRegion);

    return S_OK;
}

HRESULT BgfxEngine::InvalidateSystem(const RECT* const prcDirtyClient)
{
    UNREFERENCED_PARAMETER(prcDirtyClient);

    return S_OK;
}

HRESULT BgfxEngine::InvalidateSelection(SMALL_RECT* const rgsrSelection, UINT const cRectangles)
{
    UNREFERENCED_PARAMETER(rgsrSelection);
    UNREFERENCED_PARAMETER(cRectangles);

    return S_OK;
}

HRESULT BgfxEngine::InvalidateScroll(const COORD* const pcoordDelta)
{
    UNREFERENCED_PARAMETER(pcoordDelta);

    return S_OK;
}

HRESULT BgfxEngine::InvalidateAll()
{
    return S_OK;
}

HRESULT BgfxEngine::StartPaint()
{
    return S_OK;
}

HRESULT BgfxEngine::EndPaint()
{
    NTSTATUS Status;

    PVOID OldRunBase;
    PVOID NewRunBase;

    Status = ServiceLocator::LocateInputServices<InputServer>()->RequestUpdateDisplay(0);
    
    if (NT_SUCCESS(Status))
    {
        for (SHORT i = 0 ; i < _displayHeight ; i++)
        {
            OldRunBase = (PVOID)(_sharedViewBase + (i * 2 * _runLength));
            NewRunBase = (PVOID)(_sharedViewBase + (i * 2 * _runLength) + _runLength);
            memcpy_s(OldRunBase, _runLength, NewRunBase, _runLength);
        }
    }

    return HRESULT_FROM_NT(Status);
}

HRESULT BgfxEngine::ScrollFrame()
{
    return S_OK;
}

HRESULT BgfxEngine::PaintBackground()
{
    PVOID OldRunBase;
    PVOID NewRunBase;

    PCD_IO_CHARACTER OldRun;
    PCD_IO_CHARACTER NewRun;

    for (SHORT i = 0 ; i < _displayHeight ; i++)
    {
        OldRunBase = (PVOID)(_sharedViewBase + (i * 2 * _runLength));
        NewRunBase = (PVOID)(_sharedViewBase + (i * 2 * _runLength) + _runLength);
        
        OldRun = (PCD_IO_CHARACTER)OldRunBase;
        NewRun = (PCD_IO_CHARACTER)NewRunBase;

        for (SHORT j = 0 ; j < _displayWidth ; j++)
        {
            NewRun[j].Character = L' ';
            NewRun[j].Atribute = 0;
        }
    }

    return S_OK;
}

HRESULT BgfxEngine::PaintBufferLine(PCWCHAR const pwsLine, size_t const cchLine, COORD const coord, size_t const cchCharWidths, bool const fTrimLeft)
{
    UNREFERENCED_PARAMETER(cchCharWidths);
    UNREFERENCED_PARAMETER(fTrimLeft);

    PVOID NewRunBase = (PVOID)(_sharedViewBase + (coord.Y * 2 * _runLength) + _runLength);
    PCD_IO_CHARACTER NewRun = (PCD_IO_CHARACTER)NewRunBase;

    USHORT PaletteEntry = GetColorPaletteEntry(_currentForegroundColor);

    for (size_t i = 0 ; i < cchLine && i < (size_t)_displayWidth ; i++)
    {
        NewRun[coord.X + i].Character = pwsLine[i];
        NewRun[coord.X + i].Atribute = PaletteEntry;
    }

    return S_OK;
}

HRESULT BgfxEngine::PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget)
{
    UNREFERENCED_PARAMETER(lines);
    UNREFERENCED_PARAMETER(color);
    UNREFERENCED_PARAMETER(cchLine);
    UNREFERENCED_PARAMETER(coordTarget);

    return S_OK;
}

HRESULT BgfxEngine::PaintSelection(SMALL_RECT* const rgsrSelection, UINT const cRectangles)
{
    UNREFERENCED_PARAMETER(rgsrSelection);
    UNREFERENCED_PARAMETER(cRectangles);

    return S_OK;
}

HRESULT BgfxEngine::PaintCursor(COORD const coordCursor, ULONG const ulCursorHeightPercent, bool const fIsDoubleWidth)
{
    // TODO: MSFT: 11448021 - Modify BGFX to support rendering full-width
    // characters and a full-width cursor.
    UNREFERENCED_PARAMETER(fIsDoubleWidth);

    NTSTATUS Status;

    CD_IO_CURSOR_INFORMATION CursorInfo;
    CursorInfo.Row = coordCursor.Y;
    CursorInfo.Column = coordCursor.X;
    CursorInfo.Height = ulCursorHeightPercent;
    CursorInfo.IsVisible = TRUE;

    Status = ServiceLocator::LocateInputServices<InputServer>()->RequestSetCursor(&CursorInfo);

    return HRESULT_FROM_NT(Status);
}

HRESULT BgfxEngine::ClearCursor()
{
    NTSTATUS Status;

    CD_IO_CURSOR_INFORMATION CursorInfo = { 0 };
    CursorInfo.IsVisible = FALSE;

    Status = ServiceLocator::LocateInputServices<InputServer>()->RequestSetCursor(&CursorInfo);

    return HRESULT_FROM_NT(Status);
}

HRESULT BgfxEngine::UpdateDrawingBrushes(COLORREF const colorForeground, COLORREF const colorBackground, bool const fIncludeBackgrounds)
{
    UNREFERENCED_PARAMETER(fIncludeBackgrounds);

    _currentForegroundColor = colorForeground;
    _currentBackgroundColor = colorBackground;

    return S_OK;
}

HRESULT BgfxEngine::UpdateFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo)
{
    UNREFERENCED_PARAMETER(pfiFontInfoDesired);
    UNREFERENCED_PARAMETER(pfiFontInfo);

    return S_OK;
}

HRESULT BgfxEngine::UpdateDpi(int const iDpi)
{
    UNREFERENCED_PARAMETER(iDpi);

    return S_OK;
}

HRESULT BgfxEngine::GetProposedFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo, int const iDpi)
{
    UNREFERENCED_PARAMETER(pfiFontInfoDesired);
    UNREFERENCED_PARAMETER(pfiFontInfo);
    UNREFERENCED_PARAMETER(iDpi);

    return S_OK;
}

SMALL_RECT BgfxEngine::GetDirtyRectInChars()
{
    SMALL_RECT r;
    r.Bottom = _displayHeight > 0 ? (SHORT)(_displayHeight - 1) : 0;
    r.Top = 0;
    r.Left = 0;
    r.Right = _displayWidth > 0 ? (SHORT)(_displayWidth - 1) : 0;

    return r;
}

COORD BgfxEngine::GetFontSize()
{
    return _fontSize;
}

bool BgfxEngine::IsCharFullWidthByFont(WCHAR const wch)
{
    UNREFERENCED_PARAMETER(wch);

    return false;
}

USHORT BgfxEngine::GetColorPaletteEntry(DWORD dwColor)
{
    USHORT PaletteEntry = USHORT_MAX;

    BYTE Red   = (BYTE)((dwColor & 0x000000FF) >> 0);
    BYTE Green = (BYTE)((dwColor & 0x0000FF00) >> 8);
    BYTE Blue  = (BYTE)((dwColor & 0x00FF0000) >> 16);

    //
    // Try to find an exact match in the color palette.
    //

    for (USHORT i = 0 ; i < 15 ; i++)
    {
        if (CdpPalette[i] == (ULONG)PALETTE_ENTRY(Red, Green, Blue))
        {
            PaletteEntry = i;
            break;
        }
    }

    //
    // If that didn't work, find a color in the palette that is close enough.
    //

    if (PaletteEntry == USHORT_MAX)
    {
        if (Red > 0 && Green > 0 && Blue > 0)
        {
            PaletteEntry = 15;
        }
        else if (Red == 0 && Green > 0 && Blue > 0)
        {
            PaletteEntry = 11;
        }
        else if (Red > 0 && Green == 0 && Blue > 0)
        {
            PaletteEntry = 13;
        }
        else if (Red > 0 && Green > 0 && Blue == 0)
        {
            PaletteEntry = 14;
        }
        else if (Red > 0 && Green == 0 && Blue == 0)
        {
            PaletteEntry = 12;
        }
        else if (Red == 0 && Green > 0 && Blue == 0)
        {
            PaletteEntry = 10;
        }
        else if (Red == 0 && Green == 0 && Blue > 0)
        {
            PaletteEntry = 9;
        }
        else
        {
            // White in all other cases so that the text remain visible.
            PaletteEntry = 15;
        }
    }

    return PaletteEntry;
}
