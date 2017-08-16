/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtRenderer.hpp

Abstract:
- This is the definition of the VT specific implementation of the renderer.

Author(s):
- Michael Niksa (MiNiksa) 24-Jul-2017
--*/

#pragma once

#include "..\inc\IRenderEngine.hpp"
#include <string>

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class VtEngine sealed : public IRenderEngine
            {
            public:
                VtEngine();
                ~VtEngine();

                HRESULT InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles);
                HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta);
                HRESULT InvalidateSystem(_In_ const RECT* const prcDirtyClient);
                HRESULT Invalidate(_In_ const SMALL_RECT* const psrRegion);
                HRESULT InvalidateAll();

                HRESULT StartPaint();
                HRESULT EndPaint();

                HRESULT ScrollFrame();

                HRESULT PaintBackground();
                HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                        _In_reads_(cchLine) const unsigned char* const rgWidths,
                                        _In_ size_t const cchLine,
                                        _In_ COORD const coordTarget,
                                        _In_ bool const fTrimLeft);
                HRESULT PaintBufferGridLines(_In_ GridLines const lines, _In_ COLORREF const color, _In_ size_t const cchLine, _In_ COORD const coordTarget);
                HRESULT PaintSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles);

                HRESULT PaintCursor(_In_ COORD const coordCursor, _In_ ULONG const ulCursorHeightPercent, _In_ bool const fIsDoubleWidth);
                HRESULT ClearCursor();

                HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds);
                HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired, _Out_ FontInfo* const pfiFontInfo);
                HRESULT UpdateDpi(_In_ int const iDpi);
                HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport);

                HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired, _Out_ FontInfo* const pfiFont, _In_ int const iDpi);

                SMALL_RECT GetDirtyRectInChars();
                COORD GetFontSize();
                bool IsCharFullWidthByFont(_In_ WCHAR const wch);

            private:
                wil::unique_hfile _hFile;
                HRESULT _Write(_In_reads_(cch) PCSTR psz, _In_ size_t const cch);
                HRESULT _Write(_In_ std::string& str);
                
                void _OrRect(_In_ SMALL_RECT* const pRectExisting, _In_ const SMALL_RECT* const pRectToOr) const;
                HRESULT _InvalidCombine(_In_ const SMALL_RECT* const psrc);
                HRESULT _InvalidOffset(_In_ const COORD* const ppt);
                HRESULT _MoveCursor(_In_ const COORD coord);
                // HRESULT _InvalidRestrict();

                COLORREF _LastFG;
                COLORREF _LastBG;

                SMALL_RECT _srLastViewport;

                SMALL_RECT _srcInvalid;
                bool _fInvalidRectUsed;
                COORD _lastRealCursor;
                COORD _lastText;
                COORD _scrollDelta;
            };
        };
    };
};