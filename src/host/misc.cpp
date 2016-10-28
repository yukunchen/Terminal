/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "misc.h"

#include "_output.h"
#include "output.h"

#include "consrv.h"
#include "cursor.h"
#include "dbcs.h"
#include "utils.hpp"
#include "window.hpp"

#pragma hdrstop

#define CHAR_NULL      ((char)0)

// Routine Description:
// - Converts unicode characters to ANSI given a destination codepage
// Arguments:
// - uiCodePage - codepage for use in conversion
// - pwchSource - unicode string to convert
// - cchSource - length of pwchSource in characters
// - pchTarget - pointer to destination buffer to receive converted ANSI string
// - cchTarget - size of destination buffer in characters
// Return Value:
// - Returns the number characters written to pchTarget, or 0 on failure
int ConvertToOem(_In_ const UINT uiCodePage,
                 _In_reads_(cchSource) const WCHAR * const pwchSource,
                 _In_ const UINT cchSource,
                 _Out_writes_(cchTarget) CHAR * const pchTarget,
                 _In_ const UINT cchTarget)
{
    ASSERT(pwchSource != (LPWSTR) pchTarget);
    DBGCHARS(("ConvertToOem U->%d %.*ls\n", uiCodePage, cchSource > 10 ? 10 : cchSource, pwchSource));
#pragma prefast(suppress:__WARNING_W2A_BEST_FIT, "WC_NO_BEST_FIT_CHARS doesn't work in many codepages. Retain old behavior.")
    return WideCharToMultiByte(uiCodePage, 0, pwchSource, cchSource, pchTarget, cchTarget, nullptr, nullptr);
}

// Data in the output buffer is the true unicode value.
int ConvertInputToUnicode(_In_ const UINT uiCodePage,
                          _In_reads_(cchSource) const CHAR * const pchSource,
                          _In_ const UINT cchSource,
                          _Out_writes_(cchTarget) WCHAR * const pwchTarget,
                          _In_ const UINT cchTarget)
{
    DBGCHARS(("ConvertInputToUnicode %d->U %.*s\n", uiCodePage, cchSource > 10 ? 10 : cchSource, pchSource));

    return MultiByteToWideChar(uiCodePage, 0, pchSource, cchSource, pwchTarget, cchTarget);
}

// Output data is always translated via the ansi codepage so glyph translation works.
int ConvertOutputToUnicode(_In_ UINT uiCodePage,
                           _In_reads_(cchSource) const CHAR * const pchSource,
                           _In_ UINT cchSource,
                           _Out_writes_(cchTarget) WCHAR *pwchTarget,
                           _In_ UINT cchTarget)
{
    ASSERT(cchTarget > 0);
    pwchTarget[0] = L'\0';

    DBGCHARS(("ConvertOutputToUnicode %d->U %.*s\n", uiCodePage, cchSource > 10 ? 10 : cchSource, pchSource));

    LPSTR const pszT = new CHAR[cchSource];
    if (pszT == nullptr)
    {
        return 0;
    }

    memmove(pszT, pchSource, cchSource);
    ULONG const Length = MultiByteToWideChar(uiCodePage, MB_USEGLYPHCHARS, pszT, cchSource, pwchTarget, cchTarget);
    delete[] pszT;
    return Length;
}

WCHAR CharToWchar(_In_reads_(cch) const char * const pch, _In_ const UINT cch)
{
    WCHAR wc = L'\0';

    ASSERT(IsDBCSLeadByteConsole(*pch, &g_ciConsoleInformation.OutputCPInfo) || cch == 1);

    ConvertOutputToUnicode(g_ciConsoleInformation.OutputCP, pch, cch, &wc, 1);

    return wc;
}

void SetConsoleCPInfo(_In_ const BOOL fOutput)
{
    if (fOutput)
    {
        // If we're changing the output codepage, we want to update the font as well to give the engine an opportunity
        // to pick a more appropriate font should the current one be unable to render in the new codepage.
        // To do this, we create a copy of the existing font but we change the codepage value to be the new one that was just set in the global structures.
        // NOTE: We need to do this only if everything is set up. This can get called while we're still initializing, so carefully check things for nullptr.
        SCREEN_INFORMATION* const psi = g_ciConsoleInformation.CurrentScreenBuffer;
        if (psi != nullptr)
        {
            TEXT_BUFFER_INFO* const pti = psi->TextInfo;
            if (pti != nullptr)
            {
                const FontInfo* const pfiOld = pti->GetCurrentFont();
                FontInfo fiNew = FontInfo(pfiOld->GetFaceName(), pfiOld->GetFamily(), pfiOld->GetWeight(), pfiOld->GetUnscaledSize(), g_ciConsoleInformation.OutputCP);
                psi->UpdateFont(&fiNew);
            }
        }

        if (!GetCPInfo(g_ciConsoleInformation.OutputCP, &g_ciConsoleInformation.OutputCPInfo))
        {
            g_ciConsoleInformation.OutputCPInfo.LeadByte[0] = 0;
        }
    }
    else
    {
        if (!GetCPInfo(g_ciConsoleInformation.CP, &g_ciConsoleInformation.CPInfo))
        {
            g_ciConsoleInformation.CPInfo.LeadByte[0] = 0;
        }
    }
}

// Routine Description:
// - This routine check bisected on Unicode string end.
// Arguments:
// - pwchBuffer - Pointer to Unicode string buffer.
// - cWords - Number of Unicode string.
// - cBytes - Number of bisect position by byte counts.
// Return Value:
// - TRUE - Bisected character.
// - FALSE - Correctly.
BOOL CheckBisectStringW(_In_reads_bytes_(cBytes) const WCHAR * pwchBuffer,
                        _In_ DWORD cWords,
                        _In_ DWORD cBytes)
{
    while (cWords && cBytes)
    {
        if (IsCharFullWidth(*pwchBuffer))
        {
            if (cBytes < 2)
            {
                return TRUE;
            }
            else
            {
                cWords--;
                cBytes -= 2;
                pwchBuffer++;
            }
        }
        else
        {
            cWords--;
            cBytes--;
            pwchBuffer++;
        }
    }

    return FALSE;
}

// Routine Description:
// - This routine check bisected on Unicode string end.
// Arguments:
// - pScreenInfo - Pointer to screen information structure.
// - pwchBuffer - Pointer to Unicode string buffer.
// - cWords - Number of Unicode string.
// - cBytes - Number of bisect position by byte counts.
// - fEcho - TRUE if called by Read (echoing characters)
// Return Value:
// - TRUE - Bisected character.
// - FALSE - Correctly.
BOOL CheckBisectProcessW(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                         _In_reads_bytes_(cBytes) const WCHAR * pwchBuffer,
                         _In_ DWORD cWords,
                         _In_ DWORD cBytes,
                         _In_ SHORT sOriginalXPosition,
                         _In_ BOOL fEcho)
{
    if (IsFlagSet(pScreenInfo->OutputMode, ENABLE_PROCESSED_OUTPUT))
    {
        while (cWords && cBytes)
        {
            WCHAR const Char = *pwchBuffer;
            if (Char >= (WCHAR)' ')
            {
                if (IsCharFullWidth(Char))
                {
                    if (cBytes < 2)
                    {
                        return TRUE;
                    }
                    else
                    {
                        cWords--;
                        cBytes -= 2;
                        pwchBuffer++;
                        sOriginalXPosition += 2;
                    }
                }
                else
                {
                    cWords--;
                    cBytes--;
                    pwchBuffer++;
                    sOriginalXPosition++;
                }
            }
            else
            {
                cWords--;
                pwchBuffer++;
                switch (Char)
                {
                    case UNICODE_BELL:
                        if (fEcho)
                            goto CtrlChar;
                        break;
                    case UNICODE_BACKSPACE:
                    case UNICODE_LINEFEED:
                    case UNICODE_CARRIAGERETURN:
                        break;
                    case UNICODE_TAB:
                    {
                        ULONG TabSize = NUMBER_OF_SPACES_IN_TAB(sOriginalXPosition);
                        sOriginalXPosition = (SHORT)(sOriginalXPosition + TabSize);
                        if (cBytes < TabSize)
                            return TRUE;
                        cBytes -= TabSize;
                        break;
                    }
                    default:
                        if (fEcho)
                        {
        CtrlChar:
                            if (cBytes < 2)
                                return TRUE;
                            cBytes -= 2;
                            sOriginalXPosition += 2;
                        }
                        else
                        {
                            cBytes--;
                            sOriginalXPosition++;
                        }
                }
            }
        }
        return FALSE;
    }
    else
    {
        return CheckBisectStringW(pwchBuffer, cWords, cBytes);
    }
}
