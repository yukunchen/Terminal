/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "Common.hpp"

// This class is intended to test:
// WriteFile

class FileTests
{
    TEST_CLASS(FileTests);

    TEST_METHOD_SETUP(FileTestSetup);
    TEST_METHOD_CLEANUP(FileTestCleanup);

    TEST_METHOD(TestUtf8WriteFileInvalid);

    TEST_METHOD(TestWriteFileRaw);

};

bool FileTests::FileTestSetup()
{
    return true;
}

bool FileTests::FileTestCleanup()
{
    return true;
}

void FileTests::TestUtf8WriteFileInvalid()
{
    Log::Comment(L"Backup original console codepage.");
    UINT const uiOriginalCP = GetConsoleOutputCP();
    auto restoreOriginalCP = wil::ScopeExit([&] {
        Log::Comment(L"Restore original console codepage.");
        SetConsoleOutputCP(uiOriginalCP);
    });

    HANDLE hOut = GetStdOutputHandle();
    VERIFY_IS_NOT_NULL(hOut, L"Verify we have the standard output handle.");

    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleOutputCP(CP_UTF8), L"Set output codepage to UTF8");

    DWORD dwWritten;
    DWORD dwExpectedWritten;
    char* str;
    DWORD cbStr;

    // \x80 is an invalid UTF-8 continuation
    // \x40 is the @ symbol which is valid.
    str = "\x80\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);

    // \x80 is an invalid UTF-8 continuation
    // \x40 is the @ symbol which is valid.
    str = "\x80\x40\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);

    // \x80 is an invalid UTF-8 continuation
    // \x40 is the @ symbol which is valid.
    str = "\x80\x80\x80\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);
}

void FileTests::TestWriteFileRaw()
{
    PCSTR strTest = "a\x7s\x8d\x9f\xag\xdh";
    DWORD cchTest = (DWORD)strlen(strTest);
    String strReadBackExpected(strTest);

    HANDLE hOut = GetStdOutputHandle();
    VERIFY_IS_NOT_NULL(hOut, L"Verify we have the standard output handle.");

    CONSOLE_SCREEN_BUFFER_INFOEX csbiexBefore = { 0 };
    csbiexBefore.cbSize = sizeof(csbiexBefore);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexBefore), L"Retrieve screen buffer properties before writing.");

    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hOut, 0), L"Set raw write mode.");

    COORD const coordZero = { 0 };
    VERIFY_ARE_EQUAL(coordZero, csbiexBefore.dwCursorPosition, L"Cursor should be at 0,0 in fresh buffer.");

    DWORD dwWritten = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, strTest, cchTest, &dwWritten, nullptr), L"Write text into buffer using WriteFile");
    VERIFY_ARE_EQUAL(cchTest, dwWritten);

    CONSOLE_SCREEN_BUFFER_INFOEX csbiexAfter = { 0 };
    csbiexAfter.cbSize = sizeof(csbiexAfter);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexAfter), L"Retrieve screen buffer properties after writing.");

    csbiexBefore.dwCursorPosition.X += (SHORT)cchTest;
    VERIFY_ARE_EQUAL(csbiexBefore.dwCursorPosition, csbiexAfter.dwCursorPosition, L"Verify cursor moved expected number of squares for the write length.");

    DWORD cbReadBackBuffer = cchTest + 2; // +1 so we can read back a "space" that should be after what we wrote. +1 more so this can be null terminated for String class comparison.
    wistd::unique_ptr<char[]> strReadBack = wil::make_unique_nothrow<char[]>(cbReadBackBuffer);
    VERIFY_IS_NOT_NULL(strReadBack, L"Verify allocated buffer for string readback isn't null.");
    ZeroMemory(strReadBack.get(), cbReadBackBuffer * sizeof(char));

    DWORD dwRead = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterA(hOut, strReadBack.get(), cchTest + 1, coordZero, &dwRead), L"Read back the data in the buffer.");
    // +1 to read back the space that should be after the text we wrote

    strReadBackExpected += " "; // add in the space that should appear after the written text (buffer should be space filled when empty)

    VERIFY_ARE_EQUAL(strReadBackExpected, String(strReadBack.get()), L"Ensure that the buffer contents match what we expected based on what we wrote.");

    Sleep(2000);
}
