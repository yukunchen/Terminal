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

namespace Microsoft::Console::Render
{
    class GdiEngine final : public IRenderEngine
    {
    public:
        GdiEngine();
        ~GdiEngine() override;

        [[nodiscard]]
        HRESULT SetHwnd(const HWND hwnd);

        [[nodiscard]]
        HRESULT InvalidateSelection(const std::vector<SMALL_RECT>& rectangles) override;
        [[nodiscard]]
        HRESULT InvalidateScroll(const COORD* const pcoordDelta) override;
        [[nodiscard]]
        HRESULT InvalidateSystem(const RECT* const prcDirtyClient) override;
        [[nodiscard]]
        HRESULT Invalidate(const SMALL_RECT* const psrRegion) override;
        [[nodiscard]]
        HRESULT InvalidateCursor(const COORD* const pcoordCursor) override;
        [[nodiscard]]
        HRESULT InvalidateAll() override;
        [[nodiscard]]
        HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) override;
        [[nodiscard]]
        HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) override;

        [[nodiscard]]
        HRESULT StartPaint() override;
        [[nodiscard]]
        HRESULT EndPaint() override;

        [[nodiscard]]
        HRESULT ScrollFrame() override;

        [[nodiscard]]
        HRESULT PaintBackground() override;
        [[nodiscard]]
        HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                _In_reads_(cchLine) const unsigned char* const rgWidths,
                                const size_t cchLine,
                                const COORD coordTarget,
                                const bool fTrimLeft) override;
        [[nodiscard]]
        HRESULT PaintBufferGridLines(const GridLines lines,
                                     const COLORREF color,
                                     const size_t cchLine,
                                     const COORD coordTarget) override;
        [[nodiscard]]
        HRESULT PaintSelection(const std::vector<SMALL_RECT>& rectangles) override;

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
        HRESULT UpdateDrawingBrushes(const COLORREF colorForeground,
                                     const COLORREF colorBackground,
                                     const WORD legacyColorAttribute,
                                     const bool fIncludeBackgrounds) override;
        [[nodiscard]]
        HRESULT UpdateFont(const FontInfoDesired& FontInfoDesired,
                           _Out_ FontInfo& FontInfo) override;
        [[nodiscard]]
        HRESULT UpdateDpi(const int iDpi) override;
        [[nodiscard]]
        HRESULT UpdateViewport(const SMALL_RECT srNewViewport) override;

        [[nodiscard]]
        HRESULT GetProposedFont(const FontInfoDesired& FontDesired,
                                _Out_ FontInfo& Font,
                                const int iDpi) override;

        SMALL_RECT GetDirtyRectInChars() override;
        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
        [[nodiscard]]
        HRESULT IsCharFullWidthByFont(const WCHAR wch, _Out_ bool* const pResult) override;

        [[nodiscard]]
        HRESULT UpdateTitle(_In_ const std::wstring& newTitle) override;

    private:
        HWND _hwndTargetWindow;

        [[nodiscard]]
        static HRESULT s_SetWindowLongWHelper(const HWND hWnd,
                                                const int nIndex,
                                                const LONG dwNewLong);

        bool _fPaintStarted;

        PAINTSTRUCT _psInvalidData;
        HDC _hdcMemoryContext;
        HFONT _hfont;
        TEXTMETRICW _tmFontMetrics;

        static const size_t s_cPolyTextCache = 80;
        POLYTEXTW _pPolyText[s_cPolyTextCache];
        size_t _cPolyText;
        [[nodiscard]]
        HRESULT _FlushBufferLines();

        RECT _rcCursorInvert;

        COORD _coordFontLast;
        int _iCurrentDpi;

        static const int s_iBaseDpi = USER_DEFAULT_SCREEN_DPI;

        SIZE _szMemorySurface;
        HBITMAP _hbitmapMemorySurface;
        [[nodiscard]]
        HRESULT _PrepareMemoryBitmap(const HWND hwnd);

        SIZE _szInvalidScroll;
        RECT _rcInvalid;
        bool _fInvalidRectUsed;

        COLORREF _lastFg;
        COLORREF _lastBg;

        [[nodiscard]]
        HRESULT _InvalidCombine(const RECT* const prc);
        [[nodiscard]]
        HRESULT _InvalidOffset(const POINT* const ppt);
        [[nodiscard]]
        HRESULT _InvalidRestrict();

        [[nodiscard]]
        HRESULT _InvalidateRect(const RECT* const prc);
        [[nodiscard]]
        HRESULT _InvalidateRgn(_In_ HRGN hrgn);

        [[nodiscard]]
        HRESULT _PaintBackgroundColor(const RECT* const prc);

        HRGN _hrgnGdiPaintedSelection;
        [[nodiscard]]
        HRESULT _PaintSelectionCalculateRegion(const std::vector<SMALL_RECT>& rectangles,
                                               _Inout_ HRGN const hrgnSelection) const;

        static const ULONG s_ulMinCursorHeightPercent = 25;
        static const ULONG s_ulMaxCursorHeightPercent = 100;

        [[nodiscard]]
        HRESULT _ScaleByFont(const COORD* const pcoord, _Out_ POINT* const pPoint) const;
        [[nodiscard]]
        HRESULT _ScaleByFont(const SMALL_RECT* const psr, _Out_ RECT* const prc) const;
        [[nodiscard]]
        HRESULT _ScaleByFont(const RECT* const prc, _Out_ SMALL_RECT* const psr) const;

        static int s_ScaleByDpi(const int iPx, const int iDpi);
        static int s_ShrinkByDpi(const int iPx, const int iDpi);

        POINT _GetInvalidRectPoint() const;
        SIZE _GetInvalidRectSize() const;
        SIZE _GetRectSize(const RECT* const pRect) const;

        void _OrRect(_In_ RECT* const pRectExisting, const RECT* const pRectToOr) const;

        bool _IsFontTrueType() const;

        [[nodiscard]]
        HRESULT _GetProposedFont(const FontInfoDesired& FontDesired,
                                 _Out_ FontInfo& Font,
                                 const int iDpi,
                                 _Inout_ wil::unique_hfont& hFont);

        COORD _GetFontSize() const;
        bool _IsMinimized() const;
        bool _IsWindowValid() const;

#ifdef DBG
        // Helper functions to diagnose issues with painting from the in-memory buffer.
        // These are only actually effective/on in Debug builds when the flag is set using an attached debugger.
        bool _fDebug = false;
        void _PaintDebugRect(const RECT* const prc) const;
        void _DoDebugBlt(const RECT* const prc) const;
#endif
    };
}
