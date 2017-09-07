
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
    XtermEngine(HANDLE hPipe, _In_reads_(cColorTable) const COLORREF* const ColorTable, _In_ const WORD cColorTable);
    HRESULT StartPaint();
    HRESULT EndPaint();
    virtual HRESULT UpdateDrawingBrushes(_In_ COLORREF const colorForeground, _In_ COLORREF const colorBackground, _In_ WORD const legacyColorAttribute, _In_ bool const fIncludeBackgrounds);
    HRESULT ScrollFrame();
    HRESULT InvalidateScroll(_In_ const COORD* const pcoordDelta);
protected:
    HRESULT _MoveCursor(_In_ const COORD coord);
private:
    const COLORREF* const _ColorTable;
    const WORD _cColorTable;
};
