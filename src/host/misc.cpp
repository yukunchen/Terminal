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
// - Takes a multibyte string, allocates the appropriate amount of memory for the conversion, performs the conversion,
//   and returns the Unicode UCS-2 result in the smart pointer (and the length).
// Arguments:
// - uiCodePage - Windows Code Page representing the multibyte source text
// - rgchSource - Array of multibyte characters of source text
// - cchSource - Length of rgchSource array
// - pwsTarget - Smart pointer to receive allocated memory for converted Unicode text
// - cchTarget - Length of converted unicode text
// Return Value:
// - S_OK if succeeded or suitable HRESULT errors from memory allocation, safe math, or MultiByteToWideChar failures.
HRESULT ConvertToW(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const char* const rgchSource,
                   _In_ size_t const cchSource,
                   _Inout_ wistd::unique_ptr<wchar_t[]>& pwsTarget,
                   _Out_ size_t& cchTarget)
{
    cchTarget = 0;

    // If there's nothing to convert, bail early.
    RETURN_HR_IF(S_OK, cchSource == 0);

    int iSource; // convert to int because Mb2Wc requires it.
    RETURN_IF_FAILED(SizeTToInt(cchSource, &iSource));

    // Ask how much space we will need.
    int const iTarget = MultiByteToWideChar(uiCodePage, 0, rgchSource, iSource, nullptr, 0);
    RETURN_LAST_ERROR_IF(0 == iTarget);

    size_t cchNeeded;
    RETURN_IF_FAILED(IntToSizeT(iTarget, &cchNeeded));

    // Allocate ourselves space in a smart pointer.
    wistd::unique_ptr<wchar_t[]> pwsOut = wil::make_unique_nothrow<wchar_t[]>(cchNeeded);
    RETURN_IF_NULL_ALLOC(pwsOut);

    // Attempt conversion for real.
    RETURN_LAST_ERROR_IF(0 == MultiByteToWideChar(uiCodePage, 0, rgchSource, iSource, pwsOut.get(), iTarget));

    // Return the count of characters we produced.
    cchTarget = cchNeeded;

    // Give back the smart allocated space (and take their copy so we can free it when exiting.)
    pwsTarget.swap(pwsOut);

    return S_OK;
}

// Routine Description:
// - Takes a wide (UCS-2 Unicode) string, allocates the appropriate amount of memory for the conversion, performs the conversion,
//   and returns the Multibyte result in the smart pointer (and the length).
// Arguments:
// - uiCodePage - Windows Code Page representing the multibyte destination text
// - rgchSource - Array of Unicode (UCS-2) characters of source text
// - cchSource - Length of rgwchSource array
// - psTarget - Smart pointer to receive allocated memory for converted Multibyte text
// - cchTarget - Length of converted text
// Return Value:
// - S_OK if succeeded or suitable HRESULT errors from memory allocation, safe math, or WideCharToMultiByte failures.
HRESULT ConvertToA(_In_ const UINT uiCodePage,
                   _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                   _In_ size_t cchSource,
                   _Inout_ wistd::unique_ptr<char[]>& psTarget,
                   _Out_ size_t& cchTarget)
{
    cchTarget = 0;

    // If there's nothing to convert, bail early.
    RETURN_HR_IF(S_OK, cchSource == 0);

    int iSource; // convert to int because Wc2Mb requires it.
    RETURN_IF_FAILED(SizeTToInt(cchSource, &iSource));

    // Ask how much space we will need.
    #pragma prefast(suppress:__WARNING_W2A_BEST_FIT, "WC_NO_BEST_FIT_CHARS doesn't work in many codepages. Retain old behavior.")
    int const iTarget = WideCharToMultiByte(uiCodePage, 0, rgwchSource, iSource, nullptr, 0, nullptr, nullptr);
    RETURN_LAST_ERROR_IF(0 == iTarget);

    size_t cchNeeded;
    RETURN_IF_FAILED(IntToSizeT(iTarget, &cchNeeded));

    // Allocate ourselves space in a smart pointer
    wistd::unique_ptr<char[]> psOut = wil::make_unique_nothrow<char[]>(cchNeeded);

    // Attempt conversion for real.
    #pragma prefast(suppress:__WARNING_W2A_BEST_FIT, "WC_NO_BEST_FIT_CHARS doesn't work in many codepages. Retain old behavior.")
    RETURN_LAST_ERROR_IF(0 == WideCharToMultiByte(uiCodePage, 0, rgwchSource, iSource, psOut.get(), iTarget, nullptr, nullptr));

    // Return the count of characters we produced.
    cchTarget = cchNeeded;

    // Give back the smart allocated space (and take their copy so we can free it when exiting.)
    psTarget.swap(psOut);

    return S_OK;
}

// Routine Description:
// - Takes a wide (UCS-2 Unicode) string, and determines how many bytes it would take to store it with the given Multibyte codepage.
// Arguments:
// - uiCodePage - Windows Code Page representing the multibyte destination text
// - rgchSource - Array of Unicode (UCS-2) characters of source text
// - cchSource - Length of rgwchSource array
// - pcchTarget - Length in characters of multibyte buffer that would be required to hold this text after conversion
// Return Value:
// - S_OK if succeeded or suitable HRESULT errors from memory allocation, safe math, or WideCharToMultiByte failures.
HRESULT GetALengthFromW(_In_ const UINT uiCodePage,
                        _In_reads_or_z_(cchSource) const wchar_t* const rgwchSource,
                        _In_ size_t const cchSource,
                        _Out_ size_t* const pcchTarget)
{
    *pcchTarget = 0;

    // If there's no bytes, bail early.
    RETURN_HR_IF(S_OK, cchSource == 0);

    int iSource; // convert to int because Wc2Mb requires it
    RETURN_IF_FAILED(SizeTToInt(cchSource, &iSource));

    // Ask how many bytes this string consumes in the other codepage
    #pragma prefast(suppress:__WARNING_W2A_BEST_FIT, "WC_NO_BEST_FIT_CHARS doesn't work in many codepages. Retain old behavior.")
    int const iTarget = WideCharToMultiByte(uiCodePage, 0, rgwchSource, iSource, nullptr, 0, nullptr, nullptr);
    RETURN_LAST_ERROR_IF(0 == iTarget);

    // Convert types safely.
    RETURN_IF_FAILED(IntToSizeT(iTarget, pcchTarget));

    return S_OK;
}

// Routine Description:
// - Takes a unicode count of characters (in a size_t) and converts it to a byte count that
//   fits into a USHORT for compatibility with existing Alias functions within cmdline.cpp.
// Arguments:
// - cchUnicode - Count of UCS-2 Unicode characters (wide characters)
// - pcb - Pointer to result USHORT that will hold the equivalent byte count, if it fit.
// Return Value:
// - S_OK if succeeded or appropriate safe math failure code from multiplication and type conversion.
HRESULT GetUShortByteCount(_In_ size_t cchUnicode,
                           _Out_ USHORT* const pcb)
{
    size_t cbUnicode;
    RETURN_IF_FAILED(SizeTMult(cchUnicode, sizeof(wchar_t), &cbUnicode));
    RETURN_IF_FAILED(SizeTToUShort(cbUnicode, pcb));
    return S_OK;
}

// Routine Description:
// - Takes a unicode count of characters (in a size_t) and converts it to a byte count that
//   fits into a DWORD for compatibility with existing Alias functions within cmdline.cpp.
// Arguments:
// - cchUnicode - Count of UCS-2 Unicode characters (wide characters)
// - pcb - Pointer to result DWORD that will hold the equivalent byte count, if it fit.
// Return Value:
// - S_OK if succeeded or appropriate safe math failure code from multiplication and type conversion.
HRESULT GetDwordByteCount(_In_ size_t cchUnicode,
                          _Out_ DWORD* const pcb)
{
    size_t cbUnicode;
    RETURN_IF_FAILED(SizeTMult(cchUnicode, sizeof(wchar_t), &cbUnicode));
    RETURN_IF_FAILED(SizeTToDWord(cbUnicode, pcb));
    return S_OK;
}

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
    return LOG_IF_WIN32_BOOL_FALSE(WideCharToMultiByte(uiCodePage, 0, pwchSource, cchSource, pchTarget, cchTarget, nullptr, nullptr));
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
