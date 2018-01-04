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

#include "../inc/IRenderEngine.hpp"
#include "../../types/inc/Viewport.hpp"
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
    VtEngine(_In_ wil::unique_hfile hPipe,
             _In_ const Microsoft::Console::Types::Viewport initialViewport);
    virtual ~VtEngine() override = default;

    HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                _In_ UINT const cRectangles) override;
    virtual HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta) = 0;
    HRESULT InvalidateSystem(_In_ const RECT* const prcDirtyClient) override;
    HRESULT Invalidate(_In_ const SMALL_RECT* const psrRegion) override;
    HRESULT InvalidateAll() override;

    virtual HRESULT StartPaint() override;
    virtual HRESULT EndPaint() override;

    virtual HRESULT ScrollFrame() = 0;

    HRESULT PaintBackground() override;
    virtual HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
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
                        _In_ bool const fIsDoubleWidth) override;
    HRESULT ClearCursor() override;

    virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                         _In_ COLORREF const colorBackground,
                                         _In_ WORD const legacyColorAttribute,
                                         _In_ bool const fIncludeBackgrounds) = 0;
    HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired,
                       _Out_ FontInfo* const pfiFontInfo) override;
    HRESULT UpdateDpi(_In_ int const iDpi) override;
    HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport) override;

    HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired,
                            _Out_ FontInfo* const pfiFont, _In_ int const iDpi) override;

    SMALL_RECT GetDirtyRectInChars() override;
    HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
    HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) override;
            
    // See _PaintUtf8BufferLine for explanation of this value.
    static const size_t ERASE_CHARACTER_STRING_LENGTH = 8;

protected:
    wil::unique_hfile _hFile;

    COLORREF _LastFG;
    COLORREF _LastBG;

    Microsoft::Console::Types::Viewport _lastViewport;
    Microsoft::Console::Types::Viewport _invalidRect;

    bool _fInvalidRectUsed;
    COORD _lastRealCursor;
    COORD _lastText;
    COORD _scrollDelta;

    bool _quickReturn;
    bool _clearedAllThisFrame;

    HRESULT _Write(_In_reads_(cch) const char* const psz, _In_ size_t const cch);
    HRESULT _Write(_In_ const std::string& str);
    HRESULT _Write(_In_ const char* const psz);
    HRESULT _WriteFormattedString(_In_ const char* const pszFormat, ...);
    
    void _OrRect(_Inout_ SMALL_RECT* const pRectExisting, _In_ const SMALL_RECT* const pRectToOr) const;
    HRESULT _InvalidCombine(_In_ const Microsoft::Console::Types::Viewport invalid);
    HRESULT _InvalidOffset(_In_ const COORD* const ppt);
    HRESULT _InvalidRestrict();
    bool _AllIsInvalid() const;
    
    HRESULT _StopCursorBlinking();
    HRESULT _StartCursorBlinking();
    HRESULT _HideCursor();
    HRESULT _ShowCursor();
    HRESULT _EraseLine();
    HRESULT _InsertDeleteLine(_In_ const short sLines, _In_ const bool fInsertLine);
    HRESULT _DeleteLine(_In_ const short sLines);
    HRESULT _InsertLine(_In_ const short sLines);
    HRESULT _CursorForward(_In_ const short chars);
    HRESULT _EraseCharacter(_In_ const short chars);
    HRESULT _CursorPosition(_In_ const COORD coord);
    HRESULT _CursorHome();
    HRESULT _ClearScreen();
    HRESULT _SetGraphicsRendition16Color(_In_ const WORD wAttr,
                                         _In_ const bool fIsForeground);
    HRESULT _SetGraphicsRenditionRGBColor(_In_ const COLORREF color,
                                          _In_ const bool fIsForeground);
    HRESULT _ResizeWindow(_In_ const short sWidth, _In_ const short sHeight);
    
    virtual HRESULT _MoveCursor(_In_ const COORD coord) = 0;
    HRESULT _RgbUpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                     _In_ COLORREF const colorBackground,
                                     _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                     _In_ const WORD cColorTable);
    HRESULT _16ColorUpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                         _In_ COLORREF const colorBackground,
                                         _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                         _In_ const WORD cColorTable);

    bool _WillWriteSingleChar() const;

    HRESULT _PaintUtf8BufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                 _In_reads_(cchLine) const unsigned char* const rgWidths,
                                 _In_ size_t const cchLine,
                                 _In_ COORD const coordTarget);

    HRESULT _PaintAsciiBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                  _In_reads_(cchLine) const unsigned char* const rgWidths,
                                  _In_ size_t const cchLine,
                                  _In_ COORD const coordTarget);

    /////////////////////////// Unit Testing Helpers ///////////////////////////
#ifdef UNIT_TESTING
    std::function<bool(const char* const, size_t const)> _pfnTestCallback;
    bool _usingTestCallback;

    friend class VtRendererTest;
#endif

    void SetTestCallback(_In_ std::function<bool(const char* const, size_t const)> pfn);

};
