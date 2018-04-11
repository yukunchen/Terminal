/*++
Copyright (c) Microsoft Corporation

Module Name:
- XtermEngine.hpp

Abstract:
- This is the definition of the VT specific implementation of the renderer.
    This is the xterm implementation, which supports advanced sequences such as
    inserting and deleting lines, but only 16 colors.

    This engine supports both xterm and xterm-ascii VT modes.
    The difference being that xterm-ascii will render any characters above 0x7f
        as '?', in order to support older legacy tools.

Author(s):
- Mike Griese (migrie) 01-Sept-2017
--*/

#pragma once

#include "vtrenderer.hpp"

namespace Microsoft::Console::Render
{
    class XtermEngine : public VtEngine
    {
    public:
        XtermEngine(_In_ wil::unique_hfile hPipe,
                    const Microsoft::Console::IDefaultColorProvider& colorProvider,
                    const Microsoft::Console::Types::Viewport initialViewport,
                    _In_reads_(cColorTable) const COLORREF* const ColorTable,
                    const WORD cColorTable,
                    const bool fUseAsciiOnly);

        virtual ~XtermEngine() override = default;

        [[nodiscard]]
        HRESULT StartPaint() override;
        [[nodiscard]]
        HRESULT EndPaint() override;
        [[nodiscard]]
        virtual HRESULT UpdateDrawingBrushes(const COLORREF colorForeground,
                                            const COLORREF colorBackground,
                                            const WORD legacyColorAttribute,
                                            const bool fIncludeBackgrounds) override;
        [[nodiscard]]
        HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                                _In_reads_(cchLine) const unsigned char* const rgWidths,
                                const size_t cchLine,
                                const COORD coordTarget,
                                const bool fTrimLeft) override;
        [[nodiscard]]
        HRESULT ScrollFrame() override;
        [[nodiscard]]
        HRESULT UpdateTitle(const std::wstring& newTitle) override;
        [[nodiscard]]
        HRESULT InvalidateScroll(const COORD* const pcoordDelta) override;
    protected:
        [[nodiscard]]
        HRESULT _MoveCursor(const COORD coord);
        const COLORREF* const _ColorTable;
        const WORD _cColorTable;
        const bool _fUseAsciiOnly;

    #ifdef UNIT_TESTING
        friend class VtRendererTest;
    #endif
    };
}
