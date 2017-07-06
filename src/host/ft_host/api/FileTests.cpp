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
    BEGIN_TEST_CLASS(FileTests)
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
    END_TEST_CLASS();

    TEST_METHOD_SETUP(FileTestSetup);
    TEST_METHOD_CLEANUP(FileTestCleanup);

    TEST_METHOD(TestUtf8WriteFileInvalid);

    TEST_METHOD(TestWriteFileRaw);
    TEST_METHOD(TestWriteFileProcessed);

    BEGIN_TEST_METHOD(TestWriteFileWrapEOL)
        TEST_METHOD_PROPERTY(L"Data:fFlagOn", L"{true, false}")
    END_TEST_METHOD()

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
    // \x7 is bell
    // \x8 is backspace
    // \x9 is tab
    // \xa is linefeed
    // \xd is carriage return
    // All should be ignored/printed in raw mode.
    PCSTR strTest = "z\x7y\x8z\x9y\xaz\xdy";
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
}

void WriteFileHelper(HANDLE hOut,
                     CONSOLE_SCREEN_BUFFER_INFOEX& csbiexBefore,
                     CONSOLE_SCREEN_BUFFER_INFOEX& csbiexAfter,
                     PCSTR psTest,
                     DWORD cchTest)
{
    csbiexBefore.cbSize = sizeof(csbiexBefore);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexBefore), L"Retrieve screen buffer properties before writing.");

    DWORD dwWritten = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, psTest, cchTest, &dwWritten, nullptr), L"Write text into buffer using WriteFile");
    VERIFY_ARE_EQUAL(cchTest, dwWritten, L"Verify all characters were written.");
        
    csbiexAfter.cbSize = sizeof(csbiexAfter);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexAfter), L"Retrieve screen buffer properties after writing.");
}

void ReadBackHelper(HANDLE hOut,
                    COORD coordReadBackPos,
                    DWORD dwReadBackLength,
                    wistd::unique_ptr<char[]>& pszReadBack)
{
    // Add one so it can be zero terminated.
    DWORD cbBuffer = dwReadBackLength + 1;
    wistd::unique_ptr<char[]> pszRead = wil::make_unique_nothrow<char[]>(cbBuffer);
    VERIFY_IS_NOT_NULL(pszRead, L"Verify allocated buffer for readback is not null.");
    ZeroMemory(pszRead.get(), cbBuffer * sizeof(char));

    DWORD dwRead = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterA(hOut, pszRead.get(), dwReadBackLength, coordReadBackPos, &dwRead), L"Read back data in the buffer.");
    VERIFY_ARE_EQUAL(dwReadBackLength, dwRead, L"Verify API reports we read back the number of characters we asked for.");

    pszReadBack.swap(pszRead);
}

void FileTests::TestWriteFileProcessed()
{
    // \x7 is bell
    // \x8 is backspace
    // \x9 is tab
    // \xa is linefeed
    // \xd is carriage return
    // All should cause activity in processed mode.

    HANDLE hOut = GetStdOutputHandle();
    VERIFY_IS_NOT_NULL(hOut, L"Verify we have the standard output handle.");

    CONSOLE_SCREEN_BUFFER_INFOEX csbiexOriginal = { 0 };
    csbiexOriginal.cbSize = sizeof(csbiexOriginal);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexOriginal), L"Retrieve screen buffer properties at beginning of test.");

    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hOut, ENABLE_PROCESSED_OUTPUT), L"Set processed write mode.");

    COORD const coordZero = { 0 };
    VERIFY_ARE_EQUAL(coordZero, csbiexOriginal.dwCursorPosition, L"Cursor should be at 0,0 in fresh buffer.");

    // Declare variables needed for each character test.
    CONSOLE_SCREEN_BUFFER_INFOEX csbiexBefore = { 0 };
    CONSOLE_SCREEN_BUFFER_INFOEX csbiexAfter = { 0 };
    COORD coordExpected = { 0 };
    PCSTR pszTest;
    DWORD cchTest;
    PCSTR pszReadBackExpected;
    DWORD cchReadBack;
    wistd::unique_ptr<char[]> pszReadBack;

    // 1. Test bell (\x7)
    {
        pszTest = "z\x7";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "z ";
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write z and a bell. Cursor should move once as bell should have made audible noise (can't really test) and not moved or printed anything.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.X += 1;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved once for printable character and not for bell.");

        // Read back written data.
        ReadBackHelper(hOut, csbiexBefore.dwCursorPosition, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }


    // 2. Test backspace (\x8)
    {
        pszTest = "yx\x8";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "yx ";
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write two characters and a backspace. Cursor should move only one forward as the backspace should have moved the cursor back one after printing the second character.
        // The backspace character itself is typically non-destructive so it should only affect the cursor, not the buffer contents.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.X += 1;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved twice forward for printable characters and once backward for backspace.");

        // Read back written data.
        ReadBackHelper(hOut, csbiexBefore.dwCursorPosition, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }

    // 3. Test tab (\x9)
    {
        // The tab character will space pad out the buffer to the next multiple-of-8 boundary.
        // NOTE: This is dependent on the previous tests running first.
        pszTest = "\x9";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "     ";
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write tab character. Cursor should move out to the next multiple-of-8 and leave space characters in its wake.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.X = 8;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved forward to position 8 for tab.");
        
        // Read back written data.
        ReadBackHelper(hOut, csbiexBefore.dwCursorPosition, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }

    // 4. Test linefeed (\xa)
    {
        // The line feed character should move us down to the next line.
        pszTest = "\xaQ";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "Q ";
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write line feed character. Cursor should move down a line and then the Q from our string should be printed.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected.X = 1;
        coordExpected.Y = 1;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved down a line and then one character over for linefeed + Q.");

        // Read back written data from the 2nd line.
        COORD coordRead;
        coordRead.Y = 1;
        coordRead.X = 0;
        ReadBackHelper(hOut, coordRead, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }

    // 5. Test carriage return (\xd)
    {
        // The carriage return character should move us to the front of the line.
        pszTest = "J\xd";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "QJ "; // J written, then move to beginning of line.
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write text and carriage return character. Cursor should end up at the beginning of this line. The J should have been printed in the line before we moved.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.X = 0;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved to beginning of line for carriage return character.");

        // Read back text written from the 2nd line. 
        ReadBackHelper(hOut, csbiexAfter.dwCursorPosition, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }
    
    // 6. Print a character over the top of the existing
    {
        // After the carriage return, try typing on top of the Q with a K
        pszTest = "K";
        cchTest = (DWORD)strlen(pszTest);
        pszReadBackExpected = "KJ "; // NOTE: This is based on the previous test(s).
        cchReadBack = (DWORD)strlen(pszReadBackExpected);

        // Write text. Cursor should end up on top of the J.
        WriteFileHelper(hOut, csbiexBefore, csbiexAfter, pszTest, cchTest);
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.X += 1;
        VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved over one for printing character.");

        // Read back text written from the 2nd line.
        ReadBackHelper(hOut, csbiexBefore.dwCursorPosition, cchReadBack, pszReadBack);
        VERIFY_ARE_EQUAL(String(pszReadBackExpected), String(pszReadBack.get()), L"Verify text matches what we expected to be written into the buffer.");
    }
}

void FileTests::TestWriteFileWrapEOL()
{
    bool fFlagOn;
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"fFlagOn", fFlagOn));

    HANDLE hOut = GetStdOutputHandle();
    VERIFY_IS_NOT_NULL(hOut, L"Verify we have the standard output handle.");

    CONSOLE_SCREEN_BUFFER_INFOEX csbiexOriginal = { 0 };
    csbiexOriginal.cbSize = sizeof(csbiexOriginal);
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexOriginal), L"Retrieve screen buffer properties at beginning of test.");

    if (fFlagOn)
    {
        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hOut, ENABLE_WRAP_AT_EOL_OUTPUT), L"Set wrap at EOL.");
    }
    else
    {
        VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hOut, 0), L"Make sure wrap at EOL is off.");
    }

    COORD const coordZero = { 0 };
    VERIFY_ARE_EQUAL(coordZero, csbiexOriginal.dwCursorPosition, L"Cursor should be at 0,0 in fresh buffer.");

    // Fill first row of the buffer with Z characters until 1 away from the end.
    for (SHORT i = 0; i < csbiexOriginal.dwSize.X - 1; i++)
    {
        WriteFile(hOut, "Z", 1, nullptr, nullptr);
    }

    CONSOLE_SCREEN_BUFFER_INFOEX csbiexBefore = { 0 };
    csbiexBefore.cbSize = sizeof(csbiexBefore);
    CONSOLE_SCREEN_BUFFER_INFOEX csbiexAfter = { 0 };
    csbiexAfter.cbSize = sizeof(csbiexAfter);
    COORD coordExpected = { 0 };

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexBefore), L"Get cursor position information before attempting to wrap at end of line.");
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteFile(hOut, "Y", 1, nullptr, nullptr), L"Write of final character in line succeeded.");
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(hOut, &csbiexAfter), L"Get cursor position information after attempting to wrap at end of line.");

    if (fFlagOn)
    {
        Log::Comment(L"Cursor should go down a row if we tried to print at end of line.");
        coordExpected = csbiexBefore.dwCursorPosition;
        coordExpected.Y++;
        coordExpected.X = 0;
    }
    else
    {
        Log::Comment(L"Cursor shouldn't move when printing at end of line.");
        coordExpected = csbiexBefore.dwCursorPosition;
    }

    VERIFY_ARE_EQUAL(coordExpected, csbiexAfter.dwCursorPosition, L"Verify cursor moved as expected based on flag state.");
}
