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

#include "vtrenderer.hpp"
#pragma once
namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class XtermEngine;
        }
    }
};

class Microsoft::Console::Render::XtermEngine : public VtEngine
{
public:
    XtermEngine(_In_ wil::unique_hfile hPipe,
                _In_ const Microsoft::Console::Types::Viewport initialViewport,
                _In_reads_(cColorTable) const COLORREF* const ColorTable,
                _In_ const WORD cColorTable,
                _In_ const bool fUseAsciiOnly);
    
    virtual ~XtermEngine() override = default;
    
    HRESULT StartPaint() override;
    HRESULT EndPaint() override;
    virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                         _In_ COLORREF const colorBackground,
                                         _In_ WORD const legacyColorAttribute,
                                         _In_ bool const fIncludeBackgrounds) override;
    HRESULT PaintBufferLine(_In_reads_(cchLine) PCWCHAR const pwsLine,
                            _In_reads_(cchLine) const unsigned char* const rgWidths,
                            _In_ size_t const cchLine,
                            _In_ COORD const coordTarget,
                            _In_ bool const fTrimLeft) override;
    HRESULT ScrollFrame() override;
    HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta) override;
protected:
    HRESULT _MoveCursor(_In_ const COORD coord);
    const COLORREF* const _ColorTable;
    const WORD _cColorTable;
    const bool _fUseAsciiOnly;
    
#ifdef UNIT_TESTING
    friend class VtRendererTest;
#endif
};
