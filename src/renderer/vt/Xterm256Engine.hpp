
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
    Xterm256Engine(wil::unique_hfile hPipe);
    HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds);

};
