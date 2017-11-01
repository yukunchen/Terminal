#pragma once

#include "..\..\renderer\inc\IRenderEngine.hpp"
#include "..\..\renderer\inc\MinimalCursor.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class WddmConEngine sealed : public IRenderEngine
            {
            public:
                WddmConEngine();
                ~WddmConEngine();

                HRESULT Initialize();
                bool IsInitialized();

                // Used to release device resources so that another instance of
                // conhost can render to the screen (i.e. only one DirectX
                // application may control the screen at a time.)
                HRESULT Enable();
                HRESULT Disable();

                RECT GetDisplaySize();

                // IRenderEngine Members
                HRESULT Invalidate(const SMALL_RECT* const psrRegion);
                HRESULT InvalidateSystem(const RECT* const prcDirtyClient);
                HRESULT InvalidateSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);
                HRESULT InvalidateScroll(const COORD* const pcoordDelta);
                HRESULT InvalidateAll();

                HRESULT StartPaint();
                HRESULT EndPaint();

                HRESULT ScrollFrame();

                HRESULT PaintBackground();
                HRESULT PaintBufferLine(PCWCHAR const pwsLine, const unsigned char* const rgWidths, size_t const cchLine, COORD const coord, bool const fTrimLeft);
                HRESULT PaintBufferGridLines(GridLines const lines, COLORREF const color, size_t const cchLine, COORD const coordTarget);
                HRESULT PaintSelection(const SMALL_RECT* const rgsrSelection, UINT const cRectangles);

                HRESULT PaintCursorEx(_In_ ULONG const ulCursorHeightPercent,
                                      _In_ bool const fIsDoubleWidth,
                                      _In_ CursorType const cursorType,
                                      _In_ bool const fUseColor,
                                      _In_ COLORREF const cursorColor) override;
                HRESULT ClearCursor();

                HRESULT UpdateDrawingBrushes(COLORREF const colorForeground, COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, bool const fIncludeBackgrounds);
                HRESULT UpdateFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo);
                HRESULT UpdateDpi(int const iDpi);
                HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport);

                HRESULT GetProposedFont(FontInfoDesired const* const pfiFontInfoDesired, FontInfo* const pfiFontInfo, int const iDpi);

                SMALL_RECT GetDirtyRectInChars();
                COORD GetFontSize();
                bool IsCharFullWidthByFont(WCHAR const wch);
                
                IRenderCursor* const GetCursor() override;

            private:
                HANDLE _hWddmConCtx;

                // Helpers
                void FreeResources(ULONG displayHeight);

                // Variables
                LONG _displayHeight;
                LONG _displayWidth;

                PCD_IO_ROW_INFORMATION *_displayState;

                WORD _currentLegacyColorAttribute;

                MinimalCursor _cursor;
            };
        };
    };
};
