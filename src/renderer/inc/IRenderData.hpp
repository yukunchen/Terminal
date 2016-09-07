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

#include "conimeinfo.h"

class TEXT_BUFFER_INFO;
class TextAttribute;
class Cursor;
class SCREEN_INFORMATION;

namespace Microsoft
{
    namespace Console
    {
        namespace Render
        {
            class IRenderData
            {
            public:
                virtual const SMALL_RECT* GetViewport() = 0;
                virtual const TEXT_BUFFER_INFO* GetTextBuffer() = 0;
                virtual const FontInfo* GetFontInfo() = 0;
                virtual const TextAttribute* const GetDefaultBrushColors() = 0;
                virtual const void GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors) = 0;
                virtual const Cursor* GetCursor() = 0;
                virtual const CONSOLE_IME_INFORMATION* GetImeData() = 0;
                virtual const TEXT_BUFFER_INFO* GetImeCompositionStringBuffer(_In_ size_t iIndex) = 0;

                virtual const bool IsGridLineDrawingAllowed() = 0;
                
                virtual _Check_return_
                    NTSTATUS GetSelectionRects(
                        _Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                        _Out_ UINT* const pcRectangles) = 0;
            };
        };
    };
};
