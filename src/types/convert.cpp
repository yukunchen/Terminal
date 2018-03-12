/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/convert.hpp"

#ifdef BUILD_ONECORE_INTERACTIVITY
#include "../../interactivity/inc/VtApiRedirection.hpp"
#endif

#pragma hdrstop

// TODO: MSFT 14150722 - can these const values be generated at
// runtime without breaking compatibility?
static const WORD altScanCode = 0x38;
static const WORD leftShiftScanCode = 0x2A;

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
[[nodiscard]]
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
[[nodiscard]]
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
[[nodiscard]]
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
[[nodiscard]]
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
[[nodiscard]]
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

std::deque<std::unique_ptr<KeyEvent>> CharToKeyEvents(_In_ const wchar_t wch,
                                                      _In_ const unsigned int codepage)
{
    const short invalidKey = -1;
    short keyState = VkKeyScanW(wch);

    if (keyState == invalidKey)
    {
        // Determine DBCS character because these character does not know by VkKeyScan.
        // GetStringTypeW(CT_CTYPE3) & C3_ALPHA can determine all linguistic characters. However, this is
        // not include symbolic character for DBCS.
        //
        // IsCharFullWidth can help for DBCS symbolic character.
        WORD CharType;
        bool isFullWidth = false;
        GetStringTypeW(CT_CTYPE3, &wch, 1, &CharType);
        LOG_IF_FAILED(IsCharFullWidth(wch, &isFullWidth));

        if (IsFlagSet(CharType, C3_ALPHA) || isFullWidth)
        {
            keyState = 0;
        }
    }

    std::deque<std::unique_ptr<KeyEvent>> convertedEvents;
    if (keyState == invalidKey)
    {
        // if VkKeyScanW fails (char is not in kbd layout), we must
        // emulate the key being input through the numpad
        convertedEvents = SynthesizeNumpadEvents(wch, codepage);
    }
    else
    {
        convertedEvents = SynthesizeKeyboardEvents(wch, keyState);
    }

    return convertedEvents;
}


// Routine Description:
// - converts a wchar_t into a series of KeyEvents as if it was typed
// using the keyboard
// Arguments:
// - wch - the wchar_t to convert
// Return Value:
// - deque of KeyEvents that represent the wchar_t being typed
// Note:
// - will throw exception on error
std::deque<std::unique_ptr<KeyEvent>> SynthesizeKeyboardEvents(_In_ const wchar_t wch, _In_ const short keyState)
{
    const byte modifierState = HIBYTE(keyState);

    bool altGrSet = false;
    bool shiftSet = false;
    std::deque<std::unique_ptr<KeyEvent>> keyEvents;

    // add modifier key event if necessary
    if (AreAllFlagsSet(modifierState, VkKeyScanModState::CtrlAndAltPressed))
    {
        altGrSet = true;
        keyEvents.push_back(std::make_unique<KeyEvent>(true,
                                                       1ui16,
                                                       static_cast<WORD>(VK_MENU),
                                                       altScanCode,
                                                       UNICODE_NULL,
                                                       (ENHANCED_KEY | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED)));
    }
    else if (IsFlagSet(modifierState, VkKeyScanModState::ShiftPressed))
    {
        shiftSet = true;
        keyEvents.push_back(std::make_unique<KeyEvent>(true,
                                                       1ui16,
                                                       static_cast<WORD>(VK_SHIFT),
                                                       leftShiftScanCode,
                                                       UNICODE_NULL,
                                                       SHIFT_PRESSED));
    }

    const WORD virtualScanCode = static_cast<WORD>(MapVirtualKeyW(wch, MAPVK_VK_TO_VSC));
    KeyEvent keyEvent{ true, 1, LOBYTE(keyState), virtualScanCode, wch, 0 };

    // add modifier flags if necessary
    if (IsFlagSet(modifierState, VkKeyScanModState::ShiftPressed))
    {
        keyEvent.ActivateModifierKey(ModifierKeyState::Shift);
    }
    if (IsFlagSet(modifierState, VkKeyScanModState::CtrlPressed))
    {
        keyEvent.ActivateModifierKey(ModifierKeyState::LeftCtrl);
    }
    if (AreAllFlagsSet(modifierState, VkKeyScanModState::CtrlAndAltPressed))
    {
        keyEvent.ActivateModifierKey(ModifierKeyState::RightAlt);
    }

    // add key event down and up
    keyEvents.push_back(std::make_unique<KeyEvent>(keyEvent));
    keyEvent.SetKeyDown(false);
    keyEvents.push_back(std::make_unique<KeyEvent>(keyEvent));

    // add modifier key up event
    if (altGrSet)
    {
        keyEvents.push_back(std::make_unique<KeyEvent>(false,
                                                       1ui16,
                                                       static_cast<WORD>(VK_MENU),
                                                       altScanCode,
                                                       UNICODE_NULL,
                                                       ENHANCED_KEY));
    }
    else if (shiftSet)
    {
        keyEvents.push_back(std::make_unique<KeyEvent>(false,
                                                       1ui16,
                                                       static_cast<WORD>(VK_SHIFT),
                                                       leftShiftScanCode,
                                                       UNICODE_NULL,
                                                       0));
    }

    return keyEvents;
}

// Routine Description:
// - converts a wchar_t into a series of KeyEvents as if it was typed
// using Alt + numpad
// Arguments:
// - wch - the wchar_t to convert
// Return Value:
// - deque of KeyEvents that represent the wchar_t being typed using
// alt + numpad
// Note:
// - will throw exception on error
std::deque<std::unique_ptr<KeyEvent>> SynthesizeNumpadEvents(_In_ const wchar_t wch, _In_ const unsigned int codepage)
{
    std::deque<std::unique_ptr<KeyEvent>> keyEvents;

    //alt keydown
    keyEvents.push_back(std::make_unique<KeyEvent>(true,
                                                   1ui16,
                                                   static_cast<WORD>(VK_MENU),
                                                   altScanCode,
                                                   UNICODE_NULL,
                                                   LEFT_ALT_PRESSED));

    const int radix = 10;
    std::wstring wstr{ wch };
    std::deque<char> convertedChars;

    convertedChars = ConvertToOem(codepage, wstr);
    if (convertedChars.size() == 1)
    {
        unsigned char uch = static_cast<unsigned char>(convertedChars[0]);
        // unsigned char values are in the range [0, 255] so we need to be
        // able to store up to 4 chars from the conversion (including the end of string char)
        char charString[4] = { 0 };
        THROW_HR_IF(E_INVALIDARG, 0 != _itoa_s(uch, charString, ARRAYSIZE(charString), radix));

        for (size_t i = 0; i < ARRAYSIZE(charString); ++i)
        {
            if (charString[i] == 0)
            {
                break;
            }
            const WORD virtualKey = charString[i] - '0' + VK_NUMPAD0;
            const WORD virtualScanCode = static_cast<WORD>(MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC));

            keyEvents.push_back(std::make_unique<KeyEvent>(true,
                                                           1ui16,
                                                           virtualKey,
                                                           virtualScanCode,
                                                           UNICODE_NULL,
                                                           LEFT_ALT_PRESSED));
            keyEvents.push_back(std::make_unique<KeyEvent>(false,
                                                           1ui16,
                                                           virtualKey,
                                                           virtualScanCode,
                                                           UNICODE_NULL,
                                                           LEFT_ALT_PRESSED));
        }
    }

    // alt keyup
    keyEvents.push_back(std::make_unique<KeyEvent>(false,
                                                   1ui16,
                                                   static_cast<WORD>(VK_MENU),
                                                   altScanCode,
                                                   wch,
                                                   0));
    return keyEvents;
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
// May-23-2017 migrie   Forced Box-Drawing Characters (x2500-x257F) to narrow.
// Jan-16-2018 migrie   Seperated core lookup from asking the renderer the width
[[nodiscard]]
HRESULT IsCharFullWidth(_In_ wchar_t wch, _Out_ bool* const isFullWidth)
{
    // See http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt

    // 0x00-0x1F is ambiguous by font
    if (0x20 <= wch && wch <= 0x7e)
    {
        /* ASCII */
        *isFullWidth = FALSE;
    }
    // 0x80 - 0x0451 varies from narrow to ambiguous by character and font (Unicode 9.0)
    else if (0x0452 <= wch && wch <= 0x10FF)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        *isFullWidth = FALSE;
    }
    else if (0x1100 <= wch && wch <= 0x115F)
    {
        // From Unicode 9.0, Hangul Choseong is wide
        *isFullWidth = TRUE;
    }
    else if (0x1160 <= wch && wch <= 0x200F)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        *isFullWidth = FALSE;
    }
    // 0x2500 - 0x257F is the box drawing character range -
    // Technically, these are ambiguous width characters, but applications that
    // use them generally assume that they're narrow to ensure proper alignment.
    else if (0x2500 <= wch && wch <= 0x257F)
    {
        *isFullWidth = FALSE;
    }
    // 0x2010 - 0x2B59 varies between narrow, ambiguous, and wide by character and font (Unicode 9.0)
    else if (0x2B5A <= wch && wch <= 0x2E44)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        *isFullWidth = FALSE;
    }
    else if (0x2E80 <= wch && wch <= 0x303e)
    {
        // From Unicode 9.0, this range is wide (assorted languages)
        *isFullWidth = TRUE;
    }
    else if (0x3041 <= wch && wch <= 0x3094)
    {
        /* Hiragana */
        *isFullWidth = TRUE;
    }
    else if (0x30a1 <= wch && wch <= 0x30f6)
    {
        /* Katakana */
        *isFullWidth = TRUE;
    }
    else if (0x3105 <= wch && wch <= 0x312c)
    {
        /* Bopomofo */
        *isFullWidth = TRUE;
    }
    else if (0x3131 <= wch && wch <= 0x318e)
    {
        /* Hangul Elements */
        *isFullWidth = TRUE;
    }
    else if (0x3190 <= wch && wch <= 0x3247)
    {
        // From Unicode 9.0, this range is wide
        *isFullWidth = TRUE;
    }
    else if (0x3251 <= wch && wch <= 0xA4C6)
    {
        // This exception range is narrow width hexagrams.
        if (0x4DC0 <= wch && wch <= 0x4DFF)
        {
            *isFullWidth = FALSE;
        }
        else
        {
            // From Unicode 9.0, this range is wide
            // CJK Unified Ideograph and Yi and Reserved.
            // Includes Han Ideographic range.
            *isFullWidth = TRUE;
        }
    }
    else if (0xA4D0 <= wch && wch <= 0xABF9)
    {
        // This exception range is wide Hangul Choseong
        if (0xA960 <= wch && wch <= 0xA97C)
        {
            *isFullWidth = TRUE;
        }
        else
        {
            // From Unicode 9.0, this range is narrow (assorted languages)
            *isFullWidth = FALSE;
        }
    }
    else if (0xac00 <= wch && wch <= 0xd7a3)
    {
        /* Korean Hangul Syllables */
        *isFullWidth = TRUE;
    }
    else if (0xD7B0 <= wch && wch <= 0xD7FB)
    {
        // From Unicode 9.0, this range is narrow
        // Hangul Jungseong and Hangul Jongseong
        *isFullWidth = FALSE;
    }
    // 0xD800-0xDFFF is reserved for UTF-16 surrogate pairs.
    // 0xE000-0xF8FF is reserved for private use characters and is therefore always ambiguous.
    else if (0xF900 <= wch && wch <= 0xFAFF)
    {
        // From Unicode 9.0, this range is wide
        // CJK Compatibility Ideographs
        // Includes Han Compatibility Ideographs
        *isFullWidth = TRUE;
    }
    else if (0xFB00 <= wch && wch <= 0xFDFD)
    {
        // From Unicode 9.0, this range is narrow (assorted languages)
        *isFullWidth = FALSE;
    }
    else if (0xFE10 <= wch && wch <= 0xFE6B)
    {
        // This exception range has narrow combining ligatures
        if (0xFE20 <= wch && wch <= 0xFE2F)
        {
            *isFullWidth = FALSE;
        }
        else
        {
            // From Unicode 9.0, this range is wide
            // Presentation forms
            *isFullWidth = TRUE;
        }
    }
    else if (0xFE70 <= wch && wch <= 0xFEFF)
    {
        // From Unicode 9.0, this range is narrow
        *isFullWidth = FALSE;
    }
    else if (0xff01 <= wch && wch <= 0xff5e)
    {
        /* Fullwidth ASCII variants */
        *isFullWidth = TRUE;
    }
    else if (0xff61 <= wch && wch <= 0xff9f)
    {
        /* Halfwidth Katakana variants */
        *isFullWidth = FALSE;
    }
    else if ((0xffa0 <= wch && wch <= 0xffbe) ||
             (0xffc2 <= wch && wch <= 0xffc7) ||
             (0xffca <= wch && wch <= 0xffcf) ||
             (0xffd2 <= wch && wch <= 0xffd7) ||
             (0xffda <= wch && wch <= 0xffdc))
    {
        /* Halfwidth Hangule variants */
        *isFullWidth = FALSE;
    }
    else if (0xffe0 <= wch && wch <= 0xffe6)
    {
        /* Fullwidth symbol variants */
        *isFullWidth = TRUE;
    }
    // Currently we do not support codepoints above 0xffff
    else
    {
        *isFullWidth = false;
        return S_FALSE;
    }

    return S_OK;
}
