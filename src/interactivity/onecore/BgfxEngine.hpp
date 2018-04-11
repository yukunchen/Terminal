/*++
Copyright (c) Microsoft Corporation

Module Name:
- BgfxEngine.hpp

Abstract:
- OneCore implementation of the IRenderEngine interface.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

// Typically, renderers live under the renderer/xxx top-level folder. This
// renderer however has strong ties to the interactivity library. More
// specifically, it makes use of the Console IO Server communication class.
// It is also a one-file renderer and I had problems with header file
// definitions. Placing this renderer in the OneCore Interactivity library fixes
// the header issues, and is more sensible given its ties to ConIoSrv.
// (hegatta, 2017)

#pragma once

#include "..\..\renderer\inc\IRenderEngine.hpp"

namespace Microsoft::Console::Render
{
    class BgfxEngine sealed : public IRenderEngine
    {
    public:
        BgfxEngine(PVOID SharedViewBase, LONG DisplayHeight, LONG DisplayWidth, LONG FontWidth, LONG FontHeight);
        ~BgfxEngine() override = default;

        // IRenderEngine Members
        [[nodiscard]]
        HRESULT Invalidate(const SMALL_RECT* const psrRegion);
        [[nodiscard]]
        HRESULT InvalidateCursor(const COORD* const pcoordCursor) override;
        [[nodiscard]]
        HRESULT InvalidateSystem(const RECT* const prcDirtyClient);
        [[nodiscard]]
        HRESULT InvalidateSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);
        [[nodiscard]]
        HRESULT InvalidateScroll(const COORD* const pcoordDelta);
        [[nodiscard]]
        HRESULT InvalidateAll();
        [[nodiscard]]
        HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) override;
        [[nodiscard]]
        HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) override;

        [[nodiscard]]
        HRESULT StartPaint();
        [[nodiscard]]
        HRESULT EndPaint();

        [[nodiscard]]
        HRESULT ScrollFrame();

        [[nodiscard]]
        HRESULT PaintBackground();
        [[nodiscard]]
        HRESULT PaintBufferLine(PCWCHAR const pwsLine,
                                const unsigned char* const rgWidths,
                                size_t const cchLine,
                                COORD const coord,
                                bool const fTrimLeft);
        [[nodiscard]]
        HRESULT PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget);
        [[nodiscard]]
        HRESULT PaintSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);

        [[nodiscard]]
        HRESULT PaintCursor(const COORD coordCursor,
                            const ULONG ulCursorHeightPercent,
                            const bool fIsDoubleWidth,
                            const CursorType cursorType,
                            const bool fUseColor,
                            const COLORREF cursorColor) override;
        [[nodiscard]]
        HRESULT ClearCursor() override;

        [[nodiscard]]
        HRESULT UpdateDrawingBrushes(COLORREF const colorForeground,
                                     COLORREF const colorBackground,
                                     const WORD legacyColorAttribute,
                                     bool const fIncludeBackgrounds);
        [[nodiscard]]
        HRESULT UpdateFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo);
        [[nodiscard]]
        HRESULT UpdateDpi(int const iDpi);
        [[nodiscard]]
        HRESULT UpdateViewport(const SMALL_RECT srNewViewport);

        [[nodiscard]]
        HRESULT GetProposedFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo, int const iDpi);

        SMALL_RECT GetDirtyRectInChars();
        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
        [[nodiscard]]
        HRESULT IsCharFullWidthByFont(const WCHAR wch, _Out_ bool* const pResult) override;

        [[nodiscard]]
        HRESULT UpdateTitle(_In_ const std::wstring& newTitle) override;

    private:
        ULONG_PTR _sharedViewBase;
        SIZE_T _runLength;

        LONG _displayHeight;
        LONG _displayWidth;

        COORD _fontSize;

        WORD _currentLegacyColorAttribute;
    };
}
