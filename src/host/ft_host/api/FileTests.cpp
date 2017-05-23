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

    TEST_METHOD(TestUtf8WriteFileInvalid);

};

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

    str = "\x80\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);

    str = "\x80\x40\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);

    str = "\x80\x80\x80\x40";
    cbStr = (DWORD)strlen(str);
    dwWritten = 0;
    dwExpectedWritten = cbStr;

    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, str, cbStr, &dwWritten, nullptr));
    VERIFY_ARE_EQUAL(dwExpectedWritten, dwWritten);
}
