/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_output.h"
#include "output.h"

#include "clipboard.hpp"
#include "cursor.h"
#include "dbcs.h"
#include "globals.h"
#include "misc.h"

#pragma hdrstop

#define MAX_POLY_LINES 80

#define WCHAR_OF_PCI(p) (((PCHAR_INFO)(p))->Char.UnicodeChar)
#define ATTR_OF_PCI(p)  (((PCHAR_INFO)(p))->Attributes)
#define SIZEOF_CI_CELL  sizeof(CHAR_INFO)

typedef struct _KEISEN_INFORMATION
{
    COORD Coord;
    WORD n;
} KEISEN_INFORMATION, *PKEISEN_INFORMATION;

void StreamWriteToScreenBuffer(_Inout_updates_(cchBuffer) PWCHAR pwchBuffer,
                               _In_ SHORT cchBuffer,
                               _In_ PSCREEN_INFORMATION pScreenInfo,
                               _Inout_updates_(cchBuffer) PCHAR const pchBuffer,
                               _In_ const bool fWasLineWrapped)
{
    DBGOUTPUT(("StreamWriteToScreenBuffer\n"));
    COORD const TargetPoint = pScreenInfo->TextInfo->GetCursor()->GetPosition();
    ROW* const Row = pScreenInfo->TextInfo->GetRowByOffset(TargetPoint.Y);
    DBGOUTPUT(("Row = 0x%p, TargetPoint = (0x%x,0x%x)\n", Row, TargetPoint.X, TargetPoint.Y));

    // TODO: from BisectWrite to the end of the if statement seems to never execute
    // both callers of this function appear to already have handled the line length for double-width characters
    BisectWrite(cchBuffer, TargetPoint, pScreenInfo);
    if (TargetPoint.Y == pScreenInfo->ScreenBufferSize.Y - 1 &&
        TargetPoint.X + cchBuffer >= pScreenInfo->ScreenBufferSize.X &&
        *(pchBuffer + pScreenInfo->ScreenBufferSize.X - TargetPoint.X - 1) & CHAR_ROW::ATTR_LEADING_BYTE)
    {
        *(pwchBuffer + pScreenInfo->ScreenBufferSize.X - TargetPoint.X - 1) = UNICODE_SPACE;
        *(pchBuffer + pScreenInfo->ScreenBufferSize.X - TargetPoint.X - 1) = 0;
        if (cchBuffer > pScreenInfo->ScreenBufferSize.X - TargetPoint.X)
        {
            *(pwchBuffer + pScreenInfo->ScreenBufferSize.X - TargetPoint.X) = UNICODE_SPACE;
            *(pchBuffer + pScreenInfo->ScreenBufferSize.X - TargetPoint.X) = 0;
        }
    }

    // copy chars
    memmove(&Row->CharRow.KAttrs[TargetPoint.X], pchBuffer, cchBuffer * sizeof(CHAR));

    memmove(&Row->CharRow.Chars[TargetPoint.X], pwchBuffer, cchBuffer * sizeof(WCHAR));

    Row->CharRow.SetWrapStatus(fWasLineWrapped); // caller knows the wrap status as this func is called only for drawing one line at a time

    // recalculate first and last non-space char
    if (TargetPoint.X < Row->CharRow.Left)
    {
        Row->CharRow.MeasureAndSaveLeft(pScreenInfo->ScreenBufferSize.X);
    }

    if ((TargetPoint.X + cchBuffer) >= Row->CharRow.Right)
    {
        Row->CharRow.MeasureAndSaveRight(pScreenInfo->ScreenBufferSize.X);
    }
    TextAttributeRun CurrentBufferAttrs;
    CurrentBufferAttrs.SetLength(cchBuffer);
    CurrentBufferAttrs.SetAttributes(pScreenInfo->GetAttributes());
    Row->AttrRow.InsertAttrRuns(&CurrentBufferAttrs, 1, TargetPoint.X, (SHORT)(TargetPoint.X + cchBuffer - 1), pScreenInfo->ScreenBufferSize.X);

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

        ROW* Row = pScreenInfo->TextInfo->GetRowByOffset(coordDest.Y);
        for (SHORT i = 0; i < YSize; i++)
        {
            // ensure we have a valid row pointer, if not, skip.
            if (Row == nullptr)
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
            BisectWrite(XSize, TPoint, pScreenInfo);

            if (TPoint.Y == pScreenInfo->ScreenBufferSize.Y - 1 &&
                TPoint.X + XSize - 1 >= pScreenInfo->ScreenBufferSize.X)
            {
                const BYTE* const pbLeadingByte = SourcePtr + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1;
                if (((pbLeadingByte + sizeof(CHAR_INFO)) > pbSourceEnd) || (pbLeadingByte < prgbSrc))
                {
                    ASSERT(false);
                    return STATUS_BUFFER_OVERFLOW;
                }

                if (ATTR_OF_PCI(pbLeadingByte) & COMMON_LVB_LEADING_BYTE)
                {
                    WCHAR_OF_PCI(pbLeadingByte) = UNICODE_SPACE;
                    ATTR_OF_PCI(pbLeadingByte) &= ~COMMON_LVB_SBCSDBCS;
                    if (XSize - 1 > pScreenInfo->ScreenBufferSize.X - TPoint.X - 1)
                    {
                        const BYTE* const pbTrailingByte = SourcePtr + pScreenInfo->ScreenBufferSize.X - TPoint.X;
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

            WCHAR* Char = &Row->CharRow.Chars[coordDest.X];
            CHAR* AttrP = (CHAR*)&Row->CharRow.KAttrs[coordDest.X]; // CJK Languages (FE)

            TextAttributeRun* pAttrRun = rAttrRunsBuff;
            pAttrRun->SetLength(0);
            SHORT NumAttrRuns = 1;

            pAttrRun->SetAttributesFromLegacy(ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS);

            Row->CharRow.SetWrapStatus(false); // clear wrap status for rectangle drawing

            for (SHORT j = psrSrc->Left; j <= psrSrc->Right; j++, SourcePtr += SIZEOF_CI_CELL)
            {
                if (SourcePtr + sizeof(CHAR_INFO) > pbSourceEnd || SourcePtr < prgbSrc)
                {
                    ASSERT(false);
                    return STATUS_BUFFER_OVERFLOW;
                }

                *Char++ = WCHAR_OF_PCI(SourcePtr);
                // MSKK Apr.02.1993 V-HirotS For KAttr
                *AttrP++ = (CHAR) ((ATTR_OF_PCI(SourcePtr) & COMMON_LVB_SBCSDBCS) >> 8);

                if (pAttrRun->GetAttributes()->IsEqualToLegacy((ATTR_OF_PCI(SourcePtr) & ~COMMON_LVB_SBCSDBCS)))
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

            // recalculate first and last non-space char
            if (coordDest.X < Row->CharRow.Left)
            {
                PWCHAR LastChar = &Row->CharRow.Chars[pScreenInfo->ScreenBufferSize.X];

                for (Char = &Row->CharRow.Chars[coordDest.X]; Char < LastChar && *Char == (WCHAR)' '; Char++)
                {
                    /* do nothing */ ;
                }
                Row->CharRow.Left = (SHORT)(Char - Row->CharRow.Chars);
            }

            if ((coordDest.X + XSize) >= Row->CharRow.Right)
            {
                SHORT LastNonSpace;
                PWCHAR FirstChar = Row->CharRow.Chars;

                LastNonSpace = (SHORT)(coordDest.X + XSize - 1);
                for (Char = &Row->CharRow.Chars[(coordDest.X + XSize - 1)]; *Char == (WCHAR)' ' && Char >= FirstChar; Char--)
                    LastNonSpace--;

                // if the attributes change after the last non-space, make the
                // index of the last attribute change + 1 the length.  otherwise
                // make the length one more than the last non-space.
                Row->CharRow.Right = (SHORT)(LastNonSpace + 1);
            }

            Row->AttrRow.InsertAttrRuns(rAttrRunsBuff, NumAttrRuns, coordDest.X, (SHORT)(coordDest.X + XSize - 1), pScreenInfo->ScreenBufferSize.X);

            Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);
        }
        pScreenInfo->ResetTextFlags(coordDest.X, coordDest.Y, (SHORT)(coordDest.X + XSize - 1), (SHORT)(coordDest.Y + YSize - 1));

        // The above part of the code seems ridiculous in terms of complexity. especially the dbcs stuff.
        // So we'll copy the attributes here in a not terrible way.

        if (fWriteAttributes)
        {
            TextAttribute* pSrcAttr = &(pTextAttributes[(psrSrc->Top * coordSrcDimensions.X) + psrSrc->Left]);
            int rowWidth = pScreenInfo->ScreenBufferSize.X;
            int srcWidth = psrSrc->Right - psrSrc->Left;
            for (int y = coordDest.Y; y < coordSrcDimensions.Y + coordDest.Y; y++)
            {
                ROW* pRow = pScreenInfo->TextInfo->GetRowByOffset(y);
                TextAttribute* rTargetAttributes = new TextAttribute[rowWidth];
                NTSTATUS Status = NT_TESTNULL(rTargetAttributes);
                if (SUCCEEDED(Status))
                {
                    Status = pRow->AttrRow.UnpackAttrs(rTargetAttributes, rowWidth);
                    if (SUCCEEDED(Status))
                    {
                        for (int x = coordDest.X; x < srcWidth + coordDest.X; x++)
                        {
                            rTargetAttributes[x].SetFrom(pSrcAttr);
                            pSrcAttr++; // advance to next attr
                        }
                        pRow->AttrRow.PackAttrs(rTargetAttributes, rowWidth);
                    }

                    delete[] rTargetAttributes;
                    pSrcAttr++; // advance to next row.
                }
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
        if (g_pRender != nullptr)
        {
            // convert inclusive rectangle to exclusive rectangle
            SMALL_RECT srExclusive = *psrRegion;
            srExclusive.Right++;
            srExclusive.Bottom++;

            g_pRender->TriggerRedraw(&srExclusive);
        }
    }
}

// Routine Description:
// - This routine writes a screen buffer region to the screen.
// Arguments:
// - pScreenInfo - Pointer to screen buffer information.
// - psrRegion - Region to write in screen buffer coordinates.  Region is inclusive
// Return Value:
// - <none>
void WriteToScreen(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT * const psrRegion)
{
    DBGOUTPUT(("WriteToScreen\n"));
    // update to screen, if we're not iconic.
    if (!pScreenInfo->IsActiveScreenBuffer() || (g_ciConsoleInformation.Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW)))
    {
        return;
    }

    // clip region
    SMALL_RECT ClippedRegion;
    ClippedRegion.Left = max(psrRegion->Left, pScreenInfo->BufferViewport.Left);
    ClippedRegion.Top = max(psrRegion->Top, pScreenInfo->BufferViewport.Top);
    ClippedRegion.Right = min(psrRegion->Right, pScreenInfo->BufferViewport.Right);
    ClippedRegion.Bottom = min(psrRegion->Bottom, pScreenInfo->BufferViewport.Bottom);
    if (ClippedRegion.Right < ClippedRegion.Left || ClippedRegion.Bottom < ClippedRegion.Top)
    {
        return;
    }

    WriteRegionToScreen(pScreenInfo, &ClippedRegion);

    WriteConvRegionToScreen(pScreenInfo, psrRegion);
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

    if (*pcRecords == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumWritten = 0;
    ULONG NumRecordsSavedForUnicode = 0;
    SHORT X = coordWrite.X;
    SHORT Y = coordWrite.Y;
    if (X >= pScreenInfo->ScreenBufferSize.X || X < 0 || Y >= pScreenInfo->ScreenBufferSize.Y || Y < 0)
    {
        *pcRecords = 0;
        return STATUS_SUCCESS;
    }

    WCHAR rgwchBuffer[128] = {0};
    BYTE rgbBufferA[128] = {0};

    PBYTE BufferA = nullptr;
    PWCHAR TransBuffer = nullptr;
    PBYTE TransBufferA = nullptr;
    BOOL fLocalHeap = FALSE;
    if (ulStringType == CONSOLE_ASCII)
    {
        UINT const Codepage = g_ciConsoleInformation.OutputCP;

        if (*pcRecords >= ARRAYSIZE(rgwchBuffer) ||
            *pcRecords >= ARRAYSIZE(rgbBufferA))
        {

            TransBuffer = new WCHAR[*pcRecords * 2];
            if (TransBuffer == nullptr)
            {
                return STATUS_NO_MEMORY;
            }
            TransBufferA = new BYTE[*pcRecords * 2];
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
        PCHAR TmpTransA = (PCHAR)TransBufferA;   // MSKK Apr.02.1993 V-HirotS For KAttr
        for (ULONG i = 0; i < *pcRecords;)
        {
            if (TmpBuf >= pchTmpBufEnd)
            {
                ASSERT(false);
                break;
            }

            if (IsDBCSLeadByteConsole(*TmpBuf, &g_ciConsoleInformation.OutputCPInfo))
            {
                if (i + 1 >= *pcRecords)
                {
                    *TmpTrans = UNICODE_SPACE;
                    *TmpTransA = 0;
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
                    *TmpTransA++ = CHAR_ROW::ATTR_LEADING_BYTE;
                    *TmpTransA++ = CHAR_ROW::ATTR_TRAILING_BYTE;
                    i += 2;
                }
            }
            else
            {
                ConvertOutputToUnicode(Codepage, TmpBuf, 1, TmpTrans, 1);
                TmpTrans++;
                TmpBuf++;
                *TmpTransA++ = 0;   // MSKK APr.02.1993 V-HirotS For KAttr
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
        PCHAR TmpTransA;
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

            TransBufferA = new BYTE[*pcRecords * 2];
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
        TmpTransA = (PCHAR) TransBufferA;
        for (i = 0, jTmp = 0; i < *pcRecords; i++, jTmp++)
        {
            *TmpTrans++ = c = *TmpBuf++;
            *TmpTransA = 0;
            if (IsCharFullWidth(c))
            {
                *TmpTransA++ = CHAR_ROW::ATTR_LEADING_BYTE;
                *TmpTrans++ = c;
                *TmpTransA = CHAR_ROW::ATTR_TRAILING_BYTE;
                jTmp++;
            }

            TmpTransA++;
        }

        NumRecordsSavedForUnicode = *pcRecords;
        *pcRecords = jTmp;
        pvBuffer = TransBuffer;
        BufferA = TransBufferA;
    }

    ROW* Row = pScreenInfo->TextInfo->GetRowByOffset(coordWrite.Y);
    if ((ulStringType == CONSOLE_REAL_UNICODE) || (ulStringType == CONSOLE_FALSE_UNICODE) || (ulStringType == CONSOLE_ASCII))
    {
        for (;;)
        {
            if (Row == nullptr)
            {
                ASSERT(false);
                break;
            }

            SHORT LeftX = X;

            // copy the chars into their arrays
            PWCHAR Char = &Row->CharRow.Chars[X];
            PBYTE AttrP = &Row->CharRow.KAttrs[X];
            if ((ULONG)(pScreenInfo->ScreenBufferSize.X - X) >= (*pcRecords - NumWritten))
            {
                // The text will not hit the right hand edge, copy it all
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                BisectWrite((SHORT)(*pcRecords - NumWritten), TPoint, pScreenInfo);

                if (TPoint.Y == pScreenInfo->ScreenBufferSize.Y - 1 &&
                    (SHORT)(TPoint.X + *pcRecords - NumWritten) >= pScreenInfo->ScreenBufferSize.X &&
                    *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) & CHAR_ROW::ATTR_LEADING_BYTE)
                {
                    *((PWCHAR)pvBuffer + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) = UNICODE_SPACE;
                    *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) = 0;
                    if ((SHORT)(*pcRecords - NumWritten) > (SHORT)(pScreenInfo->ScreenBufferSize.X - TPoint.X - 1))
                    {
                        #pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "ScreenBufferSize limits this, but it's very obtuse.")
                        *((PWCHAR)pvBuffer + pScreenInfo->ScreenBufferSize.X - TPoint.X) = UNICODE_SPACE;
                        #pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "ScreenBufferSize limits this, but it's very obtuse.")
                        *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X) = 0;
                    }
                }

                memmove(AttrP, BufferA, (*pcRecords - NumWritten) * sizeof(CHAR));
                #pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Code appears to be fine.")
                memmove(Char, pvBuffer, (*pcRecords - NumWritten) * sizeof(WCHAR));

                X = (SHORT)(X + *pcRecords - NumWritten - 1);
                NumWritten = *pcRecords;
            }
            else
            {
                // The text will hit the right hand edge, copy only that much.
                COORD TPoint;

                TPoint.X = X;
                TPoint.Y = Y;
                BisectWrite((SHORT)(pScreenInfo->ScreenBufferSize.X - X), TPoint, pScreenInfo);
                if (TPoint.Y == pScreenInfo->ScreenBufferSize.Y - 1 &&
                    TPoint.X + pScreenInfo->ScreenBufferSize.X - X >= pScreenInfo->ScreenBufferSize.X &&
                    *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) & CHAR_ROW::ATTR_LEADING_BYTE)
                {
                    *((PWCHAR)pvBuffer + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) = UNICODE_SPACE;
                    *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X - 1) = 0;
                    if (pScreenInfo->ScreenBufferSize.X - X > pScreenInfo->ScreenBufferSize.X - TPoint.X - 1)
                    {
                        *((PWCHAR)pvBuffer + pScreenInfo->ScreenBufferSize.X - TPoint.X) = UNICODE_SPACE;
                        *((PCHAR)BufferA + pScreenInfo->ScreenBufferSize.X - TPoint.X) = 0;
                    }
                }
                memmove(AttrP, BufferA, (pScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR));
                BufferA = (PBYTE)((PBYTE)BufferA + ((pScreenInfo->ScreenBufferSize.X - X) * sizeof(CHAR)));
                memmove(Char, pvBuffer, (pScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR));
                pvBuffer = (PVOID)((PBYTE)pvBuffer + ((pScreenInfo->ScreenBufferSize.X - X) * sizeof(WCHAR)));
                NumWritten += pScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(pScreenInfo->ScreenBufferSize.X - 1);
            }

            // recalculate first and last non-space char
            if (LeftX < Row->CharRow.Left)
            {
                Row->CharRow.MeasureAndSaveLeft(pScreenInfo->ScreenBufferSize.X);
            }

            if ((X + 1) >= Row->CharRow.Right)
            {
                Row->CharRow.MeasureAndSaveRight(pScreenInfo->ScreenBufferSize.X);
            }

            if (NumWritten < *pcRecords)
            {
                // The string hit the right hand edge, so wrap around to the next line by going back round the while loop, unless we
                // are at the end of the buffer - in which case we simply abandon the remainder of the output string!
                X = 0;
                Y++;

                // if we are wrapping around, set that this row is wrapping
                Row->CharRow.SetWrapStatus(true);

                if (Y >= pScreenInfo->ScreenBufferSize.Y)
                {
                    break;  // abandon output, string is truncated
                }
            }
            else
            {
                // if we're not wrapping around, set that this row isn't wrapped.
                Row->CharRow.SetWrapStatus(false);
                break;
            }

            Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);
        }
    }
    else if (ulStringType == CONSOLE_ATTRIBUTE)
    {
        PWORD SourcePtr = (PWORD) pvBuffer;
        UINT uiScreenBufferWidth = pScreenInfo->ScreenBufferSize.X;
        TextAttributeRun* rAttrRunsBuff = new TextAttributeRun[uiScreenBufferWidth];
        if (rAttrRunsBuff == nullptr) return STATUS_NO_MEMORY;
        TextAttributeRun* pAttrRun = rAttrRunsBuff;
        SHORT NumAttrRuns;


        for (;;)
        {
            if (Row == nullptr)
            {
                ASSERT(false);
                break;
            }

            // copy the attrs into the screen buffer arrays
            pAttrRun = rAttrRunsBuff; // for each row, return to the start of the buffer
            pAttrRun->SetLength(0);
            pAttrRun->SetAttributesFromLegacy(*SourcePtr & ~COMMON_LVB_SBCSDBCS);
            NumAttrRuns = 1;
            for (SHORT j = X; j < pScreenInfo->ScreenBufferSize.X; j++, SourcePtr++)
            {
                if (pAttrRun->GetAttributes()->IsEqualToLegacy(*SourcePtr & ~COMMON_LVB_SBCSDBCS))
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

            Row->AttrRow.InsertAttrRuns(rAttrRunsBuff, NumAttrRuns, (SHORT)((Y == coordWrite.Y) ? coordWrite.X : 0), X, pScreenInfo->ScreenBufferSize.X);

            Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);

            if (NumWritten < *pcRecords)
            {
                X = 0;
                Y++;
                if (Y >= pScreenInfo->ScreenBufferSize.Y)
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
        WriteRegion.Right = (SHORT)(pScreenInfo->ScreenBufferSize.X - 1);
    }
    else
    {
        WriteRegion.Left = coordWrite.X;
        WriteRegion.Right = X;
    }
    WriteToScreen(pScreenInfo, &WriteRegion);
    if (pcColumns)
    {
        *pcColumns = X + (coordWrite.Y - Y) * pScreenInfo->ScreenBufferSize.X - coordWrite.X + 1;
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
    if (*pcElements == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumWritten = 0;
    SHORT X = coordWrite.X;
    SHORT Y = coordWrite.Y;
    if (X >= pScreenInfo->ScreenBufferSize.X || X < 0 || Y >= pScreenInfo->ScreenBufferSize.Y || Y < 0)
    {
        *pcElements = 0;
        return STATUS_SUCCESS;
    }

    if (ulElementType == CONSOLE_ASCII)
    {
        UINT const Codepage = g_ciConsoleInformation.OutputCP;
        if (pScreenInfo->FillOutDbcsLeadChar == 0)
        {
            if (IsDBCSLeadByteConsole((CHAR) wElement, &g_ciConsoleInformation.OutputCPInfo))
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

    ROW* Row = pScreenInfo->TextInfo->GetRowByOffset(coordWrite.Y);
    if ((ulElementType == CONSOLE_ASCII) || (ulElementType == CONSOLE_REAL_UNICODE) || (ulElementType == CONSOLE_FALSE_UNICODE))
    {
        DWORD StartPosFlag = 0;
        for (;;)
        {
            if (Row == nullptr)
            {
                ASSERT(false);
                break;
            }

            // copy the chars into their arrays
            SHORT LeftX = X;
            PWCHAR Char = &Row->CharRow.Chars[X];
            PCHAR AttrP = (PCHAR) & Row->CharRow.KAttrs[X];
            if ((ULONG) (pScreenInfo->ScreenBufferSize.X - X) >= (*pcElements - NumWritten))
            {
                {
                    COORD TPoint;

                    TPoint.X = X;
                    TPoint.Y = Y;
                    BisectWrite((SHORT)(*pcElements - NumWritten), TPoint, pScreenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        *Char++ = (WCHAR)wElement;
                        *AttrP &= ~CHAR_ROW::ATTR_DBCSSBCS_BYTE;
                        if (StartPosFlag++ & 1)
                        {
                            *AttrP++ |= CHAR_ROW::ATTR_TRAILING_BYTE;
                        }
                        else
                        {
                            *AttrP++ |= CHAR_ROW::ATTR_LEADING_BYTE;
                        }
                    }

                    if (StartPosFlag & 1)
                    {
                        *(Char - 1) = UNICODE_SPACE;
                        *(AttrP - 1) &= ~CHAR_ROW::ATTR_DBCSSBCS_BYTE;
                    }
                }
                else
                {
                    for (SHORT j = 0; j < (SHORT)(*pcElements - NumWritten); j++)
                    {
                        *Char++ = (WCHAR)wElement;
                        *AttrP++ &= ~CHAR_ROW::ATTR_DBCSSBCS_BYTE;
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
                    BisectWrite((SHORT)(pScreenInfo->ScreenBufferSize.X - X), TPoint, pScreenInfo);
                }
                if (IsCharFullWidth((WCHAR)wElement))
                {
                    for (SHORT j = 0; j < pScreenInfo->ScreenBufferSize.X - X; j++)
                    {
                        *Char++ = (WCHAR)wElement;
                        *AttrP &= ~CHAR_ROW::ATTR_DBCSSBCS_BYTE;
                        if (StartPosFlag++ & 1)
                        {
                            *AttrP++ |= CHAR_ROW::ATTR_TRAILING_BYTE;
                        }
                        else
                        {
                            *AttrP++ |= CHAR_ROW::ATTR_LEADING_BYTE;
                        }
                    }
                }
                else
                {
                    for (SHORT j = 0; j < pScreenInfo->ScreenBufferSize.X - X; j++)
                    {
                        *Char++ = (WCHAR)wElement;
                        *AttrP++ &= ~CHAR_ROW::ATTR_DBCSSBCS_BYTE;
                    }
                }
                NumWritten += pScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(pScreenInfo->ScreenBufferSize.X - 1);
            }

            // recalculate first and last non-space char
            if (LeftX < Row->CharRow.Left)
            {
                Row->CharRow.MeasureAndSaveLeft(pScreenInfo->ScreenBufferSize.X);
            }

            if ((X + 1) >= Row->CharRow.Right)
            {
                Row->CharRow.MeasureAndSaveRight(pScreenInfo->ScreenBufferSize.X);
            }

            // invalidate row wrap status for any bulk fill of text characters
            Row->CharRow.SetWrapStatus(false);

            if (NumWritten < *pcElements)
            {
                X = 0;
                Y++;
                if (Y >= pScreenInfo->ScreenBufferSize.Y)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);
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
            if (Row == nullptr)
            {
                ASSERT(false);
                break;
            }

            // Copy the attrs into the screen buffer arrays.
            if ((ULONG) (pScreenInfo->ScreenBufferSize.X - X) >= (*pcElements - NumWritten))
            {
                X = (SHORT)(X + *pcElements - NumWritten - 1);
                NumWritten = *pcElements;
            }
            else
            {
                NumWritten += pScreenInfo->ScreenBufferSize.X - X;
                X = (SHORT)(pScreenInfo->ScreenBufferSize.X - 1);
            }

            // Recalculate the last non-space char and merge the two
            // attribute strings.
            AttrRun.SetLength((SHORT)((Y == coordWrite.Y) ? (X - coordWrite.X + 1) : (X + 1)));
            AttrRun.SetAttributesFromLegacy(wElement & ~COMMON_LVB_SBCSDBCS);

            Row->AttrRow.InsertAttrRuns(&AttrRun, 1, (SHORT)(X - AttrRun.GetLength() + 1), X, pScreenInfo->ScreenBufferSize.X);

            // leave row wrap status alone for any attribute fills
            if (NumWritten < *pcElements)
            {
                X = 0;
                Y++;
                if (Y >= pScreenInfo->ScreenBufferSize.Y)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);
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
        WriteRegion.Top = coordWrite.Y + pScreenInfo->BufferViewport.Left + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        WriteRegion.Bottom = Y + pScreenInfo->BufferViewport.Left + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.Y;
        if (Y != coordWrite.Y)
        {
            WriteRegion.Left = 0;
            WriteRegion.Right = (SHORT)(g_ciConsoleInformation.CurrentScreenBuffer->ScreenBufferSize.X - 1);
        }
        else
        {
            WriteRegion.Left = coordWrite.X + pScreenInfo->BufferViewport.Top + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
            WriteRegion.Right = X + pScreenInfo->BufferViewport.Top + pScreenInfo->ConvScreenInfo->CaInfo.coordConView.X;
        }
        WriteConvRegionToScreen(g_ciConsoleInformation.CurrentScreenBuffer, &WriteRegion);
        *pcElements = NumWritten;
        return STATUS_SUCCESS;
    }

    WriteRegion.Top = coordWrite.Y;
    WriteRegion.Bottom = Y;
    if (Y != coordWrite.Y)
    {
        WriteRegion.Left = 0;
        WriteRegion.Right = (SHORT)(pScreenInfo->ScreenBufferSize.X - 1);
    }
    else
    {
        WriteRegion.Left = coordWrite.X;
        WriteRegion.Right = X;
    }

    WriteToScreen(pScreenInfo, &WriteRegion);
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
void FillRectangle(_In_ const CHAR_INFO * const pciFill, _In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT * const psrTarget)
{
    DBGOUTPUT(("FillRectangle\n"));

    SHORT XSize = (SHORT)(psrTarget->Right - psrTarget->Left + 1);

    ROW* Row = pScreenInfo->TextInfo->GetRowByOffset(psrTarget->Top);
    for (SHORT i = psrTarget->Top; i <= psrTarget->Bottom; i++)
    {
        if (Row == nullptr)
        {
            ASSERT(false);
            break;
        }

        // Copy the chars and attrs into their respective arrays.
        COORD TPoint;

        TPoint.X = psrTarget->Left;
        TPoint.Y = i;
        BisectWrite(XSize, TPoint, pScreenInfo);
        BOOL Width = IsCharFullWidth(pciFill->Char.UnicodeChar);
        PWCHAR Char = &Row->CharRow.Chars[psrTarget->Left];
        PCHAR AttrP = (PCHAR) & Row->CharRow.KAttrs[psrTarget->Left];
        for (SHORT j = 0; j < XSize; j++)
        {
            if (Width)
            {
                if (j < XSize - 1)
                {
                    *Char++ = pciFill->Char.UnicodeChar;
                    *Char++ = pciFill->Char.UnicodeChar;
                    *AttrP++ = CHAR_ROW::ATTR_LEADING_BYTE;
                    *AttrP++ = CHAR_ROW::ATTR_TRAILING_BYTE;
                    j++;
                }
                else
                {
                    *Char++ = UNICODE_NULL;
                    *AttrP++ = 0;
                }
            }
            else
            {
                *Char++ = pciFill->Char.UnicodeChar;
                *AttrP++ = 0;
            }
        }

        // recalculate first and last non-space char

        if (psrTarget->Left < Row->CharRow.Left)
        {
            Row->CharRow.MeasureAndSaveLeft(pScreenInfo->ScreenBufferSize.X);
        }

        if (psrTarget->Right >= Row->CharRow.Right)
        {
            Row->CharRow.MeasureAndSaveRight(pScreenInfo->ScreenBufferSize.X);
        }

        TextAttributeRun AttrRun;
        AttrRun.SetLength(XSize);
        AttrRun.SetAttributesFromLegacy(pciFill->Attributes);
        
        Row->AttrRow.InsertAttrRuns(&AttrRun, 1, psrTarget->Left, psrTarget->Right, pScreenInfo->ScreenBufferSize.X);

        // invalidate row wrapping for rectangular drawing
        Row->CharRow.SetWrapStatus(false);

        Row = pScreenInfo->TextInfo->GetNextRowNoWrap(Row);
    }

    pScreenInfo->ResetTextFlags(psrTarget->Left, psrTarget->Top, psrTarget->Right, psrTarget->Bottom);
}
