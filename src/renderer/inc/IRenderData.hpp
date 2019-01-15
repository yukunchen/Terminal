/*++
Copyright (c) Microsoft Corporation

Module Name:
- IRenderData.hpp

Abstract:
- This serves as the interface defining all information needed to render to the screen.

Author(s):
- Michael Niksa (MiNiksa) 17-Nov-2015
--*/

#pragma once

#include "../../host/conimeinfo.h"
#include "../../buffer/out/TextAttribute.hpp"
#include "../../types/inc/viewport.hpp"

class TextBuffer;
class Cursor;

namespace Microsoft::Console::Render
{
    struct RenderOverlay final
    {
        // This is where the data is stored
        const TextBuffer& buffer;

        // This is where the top left of the stored buffer should be overlayed on the screen
        // (relative to the current visible viewport)
        const COORD origin;

        // This is the area of the buffer that is actually used for overlay.
        // Anything outside of this is considered empty by the overlay and shouldn't be used
        // for painting purposes.
        const Microsoft::Console::Types::Viewport region;
    };

    class IRenderData
    {
    public:
        virtual ~IRenderData() = 0;
        virtual const Microsoft::Console::Types::Viewport& GetViewport() = 0;
        virtual const TextBuffer& GetTextBuffer() = 0;
        virtual const FontInfo* GetFontInfo() = 0;
        virtual const TextAttribute GetDefaultBrushColors() = 0;
        virtual const void GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable,
                                         _Out_ size_t* const pcColors) = 0;

        virtual const COLORREF GetForegroundColor(const TextAttribute& attr) const = 0;
        virtual const COLORREF GetBackgroundColor(const TextAttribute& attr) const = 0;

        virtual COORD GetCursorPosition() const = 0;
        virtual bool IsCursorVisible() const = 0;
        virtual bool IsCursorOn() const = 0;
        virtual ULONG GetCursorHeight() const = 0;
        virtual CursorType GetCursorStyle() const = 0;
        virtual ULONG GetCursorPixelWidth() const = 0;
        virtual COLORREF GetCursorColor() const = 0;
        virtual bool IsCursorDoubleWidth() const = 0;

        virtual const std::vector<RenderOverlay> GetOverlays() const = 0;

        virtual const bool IsGridLineDrawingAllowed() = 0;

        virtual std::vector<SMALL_RECT> GetSelectionRects() = 0;

        virtual const std::wstring GetConsoleTitle() const = 0;
    };

    inline IRenderData::~IRenderData() {}
}
