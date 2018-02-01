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

#include "..\inc\IRenderEngine.hpp"

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
                ~GdiEngine() override;

                HRESULT SetHwnd(_In_ HWND const hwnd);

                HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                            _In_ UINT const cRectangles) override;
                HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta) override;
                HRESULT InvalidateSystem(_In_ const RECT* const prcDirtyClient) override;
                HRESULT Invalidate(_In_ const SMALL_RECT* const psrRegion) override;
                HRESULT InvalidateCursor(_In_ const COORD* const pcoordCursor) override;
                HRESULT InvalidateAll() override;
                HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) override;
                HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) override;

                HRESULT StartPaint() override;
                HRESULT EndPaint() override;

                HRESULT ScrollFrame() override;

                HRESULT PaintBackground() override;
                HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                        _In_reads_(cchLine) const unsigned char* const rgWidths,
                                        _In_ size_t const cchLine,
                                        _In_ COORD const coordTarget,
                                        _In_ bool const fTrimLeft) override;
                HRESULT PaintBufferGridLines(_In_ GridLines const lines,
                                             _In_ COLORREF const color,
                                             _In_ size_t const cchLine,
                                             _In_ COORD const coordTarget) override;
                HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                       _In_ UINT const cRectangles) override;

                HRESULT PaintCursor(_In_ COORD const coordCursor,
                                    _In_ ULONG const ulCursorHeightPercent,
                                    _In_ bool const fIsDoubleWidth,
                                    _In_ CursorType const cursorType,
                                    _In_ bool const fUseColor,
                                    _In_ COLORREF const cursorColor) override;
                
                HRESULT ClearCursor() override;

                HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                             _In_ COLORREF const colorBackground,
                                             _In_ WORD const legacyColorAttribute,
                                             _In_ bool const fIncludeBackgrounds) override;
                HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired,
                                   _Out_ FontInfo* const pfiFontInfo) override;
                HRESULT UpdateDpi(_In_ int const iDpi) override;
                HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport) override;

                HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired,
                                        _Out_ FontInfo* const pfiFont,
                                        _In_ int const iDpi) override;

                SMALL_RECT GetDirtyRectInChars() override;
                HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
                HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) override;

            private:
                HWND _hwndTargetWindow;

                static HRESULT s_SetWindowLongWHelper(_In_ HWND const hWnd,
                                                      _In_ int const nIndex,
                                                      _In_ LONG const dwNewLong);

                bool _fPaintStarted;

                PAINTSTRUCT _psInvalidData;
                HDC _hdcMemoryContext;
                HFONT _hfont;
                TEXTMETRICW _tmFontMetrics;

                static const size_t s_cPolyTextCache = 80;
                POLYTEXTW _pPolyText[s_cPolyTextCache];
                size_t _cPolyText;
                HRESULT _FlushBufferLines();

                RECT _rcCursorInvert;

                COORD _coordFontLast;
                int _iCurrentDpi;

                static const int s_iBaseDpi = USER_DEFAULT_SCREEN_DPI;

                SIZE _szMemorySurface;
                HBITMAP _hbitmapMemorySurface;
                HRESULT _PrepareMemoryBitmap(_In_ HWND const hwnd);

                SIZE _szInvalidScroll;
                RECT _rcInvalid;
                bool _fInvalidRectUsed;

                COLORREF _lastFg;
                COLORREF _lastBg;

                HRESULT _InvalidCombine(_In_ const RECT* const prc);
                HRESULT _InvalidOffset(_In_ const POINT* const ppt);
                HRESULT _InvalidRestrict();

                HRESULT _InvalidateRect(_In_ const RECT* const prc);
                HRESULT _InvalidateRgn(_In_ HRGN hrgn);

                HRESULT _PaintBackgroundColor(_In_ const RECT* const prc);

                HRGN _hrgnGdiPaintedSelection;
                HRESULT _PaintSelectionCalculateRegion(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                                       _In_ UINT const cRectangles,
                                                       _Inout_ HRGN const hrgnSelection) const;

                static const ULONG s_ulMinCursorHeightPercent = 25;
                static const ULONG s_ulMaxCursorHeightPercent = 100;

                HRESULT _ScaleByFont(_In_ const COORD* const pcoord, _Out_ POINT* const pPoint) const;
                HRESULT _ScaleByFont(_In_ const SMALL_RECT* const psr, _Out_ RECT* const prc) const;
                HRESULT _ScaleByFont(_In_ const RECT* const prc, _Out_ SMALL_RECT* const psr) const;

                static int s_ScaleByDpi(_In_ const int iPx, _In_ const int iDpi);
                static int s_ShrinkByDpi(_In_ const int iPx, _In_ const int iDpi);

                POINT _GetInvalidRectPoint() const;
                SIZE _GetInvalidRectSize() const;
                SIZE _GetRectSize(_In_ const RECT* const pRect) const;

                void _OrRect(_In_ RECT* const pRectExisting, _In_ const RECT* const pRectToOr) const;

                bool _IsFontTrueType() const;

                HRESULT _GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired, _Out_ FontInfo* const pfiFont, _In_ int const iDpi, _Inout_ wil::unique_hfont& hFont);

                COORD _GetFontSize() const;
                bool _IsMinimized() const;

#ifdef DBG
                // Helper functions to diagnose issues with painting from the in-memory buffer.
                // These are only actually effective/on in Debug builds when the flag is set using an attached debugger.
                bool _fDebug = false;
                void _PaintDebugRect(_In_ const RECT* const prc) const;
                void _DoDebugBlt(_In_ const RECT* const prc) const;
#endif
            };
        };
    };
};
