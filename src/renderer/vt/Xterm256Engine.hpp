/*++
Copyright (c) Microsoft Corporation

Module Name:
- Xterm256Engine.hpp

Abstract:
- This is the definition of the VT specific implementation of the renderer.
    This is the xterm-256color implementation, which supports advanced sequences such as
    inserting and deleting lines, and true rgb color.

Author(s):
- Mike Griese (migrie) 01-Sept-2017
--*/

#pragma once

#include "XtermEngine.hpp"

namespace Microsoft::Console::Render
{
    class Xterm256Engine : public XtermEngine
    {
    public:
        Xterm256Engine(_In_ wil::unique_hfile hPipe,
                    _In_ const Microsoft::Console::IDefaultColorProvider& colorProvider,
                    _In_ const Microsoft::Console::Types::Viewport initialViewport,
                    _In_reads_(cColorTable) const COLORREF* const ColorTable,
                    _In_ const WORD cColorTable);

        virtual ~Xterm256Engine() override = default;

        HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                    _In_ COLORREF const colorBackground,
                                    _In_ WORD const legacyColorAttribute,
                                    _In_ bool const fIncludeBackgrounds) override;

    private:

    #ifdef UNIT_TESTING
        friend class VtRendererTest;
    #endif
    };
}
