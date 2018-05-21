#pragma once

#include "..\..\renderer\inc\IRenderEngine.hpp"

namespace Microsoft::Console::Render
{
    class WddmConEngine sealed : public IRenderEngine
    {
    public:
        WddmConEngine();
        ~WddmConEngine() override;

        [[nodiscard]]
        HRESULT Initialize();
        bool IsInitialized();

        // Used to release device resources so that another instance of
        // conhost can render to the screen (i.e. only one DirectX
        // application may control the screen at a time.)
        [[nodiscard]]
        HRESULT Enable();
        [[nodiscard]]
        HRESULT Disable();

        RECT GetDisplaySize();

        // IRenderEngine Members
        [[nodiscard]]
        HRESULT Invalidate(const SMALL_RECT* const psrRegion);
        [[nodiscard]]
        HRESULT InvalidateCursor(const COORD* const pcoordCursor) override;
        [[nodiscard]]
        HRESULT InvalidateSystem(const RECT* const prcDirtyClient);
        [[nodiscard]]
        HRESULT InvalidateSelection(const std::vector<SMALL_RECT>& rectangles);
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
                                bool const fTrimLeft,
                                const bool lineWrapped);
        [[nodiscard]]
        HRESULT PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget);
        [[nodiscard]]
        HRESULT PaintSelection(const std::vector<SMALL_RECT>& rectangles);

        [[nodiscard]]
        HRESULT PaintCursor(const COORD coordCursor,
                            const ULONG ulCursorHeightPercent,
                            const bool fIsDoubleWidth,
                            const CursorType cursorType,
                            const bool fUseColor,
                            const COLORREF cursorColor) override;

        [[nodiscard]]
        HRESULT ClearCursor();

        [[nodiscard]]
        HRESULT UpdateDrawingBrushes(COLORREF const colorForeground, COLORREF const colorBackground, const WORD legacyColorAttribute, bool const fIncludeBackgrounds);
        [[nodiscard]]
        HRESULT UpdateFont(const FontInfoDesired& fiFontInfoDesired, FontInfo& fiFontInfo);
        [[nodiscard]]
        HRESULT UpdateDpi(int const iDpi);
        [[nodiscard]]
        HRESULT UpdateViewport(const SMALL_RECT srNewViewport);

        [[nodiscard]]
        HRESULT GetProposedFont(const FontInfoDesired& fiFontInfoDesired, FontInfo& fiFontInfo, int const iDpi);

        SMALL_RECT GetDirtyRectInChars();
        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
        [[nodiscard]]
        HRESULT IsCharFullWidthByFont(const WCHAR wch, _Out_ bool* const pResult) override;

        [[nodiscard]]
        HRESULT UpdateTitle(_In_ const std::wstring& newTitle) override;

    private:
        HANDLE _hWddmConCtx;

        // Helpers
        void FreeResources(ULONG displayHeight);

        // Variables
        LONG _displayHeight;
        LONG _displayWidth;

        PCD_IO_ROW_INFORMATION *_displayState;

        WORD _currentLegacyColorAttribute;
    };
}
