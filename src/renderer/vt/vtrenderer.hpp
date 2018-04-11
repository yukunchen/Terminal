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
                const Microsoft::Console::IDefaultColorProvider& colorProvider,
                const Microsoft::Console::Types::Viewport initialViewport);

        virtual ~VtEngine() override = default;

        [[nodiscard]]
        HRESULT InvalidateSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                                    const UINT cRectangles) override;
        [[nodiscard]]
        virtual HRESULT InvalidateScroll(const COORD* const pcoordDelta) = 0;
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
                                        const size_t cchLine,
                                        const COORD coordTarget,
                                        const bool fTrimLeft) override;
        [[nodiscard]]
        HRESULT PaintBufferGridLines(const GridLines lines,
                                    const COLORREF color,
                                    const size_t cchLine,
                                    const COORD coordTarget) override;
        [[nodiscard]]
        HRESULT PaintSelection(_In_reads_(cRectangles) const SMALL_RECT* const rgsrSelection,
                            const UINT cRectangles) override;

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
        virtual HRESULT UpdateDrawingBrushes(const COLORREF colorForeground,
                                            const COLORREF colorBackground,
                                            const WORD legacyColorAttribute,
                                            const bool fIncludeBackgrounds) = 0;
        [[nodiscard]]
        HRESULT UpdateFont(const FontInfoDesired * const pfiFontInfoDesired,
                        _Out_ FontInfo* const pfiFontInfo) override;
        [[nodiscard]]
        HRESULT UpdateDpi(const int iDpi) override;
        [[nodiscard]]
        HRESULT UpdateViewport(const SMALL_RECT srNewViewport) override;

        [[nodiscard]]
        HRESULT GetProposedFont(const FontInfoDesired * const pfiFontDesired,
                                _Out_ FontInfo* const pfiFont, const int iDpi) override;

        SMALL_RECT GetDirtyRectInChars() override;
        [[nodiscard]]
        HRESULT GetFontSize(_Out_ COORD* const pFontSize) override;
        [[nodiscard]]
        HRESULT IsCharFullWidthByFont(const WCHAR wch, _Out_ bool* const pResult) override;

        [[nodiscard]]
        HRESULT SuppressResizeRepaint();

        [[nodiscard]]
        HRESULT RequestCursor();
        [[nodiscard]]
        HRESULT InheritCursor(const COORD coordCursor);

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
        HRESULT _Write(_In_reads_(cch) const char* const psz, const size_t cch);
        [[nodiscard]]
        HRESULT _Write(const std::string& str);
        [[nodiscard]]
        HRESULT _WriteFormattedString(const std::string* const pFormat, ...);

        void _OrRect(_Inout_ SMALL_RECT* const pRectExisting, const SMALL_RECT* const pRectToOr) const;
        [[nodiscard]]
        HRESULT _InvalidCombine(const Microsoft::Console::Types::Viewport invalid);
        [[nodiscard]]
        HRESULT _InvalidOffset(const COORD* const ppt);
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
        HRESULT _InsertDeleteLine(const short sLines, const bool fInsertLine);
        [[nodiscard]]
        HRESULT _DeleteLine(const short sLines);
        [[nodiscard]]
        HRESULT _InsertLine(const short sLines);
        [[nodiscard]]
        HRESULT _CursorForward(const short chars);
        [[nodiscard]]
        HRESULT _EraseCharacter(const short chars);
        [[nodiscard]]
        HRESULT _CursorPosition(const COORD coord);
        [[nodiscard]]
        HRESULT _CursorHome();
        [[nodiscard]]
        HRESULT _ClearScreen();
        [[nodiscard]]
        HRESULT _SetGraphicsRendition16Color(const WORD wAttr,
                                            const bool fIsForeground);
        [[nodiscard]]
        HRESULT _SetGraphicsRenditionRGBColor(const COLORREF color,
                                            const bool fIsForeground);
        [[nodiscard]]
        HRESULT _SetGraphicsRenditionDefaultColor(const bool fIsForeground);
        [[nodiscard]]
        HRESULT _ResizeWindow(const short sWidth, const short sHeight);

        [[nodiscard]]
        virtual HRESULT _MoveCursor(const COORD coord) = 0;
        [[nodiscard]]
        HRESULT _RgbUpdateDrawingBrushes(const COLORREF colorForeground,
                                        const COLORREF colorBackground,
                                        _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                        const WORD cColorTable);
        [[nodiscard]]
        HRESULT _16ColorUpdateDrawingBrushes(const COLORREF colorForeground,
                                            const COLORREF colorBackground,
                                            _In_reads_(cColorTable) const COLORREF* const ColorTable,
                                            const WORD cColorTable);

        bool _WillWriteSingleChar() const;

        [[nodiscard]]
        HRESULT _PaintUtf8BufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                    _In_reads_(cchLine) const unsigned char* const rgWidths,
                                    const size_t cchLine,
                                    const COORD coordTarget);

        [[nodiscard]]
        HRESULT _PaintAsciiBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                    _In_reads_(cchLine) const unsigned char* const rgWidths,
                                    const size_t cchLine,
                                    const COORD coordTarget);

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
