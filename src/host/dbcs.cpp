/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "dbcs.h"

#include "misc.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// Routine Description:
// - This routine setup of line character code.
// Arguments:
// - pScreenInfo - Pointer to screen information structure.
// Return Value:
// - <none>
void SetLineChar(_In_ SCREEN_INFORMATION * const pScreenInfo)
{
    pScreenInfo->LineChar[UPPER_LEFT_CORNER] = 0x250c;
    pScreenInfo->LineChar[UPPER_RIGHT_CORNER] = 0x2510;
    pScreenInfo->LineChar[HORIZONTAL_LINE] = 0x2500;
    pScreenInfo->LineChar[VERTICAL_LINE] = 0x2502;
    pScreenInfo->LineChar[BOTTOM_LEFT_CORNER] = 0x2514;
    pScreenInfo->LineChar[BOTTOM_RIGHT_CORNER] = 0x2518;
}

// Routine Description:
// - This routine check bisected on Ascii string end.
// Arguments:
// - pchBuf - Pointer to Ascii string buffer.
// - cbBuf - Number of Ascii string.
// Return Value:
// - TRUE - Bisected character.
// - FALSE - Correctly.
bool CheckBisectStringA(_In_reads_bytes_(cbBuf) PCHAR pchBuf, _In_ DWORD cbBuf, _In_ const CPINFO * const pCPInfo)
{
    while (cbBuf)
    {
        if (IsDBCSLeadByteConsole(*pchBuf, pCPInfo))
        {
            if (cbBuf <= 1)
            {
                return true;
            }
            else
            {
                pchBuf += 2;
                cbBuf -= 2;
            }
        }
        else
        {
            pchBuf++;
            cbBuf--;
        }
    }

    return false;
}

// Routine Description:
// - This routine write buffer with bisect.
void BisectWrite(_In_ const SHORT sStringLen, _In_ const COORD coordTarget, _In_ PSCREEN_INFORMATION pScreenInfo)
{
    PTEXT_BUFFER_INFO const pTextInfo = pScreenInfo->TextInfo;

#if DBG && defined(DBG_KATTR)
    BeginKAttrCheck(pScreenInfo);
#endif

    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    SHORT const RowIndex = (pTextInfo->GetFirstRowIndex() + coordTarget.Y) % coordScreenBufferSize.Y;
    ROW* const Row = &pTextInfo->Rows[RowIndex];

    ROW* RowPrev;
    if (RowIndex > 0)
    {
        RowPrev = &pTextInfo->Rows[RowIndex - 1];
    }
    else
    {
        RowPrev = &pTextInfo->Rows[coordScreenBufferSize.Y - 1];
    }

    ROW* RowNext;
    if (RowIndex + 1 < coordScreenBufferSize.Y)
    {
        RowNext = &pTextInfo->Rows[RowIndex + 1];
    }
    else
    {
        RowNext = &pTextInfo->Rows[0];
    }

    if (Row->CharRow.KAttrs != nullptr)
    {
        // Check start position of strings
        if (Row->CharRow.KAttrs[coordTarget.X] & CHAR_ROW::ATTR_TRAILING_BYTE)
        {
            if (coordTarget.X == 0)
            {
                RowPrev->CharRow.Chars[coordScreenBufferSize.X - 1] = UNICODE_SPACE;
                RowPrev->CharRow.KAttrs[coordScreenBufferSize.X - 1] = 0;
            }
            else
            {
                Row->CharRow.Chars[coordTarget.X - 1] = UNICODE_SPACE;
                Row->CharRow.KAttrs[coordTarget.X - 1] = 0;
            }
        }

        // Check end position of strings
        if (coordTarget.X + sStringLen < coordScreenBufferSize.X)
        {
            if (Row->CharRow.KAttrs[coordTarget.X + sStringLen] & CHAR_ROW::ATTR_TRAILING_BYTE)
            {
                Row->CharRow.Chars[coordTarget.X + sStringLen] = UNICODE_SPACE;
                Row->CharRow.KAttrs[coordTarget.X + sStringLen] = 0;
            }
        }
        else if (coordTarget.Y + 1 < coordScreenBufferSize.Y)
        {
            if (RowNext->CharRow.KAttrs[0] & CHAR_ROW::ATTR_TRAILING_BYTE)
            {
                RowNext->CharRow.Chars[0] = UNICODE_SPACE;
                RowNext->CharRow.KAttrs[0] = 0;
            }
        }
    }
}

// Routine Description:
// - Determine if the given Unicode char is fullwidth or not.
// Return:
// - FALSE : half width. Uses 1 column per one character
// - TRUE  : full width. Uses 2 columns per one character
// Notes:
// 04-08-92 ShunK       Created.
// Jul-27-1992 KazuM    Added Screen Information and Code Page Information.
// Jan-29-1992 V-Hirots Substruct Screen Information.
// Oct-06-1996 KazuM    Not use RtlUnicodeToMultiByteSize and WideCharToMultiByte
//                      Because 950 (Chinese Traditional) only defined 13500 chars,
//                     and unicode defined almost 18000 chars.
//                      So there are almost 4000 chars can not be mapped to big5 code.
// Apr-30-2015 MiNiksa  Corrected unknown character code assumption. Max Width in Text Metric
//                      is not reliable for calculating half/full width. Must use current
//                      display font data (cached) instead.
BOOL IsCharFullWidth(_In_ WCHAR wch)
{
    // See http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt

    // 0x00-0x1F is ambiguous by font
    if (0x20 <= wch && wch <= 0x7e)
    {
        /* ASCII */
        return FALSE;
    }
    // 0x80 - 0x0451 varies from narrow to ambiguous by character and font (Unicode 9.0)
    else if (0x0452 <= wch && wch <= 0x10FF)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        return FALSE;
    }
    else if (0x1100 <= wch && wch <= 0x115F)
    {
        // From Unicode 9.0, Hangul Choseong is wide
        return TRUE;
    }
    else if (0x1160 <= wch && wch <= 0x200F)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        return FALSE;
    }
    // 0x2010 - 0x2B59 varies between narrow, ambiguous, and wide by character and font (Unicode 9.0)
    else if (0x2B5A <= wch && wch <= 0x2E44)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        return FALSE;
    }
    else if (0x2E80 <= wch && wch <= 0x303e)
    {
        // From Unicode 9.0, this range is wide (assorted languages)
        return TRUE;
    }
    else if (0x3041 <= wch && wch <= 0x3094)
    {
        /* Hiragana */
        return TRUE;
    }
    else if (0x30a1 <= wch && wch <= 0x30f6)
    {
        /* Katakana */
        return TRUE;
    }
    else if (0x3105 <= wch && wch <= 0x312c)
    {
        /* Bopomofo */
        return TRUE;
    }
    else if (0x3131 <= wch && wch <= 0x318e)
    {
        /* Hangul Elements */
        return TRUE;
    }
    else if (0x3190 <= wch && wch <= 0x3247)
    {
        // From Unicode 9.0, this range is wide
        return TRUE;
    }
    else if (0x3251 <= wch && wch <= 0xA4C6)
    {
        // This exception range is narrow width hexagrams.
        if (0x4DC0 <= wch && wch <= 0x4DFF)
        {
            return FALSE;
        }

        // From Unicode 9.0, this range is wide
        // CJK Unified Ideograph and Yi and Reserved.
        // Includes Han Ideographic range.
        return TRUE;
    }
    else if (0xA4D0 <= wch && wch <= 0xABF9)
    {
        // This exception range is wide Hangul Choseong
        if (0xA960 <= wch && wch <= 0xA97C)
        {
            return TRUE;
        }

        // From Unicode 9.0, this range is narrow (assorted languages)
        return FALSE;
    }
    else if (0xac00 <= wch && wch <= 0xd7a3)
    {
        /* Korean Hangul Syllables */
        return TRUE;
    }
    else if (0xD7B0 <= wch && wch <= 0xD7FB)
    {
        // From Unicode 9.0, this range is narrow
        // Hangul Jungseong and Hangul Jongseong
        return FALSE;
    }
    // 0xD800-0xDFFF is reserved for UTF-16 surrogate pairs.
    // 0xE000-0xF8FF is reserved for private use characters and is therefore always ambiguous.
    else if (0xF900 <= wch && wch <= 0xFAFF)
    {
        // From Unicode 9.0, this range is wide
        // CJK Compatibility Ideographs
        // Includes Han Compatibility Ideographs
        return TRUE;
    }
    else if (0xFB00 <= wch && wch <= 0xFDFD)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        return FALSE;
    }
    else if (0xFE10 <= wch && wch <= 0xFE6B)
    {
        // This exception range has narrow combining ligatures
        if (0xFE20 <= wch && wch <= 0xFE2F)
        {
            return FALSE;
        }

        // From Unicode 9.0, this range is wide
        // Presentation forms
        return TRUE;
    }
    else if (0xFE70 <= wch && wch <= 0xFEFF)
    {
        // From Unicode 9.0, this range is narrow
        return FALSE;
    }
    else if (0xff01 <= wch && wch <= 0xff5e)
    {
        /* Fullwidth ASCII variants */
        return TRUE;
    }
    else if (0xff61 <= wch && wch <= 0xff9f)
    {
        /* Halfwidth Katakana variants */
        return FALSE;
    }
    else if ((0xffa0 <= wch && wch <= 0xffbe) ||
             (0xffc2 <= wch && wch <= 0xffc7) ||
             (0xffca <= wch && wch <= 0xffcf) ||
             (0xffd2 <= wch && wch <= 0xffd7) ||
             (0xffda <= wch && wch <= 0xffdc))
    {
        /* Halfwidth Hangule variants */
        return FALSE;
    }
    else if (0xffe0 <= wch && wch <= 0xffe6)
    {
        /* Fullwidth symbol variants */
        return TRUE;
    }
    // Currently we do not support codepoints above 0xffff
    else
    {
        if (ServiceLocator::LocateGlobals()->pRender != nullptr)
        {
            return ServiceLocator::LocateGlobals()->pRender->IsCharFullWidthByFont(wch);
        }
    }

    ASSERT(FALSE);
    return FALSE;
}

// Routine Description:
// - This routine remove DBCS padding code.
// Arguments:
// - Dst - Pointer to destination.
// - Src - Pointer to source.
// - NumBytes - Number of string.
// Return Value:
DWORD RemoveDbcsMark(_Inout_updates_(NumChars) PWCHAR Dst,
                     _In_reads_(NumChars) PWCHAR Src,
                     _In_ DWORD NumChars,
                     _In_reads_opt_(NumChars) PCHAR SrcA)
{
    if (NumChars == 0 || NumChars == 0xFFFFFFFF)
    {
        return 0;
    }

    if (SrcA)
    {
        DWORD cTotalChars = NumChars;
        PWCHAR const Tmp = Dst;
        while (NumChars--)
        {
            if (Dst >= Tmp + cTotalChars)
            {
                ASSERT(false);
                return 0;
            }

            if (!(*SrcA++ & CHAR_ROW::ATTR_TRAILING_BYTE))
            {
                *Dst++ = *Src;
            }
            Src++;
        }
        return (ULONG) (Dst - Tmp);
    }
    else
    {
        memmove(Dst, Src, NumChars * sizeof(WCHAR));
        return NumChars;
    }
}

// Routine Description:
// - This routine removes the double copies of characters used when storing DBCS/Double-wide characters in the text buffer.
// Arguments:
// - pciDst - Pointer to destination.
// - pciSrc - Pointer to source.
// - cch - Number of characters in buffer.
// Return Value:
// - The length of the destination buffer.
DWORD RemoveDbcsMarkCell(_Out_writes_(cch) PCHAR_INFO pciDst, _In_reads_(cch) const CHAR_INFO * pciSrc, _In_ DWORD cch)
{
    // Walk through the source CHAR_INFO and copy each to the destination.
    // EXCEPT for trailing bytes (this will de-duplicate the leading/trailing byte double copies of the CHAR_INFOs as stored in the buffer).

    // Set up indices used for arrays.
    DWORD iDst = 0;

    // Walk through every CHAR_INFO
    for (DWORD iSrc = 0; iSrc < cch; iSrc++)
    {
        // If it's not a trailing byte, copy it straight over, stripping out the Leading/Trailing flags from the attributes field.
        if (!IsFlagSet(pciSrc[iSrc].Attributes, COMMON_LVB_TRAILING_BYTE))
        {
            pciDst[iDst] = pciSrc[iSrc];
            pciDst[iDst].Attributes &= ~COMMON_LVB_SBCSDBCS;
            iDst++;
        }

        // If it was a trailing byte, we'll just walk past it and keep going.
    }

    // Zero out the remaining part of the destination buffer that we didn't use.
    DWORD const cchDstToClear = cch - iDst;

    if (cchDstToClear > 0)
    {
        CHAR_INFO* const pciDstClearStart = pciDst + iDst;
        ZeroMemory(pciDstClearStart, cchDstToClear * sizeof(CHAR_INFO));
    }

    // Add the additional length we just modified.
    iDst += cchDstToClear;

    // now that we're done, we should have copied, left alone, or cleared the entire length.
    ASSERT(iDst == cch);

    return iDst;
}

DWORD RemoveDbcsMarkAll(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                        _In_ ROW* pRow,
                        _In_ SHORT * const psLeftChar,
                        _In_ PRECT prcText,
                        _Inout_opt_ int * const piTextLeft,
                        _Inout_updates_(cchBuf) PWCHAR pwchBuf,
                        _In_ const SHORT cchBuf)
{
    if (cchBuf <= 0)
    {
        return cchBuf;
    }

    if (*psLeftChar > pScreenInfo->GetBufferViewport().Left && pRow->CharRow.KAttrs[*psLeftChar] & CHAR_ROW::ATTR_TRAILING_BYTE)
    {
        prcText->left -= pScreenInfo->GetScreenFontSize().X;
        --*psLeftChar;

        if (nullptr != piTextLeft)
        {
            *piTextLeft = prcText->left;
        }

#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "We're guaranteed to be able to make this access.")
        return RemoveDbcsMark(pwchBuf, &pRow->CharRow.Chars[*psLeftChar], cchBuf + 1, (PCHAR) & pRow->CharRow.KAttrs[*psLeftChar]);
    }
    else if (*psLeftChar == pScreenInfo->GetBufferViewport().Left && pRow->CharRow.KAttrs[*psLeftChar] & CHAR_ROW::ATTR_TRAILING_BYTE)
    {
        *pwchBuf = UNICODE_SPACE;
        return RemoveDbcsMark(pwchBuf + 1, &pRow->CharRow.Chars[*psLeftChar + 1], cchBuf - 1, (PCHAR) & pRow->CharRow.KAttrs[*psLeftChar + 1]) + 1;
    }
    else
    {
        return RemoveDbcsMark(pwchBuf, &pRow->CharRow.Chars[*psLeftChar], cchBuf, (PCHAR) & pRow->CharRow.KAttrs[*psLeftChar]);
    }
}

// Routine Description:
// - Checks if a char is a lead byte for a given code page.
// Arguments:
// - ch - the char to check.
// - pCPInfo - the code page to check the char in.
// Return Value:
// true if ch is a lead byte, false otherwise.
bool IsDBCSLeadByteConsole(_In_ const CHAR ch, _In_ const CPINFO * const pCPInfo)
{
    ASSERT(pCPInfo != nullptr);
    // NOTE: This must be unsigned for the comparison. If we compare signed, this will never hit
    // because lead bytes are ironically enough always above 0x80 (signed char negative range).
    unsigned char const uchComparison = (unsigned char)ch;

    int i = 0;
    // this is ok because the the array is guaranteed to have 2
    // null bytes at the end.
    while (pCPInfo->LeadByte[i])
    {
        if (pCPInfo->LeadByte[i] <= uchComparison && uchComparison <= pCPInfo->LeadByte[i + 1])
        {
            return true;
        }
        i += 2;
    }
    return false;
}

BYTE CodePageToCharSet(_In_ UINT const uiCodePage)
{
    CHARSETINFO csi;

    if (!TranslateCharsetInfo((DWORD *) IntToPtr(uiCodePage), &csi, TCI_SRCCODEPAGE))
    {
        csi.ciCharset = OEM_CHARSET;
    }

    return (BYTE) csi.ciCharset;
}

BOOL IsAvailableEastAsianCodePage(_In_ UINT const uiCodePage)
{
    BYTE const CharSet = CodePageToCharSet(uiCodePage);

    switch (CharSet)
    {
    case SHIFTJIS_CHARSET:
    case HANGEUL_CHARSET:
    case CHINESEBIG5_CHARSET:
    case GB2312_CHARSET:
        return true;
    default:
        return false;
    }
}

_Ret_range_(0, cbAnsi)
ULONG TranslateUnicodeToOem(_In_reads_(cchUnicode) PCWCHAR pwchUnicode,
                            _In_ const ULONG cchUnicode,
                            _Out_writes_bytes_(cbAnsi) PCHAR pchAnsi,
                            _In_ const ULONG cbAnsi,
                            _Out_opt_ PINPUT_RECORD pDbcsInputRecord)
{
    PWCHAR const TmpUni = new WCHAR[cchUnicode];
    if (TmpUni == nullptr)
    {
        return 0;
    }

    memcpy(TmpUni, pwchUnicode, cchUnicode* sizeof(WCHAR));

    BYTE AsciiDbcs[2];
    AsciiDbcs[1] = 0;

    ULONG i, j;
    for (i = 0, j = 0; i < cchUnicode && j < cbAnsi; i++, j++)
    {
        if (IsCharFullWidth(TmpUni[i]))
        {
            ULONG const NumBytes = sizeof(AsciiDbcs);
            ConvertToOem(ServiceLocator::LocateGlobals()->getConsoleInformation()->CP, &TmpUni[i], 1, (LPSTR) & AsciiDbcs[0], NumBytes);
            if (IsDBCSLeadByteConsole(AsciiDbcs[0], &ServiceLocator::LocateGlobals()->getConsoleInformation()->CPInfo))
            {
                if (j < cbAnsi - 1)
                {   // -1 is safe DBCS in buffer
                    pchAnsi[j] = AsciiDbcs[0];
                    j++;
                    pchAnsi[j] = AsciiDbcs[1];
                    AsciiDbcs[1] = 0;
                }
                else
                {
                    pchAnsi[j] = AsciiDbcs[0];
                    break;
                }
            }
            else
            {
                pchAnsi[j] = AsciiDbcs[0];
                AsciiDbcs[1] = 0;
            }
        }
        else
        {
            ConvertToOem(ServiceLocator::LocateGlobals()->getConsoleInformation()->CP, &TmpUni[i], 1, &pchAnsi[j], 1);
        }
    }

    if (pDbcsInputRecord)
    {
        if (AsciiDbcs[1])
        {
            pDbcsInputRecord->EventType = KEY_EVENT;
            pDbcsInputRecord->Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
        }
        else
        {
            ZeroMemory(pDbcsInputRecord, sizeof(INPUT_RECORD));
        }
    }

    delete[] TmpUni;
    return j;
}
