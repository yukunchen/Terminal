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

#include "XtermEngine.hpp"
#pragma once
namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class Xterm256Engine;
        }
    }
};

class Microsoft::Console::Render::Xterm256Engine : public XtermEngine
{
public:
    Xterm256Engine(_In_ wil::unique_hfile hPipe);
    HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground,
                                 _In_ COLORREF const colorBackground,
                                 _In_ WORD const legacyColorAttribute,
                                 _In_ bool const fIncludeBackgrounds) override;

private:
    
#ifdef UNIT_TESTING
    friend class VtRendererTest;
#endif
};
