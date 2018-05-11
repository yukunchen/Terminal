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
// - srRegion - Region to write in screen buffer coordinates.  Region is inclusive
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
    std::vector<TextAttributeRun> runs{ {} };
    runs.front().SetLength(1);
    // write attrs
    for (auto currentAttr : attrs)
    {
        ROW& row = screenInfo.GetTextBuffer().GetRowByOffset(currentLocation.Y);
        ATTR_ROW& attrRow = row.GetAttrRow();
        // clear dbcs bits
        ClearAllFlags(currentAttr, COMMON_LVB_SBCSDBCS);
        // add to attr row
        runs.front().SetAttributesFromLegacy(currentAttr);
        THROW_IF_FAILED(attrRow.InsertAttrRuns(runs,
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
// - This routine fills the screen buffer with the specified character or attribute.
// Arguments:
// - screenInfo - reference to screen buffer information.
// - wElement - Element to write.
// - coordWrite - Screen buffer coordinate to begin writing to.
// - ulElementType
//      CONSOLE_ASCII         - element is an ascii character.
//      CONSOLE_REAL_UNICODE  - element is a real unicode character. These will get converted to False Unicode as required.
//      CONSOLE_FALSE_UNICODE - element is a False Unicode character.
//      CONSOLE_ATTRIBUTE     - element is an attribute.
// - pcElements - On input, the number of elements to write.  On output, the number of elements written.
// Return Value:
[[nodiscard]]
NTSTATUS FillOutput(SCREEN_INFORMATION& screenInfo,
                    _In_ WORD wElement,
                    const COORD coordWrite,
                    const ULONG ulElementType,
                    _Inout_ PULONG pcElements)  // this value is valid even for error cases
{
    DBGOUTPUT(("FillOutput\n"));
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (*pcElements == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumWritten = 0;
    SHORT X = coordWrite.X;
    SHORT Y = coordWrite.Y;
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (X >= coordScreenBufferSize.X || X < 0 || Y >= coordScreenBufferSize.Y || Y < 0)
    {
        *pcElements = 0;
        return STATUS_SUCCESS;
    }

    if (ulElementType == CONSOLE_ASCII)
    {
        UINT const Codepage = gci.OutputCP;
        if (screenInfo.FillOutDbcsLeadChar == 0)
        {
            if (IsDBCSLeadByteConsole((CHAR) wElement, &gci.OutputCPInfo))
            {
                screenInfo.FillOutDbcsLeadChar = (CHAR) wElement;
                *pcElements = 0;
                return STATUS_SUCCESS;
            }
            else
            {
                CHAR CharTmp = (CHAR) wElement;
                ConvertOutputToUnicode(Codepage, &CharTmp, 1, (WCHAR*)&wElement, 1);
            }
        }
        else
        {
            CHAR CharTmp[2];

            CharTmp[0] = screenInfo.FillOutDbcsLeadChar;
            CharTmp[1] = (BYTE) wElement;

            screenInfo.FillOutDbcsLeadChar = 0;
            ConvertOutputToUnicode(Codepage, CharTmp, 2, (WCHAR*)&wElement, 1);
        }
    }

    ROW* pRow = &screenInfo.GetTextBuffer().GetRowByOffset(coordWrite.Y);
    if (ulElementType == CONSOLE_ASCII ||
        ulElementType == CONSOLE_REAL_UNICODE ||
        ulElementType == CONSOLE_FALSE_UNICODE)
    {
        DWORD StartPosFlag = 0;
        for (;;)
        {
            FAIL_FAST_IF(pRow == nullptr);

            // copy the chars into their arrays
            CharRow::iterator it;
            try
            {
                CharRow& charRow = pRow->GetCharRow();
                it = std::next(charRow.begin(), X);
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }
            if ((ULONG) (coordScreenBufferSize.X - X) >= (*pcElements - NumWritten))
            {
                {
                    COORD TPoint;

                    TPoint.X = X;
                    TPoint.Y = Y;
                    CleanupDbcsEdgesForWrite((SHORT)(*pcElements - NumWritten), TPoint, screenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        it->Char() = static_cast<wchar_t>(wElement);
                        if (StartPosFlag++ & 1)
                        {
                            it->DbcsAttr().SetTrailing();
                        }
                        else
                        {
                            it->DbcsAttr().SetLeading();
                        }
                        ++it;
                    }

                    if (StartPosFlag & 1)
                    {
                        (it - 1)->Char() = UNICODE_SPACE;
                        (it - 1)->DbcsAttr().SetSingle();
                    }
                }
                else
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        it->Char() = static_cast<wchar_t>(wElement);
                        it->DbcsAttr().SetSingle();
                        ++it;
                    }
                }
                X = (SHORT)(X + *pcElements - NumWritten - 1);
                NumWritten = *pcElements;
            }
            else
            {
                {
                    COORD TPoint;

                    TPoint.X = X;
                    TPoint.Y = Y;
                    CleanupDbcsEdgesForWrite((SHORT)(coordScreenBufferSize.X - X), TPoint, screenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < coordScreenBufferSize.X - X; j++)
                    {
                        it->Char() = static_cast<wchar_t>(wElement);
                        if (StartPosFlag++ & 1)
                        {
                            it->DbcsAttr().SetTrailing();
                        }
                        else
                        {
                            it->DbcsAttr().SetLeading();
                        }
                        ++it;
                    }
                }
                else
                {
                    for (SHORT j = 0; j < coordScreenBufferSize.X - X; j++)
                    {
                        it->Char() = static_cast<wchar_t>(wElement);
                        it->DbcsAttr().SetSingle();
                        ++it;
                    }
                }
                NumWritten += coordScreenBufferSize.X - X;
                X = (SHORT)(coordScreenBufferSize.X - 1);
            }

            // invalidate row wrap status for any bulk fill of text characters
            pRow->GetCharRow().SetWrapForced(false);

            if (NumWritten < *pcElements)
            {
                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            try
            {
                pRow = &screenInfo.GetTextBuffer().GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }
        }
    }
    else if (ulElementType == CONSOLE_ATTRIBUTE)
    {
        TextAttributeRun AttrRun;
        COORD TPoint;
        TPoint.X = X;
        TPoint.Y = Y;

        for (;;)
        {
            FAIL_FAST_IF(pRow == nullptr);

            // Copy the attrs into the screen buffer arrays.
            if ((ULONG) (coordScreenBufferSize.X - X) >= (*pcElements - NumWritten))
            {
                X = (SHORT)(X + *pcElements - NumWritten - 1);
                NumWritten = *pcElements;
            }
            else
            {
                NumWritten += coordScreenBufferSize.X - X;
                X = (SHORT)(coordScreenBufferSize.X - 1);
            }

            // Recalculate the last non-space char and merge the two
            // attribute strings.
            AttrRun.SetLength((SHORT)((Y == coordWrite.Y) ? (X - coordWrite.X + 1) : (X + 1)));

            // Here we're being a little clever -
            // Because RGB color can't roundtrip the API, certain VT sequences will forget the RGB color
            // because their first call to GetScreenBufferInfo returned a legacy attr.
            // If they're calling this with the default attrs, they likely wanted to use the RGB default attrs.
            // This could create a scenario where someone emitted RGB with VT,
            // THEN used the API to FillConsoleOutput with the default attrs, and DIDN'T want the RGB color
            // they had set.
            if (screenInfo.InVTMode() && screenInfo.GetAttributes().GetLegacyAttributes() == wElement)
            {
                AttrRun.SetAttributes(screenInfo.GetAttributes());
            }
            else
            {
                WORD wActual = wElement;
                ClearAllFlags(wActual, COMMON_LVB_SBCSDBCS);
                AttrRun.SetAttributesFromLegacy(wActual);
            }

            LOG_IF_FAILED(pRow->GetAttrRow().InsertAttrRuns({ AttrRun },
                                                            (SHORT)(X - AttrRun.GetLength() + 1),
                                                            X,
                                                            coordScreenBufferSize.X));

            // leave row wrap status alone for any attribute fills
            if (NumWritten < *pcElements)
            {
                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            try
            {
                pRow = &screenInfo.GetTextBuffer().GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }
        }

        screenInfo.NotifyAccessibilityEventing(coordWrite.X, coordWrite.Y, X, Y);
    }
    else
    {
        *pcElements = 0;
        return STATUS_INVALID_PARAMETER;
    }

    // determine write region.  if we're still on the same line we started
    // on, left X is the X we started with and right X is the one we're on
    // now.  otherwise, left X is 0 and right X is the rightmost column of
    // the screen buffer.
    //
    // then update the screen.
    SMALL_RECT WriteRegion;
    if (screenInfo.ConvScreenInfo)
    {
        WriteRegion.Top = coordWrite.Y + screenInfo.GetBufferViewport().Left + screenInfo.ConvScreenInfo->CaInfo.coordConView.Y;
        WriteRegion.Bottom = Y + screenInfo.GetBufferViewport().Left + screenInfo.ConvScreenInfo->CaInfo.coordConView.Y;
        if (Y != coordWrite.Y)
        {
            WriteRegion.Left = 0;
            WriteRegion.Right = (SHORT)(gci.GetActiveOutputBuffer().GetScreenBufferSize().X - 1);
        }
        else
        {
            WriteRegion.Left = coordWrite.X + screenInfo.GetBufferViewport().Top + screenInfo.ConvScreenInfo->CaInfo.coordConView.X;
            WriteRegion.Right = X + screenInfo.GetBufferViewport().Top + screenInfo.ConvScreenInfo->CaInfo.coordConView.X;
        }
        WriteConvRegionToScreen(gci.GetActiveOutputBuffer(), WriteRegion);
        *pcElements = NumWritten;
        return STATUS_SUCCESS;
    }

    WriteRegion.Top = coordWrite.Y;
    WriteRegion.Bottom = Y;
    if (Y != coordWrite.Y)
    {
        WriteRegion.Left = 0;
        WriteRegion.Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    else
    {
        WriteRegion.Left = coordWrite.X;
        WriteRegion.Right = X;
    }

    WriteToScreen(screenInfo, WriteRegion);
    *pcElements = NumWritten;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine fills a rectangular region in the screen buffer.
// Arguments:
// - screenInfo - reference to screen info
// - cell - cell to fill rectangle with
// - rect - rectangle to fill
void FillRectangle(SCREEN_INFORMATION& screenInfo,
                   const OutputCell& cell,
                   const SMALL_RECT rect)
{
    const size_t width = rect.Right - rect.Left + 1;
    const size_t height = rect.Bottom + rect.Top + 1;

    // generate line to write
    std::vector<OutputCell> rowCells(width, cell);
    if (IsGlyphFullWidth(cell.Chars()))
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
        const size_t rowIndex = rect.Top + i;

        // cleanup dbcs edges
        const COORD leftPoint{ rect.Right, gsl::narrow<SHORT>(rowIndex) };
        CleanupDbcsEdgesForWrite(rowCells.size(), leftPoint, screenInfo);

        // write cells
        screenInfo.WriteLine(rowCells, rowIndex, rect.Left);

        // force set wrap to false because this is a rectangular operation
        screenInfo.GetTextBuffer().GetRowByOffset(rowIndex).GetCharRow().SetWrapForced(false);
    }
    // notify accessibility listeners that something has changed
    screenInfo.NotifyAccessibilityEventing(rect.Left, rect.Top, rect.Right, rect.Bottom);
}
