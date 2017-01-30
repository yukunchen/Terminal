/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include <io.h>
#include <fcntl.h>

#define ENGLISH_US_CP 437u
#define JAPANESE_CP 932u

enum DbcsWriteMode
{
    CrtWrite = 0,
    WriteConsoleOutputFunc = 1,
    WriteConsoleOutputCharacterFunc = 2,
    WriteConsoleFunc = 3
};

enum DbcsReadMode
{
    ReadConsoleOutputFunc = 0,
    ReadConsoleOutputCharacterFunc = 1
};

class DbcsTests
{
    BEGIN_TEST_CLASS(DbcsTests)
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
    END_TEST_CLASS();

    TEST_METHOD_SETUP(DbcsTestSetup);

    // This test must come before ones that launch another process as launching another process can tamper with the codepage
    // in ways that this test is not expecting.
    TEST_METHOD(TestMultibyteInputRetrieval);

    BEGIN_TEST_METHOD(TestDbcsWriteRead)
        TEST_METHOD_PROPERTY(L"Data:uiCodePage", L"{437, 932}")
        TEST_METHOD_PROPERTY(L"Data:fUseTrueTypeFont", L"{true, false}")
        TEST_METHOD_PROPERTY(L"Data:WriteMode", L"{0, 1, 2, 3}")
        TEST_METHOD_PROPERTY(L"Data:fWriteInUnicode", L"{true, false}")
        TEST_METHOD_PROPERTY(L"Data:ReadMode", L"{0, 1}")
        TEST_METHOD_PROPERTY(L"Data:fReadInUnicode", L"{true, false}")
    END_TEST_METHOD()

    TEST_METHOD(TestDbcsBisect);
};

HANDLE hScreen = INVALID_HANDLE_VALUE;

bool DbcsTests::DbcsTestSetup()
{
    return true;
}

void SetupDbcsWriteReadTests(_In_ unsigned int uiCodePage,
                             _In_ bool fIsTrueType,
                             _Out_ HANDLE* const phOut,
                             _Out_ WORD* const pwAttributes)
{
    HANDLE const hOut = GetStdOutputHandle();

    // Ensure that the console is set into the appropriate codepage for the test
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleCP(uiCodePage));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleOutputCP(uiCodePage));

    // Now set up the font. Many of these APIs are oddly dependent on font, so set as appropriate.
    CONSOLE_FONT_INFOEX cfiex = { 0 };
    cfiex.cbSize = sizeof(cfiex);
    if (!fIsTrueType)
    {
        // We use Terminal as the raster font name always.
        wcscpy_s(cfiex.FaceName, L"Terminal");
        cfiex.dwFontSize.X = 8;
        cfiex.dwFontSize.Y = 18;
    }
    else
    {
        switch (uiCodePage)
        {
        case JAPANESE_CP:
            wcscpy_s(cfiex.FaceName, L"MS Gothic");
            break;
        case ENGLISH_US_CP:
            wcscpy_s(cfiex.FaceName, L"Consolas");
            break;
        }

        cfiex.dwFontSize.Y = 16;
    }

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetCurrentConsoleFontEx(hOut, FALSE, &cfiex));

    // Ensure that we set the font we expected to set
    CONSOLE_FONT_INFOEX cfiexGet = { 0 };
    cfiexGet.cbSize = sizeof(cfiexGet);
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(GetCurrentConsoleFontEx(hOut, FALSE, &cfiexGet));

    VERIFY_ARE_EQUAL(NoThrowString(cfiex.FaceName), NoThrowString(cfiexGet.FaceName));

    // Retrieve some of the information about the preferences/settings for the console buffer including
    // the size of the buffer and the default colors (attributes) to use.
    CONSOLE_SCREEN_BUFFER_INFOEX sbiex = { 0 };
    sbiex.cbSize = sizeof(sbiex);
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(GetConsoleScreenBufferInfoEx(hOut, &sbiex));

    // ensure first line of console is cleared out with spaces so nothing interferes with the text these tests will be writing.
    COORD coordZero = { 0 };
    DWORD dwWritten;
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(FillConsoleOutputCharacterW(hOut, L'\x20', sbiex.dwSize.X, coordZero, &dwWritten));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(FillConsoleOutputAttribute(hOut, sbiex.wAttributes, sbiex.dwSize.X, coordZero, &dwWritten));

    // Move the cursor to the 0,0 position into our empty line so the tests can write (important for the CRT tests that specify no location)
    if (!SetConsoleCursorPosition(GetStdOutputHandle(), coordZero))
    {
        VERIFY_FAIL(L"Failed to set cursor position");
    }

    // Give back the output handle and the default attributes so tests can verify attributes didn't change on roundtrip
    *phOut = hOut;
    *pwAttributes = sbiex.wAttributes;
}

void DbcsWriteReadTestsSendOutput(_In_ HANDLE const hOut, _In_ unsigned int const uiCodePage,
                                  _In_ DbcsWriteMode const WriteMode, _In_ bool const fIsUnicode,
                                  _In_ PCSTR pszTestString, _In_ WORD const wAttr)
{

    // DBCS is very dependent on knowing the byte length in the original codepage of the input text.
    // Save off the original length of the string so we know what its A length was.
    SHORT const cTestString = (SHORT)strlen(pszTestString);

    // If we're in Unicode mode, we will need to translate the test string to Unicode before passing into the console
    PWSTR pwszTestString = nullptr;
    if (fIsUnicode)
    {
        // Use double-call pattern to find space to allocate, allocate it, then convert.
        int const icchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestString, -1, nullptr, 0);

        pwszTestString = new WCHAR[icchNeeded];

        int const iRes = MultiByteToWideChar(uiCodePage, 0, pszTestString, -1, pwszTestString, icchNeeded);
        CheckLastErrorZeroFail(iRes, L"MultiByteToWideChar");
    }

    // Calculate the number of cells/characters/calls we will need to fill with our input depending on the mode.
    SHORT cChars = 0;
    if (fIsUnicode)
    {
        cChars = (SHORT)wcslen(pwszTestString);
    }
    else
    {
        cChars = cTestString;
    }

    // These parameters will be used to print out the written rectangle if we used the console APIs (not the CRT APIs)
    // This information will be stored and printed out at the very end after we move the cursor off of the text we just printed.
    // The cursor auto-moves for CRT, but we have to manually move it for some of the Console APIs.
    bool fUseRectWritten = false;
    SMALL_RECT srWrittenExpected = { 0 };
    SMALL_RECT srWritten = { 0 };

    bool fUseDwordWritten = false;
    DWORD dwWritten = 0;

    switch (WriteMode)
    {
    case DbcsWriteMode::CrtWrite:
    {
        // Align the CRT's mode with the text we're about to write.
        // If you call a W function on the CRT while the mode is still set to A,
        // the CRT will helpfully back-convert your text from W to A before sending it to the driver.
        if (fIsUnicode)
        {
            _setmode(_fileno(stdout), _O_WTEXT);
        }
        else
        {
            _setmode(_fileno(stdout), _O_TEXT);
        }

        // Write each character in the string individually out through the CRT
        if (fIsUnicode)
        {
            for (SHORT i = 0; i < cChars; i++)
            {
                putwchar(pwszTestString[i]);
            }
        }
        else
        {
            for (SHORT i = 0; i < cChars; i++)
            {
                putchar(pszTestString[i]);
            }
        }
        break;
    }
    case DbcsWriteMode::WriteConsoleOutputFunc:
    {
        // If we're going to be using WriteConsoleOutput, we need to create up a nice 
        // CHAR_INFO buffer to pass into the method containing the string and possibly attributes
        CHAR_INFO* rgChars = new CHAR_INFO[cChars];

        for (SHORT i = 0; i < cChars; i++)
        {
            rgChars[i].Attributes = wAttr;

            if (fIsUnicode)
            {
                rgChars[i].Char.UnicodeChar = pwszTestString[i];
            }
            else
            {
                // Ensure the top half of the union is filled with 0 for comparison purposes later.
                rgChars[i].Char.UnicodeChar = 0;
                rgChars[i].Char.AsciiChar = pszTestString[i];
            }
        }

        // This is the stated size of the buffer we're passing.
        // This console API can treat the buffer as a 2D array. We're only doing 1 dimension so the Y is 1 and the X is the number of CHAR_INFO charcters.
        COORD coordBufferSize = { 0 };
        coordBufferSize.Y = 1;
        coordBufferSize.X = cChars;

        // We want to write to the coordinate 0,0 of the buffer. The test setup function has blanked out that line.
        COORD coordBufferTarget = { 0 };

        // inclusive rectangle (bottom and right are INSIDE the read area. usually are exclusive.)
        SMALL_RECT srWriteRegion = { 0 };

        // Since we could have full-width characters, we have to "allow" the console to write up to the entire A string length (up to double the W length)
        srWriteRegion.Right = cTestString - 1;

        // Save the expected written rectangle for comparison after the call
        srWrittenExpected = { 0 };
        srWrittenExpected.Right = cChars - 1; // we expect that the written report will be the number of characters inserted, not the size of buffer consumed

        // NOTE: Don't VERIFY these calls or we will overwrite the text in the buffer with the log message.
        if (fIsUnicode)
        {
            WriteConsoleOutputW(hOut, rgChars, coordBufferSize, coordBufferTarget, &srWriteRegion);
        }
        else
        {
            WriteConsoleOutputA(hOut, rgChars, coordBufferSize, coordBufferTarget, &srWriteRegion);
        }

        // Save write region so we can print it out after we move the cursor out of the way
        srWritten = srWriteRegion;
        fUseRectWritten = true;

        delete[] rgChars;
        break;
    }
    case DbcsWriteMode::WriteConsoleOutputCharacterFunc:
    {
        COORD coordBufferTarget = { 0 };

        if (fIsUnicode)
        {
            WriteConsoleOutputCharacterW(hOut, pwszTestString, cChars, coordBufferTarget, &dwWritten);
        }
        else
        {
            WriteConsoleOutputCharacterA(hOut, pszTestString, cChars, coordBufferTarget, &dwWritten);
        }

        fUseDwordWritten = true;
        break;
    }
    case DbcsWriteMode::WriteConsoleFunc:
    {
        if (fIsUnicode)
        {
            WriteConsoleW(hOut, pwszTestString, cChars, &dwWritten, nullptr);
        }
        else
        {
            WriteConsoleA(hOut, pszTestString, cChars, &dwWritten, nullptr);
        }

        fUseDwordWritten = true;
        break;
    }
    default:
        VERIFY_FAIL(L"Unsupported write mode.");
    }

    // Free memory if appropriate (if we had to convert A to W)
    if (nullptr != pwszTestString)
    {
        delete[] pwszTestString;
    }

    // Move the cursor down a line in case log info prints out.
    COORD coordSetCursor = { 0 };
    coordSetCursor.Y = 1;
    SetConsoleCursorPosition(hOut, coordSetCursor);

    // If we had log info to print, print it now that it's safe (cursor out of the test data we printed)
    // This only matters for when the test is run in the same window as the runner and could print log information.
    if (fUseRectWritten)
    {
        Log::Comment(NoThrowString().Format(L"WriteRegion T: %d L: %d B: %d R: %d", srWritten.Top, srWritten.Left, srWritten.Bottom, srWritten.Right));
        VERIFY_ARE_EQUAL(srWrittenExpected, srWritten);
    }
    else if (fUseDwordWritten)
    {
        Log::Comment(NoThrowString().Format(L"Chars Written: %d", dwWritten));
        VERIFY_ARE_EQUAL(cChars, dwWritten);
    }
}

// 3
void DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(_In_ unsigned int const uiCodePage,
                                                    _In_ PCSTR pszTestData,
                                                    _In_ WORD const wAttrOriginal,
                                                    _In_ WORD const wAttrWritten,
                                                    _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                    _In_ size_t const cExpected)
{
    UNREFERENCED_PARAMETER(wAttrOriginal);
    Log::Comment(L"Pattern 3");
    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");

    size_t const cWideTestData = wcslen(pwszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cWideTestData);

    for (size_t i = 0; i < cWideTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        wchar_t const wch = pwszTestData[i];

        pciCurrent->Attributes = wAttrWritten;
        pciCurrent->Char.UnicodeChar = wch;
    }
}

// 1
void DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(_In_ unsigned int const uiCodePage,
                                                     _In_ PCSTR pszTestData,
                                                     _In_ WORD const wAttrOriginal,
                                                     _In_ WORD const wAttrWritten,
                                                     _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                     _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 1");
    DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    for (size_t i = 0; i < cExpected; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];

        if (0 == pciCurrent->Attributes && 0 == pciCurrent->Char.UnicodeChar)
        {
            pciCurrent->Attributes = wAttrOriginal;
            pciCurrent->Char.UnicodeChar = L'\x20';
        }
    }
}

// 2
void DbcsWriteReadTestsPrepSpacePaddedDedupeTruncatedWPattern(_In_ unsigned int const uiCodePage,
                                                              _In_ PCSTR pszTestData,
                                                              _In_ WORD const wAttrOriginal,
                                                              _In_ WORD const wAttrWritten,
                                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 2");

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");

    size_t const cWideData = wcslen(pwszTestData);

    // The maximum number of columns the console will consume is the number of wide characters there are in the string.
    // This is whether or not the characters themselves are halfwidth or fullwidth (1 col or 2 col respectively.)
    // This means that for 4 wide characters that are halfwidth (1 col), the console will copy out all 4 of them.
    // For 4 wide characters that are fullwidth (2 col each), the console will copy out 2 of them (because it will count each fullwidth as 2 when filling)
    // For a mixed string that is something like half, full, half (4 columns, 3 wchars), we will receive half, full (3 columns worth) and truncate the last half.

    size_t const cMaxColumns = cWideData;
    size_t iColumnsConsumed = 0;

    size_t iNarrow = 0;
    size_t iWide = 0;
    size_t iExpected = 0;

    size_t iNulls = 0;

    while (iColumnsConsumed < cMaxColumns)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[iExpected];
        char const chCurrent = pszTestData[iWide];
        wchar_t const wchCurrent = pwszTestData[iWide];

        pciCurrent->Attributes = wAttrWritten;
        pciCurrent->Char.UnicodeChar = wchCurrent;

        if (IsDBCSLeadByteEx(uiCodePage, chCurrent))
        {
            iColumnsConsumed += 2;
            iNarrow += 2;
            iNulls++;
        }
        else
        {
            iColumnsConsumed++;
            iNarrow++;
        }

        iWide++;
        iExpected++;
    }

    // Fill remaining with spaces and original attribute
    while (iExpected < cExpected - iNulls)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[iExpected];
        pciCurrent->Attributes = wAttrOriginal;
        pciCurrent->Char.UnicodeChar = L'\x20';

        iExpected++;
    }
}

// 13
void DbcsWriteReadTestsPrepSpacePaddedDedupeAPattern(_In_ unsigned int const uiCodePage,
                                                     _In_ PCSTR pszTestData,
                                                     _In_ WORD const wAttrOriginal,
                                                     _In_ WORD const wAttrWritten,
                                                     _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                     _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 13");

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");

    size_t const cWideData = wcslen(pwszTestData);

    // The maximum number of columns the console will consume is the number of wide characters there are in the string.
    // This is whether or not the characters themselves are halfwidth or fullwidth (1 col or 2 col respectively.)
    // This means that for 4 wide characters that are halfwidth (1 col), the console will copy out all 4 of them.
    // For 4 wide characters that are fullwidth (2 col each), the console will copy out 2 of them (because it will count each fullwidth as 2 when filling)
    // For a mixed string that is something like half, full, half (4 columns, 3 wchars), we will receive half, full (3 columns worth) and truncate the last half.

    size_t const cMaxColumns = cWideData;

    bool fIsNextTrailing = false;
    size_t i = 0;
    for (; i < cMaxColumns; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        char const chCurrent = pszTestData[i];

        pciCurrent->Attributes = wAttrWritten;
        pciCurrent->Char.AsciiChar = chCurrent;

        if (IsDBCSLeadByteEx(uiCodePage, chCurrent))
        {
            pciCurrent->Attributes |= COMMON_LVB_LEADING_BYTE;
            fIsNextTrailing = true;
        }
        else if (fIsNextTrailing)
        {
            pciCurrent->Attributes |= COMMON_LVB_TRAILING_BYTE;
            fIsNextTrailing = false;
        }
    }

    // Fill remaining with spaces and original attribute
    while (i < cExpected)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Attributes = wAttrOriginal;
        pciCurrent->Char.UnicodeChar = L'\x20';

        i++;
    }
}

// 5
void DbcsWriteReadTestsPrepDoubledWPattern(_In_ unsigned int const uiCodePage,
                                           _In_ PCSTR pszTestData,
                                           _In_ WORD const wAttrOriginal,
                                           _In_ WORD const wAttrWritten,
                                           _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                           _In_ size_t const cExpected)
{
    UNREFERENCED_PARAMETER(wAttrOriginal);
    Log::Comment(L"Pattern 5");
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");

    size_t iWide = 0;
    wchar_t wchRepeat = L'\0';
    bool fIsNextTrailing = false;
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        char const chTest = pszTestData[i];
        wchar_t const wchCopy = pwszTestData[iWide];

        pciCurrent->Attributes = wAttrWritten;

        if (IsDBCSLeadByteEx(uiCodePage, chTest))
        {
            pciCurrent->Char.UnicodeChar = wchCopy;
            iWide++;

            pciCurrent->Attributes |= COMMON_LVB_LEADING_BYTE;

            wchRepeat = wchCopy;
            fIsNextTrailing = true;
        }
        else if (fIsNextTrailing)
        {
            pciCurrent->Char.UnicodeChar = wchRepeat;

            pciCurrent->Attributes |= COMMON_LVB_TRAILING_BYTE;

            fIsNextTrailing = false;
        }
        else
        {
            pciCurrent->Char.UnicodeChar = wchCopy;
            iWide++;
        }
    }
}

// 4
void DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(_In_ unsigned int const uiCodePage,
                                                              _In_ PCSTR pszTestData,
                                                              _In_ WORD const wAttrOriginal,
                                                              _In_ WORD const wAttrWritten,
                                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 4");
    DbcsWriteReadTestsPrepDoubledWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    for (size_t i = 0; i < cExpected; i++)
    {
        CHAR_INFO* pciCurrent = &pciExpected[i];

        if (WI_IS_FLAG_SET(pciCurrent->Attributes, COMMON_LVB_TRAILING_BYTE))
        {
            pciCurrent->Char.UnicodeChar = 0xFFFF;
        }
    }
}

// 7
void DbcsWriteReadTestsPrepAStompsWNegativeOnePatternTruncateSpacePadded(_In_ unsigned int const uiCodePage,
                                                                         _In_ PCSTR pszTestData,
                                                                         _In_ WORD const wAttrOriginal,
                                                                         _In_ WORD const wAttrWritten,
                                                                         _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                                         _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 7");
    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    // Stomp all A portions of the structure from the existing pattern with the A characters
    size_t const cTestData = strlen(pszTestData);
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.AsciiChar = pszTestData[i];
    }

    // Now truncate down and space fill the space based on the max column count.
    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");

    size_t const cWideData = wcslen(pwszTestData);

    // The maximum number of columns the console will consume is the number of wide characters there are in the string.
    // This is whether or not the characters themselves are halfwidth or fullwidth (1 col or 2 col respectively.)
    // This means that for 4 wide characters that are halfwidth (1 col), the console will copy out all 4 of them.
    // For 4 wide characters that are fullwidth (2 col each), the console will copy out 2 of them (because it will count each fullwidth as 2 when filling)
    // For a mixed string that is something like half, full, half (4 columns, 3 wchars), we will receive half, full (3 columns worth) and truncate the last half.

    size_t const cMaxColumns = cWideData;

    for (size_t i = cMaxColumns; i < cExpected; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.UnicodeChar = L'\x20';
        pciCurrent->Attributes = wAttrOriginal;
    }
}

// 6
void DbcsWriteReadTestsPrepAPattern(_In_ unsigned int const uiCodePage,
                                    _In_ PCSTR pszTestData,
                                    _In_ WORD const wAttrOriginal,
                                    _In_ WORD const wAttrWritten,
                                    _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                    _In_ size_t const cExpected)
{
    UNREFERENCED_PARAMETER(wAttrOriginal);
    Log::Comment(L"Pattern 6");
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);

    bool fIsNextTrailing = false;
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        char const ch = pszTestData[i];

        pciCurrent->Attributes = wAttrWritten;
        pciCurrent->Char.AsciiChar = ch;

        if (IsDBCSLeadByteEx(uiCodePage, ch))
        {
            pciCurrent->Attributes |= COMMON_LVB_LEADING_BYTE;
            fIsNextTrailing = true;
        }
        else if (fIsNextTrailing)
        {
            pciCurrent->Attributes |= COMMON_LVB_TRAILING_BYTE;
            fIsNextTrailing = false;
        }
    }
}

// 10
void DbcsWriteReadTestsPrepWNullCoverACharPattern(_In_ unsigned int const uiCodePage,
                                                  _In_ PCSTR pszTestData,
                                                  _In_ WORD const wAttrOriginal,
                                                  _In_ WORD const wAttrWritten,
                                                  _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                  _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 10");
    DbcsWriteReadTestsPrepAPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");
    size_t const cWideData = wcslen(pwszTestData);

    size_t i = 0;
    for (; i < cWideData; i++)
    {
        pciExpected[i].Char.UnicodeChar = pwszTestData[i];
    }

    for (; i < cExpected; i++)
    {
        pciExpected[i].Char.UnicodeChar = L'\0';
    }
}

// 11
void DbcsWriteReadTestsPrepWSpaceCoverACharFixAttrPattern(_In_ unsigned int const uiCodePage,
                                                          _In_ PCSTR pszTestData,
                                                          _In_ WORD const wAttrOriginal,
                                                          _In_ WORD const wAttrWritten,
                                                          _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                          _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 11");
    DbcsWriteReadTestsPrepWNullCoverACharPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");
    size_t const cWideData = wcslen(pwszTestData);

    size_t i = 0;
    for (; i < cWideData; i++)
    {
        pciExpected[i].Attributes = wAttrWritten;
    }

    for (; i < cExpected; i++)
    {
        pciExpected[i].Char.UnicodeChar = L'\x20';
        pciExpected[i].Attributes = wAttrOriginal;
    }
}

//8
void DbcsWriteReadTestsPrepAOnDoubledWNegativeOneTrailingPattern(_In_ unsigned int const uiCodePage,
                                                                 _In_ PCSTR pszTestData,
                                                                 _In_ WORD const wAttrOriginal,
                                                                 _In_ WORD const wAttrWritten,
                                                                 _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                                 _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 8");

    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    // Stomp all A portions of the structure from the existing pattern with the A characters
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.AsciiChar = pszTestData[i];
    }
}

// 9
void DbcsWriteReadTestsPrepAOnDoubledWPattern(_In_ unsigned int const uiCodePage,
                                              _In_ PCSTR pszTestData,
                                              _In_ WORD const wAttrOriginal,
                                              _In_ WORD const wAttrWritten,
                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 9");

    DbcsWriteReadTestsPrepDoubledWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    // Stomp all A portions of the structure from the existing pattern with the A characters
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.AsciiChar = pszTestData[i];
    }
}

// 12 
void DbcsWriteReadTestsPrepACoverAttrSpacePaddedDedupeTruncatedWPattern(_In_ unsigned int const uiCodePage,
                                                                        _In_ PCSTR pszTestData,
                                                                        _In_ WORD const wAttrOriginal,
                                                                        _In_ WORD const wAttrWritten,
                                                                        _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                                        _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 12");
    DbcsWriteReadTestsPrepSpacePaddedDedupeTruncatedWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");
    size_t const cWideData = wcslen(pwszTestData);

    size_t i = 0;
    bool fIsNextTrailing = false;
    for (; i < cWideData; i++)
    {
        pciExpected[i].Attributes = wAttrWritten;

        if (IsDBCSLeadByteEx(uiCodePage, pszTestData[i]))
        {
            pciExpected[i].Attributes |= COMMON_LVB_LEADING_BYTE;
            fIsNextTrailing = true;
        }
        else if (fIsNextTrailing)
        {
            pciExpected[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
            fIsNextTrailing = false;
        }

    }

    for (; i < cExpected; i++)
    {
        pciExpected[i].Attributes = wAttrOriginal;
    }
}

// 14
void DbcsWriteReadTestsPrepTrueTypeCharANullWithAttrs(_In_ unsigned int const uiCodePage,
                                                  _In_ PCSTR pszTestData,
                                                  _In_ WORD const wAttrOriginal,
                                                  _In_ WORD const wAttrWritten,
                                                  _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                  _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 14");
    int const iwchNeeded = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(uiCodePage, 0, pszTestData, -1, pwszTestData, iwchNeeded);
    CheckLastErrorZeroFail(iSuccess, L"MultiByteToWideChar");
    size_t const cWideData = wcslen(pwszTestData);
    
    // Fill the number of columns worth of wide characters with the write attribute. The rest get the original attribute.
    size_t i;
    for (i = 0; i < cWideData; i++)
    {
        pciExpected[i].Attributes = wAttrWritten;
    }

    for (; i < cExpected; i++)
    {
        pciExpected[i].Attributes = wAttrOriginal;
    }

    // For characters, if the string contained NO double-byte characters, it will return. Otherwise, it won't return due to
    // a long standing bug in the console's way it calls RtlUnicodeToOemN
    size_t const cTestData = strlen(pszTestData);
    if (cWideData == cTestData)
    {
        for (i = 0; i < cTestData; i++)
        {
            pciExpected[i].Char.AsciiChar = pszTestData[i];
        }
    }
}

void DbcsWriteReadTestsPrepExpectedReadConsoleOutput(_In_ unsigned int const uiCodePage,
                                                     _In_ PCSTR pszTestData,
                                                     _In_ WORD const wAttrOriginal,
                                                     _In_ WORD const wAttrWritten,
                                                     _In_ DbcsWriteMode const WriteMode,
                                                     _In_ bool const fWriteWithUnicode,
                                                     _In_ bool const fIsTrueTypeFont,
                                                     _In_ bool const fReadWithUnicode,
                                                     _Inout_updates_all_(cExpectedNeeded) CHAR_INFO* const rgciExpected,
                                                     _In_ size_t const cExpectedNeeded)
{
    switch (WriteMode)
    {
    case DbcsWriteMode::WriteConsoleOutputFunc:
    {
        // If we wrote with WriteConsoleOutput*, things are going to be munged depending on the font and the A/W status of both the write and the read.
        if (!fReadWithUnicode)
        {
            // If we read it back with the A functions, the font might matter.
            // We will get different results dependent on whether the original text was written with the W or A method.
            if (fWriteWithUnicode)
            {
                if (fIsTrueTypeFont)
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputA under TT font, we will get a deduplicated
                    // set of Unicode characters (YES. Unicode characters despite calling the A API to read back) that is space padded out
                    // There will be no lead/trailing markings.
                    DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputA under Raster font, we will get the 
                    // double-byte sequences stomped on top of a Unicode filled CHAR_INFO structure that used -1 for trailing bytes.
                    DbcsWriteReadTestsPrepAStompsWNegativeOnePatternTruncateSpacePadded(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
            else
            {
                // When written with WriteConsoleOutputA and read back with ReadConsoleOutputA,
                // we will get back the double-byte sequences appropriately labeled with leading/trailing bytes.
                //DbcsWriteReadTestsPrepAPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                DbcsWriteReadTestsPrepAOnDoubledWNegativeOneTrailingPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
        }
        else
        {
            // If we read it back with the W functions, both the font and the original write mode (A vs. W) matter
            if (fIsTrueTypeFont)
            {
                if (fWriteWithUnicode)
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputW when the font is TrueType,
                    // we will get a deduplicated set of Unicode characters with no lead/trailing markings and space padded at the end.
                    DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputA when the font is TrueType,
                    // we will get back Unicode characters doubled up and marked with leading and trailing bytes...
                    // ... except all the trailing bytes character values will be -1.
                    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
            else
            {
                if (fWriteWithUnicode)
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputW when the font is Raster,
                    // we will get a deduplicated set of Unicode characters with no lead/trailing markings and space padded at the end...
                    // ... except something weird happens with truncation (TODO figure out what)
                    DbcsWriteReadTestsPrepSpacePaddedDedupeTruncatedWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputA and read back with ReadConsoleOutputW when the font is Raster,
                    // we will get back de-duplicated Unicode characters with no lead / trail markings.The extra array space will remain null.
                    DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
        }
        break;
    }
    case DbcsWriteMode::CrtWrite:
    case DbcsWriteMode::WriteConsoleOutputCharacterFunc:
    case DbcsWriteMode::WriteConsoleFunc:
    {
        // Writing with the CRT down here.
        if (!fReadWithUnicode)
        {
            // If we wrote with the CRT and are reading with A functions, the font doesn't matter.
            // We will always get back the double-byte sequences appropriately labeled with leading/trailing bytes.
            //DbcsWriteReadTestsPrepPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            DbcsWriteReadTestsPrepAOnDoubledWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
        }
        else
        {
            // If we wrote with the CRT and are reading back with the W functions, the font does matter.
            if (fIsTrueTypeFont)
            {
                // In a TrueType font, we will get back Unicode characters doubled up and marked with leading and trailing bytes.
                DbcsWriteReadTestsPrepDoubledWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
            else
            {
                // In a Raster font, we will get back de-duplicated Unicode characters with no lead/trail markings. The extra array space will remain null.
                DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
        }
        break;
    }
    default:
        VERIFY_FAIL(L"Unsupported write mode");
    }
}

void DbcsWriteReadTestsPrepExpectedReadConsoleOutputCharacter(_In_ unsigned int const uiCodePage,
                                                              _In_ PCSTR pszTestData,
                                                              _In_ WORD const wAttrOriginal,
                                                              _In_ WORD const wAttrWritten,
                                                              _In_ DbcsWriteMode const WriteMode,
                                                              _In_ bool const fWriteWithUnicode,
                                                              _In_ bool const fIsTrueTypeFont,
                                                              _In_ bool const fReadWithUnicode,
                                                              _Inout_updates_all_(cExpectedNeeded) CHAR_INFO* const rgciExpected,
                                                              _In_ size_t const cExpectedNeeded)
{
    if (DbcsWriteMode::WriteConsoleOutputFunc == WriteMode && fWriteWithUnicode)
    {
        if (fIsTrueTypeFont)
        {
            if (fReadWithUnicode)
            {
                DbcsWriteReadTestsPrepWSpaceCoverACharFixAttrPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
            else
            {
                DbcsWriteReadTestsPrepTrueTypeCharANullWithAttrs(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
        }
        else
        {
            if (fReadWithUnicode)
            {
                DbcsWriteReadTestsPrepACoverAttrSpacePaddedDedupeTruncatedWPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
            else
            {
                DbcsWriteReadTestsPrepSpacePaddedDedupeAPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
        }
    }
    else
    {
        if (!fReadWithUnicode)
        {
            DbcsWriteReadTestsPrepAPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
        }
        else
        {
            DbcsWriteReadTestsPrepWNullCoverACharPattern(uiCodePage, pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
        }
    }
}

void DbcsWriteReadTestsPrepExpected(_In_ unsigned int const uiCodePage,
                                    _In_ PCSTR pszTestData,
                                    _In_ WORD const wAttrOriginal,
                                    _In_ WORD const wAttrWritten,
                                    _In_ DbcsWriteMode const WriteMode,
                                    _In_ bool const fWriteWithUnicode,
                                    _In_ bool const fIsTrueTypeFont,
                                    _In_ DbcsReadMode const ReadMode,
                                    _In_ bool const fReadWithUnicode,
                                    _Outptr_result_buffer_(*pcExpected) CHAR_INFO** const ppciExpected,
                                    _Out_ size_t* const pcExpected)
{
    // We will expect to read back one CHAR_INFO for every A character we sent to the console using the assumption above.
    // We expect that reading W characters will always be less than or equal to that.
    size_t const cExpectedNeeded = strlen(pszTestData);

    // Allocate and zero out the space so comparisons don't fail from garbage bytes.
    CHAR_INFO* rgciExpected = new CHAR_INFO[cExpectedNeeded];
    ZeroMemory(rgciExpected, sizeof(CHAR_INFO) * cExpectedNeeded);

    switch (ReadMode)
    {
    case DbcsReadMode::ReadConsoleOutputFunc:
    {
        DbcsWriteReadTestsPrepExpectedReadConsoleOutput(uiCodePage,
                                                        pszTestData,
                                                        wAttrOriginal,
                                                        wAttrWritten,
                                                        WriteMode,
                                                        fWriteWithUnicode,
                                                        fIsTrueTypeFont,
                                                        fReadWithUnicode,
                                                        rgciExpected,
                                                        cExpectedNeeded);
        break;
    }
    case DbcsReadMode::ReadConsoleOutputCharacterFunc:
    {
        DbcsWriteReadTestsPrepExpectedReadConsoleOutputCharacter(uiCodePage,
                                                                 pszTestData,
                                                                 wAttrOriginal,
                                                                 wAttrWritten,
                                                                 WriteMode,
                                                                 fWriteWithUnicode,
                                                                 fIsTrueTypeFont,
                                                                 fReadWithUnicode,
                                                                 rgciExpected,
                                                                 cExpectedNeeded);
        break;
    }
    default:
    {
        VERIFY_FAIL(L"Unknown read mode.");
        break;
    }
    }

    // Return the expected array and the length that should be used for comparison at the end of the test.
    *ppciExpected = rgciExpected;
    *pcExpected = cExpectedNeeded;
}

void DbcsWriteReadTestsRetrieveOutput(_In_ HANDLE const hOut,
                                      _In_ DbcsReadMode const ReadMode, _In_ bool const fReadUnicode,
                                      _Out_writes_(cChars) CHAR_INFO* const rgChars, _In_ SHORT const cChars)
{
    COORD coordBufferTarget = { 0 };

    switch (ReadMode)
    {
    case DbcsReadMode::ReadConsoleOutputFunc:
    {
        // Since we wrote (in SendOutput function) to the 0,0 line, we need to read back the same width from that line.
        COORD coordBufferSize = { 0 };
        coordBufferSize.Y = 1;
        coordBufferSize.X = cChars;

        SMALL_RECT srReadRegion = { 0 }; // inclusive rectangle (bottom and right are INSIDE the read area. usually are exclusive.)
        srReadRegion.Right = cChars - 1;

        // return value for read region shouldn't change
        SMALL_RECT const srReadRegionExpected = srReadRegion;

        if (!fReadUnicode)
        {
            VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputA(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion));
        }
        else
        {
            VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputW(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion));
        }

        Log::Comment(NoThrowString().Format(L"ReadRegion T: %d L: %d B: %d R: %d", srReadRegion.Top, srReadRegion.Left, srReadRegion.Bottom, srReadRegion.Right));
        VERIFY_ARE_EQUAL(srReadRegionExpected, srReadRegion);
        break;
    }
    case DbcsReadMode::ReadConsoleOutputCharacterFunc:
    {
        DWORD dwRead = 0;
        if (!fReadUnicode)
        {
            PSTR psRead = new char[cChars];
            VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputCharacterA(hOut, psRead, cChars, coordBufferTarget, &dwRead));

            for (size_t i = 0; i < dwRead; i++)
            {
                rgChars[i].Char.AsciiChar = psRead[i];
            }

            delete[] psRead;
        }
        else
        {
            PWSTR pwsRead = new wchar_t[cChars];
            VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputCharacterW(hOut, pwsRead, cChars, coordBufferTarget, &dwRead));

            for (size_t i = 0; i < dwRead; i++)
            {
                rgChars[i].Char.UnicodeChar = pwsRead[i];
            }

            delete[] pwsRead;
        }

        PWORD pwAttrs = new WORD[cChars];
        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputAttribute(hOut, pwAttrs, cChars, coordBufferTarget, &dwRead));

        for (size_t i = 0; i < dwRead; i++)
        {
            rgChars[i].Attributes = pwAttrs[i];
        }

        delete[] pwAttrs;
        break;
    }
    default:
        VERIFY_FAIL(L"Unknown read mode");
        break;
    }
}

void DbcsWriteReadTestsVerify(_In_reads_(cExpected) CHAR_INFO* const rgExpected, _In_ size_t const cExpected,
                              _In_reads_(cExpected) CHAR_INFO* const rgActual)
{
    // We will walk through for the number of CHAR_INFOs expected. 
    for (size_t i = 0; i < cExpected; i++)
    {
        // Uncomment these lines for help debugging the verification.
        /*Log::Comment(VerifyOutputTraits<CHAR_INFO>::ToString(rgExpected[i]));
        Log::Comment(VerifyOutputTraits<CHAR_INFO>::ToString(rgActual[i]));*/

        VERIFY_ARE_EQUAL(rgExpected[i], rgActual[i]);
    }
}

void DbcsWriteReadTestRunner(_In_ unsigned int const uiCodePage,
                             _In_ PCSTR pszTestData,
                             _In_opt_ WORD* const pwAttrOverride,
                             _In_ bool const fUseTrueType,
                             _In_ DbcsWriteMode const WriteMode,
                             _In_ bool const fWriteInUnicode,
                             _In_ DbcsReadMode const ReadMode,
                             _In_ bool const fReadWithUnicode)
{
    // First we need to set up the tests by clearing out the first line of the buffer,
    // retrieving the appropriate output handle, and getting the colors (attributes)
    // used by default in the buffer (set during clearing as well).
    HANDLE hOut;
    WORD wAttributes;
    SetupDbcsWriteReadTests(uiCodePage, fUseTrueType, &hOut, &wAttributes);
    WORD const wAttrOriginal = wAttributes;

    // Some tests might want to override the colors applied to ensure both parts of the CHAR_INFO union 
    // work for methods that support sending that union. (i.e. not the CRT path)
    if (nullptr != pwAttrOverride)
    {
        wAttributes = *pwAttrOverride;
    }

    // The console bases the space it walks for DBCS conversions on the length of the A version of the text.
    // Store that length now so we have it for our read/write operations.
    size_t const cTestData = strlen(pszTestData);

    // Write the string under test into the appropriate WRITE API for this test.
    DbcsWriteReadTestsSendOutput(hOut, uiCodePage, WriteMode, fWriteInUnicode, pszTestData, wAttributes);

    // Prepare the array of CHAR_INFO structs that we expect to receive back when we will call read in a moment.
    // This can vary based on font, unicode/non-unicode (when reading AND writing), and codepage.
    CHAR_INFO* pciExpected;
    size_t cExpected;
    DbcsWriteReadTestsPrepExpected(uiCodePage, pszTestData, wAttrOriginal, wAttributes, WriteMode, fWriteInUnicode, fUseTrueType, ReadMode, fReadWithUnicode, &pciExpected, &cExpected);

    // Now call the appropriate READ API for this test.
    CHAR_INFO* pciActual = new CHAR_INFO[cTestData];
    ZeroMemory(pciActual, sizeof(CHAR_INFO) * cTestData);
    DbcsWriteReadTestsRetrieveOutput(hOut, ReadMode, fReadWithUnicode, pciActual, (SHORT)cTestData);

    // Loop through and verify that our expected array matches what was actually returned by the given API.
    DbcsWriteReadTestsVerify(pciExpected, cExpected, pciActual);

    // Free allocated structures
    delete[] pciActual;
    delete[] pciExpected;
}

void DbcsTests::TestDbcsWriteRead()
{
    unsigned int uiCodePage;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"uiCodePage", uiCodePage));

    bool fUseTrueTypeFont;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fUseTrueTypeFont", fUseTrueTypeFont));

    int iWriteMode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"WriteMode", iWriteMode));
    DbcsWriteMode WriteMode = (DbcsWriteMode)iWriteMode;

    bool fWriteInUnicode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fWriteInUnicode", fWriteInUnicode));

    int iReadMode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"ReadMode", iReadMode));
    DbcsReadMode ReadMode = (DbcsReadMode)iReadMode;

    bool fReadInUnicode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fReadInUnicode", fReadInUnicode));

    PCWSTR pwszWriteMode = L"";
    switch (WriteMode)
    {
    case DbcsWriteMode::CrtWrite:
        pwszWriteMode = L"CRT";
        break;
    case DbcsWriteMode::WriteConsoleOutputFunc:
        pwszWriteMode = L"WriteConsoleOutput";
        break;
    case DbcsWriteMode::WriteConsoleOutputCharacterFunc:
        pwszWriteMode = L"WriteConsoleOutputCharacter";
        break;
    case DbcsWriteMode::WriteConsoleFunc:
        pwszWriteMode = L"WriteConsole";
        break;
    default:
        VERIFY_FAIL(L"Write mode not supported");
    }

    PCWSTR pwszReadMode = L"";
    switch (ReadMode)
    {
    case DbcsReadMode::ReadConsoleOutputFunc:
        pwszReadMode = L"ReadConsoleOutput";
        break;
    case DbcsReadMode::ReadConsoleOutputCharacterFunc:
        pwszReadMode = L"ReadConsoleOutputCharacter";
        break;
    default:
        VERIFY_FAIL(L"Read mode not supported");
    }

    auto testInfo = NoThrowString().Format(L"\r\n\r\n\r\nUse '%ls' font. Write with %ls '%ls'. Check Read with %ls '%ls' API. Use %d codepage.\r\n",
                                           fUseTrueTypeFont ? L"TrueType" : L"Raster",
                                           pwszWriteMode,
                                           fWriteInUnicode ? L"W" : L"A",
                                           pwszReadMode,
                                           fReadInUnicode ? L"W" : L"A",
                                           uiCodePage);

    Log::Comment(testInfo);

    PCSTR pszTestData = "";
    switch (uiCodePage)
    {
    case ENGLISH_US_CP:
        pszTestData = "QWERTYUIOP";
        break;
    case JAPANESE_CP:
        pszTestData = "Q\x82\xA2\x82\xa9\x82\xc8ZYXWVUT\x82\xc9";
        break;
    default:
        VERIFY_FAIL(L"No test data for this codepage");
        break;
    }

    WORD wAttributes = 0;

    if (WriteMode == 1)
    {
        Log::Comment(L"We will also try to change the color since WriteConsoleOutput supports it.");
        wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    }

    DbcsWriteReadTestRunner(uiCodePage,
                            pszTestData,
                            wAttributes != 0 ? &wAttributes : nullptr,
                            fUseTrueTypeFont,
                            WriteMode,
                            fWriteInUnicode,
                            ReadMode,
                            fReadInUnicode);

    Log::Comment(testInfo);

}

// This test covers bisect-prevention handling. This is the behavior where a double-wide character will not be spliced
// across a line boundary and will instead be advanced onto the next line.
// It additionally exercises the word wrap functionality to ensure that the bisect calculations continue
// to apply properly when wrap occurs.
void DbcsTests::TestDbcsBisect()
{
    HANDLE const hOut = GetStdOutputHandle();

    UINT dwCP = GetConsoleCP();
    VERIFY_ARE_EQUAL(dwCP, JAPANESE_CP);

    UINT dwOutputCP = GetConsoleOutputCP();
    VERIFY_ARE_EQUAL(dwOutputCP, JAPANESE_CP);

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex = { 0 };
    sbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    BOOL fSuccess = GetConsoleScreenBufferInfoEx(hOut, &sbiex);

    if (CheckLastError(fSuccess, L"GetConsoleScreenBufferInfoEx"))
    {
        Log::Comment(L"Set cursor position to the last column in the buffer width.");
        sbiex.dwCursorPosition.X = sbiex.dwSize.X - 1;

        COORD const coordEndOfLine = sbiex.dwCursorPosition; // this is the end of line position we're going to write at
        COORD coordStartOfNextLine;
        coordStartOfNextLine.X = 0;
        coordStartOfNextLine.Y = sbiex.dwCursorPosition.Y + 1;

        fSuccess = SetConsoleCursorPosition(hOut, sbiex.dwCursorPosition);
        if (CheckLastError(fSuccess, L"SetConsoleScreenBufferInfoEx"))
        {
            Log::Comment(L"Attempt to write (standard WriteConsole) a double-wide character and ensure that it is placed onto the following line, not bisected.");
            DWORD dwWritten = 0;
            WCHAR const wchHiraganaU = L'\x3046';
            WCHAR const wchSpace = L' ';
            fSuccess = WriteConsoleW(hOut, &wchHiraganaU, 1, &dwWritten, nullptr);

            if (CheckLastError(fSuccess, L"WriteConsoleW"))
            {
                VERIFY_ARE_EQUAL(1u, dwWritten, L"We should have only written the one character.");

                // Read the end of line character and the start of the next line.
                // A proper bisect should have left the end of line character empty (a space)
                // and then put the character at the beginning of the next line.

                Log::Comment(L"Confirm that the end of line was left empty to prevent bisect.");
                WCHAR wchBuffer;
                fSuccess = ReadConsoleOutputCharacterW(hOut, &wchBuffer, 1, coordEndOfLine, &dwWritten);
                if (CheckLastError(fSuccess, L"ReadConsoleOutputCharacterW"))
                {
                    VERIFY_ARE_EQUAL(1u, dwWritten, L"We should have only read one character back at the end of the line.");

                    VERIFY_ARE_EQUAL(wchSpace, wchBuffer, L"A space character should have been left at the end of the line.");

                    Log::Comment(L"Confirm that the wide character was written on the next line down instead.");
                    WCHAR wchBuffer2[2];
                    fSuccess = ReadConsoleOutputCharacterW(hOut, wchBuffer2, 2, coordStartOfNextLine, &dwWritten);
                    if (CheckLastError(fSuccess, L"ReadConsoleOutputCharacterW"))
                    {
                        VERIFY_ARE_EQUAL(1u, dwWritten, L"We should have only read one character back at the beginning of the next line.");

                        VERIFY_ARE_EQUAL(wchHiraganaU, wchBuffer2[0], L"The same character we passed in should have been read back.");

                        Log::Comment(L"Confirm that the cursor has advanced past the double wide character.");
                        fSuccess = GetConsoleScreenBufferInfoEx(hOut, &sbiex);
                        if (CheckLastError(fSuccess, L"GetConsoleScreenBufferInfoEx"))
                        {
                            VERIFY_ARE_EQUAL(coordStartOfNextLine.Y, sbiex.dwCursorPosition.Y, L"Cursor has moved down to next line.");
                            VERIFY_ARE_EQUAL(coordStartOfNextLine.X + 2, sbiex.dwCursorPosition.X, L"Cursor has advanced two spaces on next line for double wide character.");

                            // TODO: This bit needs to move into a UIA test
                            /*Log::Comment(L"We can only run the resize test in the v2 console. We'll skip it if it turns out v2 is off.");
                            if (IsV2Console())
                            {
                                Log::Comment(L"Test that the character moves back up when the window is unwrapped. Make the window one larger.");
                                sbiex.srWindow.Right++;
                                sbiex.dwSize.X++;
                                fSuccess = SetConsoleScreenBufferInfoEx(hOut, &sbiex);
                                if (CheckLastError(fSuccess, L"SetConsoleScreenBufferInfoEx"))
                                {
                                    ZeroMemory(wchBuffer2, ARRAYSIZE(wchBuffer2) * sizeof(WCHAR));
                                    Log::Comment(L"Check that the character rolled back up onto the previous line.");
                                    fSuccess = ReadConsoleOutputCharacterW(hOut, wchBuffer2, 2, coordEndOfLine, &dwWritten);
                                    if (CheckLastError(fSuccess, L"ReadConsoleOutputCharacterW"))
                                    {
                                        VERIFY_ARE_EQUAL(1u, dwWritten, L"We should have read 1 character up on the previous line.");

                                        VERIFY_ARE_EQUAL(wchHiraganaU, wchBuffer2[0], L"The character should now be up one line.");

                                        Log::Comment(L"Now shrink the window one more time and make sure the character rolls back down a line.");
                                        sbiex.srWindow.Right--;
                                        sbiex.dwSize.X--;
                                        fSuccess = SetConsoleScreenBufferInfoEx(hOut, &sbiex);
                                        if (CheckLastError(fSuccess, L"SetConsoleScreenBufferInfoEx"))
                                        {
                                            ZeroMemory(wchBuffer2, ARRAYSIZE(wchBuffer2) * sizeof(WCHAR));
                                            Log::Comment(L"Check that the character rolled down onto the next line again.");
                                            fSuccess = ReadConsoleOutputCharacterW(hOut, wchBuffer2, 2, coordStartOfNextLine, &dwWritten);
                                            if (CheckLastError(fSuccess, L"ReadConsoleOutputCharacterW"))
                                            {
                                                VERIFY_ARE_EQUAL(1u, dwWritten, L"We should have read 1 character back down again on the next line.");

                                                VERIFY_ARE_EQUAL(wchHiraganaU, wchBuffer2[0], L"The character should now be down on the 2nd line again.");
                                            }
                                        }
                                    }
                                }
                            }*/
                        }
                    }
                }
            }
        }
    }
}

struct MultibyteInputData
{
    PCWSTR pwszInputText;
    PCSTR pszExpectedText;
};

const MultibyteInputData MultibyteTestDataSet[] =
{
    { L"\x3042", "\x82\xa0" },
    { L"\x3042" L"3", "\x82\xa0\x33" },
    { L"3" L"\x3042", "\x33\x82\xa0" },
    { L"3" L"\x3042" L"\x3044", "\x33\x82\xa0\x82\xa2" },
    { L"3" L"\x3042" L"\x3044" L"\x3042", "\x33\x82\xa0\x82\xa2\x82\xa0" },
    { L"3" L"\x3042" L"\x3044" L"\x3042" L"\x3044", "\x33\x82\xa0\x82\xa2\x82\xa0\x82\xa2" },
};

void WriteStringToInput(HANDLE hIn, PCWSTR pwszString)
{
    size_t const cchString = wcslen(pwszString);
    size_t const cRecords = cchString * 2; // We need double the input records for button down then button up.

    INPUT_RECORD* const irString = new INPUT_RECORD[cRecords];

    for (size_t i = 0; i < cRecords; i++)
    {
        irString[i].EventType = KEY_EVENT;
        irString[i].Event.KeyEvent.bKeyDown = (i % 2 == 0) ? TRUE : FALSE;
        irString[i].Event.KeyEvent.dwControlKeyState = 0;
        irString[i].Event.KeyEvent.uChar.UnicodeChar = pwszString[i / 2];
        irString[i].Event.KeyEvent.wRepeatCount = 1;
        irString[i].Event.KeyEvent.wVirtualKeyCode = 0;
        irString[i].Event.KeyEvent.wVirtualKeyCode = 0;
    }

    DWORD dwWritten;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleInputW(hIn, irString, (DWORD)cRecords, &dwWritten));

    VERIFY_ARE_EQUAL(cRecords, dwWritten, L"We should have written the number of records that were sent in by our buffer.");

    delete[] irString;
}

void ReadStringWithGetCh(PCSTR pszExpectedText)
{
    size_t const cchString = strlen(pszExpectedText);

    for (size_t i = 0; i < cchString; i++)
    {
        if (!VERIFY_ARE_EQUAL((BYTE)pszExpectedText[i], _getch()))
        {
            break;
        }
    }
}

void ReadStringWithReadConsoleInputAHelper(HANDLE hIn, PCSTR pszExpectedText, size_t cbBuffer)
{
    Log::Comment(String().Format(L"  = Attempting to read back the text with a %d record length buffer. =", cbBuffer));

    // Find out how many bytes we need to read.
    size_t const cchExpectedText = strlen(pszExpectedText);

    // Increment read buffer of the size we were told.
    INPUT_RECORD* const irRead = new INPUT_RECORD[cbBuffer];

    // Loop reading and comparing until we've read enough times to get all the text we expect.
    size_t cchRead = 0;

    while (cchRead < cchExpectedText)
    {
        // expected read is either the size of the buffer or the number of characters remaining, whichever is smaller.
        DWORD const dwReadExpected = (DWORD)min(cbBuffer, cchExpectedText - cchRead);

        DWORD dwRead;
        if (!VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleInputA(hIn, irRead, (DWORD)cbBuffer, &dwRead), L"Attempt to read input into buffer."))
        {
            break;
        }

        VERIFY_IS_GREATER_THAN_OR_EQUAL(dwRead, (DWORD)0, L"Verify we read non-negative bytes.");

        for (size_t i = 0; i < dwRead; i++)
        {
            // We might read more events than the ones we're looking for because some other type of event was
            // inserted into the queue by outside action. Only look at the key down events.
            if (irRead[i].EventType == KEY_EVENT &&
                irRead[i].Event.KeyEvent.bKeyDown == TRUE)
            {
                if (!VERIFY_ARE_EQUAL((BYTE)pszExpectedText[cchRead], (BYTE)irRead[i].Event.KeyEvent.uChar.AsciiChar))
                {
                    break;
                }
                cchRead++;
            }
        }
    }
}

void ReadStringWithReadConsoleInputA(HANDLE hIn, PCWSTR pwszWriteText, PCSTR pszExpectedText)
{
    // Figure out how long the expected length is.
    size_t const cchExpectedText = strlen(pszExpectedText);

    // Test every buffer size variation from 1 to the size of the string.
    for (size_t i = 1; i <= cchExpectedText; i++)
    {
        FlushConsoleInputBuffer(hIn);
        WriteStringToInput(hIn, pwszWriteText);
        ReadStringWithReadConsoleInputAHelper(hIn, pszExpectedText, i);
    }
}

void DbcsTests::TestMultibyteInputRetrieval()
{
    SetConsoleCP(932);

    UINT dwCP = GetConsoleCP();
    if (!VERIFY_ARE_EQUAL(JAPANESE_CP, dwCP, L"Ensure input codepage is Japanese."))
    {
        return;
    }

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (!VERIFY_ARE_NOT_EQUAL(INVALID_HANDLE_VALUE, hIn, L"Get input handle."))
    {
        return;
    }

    size_t const cDataSet = ARRAYSIZE(MultibyteTestDataSet);

    // for each item in our test data set...
    for (size_t i = 0; i < cDataSet; i++)
    {
        MultibyteInputData data = MultibyteTestDataSet[i];

        Log::Comment(String().Format(L"=== TEST #%d ===", i));
        Log::Comment(String().Format(L"=== Input '%ws' ===", data.pwszInputText));

        // test by writing the string and reading back the _getch way.
        Log::Comment(L" == SUBTEST A: Use _getch to retrieve. == ");
        FlushConsoleInputBuffer(hIn);
        WriteStringToInput(hIn, data.pwszInputText);
        ReadStringWithGetCh(data.pszExpectedText);

        // test by writing the string and reading back with variable length buffers the ReadConsoleInputA way.
        Log::Comment(L" == SUBTEST B: Use ReadConsoleInputA with variable length buffers to retrieve. == ");
        ReadStringWithReadConsoleInputA(hIn, data.pwszInputText, data.pszExpectedText);
    }

    FlushConsoleInputBuffer(hIn);
}
