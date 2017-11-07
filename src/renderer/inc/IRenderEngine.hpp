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

#include "IRenderCursor.hpp"
#include "FontInfoDesired.hpp"

// Valid COLORREFs are of the pattern 0x00bbggrr. -1 works as an invalid color, 
//      as the highest byte of a valid color is always 0.
const COLORREF INVALID_COLOR = 0xffffffff;

namespace Microsoft
{
    namespace Console
    {
        namespace Render
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

                virtual HRESULT StartPaint() = 0;
                virtual HRESULT EndPaint() = 0;

                virtual HRESULT ScrollFrame() = 0;

                virtual HRESULT Invalidate(const SMALL_RECT* const psrRegion) = 0;
                virtual HRESULT InvalidateSystem(const RECT* const prcDirtyClient) = 0;
                virtual HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles) = 0;
                virtual HRESULT InvalidateScroll(const COORD* const pcoordDelta) = 0;
                virtual HRESULT InvalidateAll() = 0;
                
                virtual HRESULT PaintBackground() = 0;
                virtual HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine, _In_reads_(cchLine) const unsigned char* const rgWidths, _In_ size_t const cchLine, _In_ COORD const coord, _In_ bool const fTrimLeft) = 0;
                virtual HRESULT PaintBufferGridLines(_In_ GridLines const lines, _In_ COLORREF const color, _In_ size_t const cchLine, _In_ COORD const coordTarget) = 0;
                virtual HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles) = 0;

                virtual HRESULT PaintCursor(_In_ ULONG const ulCursorHeightPercent, _In_ bool const fIsDoubleWidth) = 0;
                virtual HRESULT ClearCursor() = 0;                

                virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds) = 0;
                virtual HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired, _Out_ FontInfo* const pfiFontInfo) = 0;
                virtual HRESULT UpdateDpi(_In_ int const iDpi) = 0;
                virtual HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport) = 0;

                virtual HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontInfoDesired, _Out_ FontInfo* const pfiFontInfo, _In_ int const iDpi) = 0;

                virtual SMALL_RECT GetDirtyRectInChars() = 0;
                virtual HRESULT GetFontSize(_Out_ COORD* const pFontSize) = 0;
                virtual HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) = 0;

                virtual IRenderCursor* const GetCursor() = 0;
            };
        };
    };
};

DEFINE_ENUM_FLAG_OPERATORS(Microsoft::Console::Render::IRenderEngine::GridLines)
