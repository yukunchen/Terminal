/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

// This class is intended to test:
// GetConsoleTitle
class TitleTests
{
    TEST_CLASS(TitleTests);

    TEST_METHOD_SETUP(TestSetup);
    TEST_METHOD_CLEANUP(TestCleanup);

    TEST_METHOD(TestGetConsoleTitleA);
    TEST_METHOD(TestGetConsoleTitleW);
};

bool TitleTests::TestSetup()
{
    return Common::TestBufferSetup();
}

bool TitleTests::TestCleanup()
{
    return Common::TestBufferCleanup();
}

void TestGetConsoleTitleAFillHelper(_Out_writes_all_(cchBuffer) char* const chBuffer,
                                    _In_ size_t const cchBuffer,
                                    _In_ char const chFill)
{
    for (size_t i = 0; i < cchBuffer; i++)
    {
        chBuffer[i] = chFill;
    }
}

void TestGetConsoleTitleWFillHelper(_Out_writes_all_(cchBuffer) wchar_t* const wchBuffer,
                                    _In_ size_t const cchBuffer,
                                    _In_ wchar_t const wchFill)
{
    for (size_t i = 0; i < cchBuffer; i++)
    {
        wchBuffer[i] = wchFill;
    }
}

void TestGetConsoleTitleAPrepExpectedHelper(_In_reads_(cchTitle) const char* const chTitle,
                                            _In_ size_t const cchTitle,
                                            _Inout_updates_all_(cchReadBuffer) char* const chReadBuffer,
                                            _In_ size_t const cchReadBuffer,
                                            _Inout_updates_all_(cchReadExpected) char* const chReadExpected,
                                            _In_ size_t const cchReadExpected,
                                            _In_ size_t const cchTryToRead)
{
    // Fill our read buffer and expected with all Zs to start
    TestGetConsoleTitleAFillHelper(chReadBuffer, cchReadBuffer, 'Z');
    TestGetConsoleTitleAFillHelper(chReadExpected, cchReadExpected, 'Z');

    // Prep expected data
    if (cchTryToRead >= cchTitle)
    {
        strncpy_s(chReadExpected, cchReadExpected, chTitle, cchTitle);  // Copy as much room as we said we had leaving space for null terminator

        if (cchReadExpected > cchTitle)
        {
            chReadExpected[cchTitle] = L'\0'; // null terminate the end if the copy desired is less than the full title.
        }
    }
}

void TestGetConsoleTitleWPrepExpectedHelper(_In_reads_(cchTitle) const wchar_t* const wchTitle,
                                            _In_ size_t const cchTitle,
                                            _Inout_updates_all_(cchReadBuffer) wchar_t* const wchReadBuffer,
                                            _In_ size_t const cchReadBuffer,
                                            _Inout_updates_all_(cchReadExpected) wchar_t* const wchReadExpected,
                                            _In_ size_t const cchReadExpected,
                                            _In_ size_t const cchTryToRead)
{
    // Fill our read buffer and expected with all Zs to start
    TestGetConsoleTitleWFillHelper(wchReadBuffer, cchReadBuffer, L'Z');
    TestGetConsoleTitleWFillHelper(wchReadExpected, cchReadExpected, L'Z');

    // Prep expected data
    size_t const cchCopy = min(cchTitle, cchTryToRead);
    wcsncpy_s(wchReadExpected, cchReadBuffer, wchTitle, cchCopy); // Copy as much room as we said we had leaving space for null terminator
    wchReadExpected[cchCopy - 1] = L'\0'; // null terminate no matter what was copied
}

void TestGetConsoleTitleAVerifyHelper(_Inout_updates_(cchReadBuffer) char* const chReadBuffer,
                                      _In_ size_t const cchReadBuffer,
                                      _In_ size_t const cchTryToRead,
                                      _In_ DWORD const dwExpectedRetVal,
                                      _In_ DWORD const dwExpectedLastError,
                                      _In_reads_(cchExpected) const char* const chReadExpected,
                                      _In_ size_t const cchExpected)
{
    VERIFY_ARE_EQUAL(cchExpected, cchReadBuffer);

    SetLastError(0);
    DWORD const dwRetVal = GetConsoleTitleA(chReadBuffer, cchTryToRead);
    DWORD const dwLastError = GetLastError();

    VERIFY_ARE_EQUAL(dwExpectedRetVal, dwRetVal);
    VERIFY_ARE_EQUAL(dwExpectedLastError, dwLastError);

    if (chReadExpected != nullptr)
    {
        for (size_t i = 0; i < cchExpected; i++)
        {
            // We must verify every individual character, not as a string, because we might be expecting a null
            // in the middle and need to verify it then keep going and read what's past that.
            VERIFY_ARE_EQUAL(chReadExpected[i], chReadBuffer[i]);
        }
    }
    else
    {
        VERIFY_ARE_EQUAL(chReadExpected, chReadBuffer);
        VERIFY_ARE_EQUAL(0, cchTryToRead);
    }
}

void TestGetConsoleTitleWVerifyHelper(_Inout_updates_(cchReadBuffer) wchar_t* const wchReadBuffer,
                                      _In_ size_t const cchReadBuffer,
                                      _In_ size_t const cchTryToRead,
                                      _In_ DWORD const dwExpectedRetVal,
                                      _In_ DWORD const dwExpectedLastError,
                                      _In_reads_(cchExpected) const wchar_t* const  wchReadExpected,
                                      _In_ size_t const cchExpected)
{
    VERIFY_ARE_EQUAL(cchExpected, cchReadBuffer);

    SetLastError(0);
    DWORD const dwRetVal = GetConsoleTitleW(wchReadBuffer, cchTryToRead);
    DWORD const dwLastError = GetLastError();

    VERIFY_ARE_EQUAL(dwExpectedRetVal, dwRetVal);
    VERIFY_ARE_EQUAL(dwExpectedLastError, dwLastError);

    if (wchReadExpected != nullptr)
    {
        for (size_t i = 0; i < cchExpected; i++)
        {
            // We must verify every individual character, not as a string, because we might be expecting a null
            // in the middle and need to verify it then keep going and read what's past that.
            VERIFY_ARE_EQUAL(wchReadExpected[i], wchReadBuffer[i]);
        }
    }
    else
    {
        VERIFY_ARE_EQUAL(wchReadExpected, wchReadBuffer);
        VERIFY_ARE_EQUAL(0, cchTryToRead);
    }
}

void TitleTests::TestGetConsoleTitleA()
{
    const char* const szTestTitle = "TestTitle";
    size_t const cchTestTitle = strlen(szTestTitle);

    Log::Comment(NoThrowString().Format(L"Set up the initial console title to '%S'.", szTestTitle));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleTitleA(szTestTitle));

    size_t cchReadBuffer = cchTestTitle + 1 + 4; // string length + null terminator + 4 bonus spots to check overruns/extra length.
    wistd::unique_ptr<char[]> chReadBuffer = wil::make_unique_nothrow<char[]>(cchReadBuffer);
    VERIFY_IS_NOT_NULL(chReadBuffer.get());

    wistd::unique_ptr<char[]> chReadExpected = wil::make_unique_nothrow<char[]>(cchReadBuffer);
    VERIFY_IS_NOT_NULL(chReadExpected.get());

    size_t cchTryToRead = 0;

    Log::Comment(L"Test 1: Say we have half the buffer size necessary.");
    cchTryToRead = cchTestTitle / 2;

    // Prepare the buffers and expected data
    TestGetConsoleTitleAPrepExpectedHelper(szTestTitle,
                                           cchTestTitle + 1,
                                           chReadBuffer.get(),
                                           cchReadBuffer,
                                           chReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleAVerifyHelper(chReadBuffer.get(), cchReadBuffer, cchTryToRead, 0, S_OK, chReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 2: Say we have have exactly the string length with no null space.");
    cchTryToRead = cchTestTitle;

    // Prepare the buffers and expected data
    TestGetConsoleTitleAPrepExpectedHelper(szTestTitle,
                                           cchTestTitle + 1,
                                           chReadBuffer.get(),
                                           cchReadBuffer,
                                           chReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleAVerifyHelper(chReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, chReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 3: Say we have have the string length plus one null space.");
    cchTryToRead = cchTestTitle + 1;

    // Prepare the buffers and expected data
    TestGetConsoleTitleAPrepExpectedHelper(szTestTitle,
                                           cchTestTitle + 1,
                                           chReadBuffer.get(),
                                           cchReadBuffer,
                                           chReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleAVerifyHelper(chReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, chReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 4: Say we have the string length with a null space and an extra space.");
    cchTryToRead = cchTestTitle + 1 + 1;

    // Prepare the buffers and expected data
    TestGetConsoleTitleAPrepExpectedHelper(szTestTitle,
                                           cchTestTitle + 1,
                                           chReadBuffer.get(),
                                           cchReadBuffer,
                                           chReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleAVerifyHelper(chReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, chReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 5: Say we have no buffer.");
    cchTryToRead = cchTestTitle;

    // Run the call and test it out.
    TestGetConsoleTitleAVerifyHelper(nullptr, 0, 0, cchTestTitle, S_OK, nullptr, 0);
}


void TitleTests::TestGetConsoleTitleW()
{
    const wchar_t* const wszTestTitle = L"TestTitle";
    size_t const cchTestTitle = wcslen(wszTestTitle);

    Log::Comment(NoThrowString().Format(L"Set up the initial console title to '%s'.", wszTestTitle));
    VERIFY_WIN32_BOOL_SUCCEEDED_RETURN(SetConsoleTitleW(wszTestTitle));

    size_t cchReadBuffer = cchTestTitle + 1 + 4; // string length + null terminator + 4 bonus spots to check overruns/extra length.
    wistd::unique_ptr<wchar_t[]> wchReadBuffer = wil::make_unique_nothrow<wchar_t[]>(cchReadBuffer);
    VERIFY_IS_NOT_NULL(wchReadBuffer.get());

    wistd::unique_ptr<wchar_t[]> wchReadExpected = wil::make_unique_nothrow<wchar_t[]>(cchReadBuffer);
    VERIFY_IS_NOT_NULL(wchReadExpected.get());

    size_t cchTryToRead = 0;

    Log::Comment(L"Test 1: Say we have half the buffer size necessary.");
    cchTryToRead = cchTestTitle / 2;

    // Prepare the buffers and expected data
    TestGetConsoleTitleWPrepExpectedHelper(wszTestTitle,
                                           cchTestTitle + 1,
                                           wchReadBuffer.get(),
                                           cchReadBuffer,
                                           wchReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleWVerifyHelper(wchReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, wchReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 2: Say we have have exactly the string length with no null space.");
    cchTryToRead = cchTestTitle;

    // Prepare the buffers and expected data
    TestGetConsoleTitleWPrepExpectedHelper(wszTestTitle,
                                           cchTestTitle + 1,
                                           wchReadBuffer.get(),
                                           cchReadBuffer,
                                           wchReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleWVerifyHelper(wchReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, wchReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 3: Say we have have the string length plus one null space.");
    cchTryToRead = cchTestTitle + 1;

    // Prepare the buffers and expected data
    TestGetConsoleTitleWPrepExpectedHelper(wszTestTitle,
                                           cchTestTitle + 1,
                                           wchReadBuffer.get(),
                                           cchReadBuffer,
                                           wchReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleWVerifyHelper(wchReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, wchReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 4: Say we have the string length with a null space and an extra space.");
    cchTryToRead = cchTestTitle + 1 + 1;

    // Prepare the buffers and expected data
    TestGetConsoleTitleWPrepExpectedHelper(wszTestTitle,
                                           cchTestTitle + 1,
                                           wchReadBuffer.get(),
                                           cchReadBuffer,
                                           wchReadExpected.get(),
                                           cchReadBuffer,
                                           cchTryToRead);

    // Run the call and test it out.
    TestGetConsoleTitleWVerifyHelper(wchReadBuffer.get(), cchReadBuffer, cchTryToRead, cchTestTitle, S_OK, wchReadExpected.get(), cchReadBuffer);

    Log::Comment(L"Test 5: Say we have no buffer.");
    cchTryToRead = cchTestTitle;

    // Run the call and test it out.
    TestGetConsoleTitleWVerifyHelper(nullptr, 0, 0, cchTestTitle, S_OK, nullptr, 0);
}


