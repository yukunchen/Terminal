/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include <io.h>
#include <fcntl.h>

#define JAPANESE_CP 932u

class DbcsTests
{
    BEGIN_TEST_CLASS(DbcsTests)
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
        END_TEST_CLASS();

    TEST_METHOD_SETUP(DbcsTestSetup);

    // This test must come before ones that launch another process as launching another process can tamper with the codepage
    // in ways that this test is not expecting.
    TEST_METHOD(TestMultibyteInputRetrieval);

    /*TEST_METHOD(TestDbcsReadWriteTrueType);*/
    TEST_METHOD(TestDbcsRasterWriteCrtAReadA);
    TEST_METHOD(TestDbcsRasterWriteCrtAReadW);
    TEST_METHOD(TestDbcsRasterWriteCrtWReadA);
    TEST_METHOD(TestDbcsRasterWriteCrtWReadW);
    TEST_METHOD(TestDbcsRasterWriteAReadA);
    TEST_METHOD(TestDbcsRasterWriteAReadW);
    TEST_METHOD(TestDbcsRasterWriteWReadA);
    TEST_METHOD(TestDbcsRasterWriteWReadW);

    /*TEST_METHOD(TestDbcsReadWrite2);*/

    TEST_METHOD(TestDbcsBisect);
};

HANDLE hScreen = INVALID_HANDLE_VALUE;

bool DbcsTests::DbcsTestSetup()
{
    return true;
}

// TODO: MSFT 10187355 - This test needs to move into UIA as it is dependent on the font.
// This test covers read/write of double byte characters including verification of correct attribute handling across 
// the two-wide double byte characters.

void SetupDbcsWriteReadTests(_In_ bool fIsTrueType,
                             _Out_ HANDLE* const phOut,
                             _Out_ WORD* const pwAttributes)
{
    HANDLE const hOut = GetStdOutputHandle();

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleCP(JAPANESE_CP));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleOutputCP(JAPANESE_CP));

    CONSOLE_FONT_INFOEX cfiex = { 0 };
    cfiex.cbSize = sizeof(cfiex);
    if (!fIsTrueType)
    {
        wcscpy_s(cfiex.FaceName, L"Terminal");
        cfiex.dwFontSize.X = 8;
        cfiex.dwFontSize.Y = 18;
    }
    else
    {
        wcscpy_s(cfiex.FaceName, L"MS Gothic");
        cfiex.dwFontSize.Y = 16;
    }

    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetCurrentConsoleFontEx(hOut, FALSE, &cfiex));

    COORD coordZero = { 0 };

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex = { 0 };
    sbiex.cbSize = sizeof(sbiex);
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(GetConsoleScreenBufferInfoEx(hOut, &sbiex));

    *phOut = hOut;
    *pwAttributes = sbiex.wAttributes;

    if (!SetConsoleCursorPosition(GetStdOutputHandle(), coordZero))
    {
        VERIFY_FAIL(L"Failed to set cursor position");
    }
}

void DbcsWriteReadTestsSendOutput(_In_ HANDLE const hOut,
                                  _In_ bool const fUseCrt, _In_ bool const fIsUnicode,
                                  _In_ PCSTR pszTestString, _In_ WORD const wAttr)
{
    PWSTR pwszTestString = nullptr;
    if (fIsUnicode)
    {
        int const icchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestString, -1, nullptr, 0);

        pwszTestString = new WCHAR[icchNeeded];

        int const iRes = MultiByteToWideChar(JAPANESE_CP, 0, pszTestString, -1, pwszTestString, icchNeeded);
    }

    SHORT cChars = 0;
    if (fIsUnicode)
    {
        cChars = (SHORT)wcslen(pwszTestString);
    }
    else
    {
        cChars = (SHORT)strlen(pszTestString);
    }

    if (fUseCrt)
    {
        if (fIsUnicode)
        {
            _setmode(_fileno(stdout), _O_WTEXT);
        }
        else
        {
            _setmode(_fileno(stdout), _O_TEXT);
        }

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
    }
    else
    {
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
                rgChars[i].Char.UnicodeChar = 0;
                rgChars[i].Char.AsciiChar = pszTestString[i];
            }
        }

        COORD coordBufferSize = { 0 };
        coordBufferSize.Y = 1;
        coordBufferSize.X = cChars;

        COORD coordBufferTarget = { 0 };

        SMALL_RECT srReadRegion = { 0 }; // inclusive rectangle (bottom and right are INSIDE the read area. usually are exclusive.)
        srReadRegion.Right = cChars - 1;

        // NOTE: Don't VERIFY these calls or we will overwrite the text in the buffer with the log message.
        if (fIsUnicode)
        {
            WriteConsoleOutputW(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion);
        }
        else
        {
            WriteConsoleOutputA(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion);
        }

        delete[] rgChars;

        // Move the cursor down a line in case log info prints out.
        coordBufferTarget.Y++;
        SetConsoleCursorPosition(hOut, coordBufferTarget);
    }

    if (nullptr != pwszTestString)
    {
        delete[] pwszTestString;
    }
}

void DbcsWriteReadTestsPrepExpected(_In_ PCSTR pszTestData,
                                    _In_ WORD wAttributes,
                                    _In_ bool const fIsUnicode,
                                    _Outptr_result_buffer_(*pcExpected) CHAR_INFO** const ppciExpected,
                                    _Out_ size_t* const pcExpected)
{
    // NOTE: DBCS assumes that multi-byte sequences for Japanese CP 932 are full-width (2 columns)
    // and that single-byte sequences are half-width (1 column)
    
    // We will expect to read back one CHAR_INFO for every A character we sent to the console using the assumption above.
    // We expect that reading W characters will always be smaller than that.
    size_t const cExpectedNeeded = strlen(pszTestData);

    CHAR_INFO* rgciExpected = new CHAR_INFO[cExpectedNeeded];
    ZeroMemory(rgciExpected, sizeof(CHAR_INFO) * cExpectedNeeded);

    if (!fIsUnicode)
    {
        bool fSetTrailingNext = false;
        for (size_t i = 0; i < cExpectedNeeded; i++)
        {
            rgciExpected[i].Attributes = wAttributes;

            char const ch = pszTestData[i];

            if (IsDBCSLeadByteEx(JAPANESE_CP, ch))
            {
                rgciExpected[i].Attributes |= COMMON_LVB_LEADING_BYTE;
                fSetTrailingNext = true;
            }
            else if (fSetTrailingNext)
            {
                rgciExpected[i].Attributes |= COMMON_LVB_TRAILING_BYTE;
                fSetTrailingNext = false;
            }

            rgciExpected[i].Char.AsciiChar = pszTestData[i];
        }
    }
    else
    {
        int const iwchNeeded = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, (int)cExpectedNeeded, nullptr, 0);
        PWSTR pwszTestData = new WCHAR[iwchNeeded];
        int const iRes = MultiByteToWideChar(JAPANESE_CP, 0, pszTestData, (int)cExpectedNeeded, pwszTestData, iwchNeeded);
        CheckLastErrorZeroFail(iRes, L"MultiByteToWideChar");

        for (size_t i = 0; i < iwchNeeded; i++)
        {
            rgciExpected[i].Attributes = wAttributes;
            rgciExpected[i].Char.UnicodeChar = pwszTestData[i];
        }

        delete[] pwszTestData;
    }

    *ppciExpected = rgciExpected;
    *pcExpected = cExpectedNeeded;
}

void DbcsWriteReadTestsRetrieveOutput(_In_ HANDLE const hOut, _In_ bool const fReadUnicode, _Out_writes_(cChars) CHAR_INFO* const rgChars, _In_ SHORT const cChars)
{
    COORD coordBufferSize = { 0 };
    coordBufferSize.Y = 1;
    coordBufferSize.X = cChars;

    COORD coordBufferTarget = { 0 };

    SMALL_RECT srReadRegion = { 0 }; // inclusive rectangle (bottom and right are INSIDE the read area. usually are exclusive.)
    srReadRegion.Right = cChars - 1;

    if (!fReadUnicode)
    {
        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputA(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion));
    }
    else
    {
        VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(ReadConsoleOutputW(hOut, rgChars, coordBufferSize, coordBufferTarget, &srReadRegion));
    }

    Log::Comment(NoThrowString().Format(L"ReadRegion T: %d L: %d B: %d R: %d", srReadRegion.Top, srReadRegion.Left, srReadRegion.Bottom, srReadRegion.Right));
}

void DbcsWriteReadTestsVerify(_In_ bool const fIsUnicode,
                              _In_reads_(cExpected) CHAR_INFO* const rgExpected, _In_ size_t const cExpected,
                              _In_reads_(cExpected) CHAR_INFO* const rgActual)
{
    for (size_t i = 0; i < cExpected; i++)
    {
        if (!fIsUnicode)
        {
            // Reads from the A version of the API may leave garbage in the top half (Unicode-only half) of the union.
            // Mask off so we discard potentially extraneous bits on comparison.
            rgExpected[i].Char.UnicodeChar &= 0xFF;
            rgActual[i].Char.UnicodeChar &= 0xFF;
        }

        VERIFY_ARE_EQUAL(rgExpected[i], rgActual[i]);
    }
}

void DbcsWriteReadTestRunner(_In_ PCSTR pszTestData,
                             _In_opt_ WORD* const pwAttrOverride,
                             _In_ bool const fWriteWithCrt,
                             _In_ bool const fWriteInUnicode,
                             _In_ bool const fReadWithUnicode)
{
    HANDLE hOut;
    WORD wAttributes;
    SetupDbcsWriteReadTests(false, &hOut, &wAttributes);

    if (nullptr != pwAttrOverride)
    {
        wAttributes = *pwAttrOverride;
    }

    size_t const cTestData = strlen(pszTestData);
    DbcsWriteReadTestsSendOutput(hOut, fWriteWithCrt, fWriteInUnicode, pszTestData, wAttributes);

    CHAR_INFO* pciExpected;
    size_t cExpected;
    DbcsWriteReadTestsPrepExpected(pszTestData, wAttributes, fReadWithUnicode, &pciExpected, &cExpected);

    CHAR_INFO* pciActual = new CHAR_INFO[cTestData];
    DbcsWriteReadTestsRetrieveOutput(hOut, fReadWithUnicode, pciActual, (SHORT)cTestData);

    DbcsWriteReadTestsVerify(fReadWithUnicode, pciExpected, cExpected, pciActual);

    delete[] pciActual;
    delete[] pciExpected;
}

void DbcsTests::TestDbcsRasterWriteCrtAReadA()
{
    Log::Comment(L"Write with CRT 'A'. Check Read with 'A' API.");

    PCSTR pszTestData = "A\x82\xa0Z";
    DbcsWriteReadTestRunner(pszTestData, nullptr, true, false, false);
}

void DbcsTests::TestDbcsRasterWriteCrtAReadW()
{
    Log::Comment(L"Write with CRT 'A'. Check Read with 'W' API.");

    PCSTR pszTestData = "A\x82\xA0Z";
    DbcsWriteReadTestRunner(pszTestData, nullptr, true, false, true);
}

void DbcsTests::TestDbcsRasterWriteCrtWReadA()
{
    Log::Comment(L"Write with CRT 'W'. Check Read with 'A' API.");

    PCSTR pszTestData = "A\x82\xa0Z";
    DbcsWriteReadTestRunner(pszTestData, nullptr, true, true, false);
}

void DbcsTests::TestDbcsRasterWriteCrtWReadW()
{
    Log::Comment(L"Write with CRT 'W'. Check Read with 'W' API.");

    PCSTR pszTestData = "A\x82\xA0Z";
    DbcsWriteReadTestRunner(pszTestData, nullptr, true, true, true);
}

void DbcsTests::TestDbcsRasterWriteAReadA()
{
    Log::Comment(L"Check Write with 'A' API and read with 'A' API.");

    // For this test, try changing attrs at the same time
    WORD wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    PCSTR pszTestData = "Q\x82\xA2Y";

    DbcsWriteReadTestRunner(pszTestData, &wAttributes, false, false, false);
}

void DbcsTests::TestDbcsRasterWriteAReadW()
{
    Log::Comment(L"Check Write with 'A' API and read with 'W' API.");

    // For this test, try changing attrs at the same time
    WORD wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    PCSTR pszTestData = "Q\x82\xA2Y";

    DbcsWriteReadTestRunner(pszTestData, &wAttributes, false, false, true);
}

void DbcsTests::TestDbcsRasterWriteWReadA()
{
    Log::Comment(L"Check Write with 'W' API and read with 'A' API.");

    // For this test, try changing attrs at the same time
    WORD wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    PCSTR pszTestData = "Q\x82\xA2Y";

    DbcsWriteReadTestRunner(pszTestData, &wAttributes, false, true, false);
}

void DbcsTests::TestDbcsRasterWriteWReadW()
{
    Log::Comment(L"Check Write with 'W' API and read with 'W' API.");

    // For this test, try changing attrs at the same time
    WORD wAttributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_GREEN;
    PCSTR pszTestData = "Q\x82\xA2Y";

    DbcsWriteReadTestRunner(pszTestData, &wAttributes, false, true, true);
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
