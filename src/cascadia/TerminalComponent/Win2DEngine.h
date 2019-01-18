/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once
#include "TerminalCanvasView.h"
#include "../renderer/inc/RenderEngineBase.hpp"
#include "../../types/inc/Viewport.hpp"

namespace Microsoft::Console::Render
{
    class Win2DEngine;
}

class ::Microsoft::Console::Render::Win2DEngine final : public RenderEngineBase
{
public:

    Win2DEngine(TerminalCanvasView& canvasView, ::Microsoft::Console::Types::Viewport initialView);
    virtual ~Win2DEngine() override {};

    [[nodiscard]]
    HRESULT InvalidateSelection(const std::vector<SMALL_RECT>& rectangles) noexcept override;

    [[nodiscard]]
    HRESULT InvalidateScroll(const COORD* const pcoordDelta) noexcept override;

    [[nodiscard]]
    HRESULT InvalidateSystem(const RECT* const prcDirtyClient) noexcept override;

    [[nodiscard]]
    HRESULT Invalidate(const SMALL_RECT* const psrRegion) noexcept override;

    [[nodiscard]]
    HRESULT InvalidateCursor(const COORD* const pcoordCursor) noexcept override;

    [[nodiscard]]
    HRESULT InvalidateAll() noexcept override;

    [[nodiscard]]
    HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) noexcept override;

    [[nodiscard]]
    HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) noexcept override;

    [[nodiscard]]
    HRESULT StartPaint() noexcept override;

    [[nodiscard]]
    HRESULT EndPaint() noexcept override;

    [[nodiscard]]
    HRESULT Present() noexcept override;

    [[nodiscard]]
    HRESULT ScrollFrame() noexcept override;

    [[nodiscard]]
    HRESULT PaintBackground() noexcept override;

    [[nodiscard]]
    HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                            _In_reads_(cchLine) const unsigned char* const rgWidths,
                            const size_t cchLine,
                            const COORD coordTarget,
                            const bool fTrimLeft,
                            const bool lineWrapped) noexcept override;

    [[nodiscard]]
    HRESULT PaintBufferGridLines(const GridLines lines,
                                const COLORREF color,
                                const size_t cchLine,
                                const COORD coordTarget) noexcept override;

    [[nodiscard]]
    HRESULT PaintSelection(const SMALL_RECT rect) noexcept override;

    [[nodiscard]]
    HRESULT PaintCursor(const CursorOptions& options) noexcept override;

    [[nodiscard]]
    HRESULT UpdateDrawingBrushes(const COLORREF colorForeground,
                                 const COLORREF colorBackground,
                                 const WORD legacyColorAttribute,
                                 const bool isBold,
                                 const bool fIncludeBackgrounds) noexcept override;

    [[nodiscard]]
    HRESULT UpdateFont(const FontInfoDesired& pfiFontInfoDesired,
                       _Out_ FontInfo& pfiFontInfo) noexcept override;

    [[nodiscard]]
    HRESULT UpdateDpi(const int iDpi) noexcept override;

    [[nodiscard]]
    HRESULT UpdateViewport(const SMALL_RECT srNewViewport) noexcept override;

    [[nodiscard]]
    HRESULT GetProposedFont(const FontInfoDesired& FontDesired,
                            _Out_ FontInfo& Font,
                            const int iDpi) noexcept override;

    SMALL_RECT GetDirtyRectInChars() override;

    [[nodiscard]]
    HRESULT GetFontSize(_Out_ COORD* const pFontSize) noexcept override;

    [[nodiscard]]
    HRESULT IsGlyphWideByFont(const std::wstring_view glyph, _Out_ bool* const pResult) noexcept override;


private:
    TerminalCanvasView& _canvasView;
    ::Microsoft::Console::Types::Viewport _viewport;
    ::Microsoft::Console::Types::Viewport _invalid;

    winrt::Windows::UI::Color _lastFg;
    winrt::Windows::UI::Color _lastBg;

    [[nodiscard]]
    HRESULT _DoUpdateTitle(const std::wstring& newTitle) noexcept override;

    void _Invalidate(::Microsoft::Console::Types::Viewport newInvalid) noexcept;

};
