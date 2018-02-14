/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"

#include "dbcs.h"
#include "misc.h"
#include "Ucs2CharRow.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"
#include "..\types\inc\Viewport.hpp"
#include <algorithm>
#include <iterator>

#pragma hdrstop

using namespace Microsoft::Console::Types;

#define MAX_POLY_LINES 80

#define WCHAR_OF_PCI(p) (((PCHAR_INFO)(p))->Char.UnicodeChar)
#define ATTR_OF_PCI(p)  (((PCHAR_INFO)(p))->Attributes)
#define SIZEOF_CI_CELL  sizeof(CHAR_INFO)

void StreamWriteToScreenBuffer(_Inout_updates_(cchBuffer) PWCHAR pwchBuffer,
                               _In_ SHORT cchBuffer,
                               _In_ PSCREEN_INFORMATION pScreenInfo,
                               _Inout_updates_(cchBuffer) DbcsAttribute* const pDbcsAttributes,
                               _In_ const bool fWasLineWrapped)
{
    DBGOUTPUT(("StreamWriteToScreenBuffer\n"));
    COORD const TargetPoint = pScreenInfo->TextInfo->GetCursor()->GetPosition();
    ROW& Row = pScreenInfo->TextInfo->GetRowByOffset(TargetPoint.Y);
    DBGOUTPUT(("&Row = 0x%p, TargetPoint = (0x%x,0x%x)\n", &Row, TargetPoint.X, TargetPoint.Y));

    // TODO: from CleanupDbcsEdgesForWrite to the end of the if statement seems to never execute
    // both callers of this function appear to already have handled the line length for double-width characters
    CleanupDbcsEdgesForWrite(cchBuffer, TargetPoint, pScreenInfo);
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (TargetPoint.Y == coordScreenBufferSize.Y - 1 &&
        TargetPoint.X + cchBuffer >= coordScreenBufferSize.X &&
        pDbcsAttributes[coordScreenBufferSize.X - TargetPoint.X - 1].IsLeading())
    {
        *(pwchBuffer + coordScreenBufferSize.X - TargetPoint.X - 1) = UNICODE_SPACE;
        pDbcsAttributes[coordScreenBufferSize.X - TargetPoint.X - 1].SetSingle();
        if (cchBuffer > coordScreenBufferSize.X - TargetPoint.X)
        {
            *(pwchBuffer + coordScreenBufferSize.X - TargetPoint.X) = UNICODE_SPACE;
            pDbcsAttributes[coordScreenBufferSize.X - TargetPoint.X].SetSingle();
        }
    }

    // copy chars
    try
    {
        ICharRow& iCharRow = Row.GetCharRow();
        if (iCharRow.GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
        {
            Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
            OverwriteColumns(pwchBuffer,
                            pwchBuffer + cchBuffer,
                            pDbcsAttributes,
                            std::next(charRow.begin(), TargetPoint.X));
        }
        else
        {
            THROW_HR_MSG(E_FAIL, "we only support ucs2 rows right now");
        }
    }
    CATCH_LOG();

    // caller knows the wrap status as this func is called only for drawing one line at a time
    Row.GetCharRow().SetWrapForced(fWasLineWrapped);

    TextAttributeRun CurrentBufferAttrs;
    CurrentBufferAttrs.SetLength(cchBuffer);
    CurrentBufferAttrs.SetAttributes(pScreenInfo->GetAttributes());
    Row.GetAttrRow().InsertAttrRuns(&CurrentBufferAttrs,
                                    1,
                                    TargetPoint.X,
                                    (SHORT)(TargetPoint.X + cchBuffer - 1),
                                    coordScreenBufferSize.X);

    pScreenInfo->ResetTextFlags(TargetPoint.X, TargetPoint.Y, TargetPoint.X + cchBuffer - 1, TargetPoint.Y);
}

// Routine Description:
// - This routine copies a rectangular region to the screen buffer. no clipping is done.
// - The source should contain Unicode or UnicodeOem chars.
// Arguments:
// - prgbSrc - pointer to source buffer (a real VGA buffer or CHAR_INFO[])
// - coordSrcDimensions - dimensions of source buffer
// - psrSrc - rectangle in source buffer to copy
// - pScreenInfo - pointer to screen info
// - coordDest - upper left coordinates of target rectangle
// - Codepage - codepage to translate real VGA buffer from,
//              0xFFFFFFF if Source is CHAR_INFO[] (not requiring translation)
// Return Value:
// - <none>
NTSTATUS WriteRectToScreenBuffer(_In_reads_(coordSrcDimensions.X * coordSrcDimensions.Y * sizeof(CHAR_INFO)) PBYTE const prgbSrc,
                                 _In_ const COORD coordSrcDimensions,
                                 _In_ const SMALL_RECT * const psrSrc,
                                 _In_ PSCREEN_INFORMATION pScreenInfo,
                                 _In_ const COORD coordDest,
                                 _In_reads_opt_(coordSrcDimensions.X * coordSrcDimensions.Y) TextAttribute* const pTextAttributes)
{
    DBGOUTPUT(("WriteRectToScreenBuffer\n"));
    const bool fWriteAttributes = pTextAttributes != nullptr;

    SHORT const XSize = (SHORT)(psrSrc->Right - psrSrc->Left + 1);
    SHORT const YSize = (SHORT)(psrSrc->Bottom - psrSrc->Top + 1);

    // Allocate enough space for the case where; every char is a different color
    TextAttributeRun* rAttrRunsBuff = new TextAttributeRun[XSize];
    NTSTATUS Status = NT_TESTNULL(rAttrRunsBuff);
    if (NT_SUCCESS(Status))
    {
        PBYTE SourcePtr = prgbSrc;
        BYTE* pbSourceEnd = prgbSrc + ((coordSrcDimensions.X * coordSrcDimensions.Y) * sizeof(CHAR_INFO));

        BOOLEAN WholeSource = FALSE;
        if (XSize == coordSrcDimensions.X)
        {
            ASSERT(psrSrc->Left == 0);
            if (psrSrc->Top != 0)
            {
                SourcePtr += SCREEN_BUFFER_POINTER(psrSrc->Left, psrSrc->Top, coordSrcDimensions.X, SIZEOF_CI_CELL);
            }
            WholeSource = TRUE;
        }

        const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
        ROW* pRow = &pScreenInfo->TextInfo->GetRowByOffset(coordDest.Y);
        for (SHORT i = 0; i < YSize; i++)
        {
            // ensure we have a valid row pointer, if not, skip.
            if (pRow == nullptr)
            {
                ASSERT(false);
                break;
            }

            if (!WholeSource)
            {
                SourcePtr = prgbSrc + SCREEN_BUFFER_POINTER(psrSrc->Left, psrSrc->Top + i, coordSrcDimensions.X, SIZEOF_CI_CELL);
            }

            // copy the chars and attrs into their respective arrays
            COORD TPoint;

            TPoint.X = coordDest.X;
            TPoint.Y = coordDest.Y + i;
            CleanupDbcsEdgesForWrite(XSize, TPoint, pScreenInfo);

            if (TPoint.Y == coordScreenBufferSize.Y - 1 &&
                TPoint.X + XSize - 1 >= coordScreenBufferSize.X)
            {
                const BYTE* const pbLeadingByte = SourcePtr + coordScreenBufferSize.X - TPoint.X - 1;
                if (((pbLeadingByte + sizeof(CHAR_INFO)) > pbSourceEnd) || (pbLeadingByte < prgbSrc))
                {
                    ASSERT(false);
                    return STATUS_BUFFER_OVERFLOW;
                }

                if (ATTR_OF_PCI(pbLeadingByte) & COMMON_LVB_LEADING_BYTE)
                {
                    WCHAR_OF_PCI(pbLeadingByte) = UNICODE_SPACE;
                    ATTR_OF_PCI(pbLeadingByte) &= ~COMMON_LVB_SBCSDBCS;
                    if (XSize - 1 > coordScreenBufferSize.X - TPoint.X - 1)
                    {
                        const BYTE* const pbTrailingByte = SourcePtr + coordScreenBufferSize.X - TPoint.X;
                        if (pbTrailingByte + sizeof(CHAR_INFO) > pbSourceEnd || pbTrailingByte < prgbSrc)
                        {
                            ASSERT(false);
                            return STATUS_BUFFER_OVERFLOW;
                        }

                        WCHAR_OF_PCI(pbTrailingByte) = UNICODE_SPACE;
                        ATTR_OF_PCI(pbTrailingByte) &= ~COMMON_LVB_SBCSDBCS;
                    }
                }
            }

            // CJK Languages
            Ucs2CharRow::iterator it;
            Ucs2CharRow::const_iterator itEnd;
            try
            {
                ICharRow& iCharRow = pRow->GetCharRow();
                if (iCharRow.GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
                {
                    Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
                    it = std::next(charRow.begin(), coordDest.X);
                    itEnd = charRow.cend();
                }
                else
                {
                    return STATUS_UNSUCCESSFUL;
                }
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }


            TextAttributeRun* pAttrRun = rAttrRunsBuff;
            pAttrRun->SetLength(0);
            SHORT NumAttrRuns = 1;

            pAttrRun->SetAttributesFromLegacy(ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS);

            pRow->GetCharRow().SetWrapForced(false); // clear wrap status for rectangle drawing

            for (SHORT j = psrSrc->Left;
                 j <= psrSrc->Right && it != itEnd;
                 j++, SourcePtr += SIZEOF_CI_CELL, ++it)
            {
                if (SourcePtr + sizeof(CHAR_INFO) > pbSourceEnd || SourcePtr < prgbSrc)
                {
                    ASSERT(false);
                    return STATUS_BUFFER_OVERFLOW;
                }

                try
                {
                    it->first = WCHAR_OF_PCI(SourcePtr);
                    it->second = DbcsAttribute::FromPublicApiAttributeFormat(reinterpret_cast<const CHAR_INFO* const>(SourcePtr)->Attributes);
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                if (pAttrRun->GetAttributes().IsEqualToLegacy((ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS)))
                {
                    pAttrRun->SetLength(pAttrRun->GetLength()+1);
                }
                else
                {
                    pAttrRun++;
                    pAttrRun->SetLength(1);
                    // MSKK Apr.02.1993 V-HirotS For KAttr
                    pAttrRun->SetAttributesFromLegacy(ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS);
                    NumAttrRuns += 1;
                }
            }

            pRow->GetAttrRow().InsertAttrRuns(rAttrRunsBuff,
                                              NumAttrRuns,
                                              coordDest.X,
                                              (SHORT)(coordDest.X + XSize - 1),
                                              coordScreenBufferSize.X);

            try
            {
                pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }
        }
        pScreenInfo->ResetTextFlags(coordDest.X, coordDest.Y, (SHORT)(coordDest.X + XSize - 1), (SHORT)(coordDest.Y + YSize - 1));

        // The above part of the code seems ridiculous in terms of complexity. especially the dbcs stuff.
        // So we'll copy the attributes here in a not terrible way.
        if (fWriteAttributes)
        {
            // This is trying to get the arrtibute at (Left,Top) from the source array.
            // Because it's in a linear array, its at (y*rowLength)+x.
            const TextAttribute* const pOriginAttr = &(pTextAttributes[(psrSrc->Top * coordSrcDimensions.X) + psrSrc->Left]);
            const int rowWidth = coordScreenBufferSize.X;
            const int srcWidth = psrSrc->Right - psrSrc->Left;
            const TextAttribute* pSrcAttr = pOriginAttr;

            // For each source row, iterate over the attrs in it.
            //  We're going to create AttrRuns for each attr in the row, starting with the first one.
            //  If the current attr is different then the last one, then insert the last one, and start a new run.
            for (int y = coordDest.Y; y < coordSrcDimensions.Y + coordDest.Y; y++)
            {
                ROW& Row = pScreenInfo->TextInfo->GetRowByOffset(y);

                TextAttributeRun insert;
                int currentLength = 1;  // This is the length of the current run.
                unsigned int destX = coordDest.X;  // This is where the current run begins in the dest buffer.
                TextAttribute lastAttr = *pSrcAttr;

                // We've already gotten the value of the first attr, so start at index 1.
                pSrcAttr++;
                for (int x = 1; x < srcWidth; x++)
                {
                    if (!pSrcAttr->IsEqual(lastAttr))
                    {
                        insert.SetAttributes(lastAttr);
                        insert.SetLength(currentLength);
                        Row.GetAttrRow().InsertAttrRuns(&insert, 1, destX, (destX + currentLength) - 1, rowWidth);

                        destX = coordDest.X + x;
                        lastAttr = *pSrcAttr;
                        currentLength = 0;
                    }

                    currentLength++;
                    pSrcAttr++;
                }

                // Make sure to also insert the last run we started.
                insert.SetAttributes(lastAttr);
                insert.SetLength(currentLength);
                pRow->GetAttrRow().InsertAttrRuns(&insert, 1, destX, (destX + currentLength) - 1, rowWidth);

                pSrcAttr++; // advance to next row.
            }
        }

        delete[] rAttrRunsBuff;
    }

    return Status;

}

void WriteRegionToScreen(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ PSMALL_RECT psrRegion)
{
    if (pScreenInfo->IsActiveScreenBuffer())
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
// - pScreenInfo - Pointer to screen buffer information.
// - srRegion - Region to write in screen buffer coordinates.  Region is inclusive
// Return Value:
// - <none>
void WriteToScreen(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT srRegion)
{
    DBGOUTPUT(("WriteToScreen\n"));
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // update to screen, if we're not iconic.
    if (!pScreenInfo->IsActiveScreenBuffer() || IsFlagSet(gci.Flags, CONSOLE_IS_ICONIC))
    {
        return;
    }

    // clip region
    SMALL_RECT ClippedRegion;
    {
        const SMALL_RECT currentViewport = pScreenInfo->GetBufferViewport();
        ClippedRegion.Left = max(srRegion.Left, currentViewport.Left);
        ClippedRegion.Top = max(srRegion.Top, currentViewport.Top);
        ClippedRegion.Right = min(srRegion.Right, currentViewport.Right);
        ClippedRegion.Bottom = min(srRegion.Bottom, currentViewport.Bottom);
        if (ClippedRegion.Right < ClippedRegion.Left || ClippedRegion.Bottom < ClippedRegion.Top)
        {
            return;
        }
    }

    WriteRegionToScreen(pScreenInfo, &ClippedRegion);

    WriteConvRegionToScreen(pScreenInfo, srRegion);
}

// Routine Description:
// - This routine writes a string of characters or attributes to the screen buffer.
// Arguments:
// - pScreenInfo - Pointer to screen buffer information.
// - pvBuffer - Buffer to write from.
// - coordWrite - Screen buffer coordinate to begin writing to.
// - ulStringType - One of the following:
//                  CONSOLE_ASCII          - write a string of ascii characters.
//                  CONSOLE_REAL_UNICODE   - write a string of real unicode characters.
//                  CONSOLE_FALSE_UNICODE  - write a string of false unicode characters.
//                  CONSOLE_ATTRIBUTE      - write a string of attributes.
// - pcRecords - On input, the number of elements to write.  On output, the number of elements written.
// - pcColumns - receives the number of columns output, which could be more than NumRecords (FE fullwidth chars)
// Return Value:
NTSTATUS WriteOutputString(_In_ PSCREEN_INFORMATION pScreenInfo,
                           _In_reads_(*pcRecords) const VOID * pvBuffer,
                           _In_ const COORD coordWrite,
                           _In_ const ULONG ulStringType,
                           _Inout_ PULONG pcRecords,    // this value is valid even for error cases
                           _Out_opt_ PULONG pcColumns)
{
    DBGOUTPUT(("WriteOutputString\n"));
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    if (*pcRecords == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumWritten = 0;
    ULONG NumRecordsSavedForUnicode = 0;
    SHORT X = coordWrite.X;
    SHORT Y = coordWrite.Y;
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (X >= coordScreenBufferSize.X || X < 0 || Y >= coordScreenBufferSize.Y || Y < 0)
    {
        *pcRecords = 0;
        return STATUS_SUCCESS;
    }

    WCHAR rgwchBuffer[128] = {0};
    DbcsAttribute rgbBufferA[128];

    DbcsAttribute* BufferA = nullptr;
    PWCHAR TransBuffer = nullptr;
    DbcsAttribute* TransBufferA = nullptr;
    BOOL fLocalHeap = FALSE;
    if (ulStringType == CONSOLE_ASCII)
    {
        UINT const Codepage = gci.OutputCP;

        if (*pcRecords >= ARRAYSIZE(rgwchBuffer) ||
            *pcRecords >= ARRAYSIZE(rgbBufferA))
        {

            TransBuffer = new WCHAR[*pcRecords * 2];
            if (TransBuffer == nullptr)
            {
                return STATUS_NO_MEMORY;
            }
            TransBufferA = new DbcsAttribute[*pcRecords * 2];
            if (TransBufferA == nullptr)
            {
                delete[] TransBuffer;
                return STATUS_NO_MEMORY;
            }

            fLocalHeap = TRUE;
        }
        else
        {
            TransBuffer = rgwchBuffer;
            TransBufferA = rgbBufferA;
        }

        PCHAR TmpBuf = (PCHAR)pvBuffer;
        PCHAR pchTmpBufEnd = TmpBuf + *pcRecords;
        PWCHAR TmpTrans = TransBuffer;
        DbcsAttribute* TmpTransA = TransBufferA;
        for (ULONG i = 0; i < *pcRecords;)
        {
            if (TmpBuf >= pchTmpBufEnd)
            {
                ASSERT(false);
                break;
            }

            if (IsDBCSLeadByteConsole(*TmpBuf, &gci.OutputCPInfo))
            {
                if (i + 1 >= *pcRecords)
                {
                    *TmpTrans = UNICODE_SPACE;
                    TmpTransA->SetSingle();
                    i++;
                }
                else
                {
                    if (TmpBuf + 1 >= pchTmpBufEnd)
                    {
                        ASSERT(false);
                        break;
                    }

                    ConvertOutputToUnicode(Codepage, TmpBuf, 2, TmpTrans, 2);
                    *(TmpTrans + 1) = *TmpTrans;
                    TmpTrans += 2;
                    TmpBuf += 2;
                    TmpTransA->SetLeading();
                    ++TmpTransA;
                    TmpTransA->SetTrailing();
                    ++TmpTransA;
                    i += 2;
                }
            }
            else
            {
                ConvertOutputToUnicode(Codepage, TmpBuf, 1, TmpTrans, 1);
                TmpTrans++;
                TmpBuf++;
                TmpTransA->SetSingle();
                ++TmpTransA;
                i++;
            }
        }
        BufferA = TransBufferA;
        pvBuffer = TransBuffer;
    }

    if ((ulStringType == CONSOLE_REAL_UNICODE) || (ulStringType == CONSOLE_FALSE_UNICODE))
    {
        PWCHAR TmpBuf;
        PWCHAR TmpTrans;
        DbcsAttribute* TmpTransA;
        ULONG i, jTmp;
        WCHAR c;

        // Avoid overflow into TransBufferCharacter and TransBufferAttribute.
        // If hit by IsConsoleFullWidth() then one unicode character needs two spaces on TransBuffer.
        if (*pcRecords * 2 >= ARRAYSIZE(rgwchBuffer) ||
            *pcRecords * 2 >= ARRAYSIZE(rgbBufferA))
        {
            TransBuffer = new WCHAR[*pcRecords * 2];
            if (TransBuffer == nullptr)
            {
                return STATUS_NO_MEMORY;
            }

            TransBufferA = new DbcsAttribute[*pcRecords * 2];
            if (TransBufferA == nullptr)
            {
                delete[] TransBuffer;
                return STATUS_NO_MEMORY;
            }

            fLocalHeap = TRUE;
        }
        else
        {
            TransBuffer = rgwchBuffer;
            TransBufferA = rgbBufferA;
        }

        TmpBuf = (PWCHAR)pvBuffer;
        TmpTrans = TransBuffer;
        TmpTransA = TransBufferA;
        for (i = 0, jTmp = 0; i < *pcRecords; i++, jTmp++)
        {
            *TmpTrans++ = c = *TmpBuf++;
            TmpTransA->SetSingle();
            if (IsCharFullWidth(c))
            {
                TmpTransA->SetLeading();
                ++TmpTransA;
                *TmpTrans++ = c;
                TmpTransA->SetTrailing();
                jTmp++;
            }

            TmpTransA++;
        }

        NumRecordsSavedForUnicode = *pcRecords;
        *pcRecords = jTmp;
        pvBuffer = TransBuffer;
        BufferA = TransBufferA;
    }

    ROW* pRow = &pScreenInfo->TextInfo->GetRowByOffset(coordWrite.Y);
    if ((ulStringType == CONSOLE_REAL_UNICODE) || (ulStringType == CONSOLE_FALSE_UNICODE) || (ulStringType == CONSOLE_ASCII))
    {
        for (;;)
        {
            if (pRow == nullptr)
            {
                ASSERT(false);
                break;
            }

            // copy the chars into their arrays
            Ucs2CharRow::iterator it;
            try
            {
                ICharRow& iCharRow = pRow->GetCharRow();
                if (iCharRow.GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
                {
                    Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
                    it = std::next(charRow.begin(), X);
                }
                else
                {
                    return STATUS_UNSUCCESSFUL;
                }
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }

            if ((ULONG)(coordScreenBufferSize.X - X) >= (*pcRecords - NumWritten))
            {
                // The text will not hit the right hand edge, copy it all
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                CleanupDbcsEdgesForWrite((SHORT)(*pcRecords - NumWritten), TPoint, pScreenInfo);

                if (TPoint.Y == coordScreenBufferSize.Y - 1 &&
                    (SHORT)(TPoint.X + *pcRecords - NumWritten) >= coordScreenBufferSize.X &&
                    BufferA[coordScreenBufferSize.X - TPoint.X - 1].IsLeading())
                {
                    *((PWCHAR)pvBuffer + coordScreenBufferSize.X - TPoint.X - 1) = UNICODE_SPACE;
                    BufferA[coordScreenBufferSize.X - TPoint.X - 1].SetSingle();
                    if ((SHORT)(*pcRecords - NumWritten) > (SHORT)(coordScreenBufferSize.X - TPoint.X - 1))
                    {
                        #pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "ScreenBufferSize limits this, but it's very obtuse.")
                        *((PWCHAR)pvBuffer + coordScreenBufferSize.X - TPoint.X) = UNICODE_SPACE;
                        #pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "ScreenBufferSize limits this, but it's very obtuse.")
                        BufferA[coordScreenBufferSize.X - TPoint.X].SetSingle();
                    }
                }

                const size_t numToWrite = *pcRecords - NumWritten;
                try
                {
                    const wchar_t* const pChars = reinterpret_cast<const wchar_t* const>(pvBuffer);
                    const DbcsAttribute* const pAttrs = static_cast<const DbcsAttribute* const>(BufferA);
                    OverwriteColumns(pChars, pChars + numToWrite, pAttrs, it);
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }

                X = (SHORT)(X + numToWrite - 1);
                NumWritten = *pcRecords;
            }
            else
            {
                // The text will hit the right hand edge, copy only that much.
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                CleanupDbcsEdgesForWrite((SHORT)(coordScreenBufferSize.X - X), TPoint, pScreenInfo);
                if (TPoint.Y == coordScreenBufferSize.Y - 1 &&
                    TPoint.X + coordScreenBufferSize.X - X >= coordScreenBufferSize.X &&
                    BufferA[coordScreenBufferSize.X - TPoint.X - 1].IsLeading())
                {
                    *((PWCHAR)pvBuffer + coordScreenBufferSize.X - TPoint.X - 1) = UNICODE_SPACE;
                    BufferA[coordScreenBufferSize.X - TPoint.X - 1].SetSingle();
                    if (coordScreenBufferSize.X - X > coordScreenBufferSize.X - TPoint.X - 1)
                    {
                        *((PWCHAR)pvBuffer + coordScreenBufferSize.X - TPoint.X) = UNICODE_SPACE;
                        BufferA[coordScreenBufferSize.X - TPoint.X].SetSingle();
                    }
                }
                const size_t numToWrite = coordScreenBufferSize.X - X;
                try
                {
                    const wchar_t* const pChars = reinterpret_cast<const wchar_t* const>(pvBuffer);
                    const DbcsAttribute* const pAttrs = static_cast<const DbcsAttribute* const>(BufferA);
                    OverwriteColumns(pChars, pChars + numToWrite, pAttrs, it);
                }
                catch (...)
                {
                    return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
                }
                BufferA = BufferA + numToWrite;
                pvBuffer = (PVOID)((PBYTE)pvBuffer + (numToWrite * sizeof(WCHAR)));
                NumWritten += static_cast<ULONG>(numToWrite);
                X = (SHORT)(numToWrite);
            }

            if (NumWritten < *pcRecords)
            {
                // The string hit the right hand edge, so wrap around to the next line by going back round the while loop, unless we
                // are at the end of the buffer - in which case we simply abandon the remainder of the output string!
                X = 0;
                Y++;

                // if we are wrapping around, set that this row is wrapping
                pRow->GetCharRow().SetWrapForced(true);

                if (Y >= coordScreenBufferSize.Y)
                {
                    break;  // abandon output, string is truncated
                }
            }
            else
            {
                // if we're not wrapping around, set that this row isn't wrapped.
                pRow->GetCharRow().SetWrapForced(false);
                break;
            }

            try
            {
                pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }
        }
    }
    else if (ulStringType == CONSOLE_ATTRIBUTE)
    {
        PWORD SourcePtr = (PWORD) pvBuffer;
        UINT uiScreenBufferWidth = coordScreenBufferSize.X;
        TextAttributeRun* rAttrRunsBuff = new TextAttributeRun[uiScreenBufferWidth];
        if (rAttrRunsBuff == nullptr) return STATUS_NO_MEMORY;
        TextAttributeRun* pAttrRun = rAttrRunsBuff;
        SHORT NumAttrRuns;


        for (;;)
        {
            if (pRow == nullptr)
            {
                ASSERT(false);
                break;
            }

            // copy the attrs into the screen buffer arrays
            pAttrRun = rAttrRunsBuff; // for each row, return to the start of the buffer
            pAttrRun->SetLength(0);
            pAttrRun->SetAttributesFromLegacy(*SourcePtr & ~COMMON_LVB_SBCSDBCS);
            NumAttrRuns = 1;
            for (SHORT j = X; j < coordScreenBufferSize.X; j++, SourcePtr++)
            {
                if (pAttrRun->GetAttributes().IsEqualToLegacy(*SourcePtr & ~COMMON_LVB_SBCSDBCS))
                {
                    pAttrRun->SetLength(pAttrRun->GetLength() + 1);
                }
                else
                {
                    pAttrRun++;
                    pAttrRun->SetLength(1);
                    pAttrRun->SetAttributesFromLegacy(*SourcePtr & ~COMMON_LVB_SBCSDBCS);
                    NumAttrRuns += 1;
                }
                NumWritten++;
                X++;
                if (NumWritten == *pcRecords)
                {
                    break;
                }
            }
            X--;

            // recalculate last non-space char

            pRow->GetAttrRow().InsertAttrRuns(rAttrRunsBuff,
                                              NumAttrRuns,
                                              (SHORT)((Y == coordWrite.Y) ? coordWrite.X : 0),
                                              X,
                                              coordScreenBufferSize.X);

            try
            {
                pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }

            if (NumWritten < *pcRecords)
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
        }
        pScreenInfo->ResetTextFlags(coordWrite.X, coordWrite.Y, X, Y);

        delete[] rAttrRunsBuff;
    }
    else
    {
        *pcRecords = 0;
        return STATUS_INVALID_PARAMETER;
    }
    if ((ulStringType == CONSOLE_ASCII) && (TransBuffer != nullptr))
    {
        if (fLocalHeap)
        {
            delete[] TransBuffer;
            delete[] TransBufferA;
        }
    }
    else if ((ulStringType == CONSOLE_FALSE_UNICODE) || (ulStringType == CONSOLE_REAL_UNICODE))
    {
        if (fLocalHeap)
        {
            delete[] TransBuffer;
            delete[] TransBufferA;
        }
        NumWritten = NumRecordsSavedForUnicode - (*pcRecords - NumWritten);
    }

    // determine write region.  if we're still on the same line we started
    // on, left X is the X we started with and right X is the one we're on
    // now.  otherwise, left X is 0 and right X is the rightmost column of
    // the screen buffer.
    //
    // then update the screen.
    SMALL_RECT WriteRegion;
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
    WriteToScreen(pScreenInfo, WriteRegion);
    if (pcColumns)
    {
        *pcColumns = X + (coordWrite.Y - Y) * coordScreenBufferSize.X - coordWrite.X + 1;
    }
    *pcRecords = NumWritten;
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine fills the screen buffer with the specified character or attribute.
// Arguments:
// - pScreenInfo - Pointer to screen buffer information.
// - wElement - Element to write.
// - coordWrite - Screen buffer coordinate to begin writing to.
// - ulElementType
//      CONSOLE_ASCII         - element is an ascii character.
//      CONSOLE_REAL_UNICODE  - element is a real unicode character. These will get converted to False Unicode as required.
//      CONSOLE_FALSE_UNICODE - element is a False Unicode character.
//      CONSOLE_ATTRIBUTE     - element is an attribute.
// - pcElements - On input, the number of elements to write.  On output, the number of elements written.
// Return Value:
NTSTATUS FillOutput(_In_ PSCREEN_INFORMATION pScreenInfo,
                    _In_ WORD wElement,
                    _In_ const COORD coordWrite,
                    _In_ const ULONG ulElementType,
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
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (X >= coordScreenBufferSize.X || X < 0 || Y >= coordScreenBufferSize.Y || Y < 0)
    {
        *pcElements = 0;
        return STATUS_SUCCESS;
    }

    if (ulElementType == CONSOLE_ASCII)
    {
        UINT const Codepage = gci.OutputCP;
        if (pScreenInfo->FillOutDbcsLeadChar == 0)
        {
            if (IsDBCSLeadByteConsole((CHAR) wElement, &gci.OutputCPInfo))
            {
                pScreenInfo->FillOutDbcsLeadChar = (CHAR) wElement;
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

            CharTmp[0] = pScreenInfo->FillOutDbcsLeadChar;
            CharTmp[1] = (BYTE) wElement;

            pScreenInfo->FillOutDbcsLeadChar = 0;
            ConvertOutputToUnicode(Codepage, CharTmp, 2, (WCHAR*)&wElement, 1);
        }
    }

    ROW* pRow = &pScreenInfo->TextInfo->GetRowByOffset(coordWrite.Y);
    if (ulElementType == CONSOLE_ASCII ||
        ulElementType == CONSOLE_REAL_UNICODE ||
        ulElementType == CONSOLE_FALSE_UNICODE)
    {
        DWORD StartPosFlag = 0;
        for (;;)
        {
            if (pRow == nullptr)
            {
                ASSERT(false);
                break;
            }

            // copy the chars into their arrays
            Ucs2CharRow::iterator it;
            try
            {
                ICharRow& iCharRow = pRow->GetCharRow();
                if (iCharRow.GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
                {
                    Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
                    it = std::next(charRow.begin(), X);
                }
                else
                {
                    return STATUS_UNSUCCESSFUL;
                }
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
                    CleanupDbcsEdgesForWrite((SHORT)(*pcElements - NumWritten), TPoint, pScreenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        it->first = static_cast<wchar_t>(wElement);
                        if (StartPosFlag++ & 1)
                        {
                            it->second.SetTrailing();
                        }
                        else
                        {
                            it->second.SetLeading();
                        }
                        ++it;
                    }

                    if (StartPosFlag & 1)
                    {
                        (it - 1)->first = UNICODE_SPACE;
                        (it - 1)->second.SetSingle();
                    }
                }
                else
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        it->first = static_cast<wchar_t>(wElement);
                        it->second.SetSingle();
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
                    CleanupDbcsEdgesForWrite((SHORT)(coordScreenBufferSize.X - X), TPoint, pScreenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < coordScreenBufferSize.X - X; j++)
                    {
                        it->first = static_cast<wchar_t>(wElement);
                        if (StartPosFlag++ & 1)
                        {
                            it->second.SetTrailing();
                        }
                        else
                        {
                            it->second.SetLeading();
                        }
                        ++it;
                    }
                }
                else
                {
                    for (SHORT j = 0; j < coordScreenBufferSize.X - X; j++)
                    {
                        it->first = static_cast<wchar_t>(wElement);
                        it->second.SetSingle();
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
                pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
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
            if (pRow == nullptr)
            {
                ASSERT(false);
                break;
            }

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
            if (pScreenInfo->InVTMode() && pScreenInfo->GetAttributes().GetLegacyAttributes() == wElement)
            {
                AttrRun.SetAttributes(pScreenInfo->GetAttributes());
            }
            else
            {
                WORD wActual = wElement;
                ClearAllFlags(wActual, COMMON_LVB_SBCSDBCS);
                AttrRun.SetAttributesFromLegacy(wActual);
            }

            pRow->GetAttrRow().InsertAttrRuns(&AttrRun, 1, (SHORT)(X - AttrRun.GetLength() + 1), X, coordScreenBufferSize.X);

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
                pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
            }
            catch (...)
            {
                pRow = nullptr;
            }
        }

        pScreenInfo->ResetTextFlags(coordWrite.X, coordWrite.Y, X, Y);
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
    if (pScreenInfo->ConvScreenInfo)
    {
        WriteRegion.Top = coordWrite.Y + pScreenInfo->GetBufferViewport().Left + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        WriteRegion.Bottom = Y + pScreenInfo->GetBufferViewport().Left + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        if (Y != coordWrite.Y)
        {
            WriteRegion.Left = 0;
            WriteRegion.Right = (SHORT)(gci.CurrentScreenBuffer->GetScreenBufferSize().X - 1);
        }
        else
        {
            WriteRegion.Left = coordWrite.X + pScreenInfo->GetBufferViewport().Top + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
            WriteRegion.Right = X + pScreenInfo->GetBufferViewport().Top + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
        }
        WriteConvRegionToScreen(gci.CurrentScreenBuffer, WriteRegion);
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

    WriteToScreen(pScreenInfo, WriteRegion);
    *pcElements = NumWritten;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine fills a rectangular region in the screen buffer.  no clipping is done.
// Arguments:
// - pciFill - element to copy to each element in target rect
// - pScreenInfo - pointer to screen info
// - psrTarget - rectangle in screen buffer to fill
// Return Value:
void FillRectangle(_In_ const CHAR_INFO * const pciFill,
                   _In_ PSCREEN_INFORMATION pScreenInfo,
                   _In_ const SMALL_RECT * const psrTarget)
{
    DBGOUTPUT(("FillRectangle\n"));

    SHORT XSize = (SHORT)(psrTarget->Right - psrTarget->Left + 1);

    ROW* pRow = &pScreenInfo->TextInfo->GetRowByOffset(psrTarget->Top);
    for (SHORT i = psrTarget->Top; i <= psrTarget->Bottom; i++)
    {
        if (pRow == nullptr)
        {
            ASSERT(false);
            break;
        }

        // Copy the chars and attrs into their respective arrays.
        COORD TPoint;

        TPoint.X = psrTarget->Left;
        TPoint.Y = i;
        CleanupDbcsEdgesForWrite(XSize, TPoint, pScreenInfo);
        BOOL Width = IsCharFullWidth(pciFill->Char.UnicodeChar);

        Ucs2CharRow::iterator it;
        Ucs2CharRow::const_iterator itEnd;
        try
        {
            ICharRow& iCharRow = pRow->GetCharRow();
            if (iCharRow.GetSupportedEncoding() == ICharRow::SupportedEncoding::Ucs2)
            {
                Ucs2CharRow& charRow = static_cast<Ucs2CharRow&>(iCharRow);
                it = std::next(charRow.begin(), psrTarget->Left);
                itEnd = charRow.cend();
            }
            else
            {
                LOG_HR_MSG(E_FAIL, "we don't support non UCS2 encoded char rows");
                return;
            }
        }
        catch (...)
        {
            LOG_HR(wil::ResultFromCaughtException());
            return;
        }

        for (SHORT j = 0; j < XSize && it < itEnd; j++)
        {
            if (Width)
            {
                if (j < XSize - 1)
                {
                    // we set leading an trailing in pairs so make sure that checking against the end of the
                    // iterator won't be off by 1
                    assert((itEnd - it) % 2 == 0);
                    assert((itEnd - it) >= 2);

                    it->first = pciFill->Char.UnicodeChar;
                    it->second.SetLeading();
                    ++it;

                    it->first = pciFill->Char.UnicodeChar;
                    it->second.SetTrailing();
                    ++it;

                    ++j;
                }
                else
                {
                    it->first = UNICODE_NULL;
                    it->second.SetSingle();
                    ++it;
                }
            }
            else
            {
                it->first = pciFill->Char.UnicodeChar;
                it->second.SetSingle();
                ++it;
            }
        }

        const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();

        TextAttributeRun AttrRun;
        AttrRun.SetLength(XSize);
        AttrRun.SetAttributesFromLegacy(pciFill->Attributes);

        pRow->GetAttrRow().InsertAttrRuns(&AttrRun, 1, psrTarget->Left, psrTarget->Right, coordScreenBufferSize.X);

        // invalidate row wrapping for rectangular drawing
        pRow->GetCharRow().SetWrapForced(false);

        try
        {
            pRow = &pScreenInfo->TextInfo->GetNextRowNoWrap(*pRow);
        }
        catch (...)
        {
            pRow = nullptr;
        }
    }

    pScreenInfo->ResetTextFlags(psrTarget->Left, psrTarget->Top, psrTarget->Right, psrTarget->Bottom);
}
