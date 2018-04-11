/*++
Copyright (c) Microsoft Corporation

Module Name:
- IRenderEngine.hpp

Abstract:
- This serves as the entry point for a specific graphics engine specific renderer.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "../../inc/conattrs.hpp"
#include "FontInfoDesired.hpp"

namespace Microsoft::Console::Render
{
    class IRenderEngine
    {
    public:

        enum GridLines
        {
            None = 0x0,
            Top = 0x1,
            Bottom = 0x2,
            Left = 0x4,
            Right = 0x8
        };

        virtual ~IRenderEngine() = default;

        [[nodiscard]]
        virtual HRESULT StartPaint() = 0;
        [[nodiscard]]
        virtual HRESULT EndPaint() = 0;
        [[nodiscard]]
        virtual HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) = 0;

        [[nodiscard]]
        virtual HRESULT ScrollFrame() = 0;

        [[nodiscard]]
        virtual HRESULT Invalidate(const SMALL_RECT* const psrRegion) = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateCursor(const COORD* const pcoordCursor) = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateSystem(const RECT* const prcDirtyClient) = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection, const UINT cRectangles) = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateScroll(const COORD* const pcoordDelta) = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateAll() = 0;
        [[nodiscard]]
        virtual HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) = 0;

        [[nodiscard]]
        virtual HRESULT PaintBackground() = 0;
        [[nodiscard]]
        virtual HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                        _In_reads_(cchLine) const unsigned char* const rgWidths,
                                        const size_t cchLine,
                                        const COORD coord,
                                        const bool fTrimLeft) = 0;
        [[nodiscard]]
        virtual HRESULT PaintBufferGridLines(const GridLines lines,
                                             const COLORREF color,
                                             const size_t cchLine,
                                             const COORD coordTarget) = 0;
        [[nodiscard]]
        virtual HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                       const UINT cRectangles) = 0;

        [[nodiscard]]
        virtual HRESULT PaintCursor(const COORD coordCursor,
                                    const ULONG ulCursorHeightPercent,
                                    const bool fIsDoubleWidth,
                                    const CursorType cursorType,
                                    const bool fUseColor,
                                    const COLORREF cursorColor) = 0;

        [[nodiscard]]
        virtual HRESULT ClearCursor() = 0;

        [[nodiscard]]
        virtual HRESULT UpdateDrawingBrushes(const COLORREF colorForeground,
                                             const COLORREF colorBackground,
                                             const WORD legacyColorAttribute,
                                             const bool fIncludeBackgrounds) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateFont(const FontInfoDesired& FontInfoDesired,
                                   _Out_ FontInfo& FontInfo) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateDpi(const int iDpi) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateViewport(const SMALL_RECT srNewViewport) = 0;

        [[nodiscard]]
        virtual HRESULT GetProposedFont(const FontInfoDesired& FontInfoDesired,
                                        _Out_ FontInfo& FontInfo,
                                        const int iDpi) = 0;

        virtual SMALL_RECT GetDirtyRectInChars() = 0;
        [[nodiscard]]
        virtual HRESULT GetFontSize(_Out_ COORD* const pFontSize) = 0;
        [[nodiscard]]
        virtual HRESULT IsCharFullWidthByFont(const WCHAR wch, _Out_ bool* const pResult) = 0;
    };
}

DEFINE_ENUM_FLAG_OPERATORS(Microsoft::Console::Render::IRenderEngine::GridLines)
