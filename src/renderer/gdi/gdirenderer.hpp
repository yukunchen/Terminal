/*++
Copyright (c) Microsoft Corporation

Module Name:
- GdiRenderer.hpp

Abstract:
- This is the definition of the GDI specific implementation of the renderer.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "IRenderEngine.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class GdiEngine sealed : public IRenderEngine
            {
            public:
                GdiEngine();
                ~GdiEngine();

                void SetHwnd(_In_ HWND const hwnd);

                void InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles);
                void InvalidateScroll(_In_ const COORD* const pcoordDelta);
                void InvalidateSystem(_In_ const RECT* const prcDirtyClient);
                void Invalidate(_In_ const SMALL_RECT* const psrRegion);
                void InvalidateAll();

                bool StartPaint();
                void EndPaint();

                void ScrollFrame();

                void PaintBackground();
                void PaintGutter();
                void PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine, 
                                     _In_ size_t const cchLine, 
                                     _In_ COORD const coordTarget, 
                                     _In_ size_t const cchCharWidths,
                                     _In_ bool const fTrimLeft);
                void PaintBufferGridLines(_In_ GridLines const lines, _In_ COLORREF const color, _In_ size_t const cchLine, _In_ COORD const coordTarget);
                void PaintSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles);

                void PaintCursor(_In_ COORD const coordCursor, _In_ ULONG const ulCursorHeightPercent, _In_ bool const fIsDoubleWidth);
                void ClearCursor();

                void UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ bool const fIncludeBackgrounds);
                void UpdateFont(_Inout_ FontInfo* const pfiFontInfo);
                void UpdateDpi(_In_ int const iDpi);

                void GetProposedFont(_Inout_ FontInfo* const pfiFontInfo, _In_ int const iDpi, _Out_opt_ HFONT* const phFont = nullptr);
                
                SMALL_RECT GetDirtyRectInChars();
                COORD GetFontSize();
                bool IsCharFullWidthByFont(_In_ WCHAR const wch);

            private:
                HWND _hwndTargetWindow;
                bool _fPaintStarted;
                PAINTSTRUCT _psInvalidData;
                HDC _hdcMemoryContext;
                HFONT _hfont;
                TEXTMETRICW _tmFontMetrics;

                static const size_t s_cPolyTextCache = 80;
                POLYTEXTW _pPolyText[s_cPolyTextCache];
                size_t _cPolyText;
                void _FlushBufferLines();

                RECT _rcCursorInvert;
                
                COORD _coordFontLast;
                int _iCurrentDpi;

                static const int s_iBaseDpi = USER_DEFAULT_SCREEN_DPI;

                SIZE _szMemorySurface;
                HBITMAP _hbitmapMemorySurface;
                void _PrepareMemoryBitmap(_In_ HWND const hwnd);

                SIZE _szInvalidScroll;
                RECT _rcInvalid;
                bool _fInvalidRectUsed;
                void _InvalidCombine(_In_ const RECT* const prc);
                void _InvalidOffset(_In_ const POINT* const ppt);
                void _InvalidRestrict();

                void _InvalidateRect(_In_ const RECT* const prc);
                void _InvalidateRgn(_In_ HRGN hrgn);

                void _PaintBackgroundColor(_In_ const RECT* const prc);

                HRGN _hrgnGdiPaintedSelection;
                NTSTATUS _PaintSelectionCalculateRegion(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, 
                                                        _In_ UINT const cRectangles, 
                                                        _Inout_ HRGN const hrgnSelection) const;

                static const ULONG s_ulMinCursorHeightPercent = 25;
                static const ULONG s_ulMaxCursorHeightPercent = 100;

                POINT _ScaleByFont(_In_ const COORD* const pcoord) const;
                RECT _ScaleByFont(_In_ const SMALL_RECT* const psr) const;
                SMALL_RECT _ScaleByFont(_In_ const RECT* const prc) const;

                static int s_ScaleByDpi(_In_ const int iPx, _In_ const int iDpi);
                static int s_ShrinkByDpi(_In_ const int iPx, _In_ const int iDpi);
               
                POINT _GetInvalidRectPoint() const;
                SIZE _GetInvalidRectSize() const;
                SIZE _GetRectSize(_In_ const RECT* const pRect) const;

                void _OrRect(_In_ RECT* const pRectExisting, _In_ const RECT* const pRectToOr) const;

                bool _IsFontTrueType() const;

                COORD _GetFontSize() const;
                bool _IsMinimized() const;
            };
        };
    };
};