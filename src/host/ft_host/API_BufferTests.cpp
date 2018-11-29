/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// This class is intended to test boundary conditions for:
// SetConsoleActiveScreenBuffer
class BufferTests
{
    BEGIN_TEST_CLASS(BufferTests)
        TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"conhost.exe")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"wincon.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"winconp.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"conmsgl1.h")
        TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"conmsgl2.h")
    END_TEST_CLASS()

    TEST_METHOD(TestSetConsoleActiveScreenBufferInvalid);

    BEGIN_TEST_METHOD(TestWritingInactiveScreenBuffer)
        TEST_METHOD_PROPERTY(L"Data:UseVtOutput", L"{true, false}")
    END_TEST_METHOD()
};

void BufferTests::TestSetConsoleActiveScreenBufferInvalid()
{
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(INVALID_HANDLE_VALUE));
    VERIFY_WIN32_BOOL_FAILED(SetConsoleActiveScreenBuffer(nullptr));
}

void BufferTests::TestWritingInactiveScreenBuffer()
{
    bool useVtOutput;
    VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"UseVtOutput", useVtOutput), L"Get whether this test should check VT output mode.");

    const std::wstring primary(L"You should see me");
    const std::wstring alternative(L"You should NOT see me!");
    const std::wstring newline(L"\n");

    Log::Comment(L"Set up the output mode to either use VT processing or not (see test parameter)");
    const auto out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    VERIFY_WIN32_BOOL_SUCCEEDED(GetConsoleMode(out, &mode));
    WI_UpdateFlag(mode, ENABLE_VIRTUAL_TERMINAL_PROCESSING, useVtOutput);
    VERIFY_WIN32_BOOL_SUCCEEDED(SetConsoleMode(out, mode));

    Log::Comment(L"Write one line of text to the active/main output buffer.");
    DWORD written = 0;
    // Ok in legacy mode, ok in modern mode
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleW(out, primary.data(), gsl::narrow<DWORD>(primary.size()), &written, nullptr));
    VERIFY_ARE_EQUAL(primary.size(), written);

    Log::Comment(L"Write a newline character to move the cursor down to the left most cell on the next line down.");
    // write a newline too to move the cursor down
    written = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleW(out, newline.data(), gsl::narrow<DWORD>(newline.size()), &written, nullptr));
    VERIFY_ARE_EQUAL(newline.size(), written);

    Log::Comment(L"Create an alternative backing screen buffer that we will NOT be setting as active.");
    const auto handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
    VERIFY_IS_NOT_NULL(handle);

    // Ok in legacy mode, NOT ok in modern mode.
    Log::Comment(L"Try to write a second line of different text but to the alternative backing screen buffer.");
    written = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(WriteConsoleW(handle, alternative.data(), gsl::narrow<DWORD>(alternative.size()), &written, nullptr));
    VERIFY_ARE_EQUAL(alternative.size(), written);

    std::unique_ptr<wchar_t[]> primaryBuffer = std::make_unique<wchar_t[]>(primary.size());
    std::unique_ptr<wchar_t[]> alternativeBuffer = std::make_unique<wchar_t[]>(alternative.size());

    Log::Comment(L"Read the first line out of the main/visible screen buffer. It should contain the first thing we wrote.");
    DWORD read = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterW(out, primaryBuffer.get(), gsl::narrow<DWORD>(primary.size()), { 0, 0 }, &read));
    VERIFY_ARE_EQUAL(primary.size(), read);
    VERIFY_ARE_EQUAL(String(primary.data()), String(primaryBuffer.get(), gsl::narrow<int>(primary.size())));

    Log::Comment(L"Read the second line out of the main/visible screen buffer. It should be full of blanks. The second thing we wrote wasn't to this buffer so it shouldn't show.");
    const std::wstring alternativeExpected(alternative.size(), L'\x20');
    read = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterW(out, alternativeBuffer.get(), gsl::narrow<DWORD>(alternative.size()), { 0, 1 }, &read));
    VERIFY_ARE_EQUAL(alternative.size(), read);
    VERIFY_ARE_EQUAL(String(alternativeExpected.data()), String(alternativeBuffer.get(), gsl::narrow<int>(alternative.size())));

    Log::Comment(L"Now read the first line from the alternative/non-visible screen buffer. It should contain the second thing we wrote.");
    read = 0;
    VERIFY_WIN32_BOOL_SUCCEEDED(ReadConsoleOutputCharacterW(handle, alternativeBuffer.get(), gsl::narrow<DWORD>(alternative.size()), { 0, 0 }, &read));
    VERIFY_ARE_EQUAL(alternative.size(), read);
    VERIFY_ARE_EQUAL(String(alternative.data()), String(alternativeBuffer.get(), gsl::narrow<int>(alternative.size())));

}
