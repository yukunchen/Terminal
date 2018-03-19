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
#include "../../host/TextAttribute.hpp"

class TEXT_BUFFER_INFO;
class Cursor;
class SCREEN_INFORMATION;

namespace Microsoft::Console::Render
{
    class IRenderData
    {
    public:
        virtual ~IRenderData() = 0;
        virtual const SMALL_RECT GetViewport() = 0;
        virtual const TEXT_BUFFER_INFO* GetTextBuffer() = 0;
        virtual const FontInfo* GetFontInfo() = 0;
        virtual const TextAttribute GetDefaultBrushColors() = 0;
        virtual const void GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable,
                                            _Out_ size_t* const pcColors) = 0;
        virtual const Cursor* GetCursor() = 0;
        virtual const ConsoleImeInfo* GetImeData() = 0;
        virtual const TEXT_BUFFER_INFO* GetImeCompositionStringBuffer(_In_ size_t iIndex) = 0;

        virtual const bool IsGridLineDrawingAllowed() = 0;

        [[nodiscard]]
        virtual NTSTATUS GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                                           _Out_ UINT* const pcRectangles) = 0;
    };

    inline IRenderData::~IRenderData() {}
}
