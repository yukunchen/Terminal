/*++
Copyright (c) Microsoft Corporation

Module Name:
- VtRenderer.hpp

Abstract:
- This is the definition of the VT specific implementation of the renderer.

Author(s):
- Michael Niksa (MiNiksa) 24-Jul-2017
- Mike Griese (migrie) 01-Sept-2017
--*/

#pragma once

#include "..\inc\IRenderEngine.hpp"
#include <string>
#include <functional>

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class VtEngine;
        };
    };
};

class Microsoft::Console::Render::VtEngine : public IRenderEngine
{
public:
    VtEngine(wil::unique_hfile hPipe);
    ~VtEngine();

    HRESULT InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles);
    virtual HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta) = 0;
    HRESULT InvalidateSystem(_In_ const RECT* const prcDirtyClient);
    HRESULT Invalidate(_In_ const SMALL_RECT* const psrRegion);
    HRESULT InvalidateAll();

    virtual HRESULT StartPaint();
    virtual HRESULT EndPaint();

    virtual HRESULT ScrollFrame() = 0;

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

    virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds) = 0;
    HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired, _Out_ FontInfo* const pfiFontInfo);
    HRESULT UpdateDpi(_In_ int const iDpi);
    HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport);

    HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired, _Out_ FontInfo* const pfiFont, _In_ int const iDpi);

    SMALL_RECT GetDirtyRectInChars();
    COORD GetFontSize();
    bool IsCharFullWidthByFont(_In_ WCHAR const wch);

protected:
    wil::unique_hfile _hFile;

    COLORREF _LastFG;
    COLORREF _LastBG;

    SMALL_RECT _srLastViewport;

    SMALL_RECT _srcInvalid;
    bool _fInvalidRectUsed;
    COORD _lastRealCursor;
    COORD _lastText;
    COORD _scrollDelta;

    bool _quickReturn;
    
    HRESULT _Write(_In_reads_(cch) PCSTR psz, _In_ size_t const cch);
    HRESULT _Write(_In_ std::string& str);
    
    void _OrRect(_In_ SMALL_RECT* const pRectExisting, _In_ const SMALL_RECT* const pRectToOr) const;
    HRESULT _InvalidCombine(_In_ const SMALL_RECT* const psrc);
    HRESULT _InvalidOffset(_In_ const COORD* const ppt);
    
    HRESULT _StopCursorBlinking();
    HRESULT _StartCursorBlinking();
    HRESULT _HideCursor();
    HRESULT _ShowCursor();
    HRESULT _EraseLine();
    HRESULT _DeleteLine(_In_ const short sLines);
    HRESULT _InsertLine(_In_ const short sLines);
    HRESULT _CursorPosition(_In_ const COORD coord);
    HRESULT _CursorHome();
    
    virtual HRESULT _MoveCursor(_In_ const COORD coord) = 0;
    HRESULT _RgbUpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground);
    HRESULT _16ColorUpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_reads_(cColorTable) const COLORREF* const ColorTable, _In_ const WORD cColorTable);

    std::function<bool(const char* const, size_t const)> _pfnTestCallback;
    bool _usingTestCallback;

#ifdef UNIT_TESTING
    friend class VtRendererTest;
#endif
    
    void SetTestCallback(_In_ std::function<bool(const char* const, size_t const)> pfn);
};
