/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/convert.hpp"

#pragma hdrstop

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
// - codepage - codepage for use in conversion
// - source - the string to convert
// Return Value:
// - returns a deque of converted chars
// Note:
// - will throw on error
std::deque<char> ConvertToOem(_In_ const UINT codepage,
                              _In_ const std::wstring& source)
{
    std::deque<char> outChars;
    // no point in trying to convert an empty string
    if (source == L"")
    {
        return outChars;
    }

    int bufferSize = WideCharToMultiByte(codepage,
                                         0,
                                         source.c_str(),
                                         static_cast<int>(source.size()),
                                         nullptr,
                                         0,
                                         nullptr,
                                         nullptr);
    THROW_LAST_ERROR_IF(bufferSize == 0);

    std::unique_ptr<char[]> convertedChars = std::make_unique<char[]>(bufferSize);
    THROW_IF_NULL_ALLOC(convertedChars);

    bufferSize = WideCharToMultiByte(codepage,
                                     0,
                                     source.c_str(),
                                     static_cast<int>(source.size()),
                                     convertedChars.get(),
                                     bufferSize,
                                     nullptr,
                                     nullptr);
    THROW_LAST_ERROR_IF(bufferSize == 0);

    for (int i = 0; i < bufferSize; ++i)
    {
        outChars.push_back(convertedChars[i]);
    }

    return outChars;
}

