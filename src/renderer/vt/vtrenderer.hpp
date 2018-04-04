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
#include "../../inc/IDefaultColorProvider.hpp"
#include "../../inc/ITerminalOutputConnection.hpp"
#include "../../types/inc/Viewport.hpp"
#include <string>
#include <functional>

namespace Microsoft::Console::Render
{
    class VtEngine : public IRenderEngine, public Microsoft::Console::ITerminalOutputConnection
    {
    public:
        // See _PaintUtf8BufferLine for explanation of this value.
        static const size_t ERASE_CHARACTER_STRING_LENGTH = 8;
        static const COORD INVALID_COORDS;

        VtEngine(_In_ wil::unique_hfile hPipe,
                _In_ const Microsoft::Console::IDefaultColorProvider& colorProvider,
                _In_ const Microsoft::Console::Types::Viewport initialViewport);

        virtual ~VtEngine() override = default;

        [[nodiscard]]
        HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                    _In_ UINT const cRectangles) override;
        [[nodiscard]]
        virtual HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta) = 0;
        [[nodiscard]]
        HRESULT InvalidateSystem(_In_ const RECT* const prcDirtyClient) override;
        [[nodiscard]]
        HRESULT Invalidate(_In_ const SMALL_RECT* const psrRegion) override;
        [[nodiscard]]
        HRESULT InvalidateCursor(_In_ const COORD* const pcoordCursor) override;
        [[nodiscard]]
        HRESULT InvalidateAll() override;
        [[nodiscard]]
        HRESULT InvalidateCircling(_Out_ bool* const pForcePaint) override;
        [[nodiscard]]
        HRESULT PrepareForTeardown(_Out_ bool* const pForcePaint) override;

        [[nodiscard]]
        virtual HRESULT StartPaint() override;
        [[nodiscard]]
        virtual HRESULT EndPaint() override;

        [[nodiscard]]
        virtual HRESULT ScrollFrame() = 0;

        [[nodiscard]]
        HRESULT PaintBackground() override;
        [[nodiscard]]
        virtual HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                        _In_reads_(cchLine) const unsigned char* const rgWidths,
                                        _In_ size_t const cchLine,
                                        _In_ COORD const coordTarget,
                                        _In_ bool const fTrimLeft) override;
        [[nodiscard]]
        HRESULT PaintBufferGridLines(_In_ GridLines const lines,
                                    _In_ COLORREF const color,
                                    _In_ size_t const cchLine,
                                    _In_ COORD const coordTarget) override;
        [[nodiscard]]
        HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                            _In_ UINT const cRectangles) override;

        [[nodiscard]]
        HRESULT PaintCursor(_In_ COORD const coordCursor,
                            _In_ ULONG const ulCursorHeightPercent,
                            _In_ bool const fIsDoubleWidth,
                            _In_ CursorType const cursorType,
                            _In_ bool const fUseColor,
                            _In_ COLORREF const cursorColor) override;

        [[nodiscard]]
        HRESULT ClearCursor() override;

        [[nodiscard]]
        virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                            _In_ COLORREF const colorBackground,
                                            _In_ WORD const legacyColorAttribute,
                                            _In_ bool const fIncludeBackgrounds) = 0;
        [[nodiscard]]
        HRESULT UpdateFont(_In_ FontInfoDesired const * const pfiFontInfoDesired,
                        _Out_ FontInfo* const pfiFontInfo) override;
        [[nodiscard]]
        HRESULT UpdateDpi(_In_ int const iDpi) override;
        [[nodiscard]]
        HRESULT UpdateViewport(_In_ SMALL_RECT const srNewViewport) override;

        [[nodiscard]]
        HRESULT GetProposedFont(_In_ FontInfoDesired const * const pfiFontDesired,
                                _Out_ FontInfo* const pfiFont, _In_ int const iDpi) override;

        SMALL_RECT GetDirtyRectInChars() override;
        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
        [[nodiscard]]
        HRESULT IsCharFullWidthByFont(_In_ WCHAR const wch, _Out_ bool* const pResult) override;

        [[nodiscard]]
        HRESULT SuppressResizeRepaint();

        [[nodiscard]]
        HRESULT RequestCursor();
        [[nodiscard]]
        HRESULT InheritCursor(_In_ const COORD coordCursor);

        [[nodiscard]]
        HRESULT WriteTerminalUtf8(_In_ const std::string& str);
        [[nodiscard]]
        virtual HRESULT WriteTerminalW(_In_ const std::wstring& str) = 0;

    protected:
        wil::unique_hfile _hFile;

        const Microsoft::Console::IDefaultColorProvider& _colorProvider;

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
        bool _cursorMoved;

        bool _suppressResizeRepaint;

        SHORT _virtualTop;
        bool _circled;
        bool _firstPaint;
        bool _skipCursor;

        [[nodiscard]]
        HRESULT _Write(_In_reads_(cch) const char* const psz, _In_ size_t const cch);
        [[nodiscard]]
        HRESULT _Write(_In_ const std::string& str);
        [[nodiscard]]
        HRESULT _WriteFormattedString(_In_ const std::string* const pFormat, ...);

        void _OrRect(_Inout_ SMALL_RECT* const pRectExisting, _In_ const SMALL_RECT* const pRectToOr) const;
        [[nodiscard]]
        HRESULT _InvalidCombine(_In_ const Microsoft::Console::Types::Viewport invalid);
        [[nodiscard]]
        HRESULT _InvalidOffset(_In_ const COORD* const ppt);
        [[nodiscard]]
        HRESULT _InvalidRestrict();
        bool _AllIsInvalid() const;

        [[nodiscard]]
        HRESULT _StopCursorBlinking();
        [[nodiscard]]
        HRESULT _StartCursorBlinking();
        [[nodiscard]]
        HRESULT _HideCursor();
        [[nodiscard]]
        HRESULT _ShowCursor();
        [[nodiscard]]
        HRESULT _EraseLine();
        [[nodiscard]]
        HRESULT _InsertDeleteLine(_In_ const short sLines, _In_ const bool fInsertLine);
        [[nodiscard]]
        HRESULT _DeleteLine(_In_ const short sLines);
        [[nodiscard]]
        HRESULT _InsertLine(_In_ const short sLines);
        [[nodiscard]]
        HRESULT _CursorForward(_In_ const short chars);
        [[nodiscard]]
        HRESULT _EraseCharacter(_In_ const short chars);
        [[nodiscard]]
        HRESULT _CursorPosition(_In_ const COORD coord);
        [[nodiscard]]
        HRESULT _CursorHome();
        [[nodiscard]]
        HRESULT _ClearScreen();
        [[nodiscard]]
        HRESULT _SetGraphicsRendition16Color(_In_ const WORD wAttr,
                                            _In_ const bool fIsForeground);
        [[nodiscard]]
        HRESULT _SetGraphicsRenditionRGBColor(_In_ const COLORREF color,
                                            _In_ const bool fIsForeground);
        [[nodiscard]]
        HRESULT _SetGraphicsRenditionDefaultColor(_In_ const bool fIsForeground);
        [[nodiscard]]
        HRESULT _ResizeWindow(_In_ const short sWidth, _In_ const short sHeight);

        [[nodiscard]]
        virtual HRESULT _MoveCursor(_In_ const COORD coord) = 0;
        [[nodiscard]]
        HRESULT _RgbUpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                        _In_ COLORREF const colorBackground,
                                        _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                        _In_ const WORD cColorTable);
        [[nodiscard]]
        HRESULT _16ColorUpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                            _In_ COLORREF const colorBackground,
                                            _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                            _In_ const WORD cColorTable);

        bool _WillWriteSingleChar() const;

        [[nodiscard]]
        HRESULT _PaintUtf8BufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                    _In_reads_(cchLine) const unsigned char* const rgWidths,
                                    _In_ size_t const cchLine,
                                    _In_ COORD const coordTarget);

        [[nodiscard]]
        HRESULT _PaintAsciiBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                    _In_reads_(cchLine) const unsigned char* const rgWidths,
                                    _In_ size_t const cchLine,
                                    _In_ COORD const coordTarget);

        [[nodiscard]]
        HRESULT _WriteTerminalUtf8(_In_ const std::wstring& str);
        [[nodiscard]]
        HRESULT _WriteTerminalAscii(_In_ const std::wstring& str);
        /////////////////////////// Unit Testing Helpers ///////////////////////////
    #ifdef UNIT_TESTING
        std::function<bool(const char* const, size_t const)> _pfnTestCallback;
        bool _usingTestCallback;

        friend class VtRendererTest;
    #endif

        void SetTestCallback(_In_ std::function<bool(const char* const, size_t const)> pfn);

    };
}
