#include "pch.h"
#include "Win2DEngine.h"

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Types;
using namespace winrt::Windows::UI;

Win2DEngine::Win2DEngine(TerminalCanvasView& canvasView, Viewport initialView) :
    _canvasView{ canvasView },
    _viewport{ initialView },
    _lastFg{ 255, 255, 255},
    _lastBg{ 0, 0, 0},
    _invalid{ Viewport::Empty() }
{
}

HRESULT Win2DEngine::StartPaint() noexcept
{
    return S_OK;
}

HRESULT Win2DEngine::EndPaint() noexcept
{
    _canvasView.FinishDrawingSession();
    return S_OK;
}

HRESULT Win2DEngine::Present() noexcept
{
    return S_FALSE;
}

void Win2DEngine::_Invalidate(Viewport newInvalid) noexcept
{
    if (!_invalid.IsValid())
    {
        _invalid = Viewport::OrViewports(_invalid, newInvalid);
    }
    else
    {
        _invalid = newInvalid;
    }
}

HRESULT Win2DEngine::InvalidateSelection(const std::vector<SMALL_RECT>& /*rectangles*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::InvalidateScroll(const COORD* const /*pcoordDelta*/) noexcept
{
    return InvalidateAll();
}

HRESULT Win2DEngine::InvalidateSystem(const RECT* const /*prcDirtyClient*/) noexcept
{
    return InvalidateAll();
}

HRESULT Win2DEngine::Invalidate(const SMALL_RECT* const psrRegion) noexcept
{
    auto invalid = Viewport::FromInclusive(*psrRegion);
    _Invalidate(invalid);
    return S_OK;
}

HRESULT Win2DEngine::InvalidateCursor(const COORD* const /*pcoordCursor*/) noexcept
{
    return S_OK;
}

HRESULT Win2DEngine::InvalidateAll() noexcept
{
    _Invalidate(_viewport);
    return S_OK;
}

HRESULT Win2DEngine::InvalidateCircling(_Out_ bool* const /*pForcePaint*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::PrepareForTeardown(_Out_ bool* const /*pForcePaint*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::ScrollFrame() noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::PaintBackground() noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                     _In_reads_(cchLine) const unsigned char* const /*rgWidths*/,
                                     const size_t cchLine,
                                     const COORD coordTarget,
                                     const bool /*fTrimLeft*/,
                                     const bool /*lineWrapped*/) noexcept
{
    std::wstring wstr{pwsLine, cchLine};
    _canvasView.PaintRun({wstr}, coordTarget, _lastFg, _lastBg);
    return S_FALSE;
}

HRESULT Win2DEngine::PaintBufferGridLines(const GridLines /*lines*/,
                                          const COLORREF /*color*/,
                                          const size_t /*cchLine*/,
                                          const COORD /*coordTarget*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::PaintSelection(const SMALL_RECT /*rect*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::PaintCursor(const COORD /*coordCursor*/,
                                 const ULONG /*ulCursorHeightPercent*/,
                                 const bool /*fIsDoubleWidth*/,
                                 const CursorType /*cursorType*/,
                                 const bool /*fUseColor*/,
                                 const COLORREF /*cursorColor*/) noexcept
{
    return S_FALSE;
}

BYTE GetAValue(COLORREF color)
{
    return (0xff000000 & color) >> 12;
}

HRESULT Win2DEngine::UpdateDrawingBrushes(const COLORREF colorForeground,
                                          const COLORREF colorBackground,
                                          const WORD /*legacyColorAttribute*/,
                                          const bool /*isBold*/,
                                          const bool /*fIncludeBackgrounds*/) noexcept
{
    Color fg = ColorHelper::FromArgb(255, GetRValue(colorForeground), GetGValue(colorForeground), GetBValue(colorForeground));
    Color bg = ColorHelper::FromArgb(GetAValue(colorBackground), GetRValue(colorBackground), GetGValue(colorBackground), GetBValue(colorBackground));
    if (fg != _lastFg)
    {
        _lastFg = fg;
    }

    if (bg != _lastBg)
    {
        _lastBg = bg;
    }
    return S_OK;
}

HRESULT Win2DEngine::UpdateFont(const FontInfoDesired& /*pfiFontInfoDesired*/,
                                _Out_ FontInfo& /*pfiFontInfo*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::UpdateDpi(const int /*iDpi*/) noexcept
{
    return S_FALSE;
}

HRESULT Win2DEngine::UpdateViewport(const SMALL_RECT srNewViewport) noexcept
{
    _viewport = Viewport::FromInclusive(srNewViewport);
    InvalidateAll();
    return S_OK;
}

HRESULT Win2DEngine::GetProposedFont(const FontInfoDesired& /*FontDesired*/,
                                     _Out_ FontInfo& /*Font*/,
                                     const int /*iDpi*/) noexcept
{
    return S_FALSE;
}

SMALL_RECT Win2DEngine::GetDirtyRectInChars()
{
    return _invalid.ToInclusive();
}

HRESULT Win2DEngine::GetFontSize(_Out_ COORD* const pFontSize) noexcept
{
    *pFontSize = { 1, 1 };
    return S_FALSE;
}

HRESULT Win2DEngine::IsGlyphWideByFont(const std::wstring_view /*glyph*/,
                                       _Out_ bool* const pResult) noexcept
{
    *pResult = false;
    return S_FALSE;
}

HRESULT Win2DEngine::_DoUpdateTitle(const std::wstring& /*newTitle*/) noexcept
{
    return S_FALSE;
}
