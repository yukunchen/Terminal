/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "renderData.hpp"

#include "dbcs.h"
#include "output.h"

#pragma hdrstop

RenderData::RenderData()
{

}

RenderData::~RenderData()
{

}

const SMALL_RECT* RenderData::GetViewport()
{
    return &g_ciConsoleInformation.CurrentScreenBuffer->BufferViewport;
}

const TEXT_BUFFER_INFO* RenderData::GetTextBuffer()
{
    return g_ciConsoleInformation.CurrentScreenBuffer->TextInfo;
}

const FontInfo* RenderData::GetFontInfo()
{
    return g_ciConsoleInformation.CurrentScreenBuffer->TextInfo->GetCurrentFont();
}

const WORD RenderData::GetDefaultBrushColors()
{
    return g_ciConsoleInformation.CurrentScreenBuffer->GetAttributes();
}

const void RenderData::GetColorTable(_Outptr_result_buffer_all_(*pcColors) COLORREF** const ppColorTable, _Out_ size_t* const pcColors)
{
    *ppColorTable = const_cast<COLORREF*>(g_ciConsoleInformation.GetColorTable());
    *pcColors = g_ciConsoleInformation.GetColorTableSize();
}

const Cursor* RenderData::GetCursor() 
{
    return g_ciConsoleInformation.CurrentScreenBuffer->TextInfo->GetCursor();
}

const CONSOLE_IME_INFORMATION* RenderData::GetImeData() 
{
    return &g_ciConsoleInformation.ConsoleIme;
}

const TEXT_BUFFER_INFO* RenderData::GetImeCompositionStringBuffer(_In_ size_t iIndex)
{
    if (iIndex < g_ciConsoleInformation.ConsoleIme.NumberOfConvAreaCompStr)
    {
        return g_ciConsoleInformation.ConsoleIme.ConvAreaCompStr[iIndex]->ScreenBuffer->TextInfo;
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
    if (IsFlagSet(g_ciConsoleInformation.CurrentScreenBuffer->OutputMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        return true;
    }
    else
    {
        // If someone explicitly asked for worldwide line drawing, enable it.
        if (g_ciConsoleInformation.IsGridRenderingAllowedWorldwide())
        {
            return true;
        }
        else
        {
            // Otherwise, for compatibility reasons with legacy applications that used the additional CHAR_INFO bits by accident or for their own purposes,
            // we must enable grid line drawing only in a DBCS output codepage. (Line drawing historically only worked in DBCS codepages.)
            // The only known instance of this is Image for Windows by TeraByte, Inc. (TeryByte Unlimited) which used the bits accidentally and for no purpose
            //   (according to the app developer) in conjunction with the Borland Turbo C cgscrn library.
            return !!IsAvailableEastAsianCodePage(g_ciConsoleInformation.OutputCP);
        }
    }
}
