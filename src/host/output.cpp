/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"
#include "output.h"

#include "getset.h"
#include "misc.h"
#include "Ucs2CharRow.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"
#include "..\types\inc\Viewport.hpp"

#pragma hdrstop
using namespace Microsoft::Console::Types;

// This routine figures out what parameters to pass to CreateScreenBuffer based on the data from STARTUPINFO and the
// registry defaults, and then calls CreateScreenBuffer.
[[nodiscard]]
NTSTATUS DoCreateScreenBuffer()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    CHAR_INFO Fill;
    Fill.Attributes = gci.GetFillAttribute();
    Fill.Char.UnicodeChar = UNICODE_SPACE;

    CHAR_INFO PopupFill;
    PopupFill.Attributes = gci.GetPopupFillAttribute();
    PopupFill.Char.UnicodeChar = UNICODE_SPACE;

    FontInfo fiFont(gci.GetFaceName(),
                    static_cast<BYTE>(gci.GetFontFamily()),
                    gci.GetFontWeight(),
                    gci.GetFontSize(),
                    gci.GetCodePage());

    // For East Asian version, we want to get the code page from the registry or shell32, so we can specify console
    // codepage by console.cpl or shell32. The default codepage is OEMCP.
    gci.CP = gci.GetCodePage();
    gci.OutputCP = gci.GetCodePage();

    gci.Flags |= CONSOLE_USE_PRIVATE_FLAGS;

    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(gci.GetWindowSize(),
                                                         fiFont,
                                                         gci.GetScreenBufferSize(),
                                                         Fill,
                                                         PopupFill,
                                                         gci.GetCursorSize(),
                                                         &gci.ScreenBuffers);

    // TODO: MSFT 9355013: This needs to be resolved. We increment it once with no handle to ensure it's never cleaned up
    // and one always exists for the renderer (and potentially other functions.)
    // It's currently a load-bearing piece of code. http://osgvsowi/9355013
    if (NT_SUCCESS(Status))
    {
        gci.ScreenBuffers[0].Header.IncrementOriginalScreenBuffer();
    }

    return Status;
}

// Routine Description:
// - This routine copies a rectangular region from the screen buffer. no clipping is done.
// Arguments:
// - screenInfo - reference to screen info
// - coordSourcePoint - upper left coordinates of source rectangle
// - TargetRect - rectangle in source buffer to copy
// Return Value:
// - vector of vector of output cell data for read rect
// Note:
// - will throw exception on error.
std::vector<std::vector<OutputCell>> ReadRectFromScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                                                              const COORD coordSourcePoint,
                                                              const Viewport viewport)
{
    std::vector<std::vector<OutputCell>> result;
    result.reserve(viewport.Height());

    const int ScreenBufferWidth = screenInfo.GetScreenBufferSize().X;
    std::unique_ptr<TextAttribute[]> unpackedAttrs = std::make_unique<TextAttribute[]>(ScreenBufferWidth);
    THROW_IF_NULL_ALLOC(unpackedAttrs.get());

    for (size_t rowIndex = 0; rowIndex < static_cast<size_t>(viewport.Height()); ++rowIndex)
    {
        auto cells = screenInfo.ReadLine(coordSourcePoint.Y + rowIndex, coordSourcePoint.X);
        ASSERT_FRE(cells.size() >= static_cast<size_t>(viewport.Width()));
        for (size_t colIndex = 0; colIndex < static_cast<size_t>(viewport.Width()); ++colIndex)
        {
            // if we're clipping a dbcs char then don't include it, add a space instead
            if ((colIndex == 0 && cells[colIndex].GetDbcsAttribute().IsTrailing()) ||
                (colIndex + 1 >= static_cast<size_t>(viewport.Width()) && cells[colIndex].GetDbcsAttribute().IsLeading()))
            {
                cells[colIndex].GetDbcsAttribute().SetSingle();
                cells[colIndex].GetCharData() = UNICODE_SPACE;
            }
        }
        cells.resize(viewport.Width(), cells.front());
        result.push_back(cells);
    }
    ASSERT_FRE(result.size() == static_cast<size_t>(viewport.Height()));
    ASSERT_FRE(result.at(0).size() == static_cast<size_t>(viewport.Width()));
    return result;
}

// Routine Description:
// - This routine copies a rectangular region from the screen buffer to the screen buffer.  no clipping is done.
// Arguments:
// - screenInfo - reference to screen info
// - psrSource - rectangle in source buffer to copy
// - coordTarget - upper left coordinates of new location rectangle
// Return Value:
// - status of copy
[[nodiscard]]
NTSTATUS _CopyRectangle(SCREEN_INFORMATION& screenInfo,
                        const SMALL_RECT * const psrSource,
                        const COORD coordTarget)
{
    DBGOUTPUT(("_CopyRectangle\n"));

    COORD SourcePoint;
    SourcePoint.X = psrSource->Left;
    SourcePoint.Y = psrSource->Top;

    SMALL_RECT Target;
    Target.Left = 0;
    Target.Top = 0;
    Target.Right = psrSource->Right - psrSource->Left;
    Target.Bottom = psrSource->Bottom - psrSource->Top;

    COORD Size;
    Size.X = Target.Right;
    Size.Y = Target.Bottom;
    Size.X++;
    Size.Y++;

    try
    {
        std::vector<std::vector<OutputCell>> cells = ReadRectFromScreenBuffer(screenInfo,
                                                                              SourcePoint,
                                                                              Viewport::FromInclusive(Target));
        WriteRectToScreenBuffer(screenInfo, cells, coordTarget);
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine reads a rectangular region from the screen buffer. The region is first clipped.
// Arguments:
// - ScreenInformation - Screen buffer to read from.
// - outputCells - an empty container to store cell data on output
// - ReadRegion - Region to read.
// Return Value:
[[nodiscard]]
NTSTATUS ReadScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                          _Inout_ std::vector<std::vector<OutputCell>>& outputCells,
                          _Inout_ PSMALL_RECT psrReadRegion)
{
    DBGOUTPUT(("ReadScreenBuffer\n"));
    assert(outputCells.empty());

    // calculate dimensions of caller's buffer.  have to do this calculation before clipping.
    COORD TargetSize;
    TargetSize.X = (SHORT)(psrReadRegion->Right - psrReadRegion->Left + 1);
    TargetSize.Y = (SHORT)(psrReadRegion->Bottom - psrReadRegion->Top + 1);

    if (TargetSize.X <= 0 || TargetSize.Y <= 0)
    {
        psrReadRegion->Right = psrReadRegion->Left - 1;
        psrReadRegion->Bottom = psrReadRegion->Top - 1;
        return STATUS_SUCCESS;
    }

    // do clipping.
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (psrReadRegion->Right > (SHORT)(coordScreenBufferSize.X - 1))
    {
        psrReadRegion->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    if (psrReadRegion->Bottom > (SHORT)(coordScreenBufferSize.Y - 1))
    {
        psrReadRegion->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }

    COORD TargetPoint;
    if (psrReadRegion->Left < 0)
    {
        TargetPoint.X = -psrReadRegion->Left;
        psrReadRegion->Left = 0;
    }
    else
    {
        TargetPoint.X = 0;
    }

    if (psrReadRegion->Top < 0)
    {
        TargetPoint.Y = -psrReadRegion->Top;
        psrReadRegion->Top = 0;
    }
    else
    {
        TargetPoint.Y = 0;
    }

    COORD SourcePoint;
    SourcePoint.X = psrReadRegion->Left;
    SourcePoint.Y = psrReadRegion->Top;

    SMALL_RECT Target;
    Target.Left = TargetPoint.X;
    Target.Top = TargetPoint.Y;
    Target.Right = TargetPoint.X + (psrReadRegion->Right - psrReadRegion->Left);
    Target.Bottom = TargetPoint.Y + (psrReadRegion->Bottom - psrReadRegion->Top);

    try
    {
        outputCells = ReadRectFromScreenBuffer(screenInfo, SourcePoint, Viewport::FromInclusive(Target));
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine write a rectangular region to the screen buffer. The region is first clipped.
// - The region should contain Unicode or UnicodeOem chars.
// Arguments:
// - ScreenInformation - Screen buffer to write to.
// - Buffer - Buffer to write from.
// - ReadRegion - Region to write.
// Return Value:
[[nodiscard]]
NTSTATUS WriteScreenBuffer(SCREEN_INFORMATION& screenInfo, _In_ PCHAR_INFO pciBuffer, _Inout_ PSMALL_RECT psrWriteRegion)
{
    DBGOUTPUT(("WriteScreenBuffer\n"));

    // Calculate dimensions of caller's buffer; this calculation must be done before clipping.
    COORD SourceSize;
    SourceSize.X = (SHORT)(psrWriteRegion->Right - psrWriteRegion->Left + 1);
    SourceSize.Y = (SHORT)(psrWriteRegion->Bottom - psrWriteRegion->Top + 1);

    if (SourceSize.X <= 0 || SourceSize.Y <= 0)
    {
        return STATUS_SUCCESS;
    }

    // Ensure that the write region is within the constraints of the screen buffer.
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (psrWriteRegion->Left >= coordScreenBufferSize.X || psrWriteRegion->Top >= coordScreenBufferSize.Y)
    {
        return STATUS_SUCCESS;
    }

    SMALL_RECT SourceRect;
    // Do clipping.
    if (psrWriteRegion->Right > (SHORT)(coordScreenBufferSize.X - 1))
    {
        psrWriteRegion->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    SourceRect.Right = psrWriteRegion->Right - psrWriteRegion->Left;

    if (psrWriteRegion->Bottom > (SHORT)(coordScreenBufferSize.Y - 1))
    {
        psrWriteRegion->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }
    SourceRect.Bottom = psrWriteRegion->Bottom - psrWriteRegion->Top;

    if (psrWriteRegion->Left < 0)
    {
        SourceRect.Left = -psrWriteRegion->Left;
        psrWriteRegion->Left = 0;
    }
    else
    {
        SourceRect.Left = 0;
    }

    if (psrWriteRegion->Top < 0)
    {
        SourceRect.Top = -psrWriteRegion->Top;
        psrWriteRegion->Top = 0;
    }
    else
    {
        SourceRect.Top = 0;
    }

    if (SourceRect.Left > SourceRect.Right || SourceRect.Top > SourceRect.Bottom)
    {
        return STATUS_INVALID_PARAMETER;
    }

    COORD TargetPoint;
    TargetPoint.X = psrWriteRegion->Left;
    TargetPoint.Y = psrWriteRegion->Top;

    return WriteRectToScreenBuffer((PBYTE) pciBuffer, SourceSize, &SourceRect, screenInfo, TargetPoint, nullptr);
}

// Routine Description:
// - This routine reads a string of characters or attributes from the screen buffer.
// Arguments:
// - ScreenInfo - Pointer to screen buffer information.
// - Buffer - Buffer to read into.
// - ReadCoord - Screen buffer coordinate to begin reading from.
// - StringType
//      CONSOLE_ASCII         - read a string of ASCII characters.
//      CONSOLE_REAL_UNICODE  - read a string of Real Unicode characters.
//      CONSOLE_FALSE_UNICODE - read a string of False Unicode characters.
//      CONSOLE_ATTRIBUTE     - read a string of attributes.
// - NumRecords - On input, the size of the buffer in elements.  On output, the number of elements read.
// Return Value:
[[nodiscard]]
NTSTATUS ReadOutputString(const SCREEN_INFORMATION& screenInfo,
                          _Inout_ PVOID pvBuffer,
                          const COORD coordRead,
                          const ULONG ulStringType,
                          _Inout_ PULONG pcRecords)
{
    DBGOUTPUT(("ReadOutputString\n"));
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    if (*pcRecords == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumRead = 0;
    SHORT X = coordRead.X;
    SHORT Y = coordRead.Y;
    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (X >= coordScreenBufferSize.X || X < 0 || Y >= coordScreenBufferSize.Y || Y < 0)
    {
        *pcRecords = 0;
        return STATUS_SUCCESS;
    }

    PWCHAR TransBuffer = nullptr;
    PWCHAR BufPtr;
    if (ulStringType == CONSOLE_ASCII)
    {
        TransBuffer = new WCHAR[*pcRecords];
        if (TransBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }
        BufPtr = TransBuffer;
    }
    else
    {
        BufPtr = (PWCHAR)pvBuffer;
    }

    DbcsAttribute* TransBufferA = nullptr;
    DbcsAttribute* BufPtrA = nullptr;

    {
        TransBufferA = new DbcsAttribute[*pcRecords];
        if (TransBufferA == nullptr)
        {
            if (TransBuffer != nullptr)
            {
                delete[] TransBuffer;
            }

            return STATUS_NO_MEMORY;
        }

        BufPtrA = TransBufferA;
    }

    {
        const ROW* pRow = &screenInfo.GetTextBuffer().GetRowByOffset(coordRead.Y);
        SHORT j, k;

        if (ulStringType == CONSOLE_ASCII ||
            ulStringType == CONSOLE_REAL_UNICODE ||
            ulStringType == CONSOLE_FALSE_UNICODE)
        {
            while (NumRead < *pcRecords)
            {
                // copy the chars from its array
                try
                {
                    const ICharRow& iCharRow = pRow->GetCharRow();
                    // we only support ucs2 encoded char rows
                    FAIL_FAST_IF_MSG(iCharRow.GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                                    "only support UCS2 char rows currently");

                    const Ucs2CharRow& charRow = static_cast<const Ucs2CharRow&>(iCharRow);
                    const Ucs2CharRow::const_iterator startIt = std::next(charRow.cbegin(), X);
                    size_t copyAmount = *pcRecords - NumRead;
                    wchar_t* pChars = BufPtr;
                    DbcsAttribute* pAttrs = BufPtrA;
                    if (static_cast<size_t>(coordScreenBufferSize.X - X) > copyAmount)
                    {
                        std::for_each(startIt, std::next(startIt, copyAmount), [&](const auto& vals)
                        {
                            *pChars = vals.first;
                            ++pChars;
                            *pAttrs = vals.second;
                            ++pAttrs;
                        });

                        NumRead += static_cast<ULONG>(copyAmount);
                        break;
                    }
                    else
                    {
                        copyAmount = coordScreenBufferSize.X - X;

                        std::for_each(startIt, std::next(startIt, copyAmount), [&](const auto& vals)
                        {
                            *pChars = vals.first;
                            ++pChars;
                            *pAttrs = vals.second;
                            ++pAttrs;
                        });

                        BufPtr += copyAmount;
                        BufPtrA += copyAmount;

                        NumRead += static_cast<ULONG>(copyAmount);
                        pRow = &screenInfo.GetTextBuffer().GetNextRowNoWrap(*pRow);
                    }
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }

            if (NumRead)
            {
                PWCHAR Char;
                if (ulStringType == CONSOLE_ASCII)
                {
                    Char = BufPtr = TransBuffer;
                }
                else
                {
                    Char = BufPtr = (PWCHAR)pvBuffer;
                }

                BufPtrA = TransBufferA;
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "This code is fine")
                if (BufPtrA->IsTrailing())
                {
                    j = k = (SHORT)(NumRead - 1);
                    BufPtr++;
                    *Char++ = UNICODE_SPACE;
                    BufPtrA++;
                    NumRead = 1;
                }
                else
                {
                    j = k = (SHORT)NumRead;
                    NumRead = 0;
                }

                while (j--)
                {
                    if (!BufPtrA->IsTrailing())
                    {
                        *Char++ = *BufPtr;
                        NumRead++;
                    }
                    BufPtr++;
                    BufPtrA++;
                }

                if (k && (BufPtrA - 1)->IsLeading())
                {
                    *(Char - 1) = UNICODE_SPACE;
                }
            }
        }
        else if (ulStringType == CONSOLE_ATTRIBUTE)
        {
            size_t CountOfAttr = 0;
            TextAttributeRun* pAttrRun;
            PWORD TargetPtr = (PWORD)BufPtr;

            while (NumRead < *pcRecords)
            {
                // Copy the attrs from its array.
                Ucs2CharRow::const_iterator it;
                Ucs2CharRow::const_iterator itEnd;
                try
                {
                    const ICharRow& iCharRow = pRow->GetCharRow();
                    // we only support ucs2 encoded char rows
                    FAIL_FAST_IF_MSG(iCharRow.GetSupportedEncoding() != ICharRow::SupportedEncoding::Ucs2,
                                    "only support UCS2 char rows currently");

                    const Ucs2CharRow& charRow = static_cast<const Ucs2CharRow&>(iCharRow);
                    it = std::next(charRow.cbegin(), X);
                    itEnd = charRow.cend();
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                pRow->GetAttrRow().FindAttrIndex(X, &pAttrRun, &CountOfAttr);

                k = 0;
                for (j = X; j < coordScreenBufferSize.X && it != itEnd; TargetPtr++, ++it)
                {
                    const WORD wLegacyAttributes = pAttrRun->GetAttributes().GetLegacyAttributes();
                    if ((j == X) && it->second.IsTrailing())
                    {
                        *TargetPtr = wLegacyAttributes;
                    }
                    else if (it->second.IsLeading())
                    {
                        if ((NumRead == *pcRecords - 1) || (j == coordScreenBufferSize.X - 1))
                        {
                            *TargetPtr = wLegacyAttributes;
                        }
                        else
                        {
                            *TargetPtr = wLegacyAttributes | it->second.GeneratePublicApiAttributeFormat();
                        }
                    }
                    else
                    {
                        *TargetPtr = wLegacyAttributes | it->second.GeneratePublicApiAttributeFormat();
                    }

                    NumRead++;
                    j += 1;

                    ++k;
                    if (static_cast<size_t>(k) == CountOfAttr && j < coordScreenBufferSize.X)
                    {
                        pAttrRun++;
                        k = 0;
                        CountOfAttr = pAttrRun->GetLength();
                    }

                    if (NumRead == *pcRecords)
                    {
                        delete[] TransBufferA;
                        return STATUS_SUCCESS;
                    }
                }

                try
                {
                    pRow = &screenInfo.GetTextBuffer().GetNextRowNoWrap(*pRow);
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                X = 0;
                Y++;
                if (Y >= coordScreenBufferSize.Y)
                {
                    break;
                }
            }
        }
        else
        {
            *pcRecords = 0;
            delete[] TransBufferA;

            return STATUS_INVALID_PARAMETER;
        }
    }

    if (ulStringType == CONSOLE_ASCII)
    {
        UINT const Codepage = gci.OutputCP;

        NumRead = ConvertToOem(Codepage, TransBuffer, NumRead, (LPSTR) pvBuffer, *pcRecords);

        delete[] TransBuffer;
    }

    delete[] TransBufferA;

    *pcRecords = NumRead;
    return STATUS_SUCCESS;
}

void ScreenBufferSizeChange(const COORD coordNewSize)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    try
    {
        gci.pInputBuffer->Write(std::make_unique<WindowBufferSizeEvent>(coordNewSize));
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
    }
}

void ScrollScreen(SCREEN_INFORMATION& screenInfo,
                  const SMALL_RECT * const psrScroll,
                  _In_opt_ const SMALL_RECT * const psrMerge,
                  const COORD coordTarget)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (screenInfo.IsActiveScreenBuffer())
    {
        IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
        status = NT_TESTNULL(pNotifier);

        if (NT_SUCCESS(status))
        {
            pNotifier->NotifyConsoleUpdateScrollEvent(coordTarget.X - psrScroll->Left, coordTarget.Y - psrScroll->Right);
        }
        IRenderer* const pRender = ServiceLocator::LocateGlobals().pRender;
        if (pRender != nullptr)
        {
            // psrScroll is the source rectangle which gets written with the same dimensions to the coordTarget position.
            // Therefore the final rectangle starts with the top left corner at coordTarget
            // and the size is the size of psrScroll.
            // NOTE: psrScroll is an INCLUSIVE rectangle, so we must add 1 when measuring width as R-L or B-T
            Viewport scrollRect = Viewport::FromInclusive(*psrScroll);
            SMALL_RECT rcWritten = Viewport::FromDimensions(coordTarget, scrollRect.Width(), scrollRect.Height()).ToExclusive();

            pRender->TriggerRedraw(&rcWritten);

            // psrMerge was just filled exactly where it's stated.
            if (psrMerge != nullptr)
            {
                // psrMerge is an inclusive rectangle. Make it exclusive to deal with the renderer.
                SMALL_RECT rcMerge = Viewport::FromInclusive(*psrMerge).ToExclusive();

                pRender->TriggerRedraw(&rcMerge);
            }
        }
    }
}

// Routine Description:
// - This routine rotates the circular buffer as a shorthand for scrolling the entire screen
SHORT ScrollEntireScreen(SCREEN_INFORMATION& screenInfo, const SHORT sScrollValue)
{
    // store index of first row
    SHORT const RowIndex = screenInfo.GetTextBuffer().GetFirstRowIndex();

    // update screen buffer
    screenInfo.GetTextBuffer().SetFirstRowIndex((SHORT)((RowIndex + sScrollValue) % screenInfo.GetScreenBufferSize().Y));

    return RowIndex;
}

// Routine Description:
// - This routine is a special-purpose scroll for use by AdjustCursorPosition.
// Arguments:
// - screenInfo - reference to screen buffer info.
// Return Value:
// - true if we succeeded in scrolling the buffer, otherwise false (if we're out of memory)
bool StreamScrollRegion(SCREEN_INFORMATION& screenInfo)
{
    // Rotate the circular buffer around and wipe out the previous final line.
    bool fSuccess = screenInfo.GetTextBuffer().IncrementCircularBuffer();
    if (fSuccess)
    {
        // Trigger a graphical update if we're active.
        if (screenInfo.IsActiveScreenBuffer())
        {
            COORD coordDelta = { 0 };
            coordDelta.Y = -1;

            IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
            if (pNotifier)
            {
                // Notify accessibility that a scroll has occurred.
                pNotifier->NotifyConsoleUpdateScrollEvent(coordDelta.X, coordDelta.Y);
            }

            if (ServiceLocator::LocateGlobals().pRender != nullptr)
            {
                ServiceLocator::LocateGlobals().pRender->TriggerScroll(&coordDelta);
            }
        }
    }
    return fSuccess;
}

// Routine Description:
// - This routine copies ScrollRectangle to DestinationOrigin then fills in ScrollRectangle with Fill.
// - The scroll region is copied to a third buffer, the scroll region is filled, then the original contents of the scroll region are copied to the destination.
// Arguments:
// - screenInfo - reference to screen buffer info.
// - ScrollRectangle - Region to copy
// - ClipRectangle - Optional pointer to clip region.
// - DestinationOrigin - Upper left corner of target region.
// - Fill - Character and attribute to fill source region with.
// Return Value:
[[nodiscard]]
NTSTATUS ScrollRegion(SCREEN_INFORMATION& screenInfo,
                      _Inout_ PSMALL_RECT psrScroll,
                      _In_opt_ PSMALL_RECT psrClip,
                      _In_ COORD coordDestinationOrigin,
                      _In_ CHAR_INFO ciFill)
{
    // here's how we clip:

    // Clip source rectangle to screen buffer => S
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2
    // Clip S to ClipRegion => S3

    // S2 is the region we copy to T
    // S3 is the region to fill

    if (ciFill.Char.UnicodeChar == '\0' && ciFill.Attributes == 0)
    {
        ciFill.Char.UnicodeChar = L' ';
        ciFill.Attributes = screenInfo.GetAttributes().GetLegacyAttributes();
    }

    // clip the source rectangle to the screen buffer
    if (psrScroll->Left < 0)
    {
        coordDestinationOrigin.X += -psrScroll->Left;
        psrScroll->Left = 0;
    }
    if (psrScroll->Top < 0)
    {
        coordDestinationOrigin.Y += -psrScroll->Top;
        psrScroll->Top = 0;
    }

    const COORD coordScreenBufferSize = screenInfo.GetScreenBufferSize();
    if (psrScroll->Right >= coordScreenBufferSize.X)
    {
        psrScroll->Right = (SHORT)(coordScreenBufferSize.X - 1);
    }
    if (psrScroll->Bottom >= coordScreenBufferSize.Y)
    {
        psrScroll->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
    }

    // if source rectangle doesn't intersect screen buffer, return.
    if (psrScroll->Bottom < psrScroll->Top || psrScroll->Right < psrScroll->Left)
    {
        return STATUS_SUCCESS;
    }

    // Account for the scroll margins set by DECSTBM
    SMALL_RECT srScrollMargins = screenInfo.GetScrollMargins();
    SMALL_RECT srBufferViewport = screenInfo.GetBufferViewport();

    // The margins are in viewport relative coordinates. Adjust for that.
    srScrollMargins.Top += srBufferViewport.Top;
    srScrollMargins.Bottom += srBufferViewport.Top;
    srScrollMargins.Left += srBufferViewport.Left;
    srScrollMargins.Right += srBufferViewport.Left;

    if (srScrollMargins.Bottom > srScrollMargins.Top)
    {
        if (psrScroll->Top < srScrollMargins.Top)
        {
            psrScroll->Top = srScrollMargins.Top;
        }
        if (psrScroll->Bottom >= srScrollMargins.Bottom)
        {
            psrScroll->Bottom = srScrollMargins.Bottom;
        }
    }
    SMALL_RECT OurClipRectangle;
    // clip the target rectangle
    // if a cliprectangle was provided, clip it to the screen buffer.
    // if not, set the cliprectangle to the screen buffer region.
    if (psrClip)
    {
        // clip the cliprectangle.
        if (psrClip->Left < 0)
        {
            psrClip->Left = 0;
        }
        if (psrClip->Top < 0)
        {
            psrClip->Top = 0;
        }
        if (psrClip->Right >= coordScreenBufferSize.X)
        {
            psrClip->Right = (SHORT)(coordScreenBufferSize.X - 1);
        }
        if (psrClip->Bottom >= coordScreenBufferSize.Y)
        {
            psrClip->Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
        }
    }
    else
    {
        OurClipRectangle.Left = 0;
        OurClipRectangle.Top = 0;
        OurClipRectangle.Right = (SHORT)(coordScreenBufferSize.X - 1);
        OurClipRectangle.Bottom = (SHORT)(coordScreenBufferSize.Y - 1);
        psrClip = &OurClipRectangle;
    }
    // Account for the scroll margins set by DECSTBM
    if (srScrollMargins.Bottom > srScrollMargins.Top)
    {
        if (psrClip->Top < srScrollMargins.Top)
        {
            psrClip->Top = srScrollMargins.Top;
        }
        if (psrClip->Bottom >= srScrollMargins.Bottom)
        {
            psrClip->Bottom = srScrollMargins.Bottom;
        }
    }
    // Create target rectangle based on S => T
    // Clip T to ClipRegion => T
    // Create S2 based on clipped T => S2

    SMALL_RECT ScrollRectangle2 = *psrScroll;

    SMALL_RECT TargetRectangle;
    TargetRectangle.Left = coordDestinationOrigin.X;
    TargetRectangle.Top = coordDestinationOrigin.Y;
    TargetRectangle.Right = (SHORT)(coordDestinationOrigin.X + (ScrollRectangle2.Right - ScrollRectangle2.Left + 1) - 1);
    TargetRectangle.Bottom = (SHORT)(coordDestinationOrigin.Y + (ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1) - 1);

    if (TargetRectangle.Left < psrClip->Left)
    {
        ScrollRectangle2.Left += psrClip->Left - TargetRectangle.Left;
        TargetRectangle.Left = psrClip->Left;
    }
    if (TargetRectangle.Top < psrClip->Top)
    {
        ScrollRectangle2.Top += psrClip->Top - TargetRectangle.Top;
        TargetRectangle.Top = psrClip->Top;
    }
    if (TargetRectangle.Right > psrClip->Right)
    {
        ScrollRectangle2.Right -= TargetRectangle.Right - psrClip->Right;
        TargetRectangle.Right = psrClip->Right;
    }
    if (TargetRectangle.Bottom > psrClip->Bottom)
    {
        ScrollRectangle2.Bottom -= TargetRectangle.Bottom - psrClip->Bottom;
        TargetRectangle.Bottom = psrClip->Bottom;
    }

    // clip scroll rect to clipregion => S3
    SMALL_RECT ScrollRectangle3 = *psrScroll;
    if (ScrollRectangle3.Left < psrClip->Left)
    {
        ScrollRectangle3.Left = psrClip->Left;
    }
    if (ScrollRectangle3.Top < psrClip->Top)
    {
        ScrollRectangle3.Top = psrClip->Top;
    }
    if (ScrollRectangle3.Right > psrClip->Right)
    {
        ScrollRectangle3.Right = psrClip->Right;
    }
    if (ScrollRectangle3.Bottom > psrClip->Bottom)
    {
        ScrollRectangle3.Bottom = psrClip->Bottom;
    }

    // if scroll rect doesn't intersect clip region, return.
    if (ScrollRectangle3.Bottom < ScrollRectangle3.Top || ScrollRectangle3.Right < ScrollRectangle3.Left)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    // if target rectangle doesn't intersect screen buffer, skip scrolling part.
    if (!(TargetRectangle.Bottom < TargetRectangle.Top || TargetRectangle.Right < TargetRectangle.Left))
    {
        // if we can, don't use intermediate scroll region buffer.  do this
        // by figuring out fill rectangle.  NOTE: this code will only work
        // if _CopyRectangle copies from low memory to high memory (otherwise
        // we would overwrite the scroll region before reading it).

        if (ScrollRectangle2.Right == TargetRectangle.Right &&
            ScrollRectangle2.Left == TargetRectangle.Left && ScrollRectangle2.Top > TargetRectangle.Top && ScrollRectangle2.Top < TargetRectangle.Bottom)
        {
            COORD TargetPoint;
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;

            if (ScrollRectangle2.Right == (SHORT)(coordScreenBufferSize.X - 1) &&
                ScrollRectangle2.Left == 0 && ScrollRectangle2.Bottom == (SHORT)(coordScreenBufferSize.Y - 1) && ScrollRectangle2.Top == 1)
            {
                ScrollEntireScreen(screenInfo, (SHORT)(ScrollRectangle2.Top - TargetRectangle.Top));
            }
            else
            {
                Status = _CopyRectangle(screenInfo, &ScrollRectangle2, TargetPoint);
            }
            if (NT_SUCCESS(Status))
            {
                SMALL_RECT FillRect;
                FillRect.Left = TargetRectangle.Left;
                FillRect.Right = TargetRectangle.Right;
                FillRect.Top = (SHORT)(TargetRectangle.Bottom + 1);
                FillRect.Bottom = psrScroll->Bottom;
                if (FillRect.Top < psrClip->Top)
                {
                    FillRect.Top = psrClip->Top;
                }

                if (FillRect.Bottom > psrClip->Bottom)
                {
                    FillRect.Bottom = psrClip->Bottom;
                }

                FillRectangle(&ciFill, screenInfo, &FillRect);

                ScrollScreen(screenInfo, &ScrollRectangle2, &FillRect, TargetPoint);
            }

        }

        // if no overlap, don't need intermediate copy
        else if (ScrollRectangle3.Right < TargetRectangle.Left ||
                 ScrollRectangle3.Left > TargetRectangle.Right ||
                 ScrollRectangle3.Top > TargetRectangle.Bottom ||
                 ScrollRectangle3.Bottom < TargetRectangle.Top)
        {
            COORD TargetPoint;
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;
            Status = _CopyRectangle(screenInfo, &ScrollRectangle2, TargetPoint);
            if (NT_SUCCESS(Status))
            {
                FillRectangle(&ciFill, screenInfo, &ScrollRectangle3);

                ScrollScreen(screenInfo, &ScrollRectangle2, &ScrollRectangle3, TargetPoint);
            }
        }

        // for the case where the source and target rectangles overlap, we copy the source rectangle, fill it, then copy it to the target.
        else
        {
            COORD Size;
            Size.X = (SHORT)(ScrollRectangle2.Right - ScrollRectangle2.Left + 1);
            Size.Y = (SHORT)(ScrollRectangle2.Bottom - ScrollRectangle2.Top + 1);

            SMALL_RECT TargetRect;
            TargetRect.Left = 0;
            TargetRect.Top = 0;
            TargetRect.Right = ScrollRectangle2.Right - ScrollRectangle2.Left;
            TargetRect.Bottom = ScrollRectangle2.Bottom - ScrollRectangle2.Top;

            COORD SourcePoint;
            SourcePoint.X = ScrollRectangle2.Left;
            SourcePoint.Y = ScrollRectangle2.Top;

            std::vector<std::vector<OutputCell>> outputCells;
            try
            {
                outputCells = ReadRectFromScreenBuffer(screenInfo, SourcePoint, Viewport::FromInclusive(TargetRect));
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }

            FillRectangle(&ciFill, screenInfo, &ScrollRectangle3);

            SMALL_RECT SourceRectangle;
            SourceRectangle.Top = 0;
            SourceRectangle.Left = 0;
            SourceRectangle.Right = (SHORT)(Size.X - 1);
            SourceRectangle.Bottom = (SHORT)(Size.Y - 1);

            COORD TargetPoint;
            TargetPoint.X = TargetRectangle.Left;
            TargetPoint.Y = TargetRectangle.Top;

            try
            {
                WriteRectToScreenBuffer(screenInfo, outputCells, TargetPoint);
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }
            // update regions on screen.
            ScrollScreen(screenInfo, &ScrollRectangle2, &ScrollRectangle3, TargetPoint);
        }
    }
    else
    {
        // Do fill.
        FillRectangle(&ciFill, screenInfo, &ScrollRectangle3);

        WriteToScreen(screenInfo, ScrollRectangle3);
    }
    return Status;
}

void SetActiveScreenBuffer(SCREEN_INFORMATION& screenInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.pCurrentScreenBuffer = &screenInfo;

    // initialize cursor
    screenInfo.GetTextBuffer().GetCursor().SetIsOn(false);

    // set font
    screenInfo.RefreshFontWithRenderer();

    // Empty input buffer.
    gci.pInputBuffer->FlushAllButKeys();
    SetScreenColors(screenInfo,
                    screenInfo.GetAttributes().GetLegacyAttributes(),
                    screenInfo.GetPopupAttributes()->GetLegacyAttributes(),
                    FALSE);

    // Set window size.
    screenInfo.PostUpdateWindowSize();

    gci.ConsoleIme.RefreshAreaAttributes();

    // Write data to screen.
    WriteToScreen(screenInfo, screenInfo.GetBufferViewport());
}

// TODO: MSFT 9450717 This should join the ProcessList class when CtrlEvents become moved into the server. https://osgvsowi/9450717
void CloseConsoleProcessState()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // If there are no connected processes, sending control events is pointless as there's no one do send them to. In
    // this case we'll just exit conhost.

    // N.B. We can get into this state when a process has a reference to the console but hasn't connected. For example,
    //      when it's created suspended and never resumed.
    if (gci.ProcessHandleList.IsEmpty())
    {
        ServiceLocator::RundownAndExit(STATUS_SUCCESS);
    }

    HandleCtrlEvent(CTRL_CLOSE_EVENT);
}
