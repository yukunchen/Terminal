/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "BgfxEngine.hpp"

#include "ConIoSrvComm.hpp"
#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

//
// Default non-bright white.
//

#define DEFAULT_COLOR_ATTRIBUTE (0xC)

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Interactivity::OneCore;

BgfxEngine::BgfxEngine(PVOID SharedViewBase, LONG DisplayHeight, LONG DisplayWidth, LONG FontWidth, LONG FontHeight)
    : _sharedViewBase((ULONG_PTR)SharedViewBase),
      _displayHeight(DisplayHeight),
      _displayWidth(DisplayWidth),
      _currentLegacyColorAttribute(DEFAULT_COLOR_ATTRIBUTE)
{
    _runLength = sizeof(CD_IO_CHARACTER) * DisplayWidth;

    _fontSize.X = FontWidth > SHORT_MAX ? SHORT_MAX : (SHORT)FontWidth;
    _fontSize.Y = FontHeight > SHORT_MAX ? SHORT_MAX : (SHORT)FontHeight;
}

[[nodiscard]]
HRESULT BgfxEngine::Invalidate(const SMALL_RECT* const /*psrRegion*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateCursor(const COORD* const /*pcoordCursor*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateSystem(const RECT* const /*prcDirtyClient*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateSelection(const SMALL_RECT* const /*rgsrSelection*/, UINT const /*cRectangles*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateScroll(const COORD* const /*pcoordDelta*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateAll()
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::InvalidateCircling(_Out_ bool* const pForcePaint)
{
    *pForcePaint = false;
    return S_FALSE;
}

[[nodiscard]]
HRESULT BgfxEngine::PrepareForTeardown(_Out_ bool* const pForcePaint)
{
    *pForcePaint = false;
    return S_FALSE;
}

[[nodiscard]]
HRESULT BgfxEngine::StartPaint()
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::EndPaint()
{
    NTSTATUS Status;

    PVOID OldRunBase;
    PVOID NewRunBase;

    Status = ServiceLocator::LocateInputServices<ConIoSrvComm>()->RequestUpdateDisplay(0);

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

[[nodiscard]]
HRESULT BgfxEngine::ScrollFrame()
{
    return S_OK;
}

[[nodiscard]]
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

[[nodiscard]]
HRESULT BgfxEngine::PaintBufferLine(PCWCHAR const pwsLine,
                                    const unsigned char* const /*rgWidths*/,
                                    size_t const cchLine,
                                    COORD const coord,
                                    bool const /*fTrimLeft*/)
{
    PVOID NewRunBase = (PVOID)(_sharedViewBase + (coord.Y * 2 * _runLength) + _runLength);
    PCD_IO_CHARACTER NewRun = (PCD_IO_CHARACTER)NewRunBase;

    for (size_t i = 0 ; i < cchLine && i < (size_t)_displayWidth ; i++)
    {
        NewRun[coord.X + i].Character = pwsLine[i];
        NewRun[coord.X + i].Atribute = _currentLegacyColorAttribute;
    }

    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::PaintBufferGridLines(GridLines const /*lines*/,
                                         COLORREF const /*color*/,
                                         size_t const /*cchLine*/,
                                         COORD const /*coordTarget*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::PaintSelection(const SMALL_RECT* const /*rgsrSelection*/, UINT const /*cRectangles*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::PaintCursor(_In_ COORD const coordCursor,
                                _In_ ULONG const ulCursorHeightPercent,
                                _In_ bool const /*fIsDoubleWidth*/,
                                _In_ CursorType const /*cursorType*/,
                                _In_ bool const /*fUseColor*/,
                                _In_ COLORREF const /*cursorColor*/)
{
    // TODO: MSFT: 11448021 - Modify BGFX to support rendering full-width
    // characters and a full-width cursor.
    CD_IO_CURSOR_INFORMATION CursorInfo;
    CursorInfo.Row = coordCursor.Y;
    CursorInfo.Column = coordCursor.X;
    CursorInfo.Height = ulCursorHeightPercent;
    CursorInfo.IsVisible = TRUE;

    NTSTATUS Status = ServiceLocator::LocateInputServices<ConIoSrvComm>()->RequestSetCursor(&CursorInfo);

    return HRESULT_FROM_NT(Status);

}

[[nodiscard]]
HRESULT BgfxEngine::ClearCursor()
{
    NTSTATUS Status;

    CD_IO_CURSOR_INFORMATION CursorInfo = { 0 };
    CursorInfo.IsVisible = FALSE;

    Status = ServiceLocator::LocateInputServices<ConIoSrvComm>()->RequestSetCursor(&CursorInfo);

    return HRESULT_FROM_NT(Status);
}

[[nodiscard]]
HRESULT BgfxEngine::UpdateDrawingBrushes(COLORREF const /*colorForeground*/,
                                         COLORREF const /*colorBackground*/,
                                         _In_ WORD const legacyColorAttribute,
                                         bool const /*fIncludeBackgrounds*/)
{
    _currentLegacyColorAttribute = legacyColorAttribute;

    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::UpdateFont(FontInfoDesired const* const /*pfiFontInfoDesired*/, FontInfo* const /*pfiFontInfo*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::UpdateDpi(int const /*iDpi*/)
{
    return S_OK;
}

// Method Description:
// - This method will update our internal reference for how big the viewport is.
//      Does nothing for BGFX.
// Arguments:
// - srNewViewport - The bounds of the new viewport.
// Return Value:
// - HRESULT S_OK
[[nodiscard]]
HRESULT BgfxEngine::UpdateViewport(_In_ SMALL_RECT const /*srNewViewport*/)
{
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::GetProposedFont(FontInfoDesired const* const /*pfiFontInfoDesired*/, FontInfo* const /*pfiFontInfo*/, int const /*iDpi*/)
{
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

[[nodiscard]]
HRESULT BgfxEngine::GetFontSize(_Out_ COORD* const pFontSize)
{
    *pFontSize =_fontSize;
    return S_OK;
}

[[nodiscard]]
HRESULT BgfxEngine::IsCharFullWidthByFont(WCHAR const /*wch*/, _Out_ bool* const pResult)
{

    *pResult = false;
    return S_OK;
}

// Method Description:
// - Updates the window's title string.
//      Does nothing for BGFX.
// Arguments:
// - newTitle: the new string to use for the title of the window
// Return Value:
// - S_OK
[[nodiscard]]
HRESULT BgfxEngine::UpdateTitle(_In_ const std::wstring& /*newTitle*/)
{
    return S_OK;
}
