/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include <thread>

class OutputTests
{
    TEST_CLASS(OutputTests);

    TEST_CLASS_SETUP(TestSetup);
    TEST_CLASS_CLEANUP(TestCleanup);

    TEST_METHOD(BasicWriteConsoleOutputWTest);
    TEST_METHOD(BasicWriteConsoleOutputATest);

    TEST_METHOD(WriteConsoleOutputCharacterWRunoff);

    TEST_METHOD(WriteConsoleOutputAttributeSimpleTest);
    TEST_METHOD(WriteConsoleOutputAttributeCheckerTest);

    TEST_METHOD(WriteBackspaceTest);
};

bool OutputTests::TestSetup()
{
    return Common::TestBufferSetup();
}

bool OutputTests::TestCleanup()
{
    return Common::TestBufferCleanup();
}

void OutputTests::BasicWriteConsoleOutputWTest()
{
    // Get output buffer information.
    const auto consoleOutputHandle = GetStdOutputHandle();
    SetConsoleActiveScreenBuffer(consoleOutputHandle);

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex{ 0 };
    sbiex.cbSize = sizeof(sbiex);

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(consoleOutputHandle, &sbiex));
    const auto bufferSize = sbiex.dwSize;

    // Establish a writing region that is the width of the buffer and half the height.
    const SMALL_RECT region{ 0, 0, bufferSize.X - 1, bufferSize.Y / 2 };
    const COORD regionDimensions{ region.Right - region.Left + 1, region.Bottom - region.Top + 1 };
    const auto regionSize = regionDimensions.X * regionDimensions.Y;
    const COORD regionOrigin{ 0, 0 };

    // Make a test value and fill an array (via a vector) full of it.
    CHAR_INFO testValue;
    testValue.Attributes = 0x3e;
    testValue.Char.UnicodeChar = L' ';

    std::vector<CHAR_INFO> buffer(regionSize, testValue);

    // Call the API and confirm results.
    SMALL_RECT affected = region;
    VERIFY_SUCCEEDED(WriteConsoleOutputW(consoleOutputHandle, buffer.data(), regionDimensions, regionOrigin, &affected));
    VERIFY_ARE_EQUAL(region, affected);
}

void OutputTests::BasicWriteConsoleOutputATest()
{
    // Get output buffer information.
    const auto consoleOutputHandle = GetStdOutputHandle();
    SetConsoleActiveScreenBuffer(consoleOutputHandle);

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex{ 0 };
    sbiex.cbSize = sizeof(sbiex);

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(consoleOutputHandle, &sbiex));
    const auto bufferSize = sbiex.dwSize;

    // Establish a writing region that is the width of the buffer and half the height.
    const SMALL_RECT region{ 0, 0, bufferSize.X - 1, bufferSize.Y / 2 };
    const COORD regionDimensions{ region.Right - region.Left + 1, region.Bottom - region.Top + 1 };
    const auto regionSize = regionDimensions.X * regionDimensions.Y;
    const COORD regionOrigin{ 0, 0 };

    // Make a test value and fill an array (via a vector) full of it.
    CHAR_INFO testValue;
    testValue.Attributes = 0x3e;
    testValue.Char.AsciiChar = ' ';

    std::vector<CHAR_INFO> buffer(regionSize, testValue);

    // Call the API and confirm results.
    SMALL_RECT affected = region;
    VERIFY_SUCCEEDED(WriteConsoleOutputA(consoleOutputHandle, buffer.data(), regionDimensions, regionOrigin, &affected));
    VERIFY_ARE_EQUAL(region, affected);
}

void OutputTests::WriteConsoleOutputCharacterWRunoff()
{
    // writes text that will not all fit on the screen to verify reported size is correct
    const auto consoleOutputHandle = GetStdOutputHandle();
    SetConsoleActiveScreenBuffer(consoleOutputHandle);

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex{ 0 };
    sbiex.cbSize = sizeof(sbiex);

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(consoleOutputHandle, &sbiex));
    const auto bufferSize = sbiex.dwSize;

    COORD target{ bufferSize.X - 1, bufferSize.Y - 1};

    const std::wstring text = L"hello";
    DWORD charsWritten = 0;
    VERIFY_SUCCEEDED(WriteConsoleOutputCharacterW(consoleOutputHandle,
                                                  text.c_str(),
                                                  gsl::narrow<DWORD>(text.size()),
                                                  target,
                                                  &charsWritten));
    VERIFY_ARE_EQUAL(charsWritten, 1u);


}

void OutputTests::WriteConsoleOutputAttributeSimpleTest()
{
    // Get output buffer information.
    const auto consoleOutputHandle = GetStdOutputHandle();
    SetConsoleActiveScreenBuffer(consoleOutputHandle);

    const DWORD size = 500;
    const WORD setAttr = FOREGROUND_BLUE | BACKGROUND_RED;
    const COORD coord{ 0, 0 };
    DWORD attrsWritten = 0;
    WORD attributes[size];
    std::fill_n(attributes, size, setAttr);

    // write some attribute changes
    VERIFY_SUCCEEDED(WriteConsoleOutputAttribute(consoleOutputHandle, attributes, size, coord, &attrsWritten));
    VERIFY_ARE_EQUAL(attrsWritten, size);

    // confirm change happened
    WORD resultAttrs[size];
    DWORD attrsRead = 0;
    VERIFY_SUCCEEDED(ReadConsoleOutputAttribute(consoleOutputHandle, resultAttrs, size, coord, &attrsRead));
    VERIFY_ARE_EQUAL(attrsRead, size);

    for (size_t i = 0; i < size; ++i)
    {
        VERIFY_ARE_EQUAL(attributes[i], resultAttrs[i]);
    }
}

void OutputTests::WriteConsoleOutputAttributeCheckerTest()
{
    // writes a red/green checkerboard pattern on top of some text and makes sure that the text and color attr
    // changes roundtrip properly through the API

    // Get output buffer information.
    const auto consoleOutputHandle = GetStdOutputHandle();
    SetConsoleActiveScreenBuffer(consoleOutputHandle);

    CONSOLE_SCREEN_BUFFER_INFOEX sbiex{ 0 };
    sbiex.cbSize = sizeof(sbiex);

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleScreenBufferInfoEx(consoleOutputHandle, &sbiex));
    const auto bufferSize = sbiex.dwSize;

    const WORD red = BACKGROUND_RED;
    const WORD green = BACKGROUND_GREEN;

    const DWORD height = 8;
    const DWORD width = bufferSize.X;
    // todo verify less than or equal to buffer size ^^^
    const DWORD size = width * height;
    std::unique_ptr<WORD[]> attrs = std::make_unique<WORD[]>(size);

    std::generate(attrs.get(), attrs.get() + size, [=]()
                  {
                      static int i = 0;
                      return i++ % 2 == 0 ? red : green;
                  });

    // write text
    const COORD coord{ 0, 0 };
    DWORD charsWritten = 0;
    std::unique_ptr<wchar_t[]> wchs = std::make_unique<wchar_t[]>(size);
    std::fill_n(wchs.get(), size, L'*');
    VERIFY_SUCCEEDED(WriteConsoleOutputCharacterW(consoleOutputHandle, wchs.get(), size, coord, &charsWritten));
    VERIFY_ARE_EQUAL(charsWritten, size);

    // write attribute changes
    DWORD attrsWritten = 0;
    VERIFY_SUCCEEDED(WriteConsoleOutputAttribute(consoleOutputHandle, attrs.get(), size, coord, &attrsWritten));
    VERIFY_ARE_EQUAL(attrsWritten, size);

    // get changed attributes
    std::unique_ptr<WORD[]> resultAttrs = std::make_unique<WORD[]>(size);
    DWORD attrsRead = 0;
    VERIFY_SUCCEEDED(ReadConsoleOutputAttribute(consoleOutputHandle, resultAttrs.get(), size, coord, &attrsRead));
    VERIFY_ARE_EQUAL(attrsRead, size);

    // get text
    std::unique_ptr<wchar_t[]> resultWchs = std::make_unique<wchar_t[]>(size);
    DWORD charsRead = 0;
    VERIFY_SUCCEEDED(ReadConsoleOutputCharacterW(consoleOutputHandle, resultWchs.get(), size, coord, &charsRead));
    VERIFY_ARE_EQUAL(charsRead, size);

    // confirm that attributes were set without affecting text
    for (size_t i = 0; i < size; ++i)
    {
        VERIFY_ARE_EQUAL(attrs[i], resultAttrs[i]);
        VERIFY_ARE_EQUAL(wchs[i], resultWchs[i]);
    }
}

void OutputTests::WriteBackspaceTest()
{
    // Get output buffer information.
    const auto hOut = GetStdOutputHandle();
    Log::Comment(NoThrowString().Format(
        L"Outputing \"\\b \\b\" should behave the same as \"\b\", \" \", \"\b\" in seperate WriteConsoleW calls."
    ));

    DWORD n = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbi = {0};
    COORD c = {0, 0};
    VERIFY_SUCCEEDED(SetConsoleCursorPosition(hOut, c));
    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"GoodX", 5, &n, nullptr));

    VERIFY_SUCCEEDED(GetConsoleScreenBufferInfo(hOut, &csbi));
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.X, 5);
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.Y, 0);

    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"\b", 1, &n, nullptr));
    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L" ", 1, &n, nullptr));
    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"\b", 1, &n, nullptr));

    VERIFY_SUCCEEDED(GetConsoleScreenBufferInfo(hOut, &csbi));
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.X, 4);
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.Y, 0);

    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"\n", 1, &n, nullptr));

    VERIFY_SUCCEEDED(GetConsoleScreenBufferInfo(hOut, &csbi));
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.X, 0);
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.Y, 1);

    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"badX", 4, &n, nullptr));

    VERIFY_SUCCEEDED(GetConsoleScreenBufferInfo(hOut, &csbi));
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.X, 4);
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.Y, 1);

    VERIFY_SUCCEEDED(WriteConsoleW(hOut, L"\b \b", 3, &n, nullptr));

    VERIFY_SUCCEEDED(GetConsoleScreenBufferInfo(hOut, &csbi));
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.X, 3);
    VERIFY_ARE_EQUAL(csbi.dwCursorPosition.Y, 1);

}
