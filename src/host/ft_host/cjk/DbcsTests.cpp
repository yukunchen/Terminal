/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include <io.h>
#include <fcntl.h>

#define JAPANESE_CP 932u

enum DbcsWriteMode
{
    CrtWrite = 0,
    WriteConsoleOutputFunc = 1
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
        TEST_METHOD_PROPERTY(L"Data:fUseTrueTypeFont", L"{true, false}")
        TEST_METHOD_PROPERTY(L"Data:WriteMode", L"{0, 1}")
        //TEST_METHOD_PROPERTY(L"Data:fUseCrtWrite", L"{true, false}")
        TEST_METHOD_PROPERTY(L"Data:fWriteInUnicode", L"{true, false}")
        TEST_METHOD_PROPERTY(L"Data:fReadInUnicode", L"{true, false}")
    END_TEST_METHOD()

    TEST_METHOD(TestDbcsBisect);
};

HANDLE hScreen = INVALID_HANDLE_VALUE;

bool DbcsTests::DbcsTestSetup()
{
    return true;
}

void SetupDbcsWriteReadTests(_In_ bool fIsTrueType,
                             _Out_ HANDLE* const phOut,
                             _Out_ WORD* const pwAttributes)
{
    HANDLE const hOut = GetStdOutputHandle();

    // Ensure that the console is set into the appropriate codepage for the test
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleCP(JAPANESE_CP));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleOutputCP(JAPANESE_CP));

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
        // We use MS Gothic as the default font name for Japanese codepage.
        wcscpy_s(cfiex.FaceName, L"MS Gothic");
        cfiex.dwFontSize.Y = 16;
    }

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetCurrentConsoleFontEx(hOut, FALSE, &cfiex));

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

void DbcsWriteReadTestsSendOutput(_In_ HANDLE const hOut,
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
        int const icchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestString, -1, nullptr, 0);

        pwszTestString = new WCHAR[icchNeeded];

        int const iRes = MultiByteToWideChar(JAPANESE_CP, 0, pszTestString, -1, pwszTestString, icchNeeded);
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
    bool fUseWritten = false;
    SMALL_RECT srWrittenExpected = { 0 };
    SMALL_RECT srWritten = { 0 };

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
        fUseWritten = true;

        delete[] rgChars;
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
    if (fUseWritten)
    {
        Log::Comment(NoThrowString().Format(L"WriteRegion T: %d L: %d B: %d R: %d", srWritten.Top, srWritten.Left, srWritten.Bottom, srWritten.Right));
        VERIFY_ARE_EQUAL(srWrittenExpected, srWritten);
    }
}

// 3
void DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(_In_ PCSTR pszTestData,
                                                    _In_ WORD wAttrOriginal,
                                                    _In_ WORD wAttrWritten,
                                                    _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                    _In_ size_t const cExpected)
{
    UNREFERENCED_PARAMETER(wAttrOriginal);
    Log::Comment(L"Pattern 3");
    int const iwchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, pwszTestData, iwchNeeded);
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
void DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(_In_ PCSTR pszTestData,
                                                     _In_ WORD wAttrOriginal,
                                                     _In_ WORD wAttrWritten,
                                                     _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                     _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 1");
    DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

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
void DbcsWriteReadTestsPrepSpacePaddedDedupeTruncatedWPattern(_In_ PCSTR pszTestData,
                                                              _In_ WORD wAttrOriginal,
                                                              _In_ WORD wAttrWritten,
                                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 2");
    
    int const iwchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, pwszTestData, iwchNeeded);
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

        if (IsDBCSLeadByteEx(JAPANESE_CP, chCurrent))
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

// 5
void DbcsWriteReadTestsPrepDoubledWPattern(_In_ PCSTR pszTestData,
                                           _In_ WORD wAttrOriginal,
                                           _In_ WORD wAttrWritten,
                                           _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                           _In_ size_t const cExpected)
{
    UNREFERENCED_PARAMETER(wAttrOriginal);
    Log::Comment(L"Pattern 5");
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);

    int const iwchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, pwszTestData, iwchNeeded);
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

        if (IsDBCSLeadByteEx(JAPANESE_CP, chTest))
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
void DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(_In_ PCSTR pszTestData,
                                                              _In_ WORD wAttrOriginal,
                                                              _In_ WORD wAttrWritten,
                                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 4");
    DbcsWriteReadTestsPrepDoubledWPattern(pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

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
void DbcsWriteReadTestsPrepAStompsWNegativeOnePatternTruncateSpacePadded(_In_ PCSTR pszTestData,
                                                      _In_ WORD wAttrOriginal,
                                                      _In_ WORD wAttrWritten,
                                                      _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                      _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 7");
    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    // Stomp all A portions of the structure from the existing pattern with the A characters
    size_t const cTestData = strlen(pszTestData);
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.AsciiChar = pszTestData[i];
    }

    // Now truncate down and space fill the space based on the max column count.
    int const iwchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, nullptr, 0);
    PWSTR pwszTestData = new wchar_t[iwchNeeded];
    int const iSuccess = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, -1, pwszTestData, iwchNeeded);
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
void DbcsWriteReadTestsPrepAPattern(_In_ PCSTR pszTestData,
                                    _In_ WORD wAttrOriginal,
                                    _In_ WORD wAttrWritten,
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

        if (IsDBCSLeadByteEx(JAPANESE_CP, ch))
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

//8
void DbcsWriteReadTestsPrepAOnDoubledWNegativeOneTrailingPattern(_In_ PCSTR pszTestData,
                                                                 _In_ WORD wAttrOriginal,
                                                                 _In_ WORD wAttrWritten,
                                                                 _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                                                 _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 8");

    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

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
void DbcsWriteReadTestsPrepAOnDoubledWPattern(_In_ PCSTR pszTestData,
                                              _In_ WORD wAttrOriginal,
                                              _In_ WORD wAttrWritten,
                                              _Inout_updates_all_(cExpected) CHAR_INFO* const pciExpected,
                                              _In_ size_t const cExpected)
{
    Log::Comment(L"Pattern 9");

    DbcsWriteReadTestsPrepDoubledWPattern(pszTestData, wAttrOriginal, wAttrWritten, pciExpected, cExpected);

    // Stomp all A portions of the structure from the existing pattern with the A characters
    size_t const cTestData = strlen(pszTestData);
    VERIFY_IS_GREATER_THAN_OR_EQUAL(cExpected, cTestData);
    for (size_t i = 0; i < cTestData; i++)
    {
        CHAR_INFO* const pciCurrent = &pciExpected[i];
        pciCurrent->Char.AsciiChar = pszTestData[i];
    }
}

void DbcsWriteReadTestsPrepExpected(_In_ PCSTR pszTestData,
                                    _In_ WORD wAttrOriginal,
                                    _In_ WORD wAttrWritten,
                                    _In_ DbcsWriteMode const WriteMode,
                                    _In_ bool const fWriteWithUnicode,
                                    _In_ bool const fIsTrueTypeFont,
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
                    DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputA under Raster font, we will get the 
                    // double-byte sequences stomped on top of a Unicode filled CHAR_INFO structure that used -1 for trailing bytes.
                    DbcsWriteReadTestsPrepAStompsWNegativeOnePatternTruncateSpacePadded(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
            else
            {
                // When written with WriteConsoleOutputA and read back with ReadConsoleOutputA,
                // we will get back the double-byte sequences appropriately labeled with leading/trailing bytes.
                //DbcsWriteReadTestsPrepAPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                DbcsWriteReadTestsPrepAOnDoubledWNegativeOneTrailingPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
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
                    DbcsWriteReadTestsPrepSpacePaddedDedupeWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputA when the font is TrueType,
                    // we will get back Unicode characters doubled up and marked with leading and trailing bytes...
                    // ... except all the trailing bytes character values will be -1.
                    DbcsWriteReadTestsPrepDoubledWNegativeOneTrailingPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
            else
            {
                if (fWriteWithUnicode)
                {
                    // When written with WriteConsoleOutputW and read back with ReadConsoleOutputW when the font is Raster,
                    // we will get a deduplicated set of Unicode characters with no lead/trailing markings and space padded at the end...
                    // ... except something weird happens with truncation (TODO figure out what)
                    DbcsWriteReadTestsPrepSpacePaddedDedupeTruncatedWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
                else
                {
                    // When written with WriteConsoleOutputA and read back with ReadConsoleOutputW when the font is Raster,
                    // we will get back de-duplicated Unicode characters with no lead / trail markings.The extra array space will remain null.
                    DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
                }
            }
        }
        break;
    }
    case DbcsWriteMode::CrtWrite:
    {
        // Writing with the CRT down here.

        if (!fReadWithUnicode)
        {
            // If we wrote with the CRT and are reading with A functions, the font doesn't matter.
            // We will always get back the double-byte sequences appropriately labeled with leading/trailing bytes.
            //DbcsWriteReadTestsPrepPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            DbcsWriteReadTestsPrepAOnDoubledWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
        }
        else
        {
            // If we wrote with the CRT and are reading back with the W functions, the font does matter.
            if (fIsTrueTypeFont)
            {
                // In a TrueType font, we will get back Unicode characters doubled up and marked with leading and trailing bytes.
                DbcsWriteReadTestsPrepDoubledWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
            else
            {
                // In a Raster font, we will get back de-duplicated Unicode characters with no lead/trail markings. The extra array space will remain null.
                DbcsWriteReadTestsPrepNullPaddedDedupeWPattern(pszTestData, wAttrOriginal, wAttrWritten, rgciExpected, cExpectedNeeded);
            }
        }
        break;
    }
    default:
        VERIFY_FAIL(L"Unsupported write mode");
    }

    // Return the expected array and the length that should be used for comparison at the end of the test.
    *ppciExpected = rgciExpected;
    *pcExpected = cExpectedNeeded;
}

void DbcsWriteReadTestsRetrieveOutput(_In_ HANDLE const hOut, _In_ bool const fReadUnicode, _Out_writes_(cChars) CHAR_INFO* const rgChars, _In_ SHORT const cChars)
{
    // Since we wrote (in SendOutput function) to the 0,0 line, we need to read back the same width from that line.
    COORD coordBufferSize = { 0 };
    coordBufferSize.Y = 1;
    coordBufferSize.X = cChars;

    COORD coordBufferTarget = { 0 };

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

void DbcsWriteReadTestRunner(_In_ PCSTR pszTestData,
                             _In_opt_ WORD* const pwAttrOverride,
                             _In_ bool const fUseTrueType,
                             _In_ DbcsWriteMode const WriteMode,
                             _In_ bool const fWriteInUnicode,
                             _In_ bool const fReadWithUnicode)
{
    // First we need to set up the tests by clearing out the first line of the buffer,
    // retrieving the appropriate output handle, and getting the colors (attributes)
    // used by default in the buffer (set during clearing as well).
    HANDLE hOut;
    WORD wAttributes;
    SetupDbcsWriteReadTests(fUseTrueType, &hOut, &wAttributes);
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
    DbcsWriteReadTestsSendOutput(hOut, WriteMode, fWriteInUnicode, pszTestData, wAttributes);

    // Prepare the array of CHAR_INFO structs that we expect to receive back when we will call read in a moment.
    // This can vary based on font, unicode/non-unicode (when reading AND writing), and codepage.
    CHAR_INFO* pciExpected;
    size_t cExpected;
    DbcsWriteReadTestsPrepExpected(pszTestData, wAttrOriginal, wAttributes, WriteMode, fWriteInUnicode, fUseTrueType, fReadWithUnicode, &pciExpected, &cExpected);

    // Now call the appropriate READ API for this test.
    CHAR_INFO* pciActual = new CHAR_INFO[cTestData];
    ZeroMemory(pciActual, sizeof(CHAR_INFO) * cTestData);
    DbcsWriteReadTestsRetrieveOutput(hOut, fReadWithUnicode, pciActual, (SHORT)cTestData);

    // Loop through and verify that our expected array matches what was actually returned by the given API.
    DbcsWriteReadTestsVerify(pciExpected, cExpected, pciActual);

    // Free allocated structures
    delete[] pciActual;
    delete[] pciExpected;
}

void DbcsTests::TestDbcsWriteRead()
{
    bool fUseTrueTypeFont;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fUseTrueTypeFont", fUseTrueTypeFont));

    /*bool fUseCrtWrite;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fUseCrtWrite", fUseCrtWrite));*/
    
    int iWriteMode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"WriteMode", iWriteMode));
    DbcsWriteMode WriteMode = (DbcsWriteMode)iWriteMode;

    bool fWriteInUnicode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fWriteInUnicode", fWriteInUnicode));

    bool fReadInUnicode;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fReadInUnicode", fReadInUnicode));

    auto testInfo = NoThrowString().Format(L"\r\n\r\n\r\nUse '%ls' font. Write with %ls '%ls'. Check Read with %ls '%ls' API.\r\n",
                                           fUseTrueTypeFont ? L"TrueType" : L"Raster",
                                           WriteMode==0 ? L"CRT" : L"WriteConsoleOutput",
                                           fWriteInUnicode ? L"W" : L"A",
                                           L"ReadConsoleOutput",
                                           fReadInUnicode ? L"W" : L"A");

    Log::Comment(testInfo);

    PCSTR pszTestData = "Q\x82\xA2\x82\xa9\x82\xc8ZYXWVUT\x82\xc9";

    WORD wAttributes = 0;

    if (WriteMode == 1)
    {
        Log::Comment(L"We will also try to change the color since WriteConsoleOutput supports it.");
        wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    }

    DbcsWriteReadTestRunner(pszTestData,
                            wAttributes != 0 ? &wAttributes : nullptr,
                            fUseTrueTypeFont,
                            WriteMode,
                            fWriteInUnicode,
                            fReadInUnicode);

    Log::Comment(testInfo);

}

// TODO: MSFT: 10187355-  ReadWrite2 needs to be moved completely into UIA tests so it can modify fonts and confirm behavior.
//// This is sample code for DbcsReadWriteInner2 that isn't executed but is helpful in seeing how this would actually work from a consumer.
//int DbcsReadWriteInner2Sample()
//{
//    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
//
//    CONSOLE_SCREEN_BUFFER_INFOEX csbiex = { 0 };
//    csbiex.cbSize = sizeof(csbiex);
//
//    GetConsoleScreenBufferInfoEx(hOut, &csbiex);
//
//    CHAR_INFO buffer[8];
//    COORD dwBufferSize = { 4, 1 };
//    COORD dwBufferCoord = { 0, 0 };
//    SMALL_RECT srWriteRegion = { 0 };
//    srWriteRegion.Left = 0;
//    srWriteRegion.Top = 0;
//    srWriteRegion.Right = 15;
//    srWriteRegion.Bottom = 1;
//
//    //‚©‚½‚©‚È
//    LPCWSTR pwszStr = L"‚©‚½‚©‚È";
//    LPCSTR pszStr = "‚©‚½‚©‚È";
//
//    buffer[0].Char.UnicodeChar = pwszStr[0];
//    buffer[0].Attributes = 7;
//    buffer[1].Char.UnicodeChar = pwszStr[1];
//    buffer[1].Attributes = 7;
//    buffer[2].Char.UnicodeChar = pwszStr[2];
//    buffer[2].Attributes = 7;
//    buffer[3].Char.UnicodeChar = pwszStr[3];
//    buffer[3].Attributes = 7;
//
//    WriteConsoleOutputW(hOut, buffer, dwBufferSize, dwBufferCoord, &srWriteRegion);
//
//    dwBufferSize.X = 8;
//    srWriteRegion.Top = 1;
//    srWriteRegion.Bottom = 2;
//    srWriteRegion.Right = 60;
//    buffer[0].Char.AsciiChar = pszStr[0];
//    buffer[0].Attributes = 7 | COMMON_LVB_LEADING_BYTE;
//    buffer[1].Char.AsciiChar = pszStr[1];
//    buffer[1].Attributes = 7 | COMMON_LVB_TRAILING_BYTE;
//    buffer[2].Char.AsciiChar = pszStr[2];
//    buffer[2].Attributes = 7 | COMMON_LVB_LEADING_BYTE;
//    buffer[3].Char.AsciiChar = pszStr[3];
//    buffer[3].Attributes = 7 | COMMON_LVB_TRAILING_BYTE;
//    buffer[4].Char.AsciiChar = pszStr[4];
//    buffer[4].Attributes = 7 | COMMON_LVB_LEADING_BYTE;
//    buffer[5].Char.AsciiChar = pszStr[5];
//    buffer[5].Attributes = 7 | COMMON_LVB_TRAILING_BYTE;
//    buffer[6].Char.AsciiChar = pszStr[6];
//    buffer[6].Attributes = 7 | COMMON_LVB_LEADING_BYTE;
//    buffer[7].Char.AsciiChar = pszStr[7];
//    buffer[7].Attributes = 7 | COMMON_LVB_TRAILING_BYTE;
//
//    WriteConsoleOutputA(hOut, buffer, dwBufferSize, dwBufferCoord, &srWriteRegion);
//
//    HKEY key;
//    RegOpenKeyExW(HKEY_CURRENT_USER, L"Console", 0, GENERIC_READ, &key);
//    DWORD dwData;
//    DWORD cbData = sizeof(dwData);
//    RegQueryValueExW(key, L"ForceV2", NULL, NULL, (LPBYTE)&dwData, &cbData);
//
//
//
//    DWORD dwLength = 4;
//    COORD dwWriteCoord = { 0, 2 };
//    DWORD dwWritten = 0;
//    WriteConsoleOutputCharacterW(hOut, pwszStr, dwLength, dwWriteCoord, &dwWritten);
//
//
//    dwLength = 8;
//    dwWriteCoord.Y = 3;
//    WriteConsoleOutputCharacterA(hOut, pszStr, dwLength, dwWriteCoord, &dwWritten);
//
//
//    CHAR_INFO readBuffer[8 * 4];
//    COORD dwReadBufferSize = { 8, 4 };
//    COORD dwReadBufferCoord = { 0, 0 };
//    SMALL_RECT srReadRegion = { 0 };
//    srReadRegion.Left = 0;
//    srReadRegion.Right = 8;
//    srReadRegion.Top = 0;
//    srReadRegion.Bottom = 4;
//
//    ReadConsoleOutputW(hOut, readBuffer, dwReadBufferSize, dwReadBufferCoord, &srReadRegion);
//
//    srReadRegion.Left = 0;
//    srReadRegion.Right = 8;
//    srReadRegion.Top = 0;
//    srReadRegion.Bottom = 4;
//
//    ReadConsoleOutputA(hOut, readBuffer, dwReadBufferSize, dwReadBufferCoord, &srReadRegion);
//
//    getchar();
//
//    return 0;
//}
//
//void DbcsReadWriteInner2ValidateRasterW(const CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex, CHAR_INFO* readBuffer)
//{
//    // Expected results are copied from a sample code program running against v1 console.
//    CHAR_INFO readBufferExpectedW[8 * 4];
//    size_t i = 0;
//
//    // Expected Results 
//    // CHAR | ATTRIBUTE
//    // ‚©   | 7
//    // ‚½   | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    // ‚©   | 7
//    // ‚½   | 7
//    // ‚©   | 7
//    // ‚È   | 7
//    // ‚©   | 7
//    // ‚½   | 7
//    // ‚©   | 7
//    // ‚È   | 7
//    // ‚©   | 7
//    // ‚½   | 7
//    // ‚©   | 7
//    // ‚È   | 7
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//    // \0   | 0
//
//    // -- ATTRIBUTES --
//    // The first 18 characters of the buffer will be filled with data and have the default attributes.
//    for (; i < 18; i++)
//    {
//        readBufferExpectedW[i].Attributes = psbiex->wAttributes;
//    }
//
//    // The remaining characters have null attributes (empty).
//    for (; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        readBufferExpectedW[i].Attributes = 0;
//    }
//
//    // -- CHARACTERS --
//    // first ensure buffer is null filled
//    for (i = 0; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        readBufferExpectedW[i].Char.UnicodeChar = L'\0';
//    }
//
//    // now insert the actual pattern we see from v1 console.
//    i = 0;
//
//    // The first line returns ka ta with four spaces. (6 total characters, oddly enough.)
//    PCWSTR pwszExpectedFirst = L"‚©‚½    "; // 4 spaces.
//
//    for (size_t j = 0; j < wcslen(pwszExpectedFirst); j++)
//    {
//        readBufferExpectedW[i++].Char.UnicodeChar = pwszExpectedFirst[j];
//    }
//
//
//    // Then we will see the entire string ka ta ka na three times for the other 3 insertions.
//    for (size_t k = 0; k < 3; k++)
//    {
//        PCWSTR pwszExpectedRest = L"‚©‚½‚©‚È";
//        for (size_t j = 0; j < wcslen(pwszExpectedRest); j++)
//        {
//            readBufferExpectedW[i++].Char.UnicodeChar = pwszExpectedRest[j];
//        }
//    }
//
//    // -- DO CHECK --
//    // Now compare.
//    for (size_t i = 0; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        VERIFY_ARE_EQUAL(readBufferExpectedW[i].Char.UnicodeChar, readBuffer[i].Char.UnicodeChar, L"Compare unicode chars");
//        VERIFY_ARE_EQUAL(readBufferExpectedW[i].Attributes, readBuffer[i].Attributes, L"Compare attributes");
//    }
//}
//
//void DbcsReadWriteInner2ValidateRasterA(const CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex, CHAR_INFO* readBuffer)
//{
//    CHAR_INFO readBufferExpectedA[8 * 4];
//    PCSTR pszExpected = "‚©‚½‚©‚È";
//    size_t i = 0;
//
//    // Expected Results 
//    // NOTE: each pair of two makes up the string above. You may need to run this test while the system non-Unicode codepage is Japanese (Japan) - CP932
//    // CHAR | ATTRIBUTE
//    // ‚©   | 7
//    // ‚½   | 7
//    // ‚©   | 7
//    // ‚È   | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//
//    // First fill the buffer with the pattern above 4 times.
//    for (size_t k = 0; k < 4; k++)
//    {
//        for (size_t j = 0; j < strlen(pszExpected); j++)
//        {
//            readBufferExpectedA[i].Char.AsciiChar = pszExpected[j];
//            readBufferExpectedA[i].Attributes = psbiex->wAttributes;
//
//            if (j % 2 == 0)
//            {
//                readBufferExpectedA[i].Attributes |= COMMON_LVB_LEADING_BYTE;
//            }
//            else
//            {
//                readBufferExpectedA[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
//            }
//
//            i++;
//        }
//    }
//
//    // Finally, fix up the first line. The first write will have filled characters 4-7 with spaces instead of the pattern.
//    // Everything else follows the pattern specifically.
//
//    for (i = 4; i < 8; i++)
//    {
//        readBufferExpectedA[i].Char.AsciiChar = '\x20'; // space
//        readBufferExpectedA[i].Attributes = psbiex->wAttributes; // standard attributes
//    }
//
//    // --  DO CHECK --
//    for (size_t i = 0; i < ARRAYSIZE(readBufferExpectedA); i++)
//    {
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Char.AsciiChar, readBuffer[i].Char.AsciiChar, L"Compare ASCII chars");
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Attributes, readBuffer[i].Attributes, L"Compare attributes");
//    }
//}
//
//void DbcsReadWriteInner2ValidateTruetypeW(const CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex, CHAR_INFO* readBuffer)
//{
//    // Expected results are copied from a sample code program running against v1 console.
//    CHAR_INFO readBufferExpectedW[8 * 4];
//    PCWSTR pwszExpected = L"‚©‚½‚©‚È";
//
//    // Expected Results 
//    // CHAR   | ATTRIBUTE
//    // ‚©     | 7
//    // ‚½     | 7
//    // ‚©     | 7
//    // ‚È     | 7
//    //        | 7
//    //        | 7
//    //        | 7
//    //        | 7
//    // ‚©     | 0x107
//    // 0xffff | 0x207
//    // ‚½     | 0x107
//    // 0xffff | 0x207
//    // ‚©     | 0x107
//    // 0xffff | 0x207
//    // ‚È     | 0x107
//    // 0xffff | 0x207
//    // ‚©     | 0x107
//    // ‚©     | 0x207
//    // ‚½     | 0x107
//    // ‚½     | 0x207
//    // ‚©     | 0x107
//    // ‚©     | 0x207
//    // ‚È     | 0x107
//    // ‚È     | 0x207
//    // ‚©     | 0x107
//    // ‚©     | 0x207
//    // ‚½     | 0x107
//    // ‚½     | 0x207
//    // ‚©     | 0x107
//    // ‚©     | 0x207
//    // ‚È     | 0x107
//    // ‚È     | 0x207
//
//    // -- ATTRIBUTES --
//    // All fields have a 7 (default attr) for color
//    for (size_t i = 0; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        readBufferExpectedW[i].Attributes = psbiex->wAttributes;
//    }
//
//    // All fields from 8 onward alternate leading and trailing byte
//    for (size_t i = 8; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        if (i % 2 == 0)
//        {
//            readBufferExpectedW[i].Attributes |= COMMON_LVB_LEADING_BYTE;
//        }
//        else
//        {
//            readBufferExpectedW[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
//        }
//    }
//
//    // -- CHARACTERS --
//    size_t i = 0;
//
//    // 1. the first 4 characters are the string
//    for (size_t j = 0; j < wcslen(pwszExpected); j++)
//    {
//        readBufferExpectedW[i++].Char.UnicodeChar = pwszExpected[j];
//    }
//
//    // 2. the next 4 characters are spaces
//    for (size_t j = 0; j < 4; j++)
//    {
//        readBufferExpectedW[i++].Char.UnicodeChar = 0x0020; // unicode space
//    }
//
//    // 3. the next 8 are the string alternating with -1 (0xffff)
//    for (size_t j = 0; j < wcslen(pwszExpected); j++)
//    {
//        readBufferExpectedW[i++].Char.UnicodeChar = pwszExpected[j];
//        readBufferExpectedW[i++].Char.UnicodeChar = 0xffff;
//    }
//
//    // 4a. The next 8 are the string with every character written twice
//    // 4b. The final 8 are the same as the previous step.
//    for (size_t k = 0; k < 2; k++)
//    {
//        for (size_t j = 0; j < wcslen(pwszExpected); j++)
//        {
//            readBufferExpectedW[i++].Char.UnicodeChar = pwszExpected[j];
//            readBufferExpectedW[i++].Char.UnicodeChar = pwszExpected[j];
//        }
//    }
//
//    // -- DO CHECK --
//    // Now compare.
//    for (size_t i = 0; i < ARRAYSIZE(readBufferExpectedW); i++)
//    {
//        VERIFY_ARE_EQUAL(readBufferExpectedW[i].Char.UnicodeChar, readBuffer[i].Char.UnicodeChar, L"Compare unicode chars");
//        VERIFY_ARE_EQUAL(readBufferExpectedW[i].Attributes, readBuffer[i].Attributes, L"Compare attributes");
//    }
//}
//
//void DbcsReadWriteInner2ValidateTruetypeA(const CONSOLE_SCREEN_BUFFER_INFOEX* const psbiex, CHAR_INFO* readBuffer)
//{
//    CHAR_INFO readBufferExpectedA[8 * 4];
//    PCSTR pszExpected = "‚©‚½‚©‚È";
//
//    // Expected Results 
//    // NOTE: each pair of two makes up the string above. You may need to run this test while the system non-Unicode codepage is Japanese (Japan) - CP932
//    // NOTE: YES THE RESPONSE IS MIXING UNICODE AND ASCII. This has always been broken this way and is a compat issue now.
//    // CHAR | ATTRIBUTE
//    // ‚©   | 7
//    // ‚½   | 7
//    // ‚©   | 7
//    // ‚È   | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    //      | 7
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xBD | 0x207
//    // 0x82 | 0x107
//    // 0xA9 | 0x207
//    // 0x82 | 0x107
//    // 0xC8 | 0x207
//
//    // -- ATTRIBUTES --
//    // All fields have a 7 (default attr) for color
//    for (size_t i = 0; i < ARRAYSIZE(readBufferExpectedA); i++)
//    {
//        readBufferExpectedA[i].Attributes = psbiex->wAttributes;
//    }
//
//    // All fields from 8 onward alternate leading and trailing byte
//    for (size_t i = 8; i < ARRAYSIZE(readBufferExpectedA); i++)
//    {
//        if (i % 2 == 0)
//        {
//            readBufferExpectedA[i].Attributes |= COMMON_LVB_LEADING_BYTE;
//        }
//        else
//        {
//            readBufferExpectedA[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
//        }
//    }
//
//    // -- CHARACTERS
//    // For perplexing reasons, the first eight characters are unicode
//    PCWSTR pwszExpectedOddUnicode = L"‚©‚½‚©‚È    ";
//    size_t i = 0;
//    for (; i < 8; i++)
//    {
//        readBufferExpectedA[i].Char.UnicodeChar = pwszExpectedOddUnicode[i];
//    }
//
//    // Then the pattern of the expected string is repeated 3 times to fill the buffer
//    for (size_t k = 0; k < 3; k++)
//    {
//        for (size_t j = 0; j < strlen(pszExpected); j++)
//        {
//            readBufferExpectedA[i++].Char.AsciiChar = pszExpected[j];
//        }
//    }
//
//    // --  DO CHECK --
//    for (size_t i = 0; i < 8; i++)
//    {
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Char.AsciiChar, readBuffer[i].Char.AsciiChar, L"Compare Unicode chars !!! this is notably wrong for compat");
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Attributes, readBuffer[i].Attributes, L"Compare attributes");
//    }
//
//    for (size_t i = 8; i < ARRAYSIZE(readBufferExpectedA); i++)
//    {
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Char.AsciiChar, readBuffer[i].Char.AsciiChar, L"Compare ASCII chars");
//        VERIFY_ARE_EQUAL(readBufferExpectedA[i].Attributes, readBuffer[i].Attributes, L"Compare attributes");
//    }
//}
//
//// This test covers an additional scenario related to read/write of double byte characters.
//// We're specifically looking for the presentation (A) or absence (W) of the lead/trailing byte flags.
//// It's also checking what happens when we use the W API to write full width characters into not enough space
//void DbcsTests::TestDbcsReadWrite2()
//{
//    HANDLE const hOut = GetStdOutputHandle();
//
//    UINT dwCP = GetConsoleCP();
//    VERIFY_ARE_EQUAL(dwCP, JAPANESE_CP);
//
//    UINT dwOutputCP = GetConsoleOutputCP();
//    VERIFY_ARE_EQUAL(dwOutputCP, JAPANESE_CP);
//
//    CONSOLE_SCREEN_BUFFER_INFOEX sbiex = { 0 };
//    sbiex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
//    BOOL fSuccess = GetConsoleScreenBufferInfoEx(hOut, &sbiex);
//
//    VERIFY_ARE_EQUAL(sbiex.dwCursorPosition.X, 4);
//    VERIFY_ARE_EQUAL(sbiex.dwCursorPosition.Y, 0);
//
//    {
//        Log::Comment(L"Attempt to write ‚©‚½‚©‚È into the buffer with various methods");
//
//        LPCWSTR pwszStr = L"‚©‚½‚©‚È";
//        DWORD cchStr = (DWORD)wcslen(pwszStr);
//
//        LPCSTR pszStr = "‚©‚½‚©‚È";
//        DWORD cbStr = (DWORD)strlen(pszStr);
//
//        // --- WriteConsoleOutput Unicode
//        CHAR_INFO writeBuffer[8 * 4];
//
//        COORD dwBufferSize = { (SHORT)cchStr, 1 }; // the buffer is 4 wide and 1 tall
//        COORD dwBufferCoord = { 0, 0 }; // the API should start reading at row 0 column 0 from the buffer we pass
//        SMALL_RECT srWriteRegion = { 0 };
//        srWriteRegion.Left = 0;
//        srWriteRegion.Right = 40; // purposefully too big. it shouldn't use all this space if it doesn't need it
//        srWriteRegion.Top = 1; // write to row 1 in the console's buffer (the first row is 0th row)
//        srWriteRegion.Bottom = srWriteRegion.Top + dwBufferSize.Y; // Bottom - Top = 1 so we only have one row to write
//
//        for (size_t i = 0; i < cchStr; i++)
//        {
//            writeBuffer[i].Char.UnicodeChar = pwszStr[i];
//            writeBuffer[i].Attributes = sbiex.wAttributes;
//        }
//
//        fSuccess = WriteConsoleOutputW(hOut, writeBuffer, dwBufferSize, dwBufferCoord, &srWriteRegion);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempted to write with WriteConsoleOutputW");
//
//        // --- WriteConsoleOutput ASCII
//
//        dwBufferSize.X = (SHORT)cbStr; // use the ascii string now for the buffer size
//
//        srWriteRegion.Right = 40; // purposefully big again, let API decide length
//        srWriteRegion.Top = 2; // write to row 2 (the third row) in the console's buffer 
//        srWriteRegion.Bottom = srWriteRegion.Top + dwBufferSize.Y;
//
//        for (size_t i = 0; i < cbStr; i++)
//        {
//            writeBuffer[i].Char.AsciiChar = pszStr[i];
//            writeBuffer[i].Attributes = sbiex.wAttributes;
//
//            // alternate leading and trailing bytes for the double byte. 0-1 is one DBCS pair. 2-3. Etc.
//            if (i % 2 == 0)
//            {
//                writeBuffer[i].Attributes |= COMMON_LVB_LEADING_BYTE;
//            }
//            else
//            {
//                writeBuffer[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
//            }
//        }
//
//        fSuccess = WriteConsoleOutputA(hOut, writeBuffer, dwBufferSize, dwBufferCoord, &srWriteRegion);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempted to write with WriteConsoleOutputA");
//
//        // --- WriteConsoleOutputCharacter Unicode
//        COORD dwWriteCoord = { 0, 3 }; // Write to fourth row (row 3), first column (col 0) in the screen buffer
//        DWORD dwWritten = 0;
//
//        fSuccess = WriteConsoleOutputCharacterW(hOut, pwszStr, cchStr, dwWriteCoord, &dwWritten);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempted to write with WriteConsoleOutputCharacterW");
//        VERIFY_ARE_EQUAL(cchStr, dwWritten, L"Verify all characters written successfully.");
//
//        // --- WriteConsoleOutputCharacter ASCII
//        dwWriteCoord.Y = 4; // move down to the fifth line
//        dwWritten = 0; // reset written count
//
//        fSuccess = WriteConsoleOutputCharacterA(hOut, pszStr, cbStr, dwWriteCoord, &dwWritten);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempted to write with WriteConsoleOutputCharacterA");
//        VERIFY_ARE_EQUAL(cbStr, dwWritten, L"Verify all character bytes written successfully.");
//
//
//        Log::Comment(L"Try to read back ‚©‚½‚©‚È from the buffer and confirm formatting.");
//
//        COORD dwReadBufferSize = { (SHORT)cbStr, 4 }; // 2 rows with 8 columns each. the string length was 4 so leave double space in case the characters are doubled on read.
//        CHAR_INFO readBuffer[8 * 4];
//
//        COORD dwReadBufferCoord = { 0, 0 };
//        SMALL_RECT srReadRegionExpected = { 0 }; // read cols 0-7 in rows 1-2 (first 8 characters of 2 rows we just wrote above)
//        srReadRegionExpected.Left = 0;
//        srReadRegionExpected.Top = 1;
//        srReadRegionExpected.Right = srReadRegionExpected.Left + dwReadBufferSize.X;
//        srReadRegionExpected.Bottom = srReadRegionExpected.Top + dwReadBufferSize.Y;
//
//        SMALL_RECT srReadRegion = srReadRegionExpected;
//
//        fSuccess = ReadConsoleOutputW(hOut, readBuffer, dwReadBufferSize, dwReadBufferCoord, &srReadRegion);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempt to read back with ReadConsoleOutputW");
//
//        Log::Comment(L"Verify results from W read...");
//        {
//            if (IsTrueTypeFont(hOut))
//            {
//                DbcsReadWriteInner2ValidateTruetypeW(&sbiex, readBuffer);
//            }
//            else
//            {
//                DbcsReadWriteInner2ValidateRasterW(&sbiex, readBuffer);
//            }
//        }
//
//        // set read region back in case it changed
//        srReadRegion = srReadRegionExpected;
//
//        fSuccess = ReadConsoleOutputA(hOut, readBuffer, dwReadBufferSize, dwReadBufferCoord, &srReadRegion);
//        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(fSuccess, L"Attempt to read back wtih ReadConsoleOutputA");
//
//        Log::Comment(L"Verify results from A read...");
//        {
//            if (IsTrueTypeFont(hOut))
//            {
//                DbcsReadWriteInner2ValidateTruetypeA(&sbiex, readBuffer);
//            }
//            else
//            {
//                DbcsReadWriteInner2ValidateRasterA(&sbiex, readBuffer);
//            }
//        }
//    }
//}

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
