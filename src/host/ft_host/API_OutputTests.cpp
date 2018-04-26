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
