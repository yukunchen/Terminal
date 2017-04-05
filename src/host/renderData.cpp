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
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->GetBufferViewport();
}

const TEXT_BUFFER_INFO* RenderData::GetTextBuffer()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo;
}

const FontInfo* RenderData::GetFontInfo()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo->GetCurrentFont();
}

const TextAttribute RenderData::GetDefaultBrushColors()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->GetAttributes();
}

const void RenderData::GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors)
{
    *ppColorTable = const_cast<COLORREF*>(ServiceLocator::LocateGlobals()->getConsoleInformation()->GetColorTable());
    *pcColors = ServiceLocator::LocateGlobals()->getConsoleInformation()->GetColorTableSize();
}

const Cursor* RenderData::GetCursor()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo->GetCursor();
}

const ConsoleImeInfo* RenderData::GetImeData()
{
    return &ServiceLocator::LocateGlobals()->getConsoleInformation()->ConsoleIme;
}

const TEXT_BUFFER_INFO* RenderData::GetImeCompositionStringBuffer(_In_ size_t iIndex)
{
    if (iIndex < ServiceLocator::LocateGlobals()->getConsoleInformation()->ConsoleIme.ConvAreaCompStr.size())
    {
        return ServiceLocator::LocateGlobals()->getConsoleInformation()->ConsoleIme.ConvAreaCompStr[iIndex]->ScreenBuffer->TextInfo;
    }
    else
    {
        return nullptr;
    }
}

NTSTATUS RenderData::GetSelectionRects(_Outptr_result_buffer_all_(*pcRectangles) SMALL_RECT** const prgsrSelection,
                                       _Out_ UINT* const pcRectangles)
{
    return Selection::Instance().GetSelectionRects(prgsrSelection, pcRectangles);
}

const bool RenderData::IsGridLineDrawingAllowed()
{
    // If virtual terminal output is set, grid line drawing is a must. It is always allowed.
    if (IsFlagSet(ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        return true;
    }
    else
    {
        // If someone explicitly asked for worldwide line drawing, enable it.
        if (ServiceLocator::LocateGlobals()->getConsoleInformation()->IsGridRenderingAllowedWorldwide())
        {
            return true;
        }
        else
        {
            // Otherwise, for compatibility reasons with legacy applications that used the additional CHAR_INFO bits by accident or for their own purposes,
            // we must enable grid line drawing only in a DBCS output codepage. (Line drawing historically only worked in DBCS codepages.)
            // The only known instance of this is Image for Windows by TeraByte, Inc. (TeryByte Unlimited) which used the bits accidentally and for no purpose
            //   (according to the app developer) in conjunction with the Borland Turbo C cgscrn library.
            return !!IsAvailableEastAsianCodePage(ServiceLocator::LocateGlobals()->getConsoleInformation()->OutputCP);
        }
    }
}
