/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"

#include "dbcs.h"
#include "misc.h"

#include "../buffer/out/CharRow.hpp"

#include "../interactivity/inc/ServiceLocator.hpp"
#include "../types/inc/Viewport.hpp"
#include "../types/inc/convert.hpp"
#include "../types/inc/Utf16Parser.hpp"

#include <algorithm>
#include <iterator>

#pragma hdrstop

using namespace Microsoft::Console::Types;


void StreamWriteToScreenBuffer(SCREEN_INFORMATION& screenInfo,
                               const std::wstring& wstr,
                               const bool fWasLineWrapped)
{
    COORD const TargetPoint = screenInfo.GetTextBuffer().GetCursor().GetPosition();
    ROW& Row = screenInfo.GetTextBuffer().GetRowByOffset(TargetPoint.Y);
    DBGOUTPUT(("&Row = 0x%p, TargetPoint = (0x%x,0x%x)\n", &Row, TargetPoint.X, TargetPoint.Y));

    CleanupDbcsEdgesForWrite(wstr.size(), TargetPoint, screenInfo);
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();

    try
    {
        const TextAttribute defaultTextAttribute = screenInfo.GetAttributes();
        const auto formattedCharData = Utf16Parser::Parse(wstr);
        const std::vector<OutputCell> cells = OutputCell::FromUtf16(formattedCharData, defaultTextAttribute);

        screenInfo.WriteLine(cells, TargetPoint.Y, TargetPoint.X);
    }
    CATCH_LOG();

    // caller knows the wrap status as this func is called only for drawing one line at a time
    Row.GetCharRow().SetWrapForced(fWasLineWrapped);

    screenInfo.NotifyAccessibilityEventing(TargetPoint.X,
                                           TargetPoint.Y,
                                           TargetPoint.X + gsl::narrow<short>(wstr.size()) - 1,
                                           TargetPoint.Y);
}

// Routine Description:
// - This routine copies a rectangular region to the screen buffer. no clipping is done.
// Arguments:
// - screenInfo - reference to screen buffer
// - cells - cells to copy from
// - coordDest - the coordinate position to overwrite data at
// Return Value:
// - <none>
// Note:
// - will throw exception on failure
void WriteRectToScreenBuffer(SCREEN_INFORMATION& screenInfo,
                             const std::vector<std::vector<OutputCell>>& cells,
                             const COORD coordDest)
{
    DBGOUTPUT(("WriteRectToScreenBuffer\n"));
    // don't do anything if we're not actually writing anything
    if (cells.empty())
    {
        return;
    }
    const size_t xSize = cells.at(0).size();
    const size_t ySize = cells.size();
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();

    // copy data to output buffer
    for (size_t iRow = 0; iRow < ySize; ++iRow)
    {
        ROW& row = screenInfo.GetTextBuffer().GetRowByOffset(coordDest.Y + static_cast<UINT>(iRow));
        // clear wrap status for rectangle drawing
        row.GetCharRow().SetWrapForced(false);

        std::vector<TextAttribute> textAttrs;

        // fix up any leading trailing bytes at edges
        COORD point;
        point.X = coordDest.X;
        point.Y = coordDest.Y + static_cast<short>(iRow);
        CleanupDbcsEdgesForWrite(xSize, point, screenInfo);

        screenInfo.WriteLine(cells.at(iRow), coordDest.Y + iRow, coordDest.X);
    }
}

void WriteRegionToScreen(SCREEN_INFORMATION& screenInfo, _In_ PSMALL_RECT psrRegion)
{
    if (screenInfo.IsActiveScreenBuffer())
    {
        if (ServiceLocator::LocateGlobals().pRender != nullptr)
        {
            // convert inclusive rectangle to exclusive rectangle
            SMALL_RECT srExclusive = Viewport::FromInclusive(*psrRegion).ToExclusive();

            ServiceLocator::LocateGlobals().pRender->TriggerRedraw(&srExclusive);
        }
    }
}

// Routine Description:
// - This routine writes a screen buffer region to the screen.
// Arguments:
// - screenInfo - reference to screen buffer information.
// - srRegion - Region to write in screen buffer coordinates. Region is inclusive
// Return Value:
// - <none>
void WriteToScreen(SCREEN_INFORMATION& screenInfo, const SMALL_RECT srRegion)
{
    DBGOUTPUT(("WriteToScreen\n"));
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // update to screen, if we're not iconic.
    if (!screenInfo.IsActiveScreenBuffer() || IsFlagSet(gci.Flags, CONSOLE_IS_ICONIC))
    {
        return;
    }

    // clip region
    SMALL_RECT ClippedRegion;
    {
        const SMALL_RECT currentViewport = screenInfo.GetBufferViewport();
        ClippedRegion.Left = std::max(srRegion.Left, currentViewport.Left);
        ClippedRegion.Top = std::max(srRegion.Top, currentViewport.Top);
        ClippedRegion.Right = std::min(srRegion.Right, currentViewport.Right);
        ClippedRegion.Bottom = std::min(srRegion.Bottom, currentViewport.Bottom);
        if (ClippedRegion.Right < ClippedRegion.Left || ClippedRegion.Bottom < ClippedRegion.Top)
        {
            return;
        }
    }

    WriteRegionToScreen(screenInfo, &ClippedRegion);

    WriteConvRegionToScreen(screenInfo, srRegion);
}

// Routine Description:
// - writes text attributes to the screen
// Arguments:
// - screenInfo - the screen info to write to
// - attrs - the attrs to write to the screen
// - target - the starting coordinate in the screen
// Return Value:
// - number of elements written
size_t WriteOutputAttributes(SCREEN_INFORMATION& screenInfo,
                             const std::vector<WORD>& attrs,
                             const COORD target)
{
    if (attrs.empty())
    {
        return 0;
    }

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (!IsCoordInBounds(target, coordScreenBufferSize))
    {
        return 0;
    }

    size_t elementsWritten = 0;
    COORD currentLocation = target;
    TextAttributeRun run;
    run.SetLength(1);
    // write attrs
    for (auto currentAttr : attrs)
    {
        ROW& row = screenInfo.GetTextBuffer().GetRowByOffset(currentLocation.Y);
        ATTR_ROW& attrRow = row.GetAttrRow();
        // clear dbcs bits
        ClearAllFlags(currentAttr, COMMON_LVB_SBCSDBCS);
        // add to attr row
        run.SetAttributesFromLegacy(currentAttr);
        THROW_IF_FAILED(attrRow.InsertAttrRuns({ &run, 1 },
                                               currentLocation.X,
                                               currentLocation.X,
                                               row.size()));
        ++elementsWritten;
        ++currentLocation.X;
        // move to next location
        if (currentLocation.X == coordScreenBufferSize.X)
        {
            currentLocation.X = 0;
            currentLocation.Y++;
        }
        if (!IsCoordInBounds(currentLocation, coordScreenBufferSize))
        {
            break;
        }
    }
    // tell screen to update screen portion
    SMALL_RECT writeRegion;
    writeRegion.Top = target.Y;
    writeRegion.Bottom = std::min(currentLocation.Y, coordScreenBufferSize.Y);
    writeRegion.Left = 0;
    writeRegion.Right = coordScreenBufferSize.X;
    WriteToScreen(screenInfo, writeRegion);

    return elementsWritten;
}

// Routine Description:
// - writes text to the screen
// Arguments:
// - screenInfo - the screen info to write to
// - chars - the text to write to the screen
// - target - the starting coordinate in the screen
// Return Value:
// - number of elements written
size_t WriteOutputStringW(SCREEN_INFORMATION& screenInfo,
                          const std::vector<wchar_t>& chars,
                          const COORD target)
{
    if (chars.empty())
    {
        return 0;
    }

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (!IsCoordInBounds(target, coordScreenBufferSize))
    {
        return 0;
    }

    // convert to utf16 output cells and write
    const std::wstring wstr{ chars.begin(), chars.end() };
    const auto formattedCharData = Utf16Parser::Parse(wstr);
    std::vector<OutputCell> cells = OutputCell::FromUtf16(formattedCharData);
    const auto cellsWritten = screenInfo.WriteLineNoWrap(cells, target.Y, target.X);

    // tell screen to update screen portion
    SMALL_RECT writeRegion;
    writeRegion.Top = target.Y;
    writeRegion.Bottom = target.Y + (gsl::narrow<SHORT>(chars.size()) / coordScreenBufferSize.X) + 1;
    writeRegion.Left = 0;
    writeRegion.Right = coordScreenBufferSize.X;
    WriteToScreen(screenInfo, writeRegion);

    // count the number of glyphs that were written
    return std::count_if(cells.begin(),
                         cells.begin() + cellsWritten,
                         [](auto&& cell) { return !cell.DbcsAttr().IsTrailing(); });
}

// Routine Description:
// - writes text to the screen
// Arguments:
// - screenInfo - the screen info to write to
// - chars - the text to write to the screen
// - target - the starting coordinate in the screen
// Return Value:
// - number of elements written
size_t WriteOutputStringA(SCREEN_INFORMATION& screenInfo,
                          const std::vector<char>& chars,
                          const COORD target)
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    try
    {
        // convert to wide chars so we can call the W version of this function
        wistd::unique_ptr<wchar_t[]> outWchs;
        size_t count = 0;
        THROW_IF_FAILED(ConvertToW(gci.OutputCP,
                                   chars.data(),
                                   chars.size(),
                                   outWchs,
                                   count));

        const std::vector<wchar_t> wchs{ outWchs.get(), outWchs.get() + count };
        const auto wideCharsWritten = WriteOutputStringW(screenInfo, wchs, target);

        // convert wideCharsWritten to amount of ascii chars written so we can properly report back
        // how many elements were actually written
        wistd::unique_ptr<char[]> outChars;
        THROW_IF_FAILED(ConvertToA(gci.OutputCP,
                                   wchs.data(),
                                   wideCharsWritten,
                                   outChars,
                                   count));
        return count;
    }
    CATCH_LOG();
    return 0;
}

// Routine Description:
// - fills the screen buffer with the specified text attribute
// Arguments:
// - screenInfo - reference to screen buffer information.
// - attr - the text attribute to use to fill
// - target - Screen buffer coordinate to begin writing to.
// - amountToWrite - the number of elements to write
// Return Value:
// - the number of elements written
size_t FillOutputAttributes(SCREEN_INFORMATION& screenInfo,
                            const TextAttribute attr,
                            const COORD target,
                            const size_t amountToWrite)
{
    if (amountToWrite == 0)
    {
        return 0;
    }

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (!IsCoordInBounds(target, coordScreenBufferSize))
    {
        return 0;
    }

    return screenInfo.FillTextAttribute(attr, target, amountToWrite);
}

// Routine Description:
// - fills the screen buffer with the specified wchar
// Arguments:
// - screenInfo - reference to screen buffer information.
// - wch - wchar to fill with
// - target - Screen buffer coordinate to begin writing to.
// - amountToWrite - the number of elements to write
// Return Value:
// - the number of elements written
size_t FillOutputW(SCREEN_INFORMATION& screenInfo,
                   const wchar_t wch,
                   const COORD target,
                   const size_t amountToWrite)
{
    if (amountToWrite == 0)
    {
        return 0;
    }

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (!IsCoordInBounds(target, coordScreenBufferSize))
    {
        return 0;
    }

    return screenInfo.FillTextGlyph({ &wch, 1 }, target, amountToWrite);
}

// Routine Description:
// - fills the screen buffer with the specified char
// Arguments:
// - screenInfo - reference to screen buffer information.
// - ch - ascii char to fill
// - target - Screen buffer coordinate to begin writing to.
// - amountToWrite - the number of elements to write
// Return Value:
// - the number of elements written
size_t FillOutputA(SCREEN_INFORMATION& screenInfo,
                   const char ch,
                   const COORD target,
                   const size_t amountToWrite)
{
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    // convert to wide chars and call W version
    wistd::unique_ptr<wchar_t[]> outWchs;
    size_t count = 0;
    THROW_IF_FAILED(ConvertToW(gci.OutputCP,
                               &ch,
                               1,
                               outWchs,
                               count));

    THROW_HR_IF(E_UNEXPECTED, count > 1);
    return FillOutputW(screenInfo, outWchs[0], target, amountToWrite);
}

// Routine Description:
// - This routine fills a rectangular region in the screen buffer.
// Arguments:
// - screenInfo - reference to screen info
// - cell - cell to fill rectangle with
// - rect - rectangle to fill
void FillRectangle(SCREEN_INFORMATION& screenInfo,
                   const OutputCell& cell,
                   const Viewport rect)
{
    const size_t width = rect.Width();
    const size_t height = rect.Height();

    // generate line to write
    std::vector<OutputCell> rowCells(width, cell);
    if (IsGlyphFullWidth(std::wstring_view{ cell.Chars().data(), cell.Chars().size() }))
    {
        // set leading and trailing attrs
        for (size_t i = 0; i < rowCells.size(); ++i)
        {
            if (i % 2 == 0)
            {
                rowCells.at(i).DbcsAttr().SetLeading();
            }
            else
            {
                rowCells.at(i).DbcsAttr().SetTrailing();
            }
        }
        // make sure not to write a partial dbcs sequence
        if (rowCells.back().DbcsAttr().IsLeading())
        {
            rowCells.back().Chars() = { UNICODE_SPACE };
            rowCells.back().DbcsAttr().SetSingle();
        }
    }

    // fill rectangle
    for (size_t i = 0; i < height; ++i)
    {
        const size_t rowIndex = rect.Top() + i;

        // cleanup dbcs edges
        const COORD leftPoint{ rect.Left(), gsl::narrow<SHORT>(rowIndex) };
        CleanupDbcsEdgesForWrite(rowCells.size(), leftPoint, screenInfo);

        // write cells
        screenInfo.WriteLine(rowCells, rowIndex, rect.Left());

        // force set wrap to false because this is a rectangular operation
        screenInfo.GetTextBuffer().GetRowByOffset(rowIndex).GetCharRow().SetWrapForced(false);
    }
    // notify accessibility listeners that something has changed
    screenInfo.NotifyAccessibilityEventing(rect.Left(), rect.Top(), rect.RightInclusive(), rect.BottomInclusive());
}
