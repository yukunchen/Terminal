/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "renderData.hpp"

#include "dbcs.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

RenderData::RenderData()
{

}

RenderData::~RenderData()
{

}

const SMALL_RECT RenderData::GetViewport()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.CurrentScreenBuffer->GetBufferViewport();
}

const TextBuffer* RenderData::GetTextBuffer()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.CurrentScreenBuffer->GetTextBuffer();
}

const FontInfo* RenderData::GetFontInfo()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.CurrentScreenBuffer->GetTextBuffer().GetCurrentFont();
}

const TextAttribute RenderData::GetDefaultBrushColors()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.CurrentScreenBuffer->GetAttributes();
}

const void RenderData::GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    *ppColorTable = const_cast<COLORREF*>(gci.GetColorTable());
    *pcColors = gci.GetColorTableSize();
}

const Cursor* RenderData::GetCursor()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.CurrentScreenBuffer->GetTextBuffer().GetCursor();
}

const ConsoleImeInfo* RenderData::GetImeData()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.ConsoleIme;
}

const TextBuffer* RenderData::GetImeCompositionStringBuffer(_In_ size_t iIndex)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (iIndex < gci.ConsoleIme.ConvAreaCompStr.size())
    {
        return &gci.ConsoleIme.ConvAreaCompStr[iIndex]->ScreenBuffer->GetTextBuffer();
    }
    else
    {
        return nullptr;
    }
}

[[nodiscard]]
NTSTATUS RenderData::GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                                       _Out_ UINT* const pcRectangles)
{
    return Selection::Instance().GetSelectionRects(prgsrSelection, pcRectangles);
}

const bool RenderData::IsGridLineDrawingAllowed()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // If virtual terminal output is set, grid line drawing is a must. It is always allowed.
    if (IsFlagSet(gci.CurrentScreenBuffer->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        return true;
    }
    else
    {
        // If someone explicitly asked for worldwide line drawing, enable it.
        if (gci.IsGridRenderingAllowedWorldwide())
        {
            return true;
        }
        else
        {
            // Otherwise, for compatibility reasons with legacy applications that used the additional CHAR_INFO bits by accident or for their own purposes,
            // we must enable grid line drawing only in a DBCS output codepage. (Line drawing historically only worked in DBCS codepages.)
            // The only known instance of this is Image for Windows by TeraByte, Inc. (TeryByte Unlimited) which used the bits accidentally and for no purpose
            //   (according to the app developer) in conjunction with the Borland Turbo C cgscrn library.
            return !!IsAvailableEastAsianCodePage(gci.OutputCP);
        }
    }
}
