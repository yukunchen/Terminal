
#include "vtrenderer.hpp"
#pragma once
namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class WinTelnetEngine;
        }

    }
};

class Microsoft::Console::Render::WinTelnetEngine : public VtEngine
{
public:
    WinTelnetEngine(HANDLE hPipe, VtIoMode IoMode);
    HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds);
    HRESULT ScrollFrame();
    HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta);
protected:
    HRESULT _MoveCursor(_In_ const COORD coord);
};
