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

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class BgfxEngine sealed : public IRenderEngine
            {
            public:
                BgfxEngine(PVOID SharedViewBase, LONG DisplayHeight, LONG DisplayWidth, LONG FontWidth, LONG FontHeight);
                ~BgfxEngine() override = default;

                // IRenderEngine Members
                HRESULT Invalidate(const SMALL_RECT* const psrRegion);
                HRESULT InvalidateSystem(const RECT* const prcDirtyClient);
                HRESULT InvalidateSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);
                HRESULT InvalidateScroll(const COORD* const pcoordDelta);
                HRESULT InvalidateAll();
                HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) override;

                HRESULT StartPaint();
                HRESULT EndPaint();

                HRESULT ScrollFrame();

                HRESULT PaintBackground();
                HRESULT PaintBufferLine(PCWCHAR const pwsLine, const unsigned char* const rgWidths, size_t const cchLine, COORD const coord, bool const fTrimLeft);
                HRESULT PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget);
                HRESULT PaintSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);

                HRESULT PaintCursor(COORD const coordCursor, ULONG const ulCursorHeightPercent, bool const fIsDoubleWidth);
                HRESULT ClearCursor();

                HRESULT UpdateDrawingBrushes(COLORREF const colorForeground, COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, bool const fIncludeBackgrounds);
                HRESULT UpdateFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo);
                HRESULT UpdateDpi(int const iDpi);
                HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport);

                HRESULT GetProposedFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo, int const iDpi);

                SMALL_RECT GetDirtyRectInChars();
                HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
                HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) override;
                
            private:
                ULONG_PTR _sharedViewBase;
                SIZE_T _runLength;

                LONG _displayHeight;
                LONG _displayWidth;

                COORD _fontSize;

                WORD _currentLegacyColorAttribute;
            };
        };
    };
};
