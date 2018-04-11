/*++
Copyright (c) Microsoft Corporation

Module Name:
- renderData.hpp

Abstract:
- This method provides an interface for rendering the final display based on the current console state

Author(s):
- Michael Niksa (miniksa) Nov 2015
--*/

#pragma once

#include "..\renderer\inc\IRenderData.hpp"

using namespace Microsoft::Console::Render;

class RenderData final : public IRenderData
{
public:
    RenderData();
    virtual ~RenderData();

    const SMALL_RECT GetViewport();
    const TextBuffer& GetTextBuffer();
    const FontInfo* GetFontInfo();
    const TextAttribute GetDefaultBrushColors();
    const void GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors);
    const Cursor& GetCursor();
    const ConsoleImeInfo* GetImeData();
    const TextBuffer& GetImeCompositionStringBuffer(_In_ size_t iIndex);

    const bool IsGridLineDrawingAllowed();

    [[nodiscard]]
    NTSTATUS GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                               _Out_ UINT* const pcRectangles);

    const std::wstring GetConsoleTitle() const override;

};
