/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// This class is intended to test:
// GetConsoleMode
// SetConsoleMode
class ModeTests
{
    BEGIN_TEST_CLASS(ModeTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"conhost.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"wincon.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"winconp.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"conmsgl1.h")
    END_TEST_CLASS()

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    TEST_METHOD(TestGetConsoleModeInvalid);
    TEST_METHOD(TestSetConsoleModeInvalid);

    TEST_METHOD(TestConsoleModeInputScenario);
    TEST_METHOD(TestConsoleModeScreenBufferScenario);

    TEST_METHOD(TestGetConsoleDisplayMode);
};

bool ModeTests::TestSetup()
{
    return Common::TestBufferSetup();
}

bool ModeTests::TestCleanup()
{
    return Common::TestBufferCleanup();
}

void ModeTests::TestGetConsoleModeInvalid()
{
    DWORD dwConsoleMode = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(GetConsoleMode(INVALID_HANDLE_VALUE, &dwConsoleMode));
    VERIFY_ARE_EQUAL(dwConsoleMode, (DWORD)-1);

    dwConsoleMode = (DWORD)-1;
    VERIFY_WIN32_BOOL_FAILED(GetConsoleMode(nullptr, &dwConsoleMode));
    VERIFY_ARE_EQUAL(dwConsoleMode, (DWORD)-1);
}

void ModeTests::TestSetConsoleModeInvalid()
{
    VERIFY_WIN32_BOOL_FAILED(SetConsoleMode(INVALID_HANDLE_VALUE, 0));
    VERIFY_WIN32_BOOL_FAILED(SetConsoleMode(nullptr, 0));

    HANDLE hConsoleInput = GetStdInputHandle();
    VERIFY_WIN32_BOOL_FAILED(SetConsoleMode(hConsoleInput, 0xFFFFFFFF), L"Can't set invalid input flags");
    VERIFY_WIN32_BOOL_FAILED(SetConsoleMode(hConsoleInput, ENABLE_ECHO_INPUT),
                             L"Can't set ENABLE_ECHO_INPUT without ENABLE_LINE_INPUT on input handle");

    VERIFY_WIN32_BOOL_FAILED(SetConsoleMode(Common::_hConsole, 0xFFFFFFFF), L"Can't set invalid output flags");
}

void ModeTests::TestConsoleModeInputScenario()
{
    HANDLE hConsoleInput = GetStdInputHandle();

    const DWORD dwInputModeToSet = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_WINDOW_INPUT;
    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(hConsoleInput, dwInputModeToSet), L"Set valid flags for input");

    DWORD dwInputMode = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleMode(hConsoleInput, &dwInputMode), L"Get recently set flags for input");
    VERIFY_ARE_EQUAL(dwInputMode, dwInputModeToSet, L"Make sure SetConsoleMode worked for input");
}

void ModeTests::TestConsoleModeScreenBufferScenario()
{
    const DWORD dwOutputModeToSet = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(Common::_hConsole, dwOutputModeToSet),
                                L"Set initial output flags");

    DWORD dwOutputMode = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleMode(Common::_hConsole, &dwOutputMode), L"Get new output flags");
    VERIFY_ARE_EQUAL(dwOutputMode, dwOutputModeToSet, L"Make sure output flags applied appropriately");

    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(Common::_hConsole, 0), L"Set zero output flags");

    dwOutputMode = (DWORD)-1;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleMode(Common::_hConsole, &dwOutputMode), L"Get zero output flags");
    VERIFY_ARE_EQUAL(dwOutputMode, (DWORD)0, L"Verify able to set zero output flags");
}

void ModeTests::TestGetConsoleDisplayMode()
{
    DWORD dwMode = 0;
    SetLastError(0);

    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleDisplayMode(&dwMode));
    VERIFY_ARE_EQUAL(0u, GetLastError());
}
