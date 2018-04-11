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
    return gci.GetActiveOutputBuffer().GetBufferViewport();
}

const TextBuffer& RenderData::GetTextBuffer()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetActiveOutputBuffer().GetTextBuffer();
}

const FontInfo* RenderData::GetFontInfo()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.GetActiveOutputBuffer().GetTextBuffer().GetCurrentFont();
}

const TextAttribute RenderData::GetDefaultBrushColors()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetActiveOutputBuffer().GetAttributes();
}

const void RenderData::GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    *ppColorTable = const_cast<COLORREF*>(gci.GetColorTable());
    *pcColors = gci.GetColorTableSize();
}

const Cursor& RenderData::GetCursor()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetActiveOutputBuffer().GetTextBuffer().GetCursor();
}

const ConsoleImeInfo* RenderData::GetImeData()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return &gci.ConsoleIme;
}

const TextBuffer& RenderData::GetImeCompositionStringBuffer(_In_ size_t iIndex)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    THROW_HR_IF(E_INVALIDARG, iIndex >= gci.ConsoleIme.ConvAreaCompStr.size());
    return gci.ConsoleIme.ConvAreaCompStr[iIndex]->ScreenBuffer->GetTextBuffer();
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
    if (IsFlagSet(gci.GetActiveOutputBuffer().OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
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

const std::wstring RenderData::GetConsoleTitle() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetTitleAndPrefix();
}
