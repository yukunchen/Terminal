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
        virtual HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles) = 0;
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
                                        _In_ size_t const cchLine,
                                        _In_ COORD const coord,
                                        _In_ bool const fTrimLeft) = 0;
        [[nodiscard]]
        virtual HRESULT PaintBufferGridLines(_In_ GridLines const lines,
                                             _In_ COLORREF const color,
                                             _In_ size_t const cchLine,
                                             _In_ COORD const coordTarget) = 0;
        [[nodiscard]]
        virtual HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                       _In_ UINT const cRectangles) = 0;

        [[nodiscard]]
        virtual HRESULT PaintCursor(_In_ COORD const coordCursor,
                                    _In_ ULONG const ulCursorHeightPercent,
                                    _In_ bool const fIsDoubleWidth,
                                    _In_ CursorType const cursorType,
                                    _In_ bool const fUseColor,
                                    _In_ COLORREF const cursorColor) = 0;

        [[nodiscard]]
        virtual HRESULT ClearCursor() = 0;

        [[nodiscard]]
        virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                             _In_ COLORREF const colorBackground,
                                             _In_ WORD const legacyColorAttribute,
                                             _In_ bool const fIncludeBackgrounds) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired,
                                   _Out_ FontInfo* const pfiFontInfo) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateDpi(_In_ int const iDpi) = 0;
        [[nodiscard]]
        virtual HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport) = 0;

        [[nodiscard]]
        virtual HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontInfoDesired,
                                        _Out_ FontInfo* const pfiFontInfo,
                                        _In_ int const iDpi) = 0;

        virtual SMALL_RECT GetDirtyRectInChars() = 0;
        [[nodiscard]]
        virtual HRESULT GetFontSize(_Out_ COORD* const pFontSize) = 0;
        [[nodiscard]]
        virtual HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) = 0;
    };
}

DEFINE_ENUM_FLAG_OPERATORS(Microsoft::Console::Render::IRenderEngine::GridLines)
