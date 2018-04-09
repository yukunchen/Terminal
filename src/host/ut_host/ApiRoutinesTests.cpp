/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "ApiRoutines.h"
#include "getset.h"
#include "dbcs.h"
#include "misc.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

class ApiRoutinesTests
{
    TEST_CLASS(ApiRoutinesTests);

    std::unique_ptr<CommonState> m_state;

    ApiRoutines _Routines;
    IApiRoutines* _pApiRoutines = &_Routines;

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state = std::make_unique<CommonState>();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();

        m_state->PrepareGlobalInputBuffer();

        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        m_state->CleanupGlobalInputBuffer();

        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();

        m_state.reset(nullptr);

        return true;
    }

    BOOL _fPrevInsertMode;
    void PrepVerifySetConsoleInputModeImpl(const ULONG ulOriginalInputMode)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.Flags = 0;
        gci.pInputBuffer->InputMode = ulOriginalInputMode & ~(ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS);
        gci.SetInsertMode(IsFlagSet(ulOriginalInputMode, ENABLE_INSERT_MODE));
        UpdateFlag(gci.Flags, CONSOLE_QUICK_EDIT_MODE, IsFlagSet(ulOriginalInputMode, ENABLE_QUICK_EDIT_MODE));
        UpdateFlag(gci.Flags, CONSOLE_AUTO_POSITION, IsFlagSet(ulOriginalInputMode, ENABLE_AUTO_POSITION));

        // Set cursor DB to on so we can verify that it turned off when the Insert Mode changes.
        gci.CurrentScreenBuffer->SetCursorDBMode(true);

        // Record the insert mode at this time to see if it changed.
        _fPrevInsertMode = gci.GetInsertMode();
    }

    void VerifySetConsoleInputModeImpl(const HRESULT hrExpected,
                                       const ULONG ulNewMode)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        InputBuffer* const pii = gci.pInputBuffer;

        // The expected mode set in the buffer is the mode given minus the flags that are stored in different fields.
        ULONG ulModeExpected = ulNewMode;
        ClearAllFlags(ulModeExpected, (ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS));
        bool const fQuickEditExpected = IsFlagSet(ulNewMode, ENABLE_QUICK_EDIT_MODE);
        bool const fAutoPositionExpected = IsFlagSet(ulNewMode, ENABLE_AUTO_POSITION);
        bool const fInsertModeExpected = IsFlagSet(ulNewMode, ENABLE_INSERT_MODE);

        // If the insert mode changed, we expect the cursor to have turned off.
        bool const fCursorDBModeExpected = ((!!_fPrevInsertMode) == fInsertModeExpected);

        // Call the API
        HRESULT const hrActual = _pApiRoutines->SetConsoleInputModeImpl(pii, ulNewMode);

        // Now do verifications of final state.
        VERIFY_ARE_EQUAL(hrExpected, hrActual);
        VERIFY_ARE_EQUAL(ulModeExpected, pii->InputMode);
        VERIFY_ARE_EQUAL(fQuickEditExpected, IsFlagSet(gci.Flags, CONSOLE_QUICK_EDIT_MODE));
        VERIFY_ARE_EQUAL(fAutoPositionExpected, IsFlagSet(gci.Flags, CONSOLE_AUTO_POSITION));
        VERIFY_ARE_EQUAL(!!fInsertModeExpected, !!gci.GetInsertMode());
        VERIFY_ARE_EQUAL(fCursorDBModeExpected, gci.CurrentScreenBuffer->GetTextBuffer().GetCursor()->IsDouble());
    }

    TEST_METHOD(ApiSetConsoleInputModeImplValidNonExtended)
    {
        Log::Comment(L"Set some perfectly valid, non-extended flags.");
        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    }

    TEST_METHOD(ApiSetConsoleInputModeImplValidExtended)
    {
        Log::Comment(L"Set some perfectly valid, extended flags.");
        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION);
    }

    TEST_METHOD(ApiSetConsoleInputModeImplExtendedTurnOff)
    {
        Log::Comment(L"Try to turn off extended flags.");
        PrepVerifySetConsoleInputModeImpl(ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS);
    }

    TEST_METHOD(ApiSetConsoleInputModeImplInvalid)
    {
        Log::Comment(L"Set some invalid flags.");
        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Should get invalid argument code because we set invalid flags.");
        Log::Comment(L"Flags should be set anyway despite invalid code.");
        VerifySetConsoleInputModeImpl(E_INVALIDARG, 0x8000000);
    }

    TEST_METHOD(ApiSetConsoleInputModeImplInsertNoCookedRead)
    {
        Log::Comment(L"Turn on insert mode without cooked read data.");
        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE);
        Log::Comment(L"Turn back off and verify.");
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS);
    }

    TEST_METHOD(ApiSetConsoleInputModeImplInsertCookedRead)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        Log::Comment(L"Turn on insert mode with cooked read data.");
        gci.lpCookedReadData = new COOKED_READ_DATA();

        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE);
        Log::Comment(L"Turn back off and verify.");
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS);

        delete gci.lpCookedReadData;
        gci.lpCookedReadData = nullptr;
    }

    TEST_METHOD(ApiSetConsoleInputModeImplEchoOnLineOff)
    {
        Log::Comment(L"Set ECHO on with LINE off. It's invalid, but it should get set anyway and return an error code.");
        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Setting ECHO without LINE should return an invalid argument code.");
        Log::Comment(L"Input mode should be set anyway despite FAILED return code.");
        VerifySetConsoleInputModeImpl(E_INVALIDARG, ENABLE_ECHO_INPUT);
    }

    TEST_METHOD(ApiSetConsoleInputModeExtendedFlagBehaviors)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        Log::Comment(L"Verify that we can set various extended flags even without the ENABLE_EXTENDED_FLAGS flag.");
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_INSERT_MODE);
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_QUICK_EDIT_MODE);
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_AUTO_POSITION);

        Log::Comment(L"Verify that we cannot unset various extended flags without the ENABLE_EXTENDED_FLAGS flag.");
        PrepVerifySetConsoleInputModeImpl(ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION);
        InputBuffer* const pii = gci.pInputBuffer;
        HRESULT const hr = _pApiRoutines->SetConsoleInputModeImpl(pii, 0);

        VERIFY_ARE_EQUAL(S_OK, hr);
        VERIFY_ARE_EQUAL(true, !!gci.GetInsertMode());
        VERIFY_ARE_EQUAL(true, IsFlagSet(gci.Flags, CONSOLE_QUICK_EDIT_MODE));
        VERIFY_ARE_EQUAL(true, IsFlagSet(gci.Flags, CONSOLE_AUTO_POSITION));
    }

    TEST_METHOD(ApiSetConsoleInputModeImplPSReadlineScenario)
    {
        Log::Comment(L"Set Powershell PSReadline expected modes.");
        PrepVerifySetConsoleInputModeImpl(0x1F7);
        Log::Comment(L"Should return an invalid argument code because ECHO is set without LINE.");
        Log::Comment(L"Input mode should be set anyway despite FAILED return code.");
        VerifySetConsoleInputModeImpl(E_INVALIDARG, 0x1E4);
    }

    TEST_METHOD(ApiGetConsoleTitleA)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.Title = L"Test window title.";

        int const iBytesNeeded = WideCharToMultiByte(gci.OutputCP,
                                                     0,
                                                     gci.Title,
                                                     -1,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     nullptr);

        wistd::unique_ptr<char[]> pszExpected = wil::make_unique_nothrow<char[]>(iBytesNeeded);
        VERIFY_IS_NOT_NULL(pszExpected);

        VERIFY_WIN32_BOOL_SUCCEEDED(WideCharToMultiByte(gci.OutputCP,
                                                        0,
                                                        gci.Title,
                                                        -1,
                                                        pszExpected.get(),
                                                        iBytesNeeded,
                                                        nullptr,
                                                        nullptr));

        char pszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        VERIFY_SUCCEEDED(_pApiRoutines->GetConsoleTitleAImpl(pszTitle, ARRAYSIZE(pszTitle), &cchWritten, &cchNeeded));

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci.Title) + 1, cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci.Title), cchNeeded);
        VERIFY_IS_TRUE(0 == strcmp(pszExpected.get(), pszTitle));
    }

    TEST_METHOD(ApiGetConsoleTitleW)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.Title = L"Test window title.";

        wchar_t pwszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        _pApiRoutines->GetConsoleTitleWImpl(pwszTitle, ARRAYSIZE(pwszTitle), &cchWritten, &cchNeeded);

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci.Title), cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci.Title), cchNeeded);
        VERIFY_IS_TRUE(0 == wcscmp(gci.Title, pwszTitle));
    }

    TEST_METHOD(ApiGetConsoleOriginalTitleA)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.OriginalTitle = L"Test original window title.";

        int const iBytesNeeded = WideCharToMultiByte(gci.OutputCP,
                                                     0,
                                                     gci.OriginalTitle,
                                                     -1,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     nullptr);

        wistd::unique_ptr<char[]> pszExpected = wil::make_unique_nothrow<char[]>(iBytesNeeded);
        VERIFY_IS_NOT_NULL(pszExpected);

        VERIFY_WIN32_BOOL_SUCCEEDED(WideCharToMultiByte(gci.OutputCP,
                                                        0,
                                                        gci.OriginalTitle,
                                                        -1,
                                                        pszExpected.get(),
                                                        iBytesNeeded,
                                                        nullptr,
                                                        nullptr));

        char pszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        VERIFY_SUCCEEDED(_pApiRoutines->GetConsoleOriginalTitleAImpl(pszTitle, ARRAYSIZE(pszTitle), &cchWritten, &cchNeeded));

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci.OriginalTitle) + 1, cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci.OriginalTitle), cchNeeded);
        VERIFY_IS_TRUE(0 == strcmp(pszExpected.get(), pszTitle));
    }

    TEST_METHOD(ApiGetConsoleOriginalTitleW)
    {
        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        gci.OriginalTitle = L"Test original window title.";

        wchar_t pwszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        _pApiRoutines->GetConsoleOriginalTitleWImpl(pwszTitle, ARRAYSIZE(pwszTitle), &cchWritten, &cchNeeded);

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci.OriginalTitle), cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci.OriginalTitle), cchNeeded);
        VERIFY_IS_TRUE(0 == wcscmp(gci.OriginalTitle, pwszTitle));
    }

    static void s_AdjustOutputWait(const bool fShouldBlock)
    {
        SetFlagIf(ServiceLocator::LocateGlobals().getConsoleInformation().Flags, CONSOLE_SELECTING, fShouldBlock);
        ClearFlagIf(ServiceLocator::LocateGlobals().getConsoleInformation().Flags, CONSOLE_SELECTING, !fShouldBlock);
    }

    TEST_METHOD(ApiWriteConsoleA)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:fInduceWait", L"{false, true}")
            TEST_METHOD_PROPERTY(L"Data:dwCodePage", L"{437, 932, 65001}")
            TEST_METHOD_PROPERTY(L"Data:dwIncrement", L"{0, 1, 2}")
        END_TEST_METHOD_PROPERTIES();

        bool fInduceWait;
        VERIFY_SUCCEEDED(TestData::TryGetValue(L"fInduceWait", fInduceWait), L"Get whether or not we should exercise this function off a wait state.");

        DWORD dwCodePage;
        VERIFY_SUCCEEDED(TestData::TryGetValue(L"dwCodePage", dwCodePage), L"Get the codepage for the test. Check a single byte, a double byte, and UTF-8.");

        DWORD dwIncrement;
        VERIFY_SUCCEEDED(TestData::TryGetValue(L"dwIncrement", dwIncrement),
                         L"Get how many chars we should feed in at a time. This validates lead bytes and bytes held across calls.");

        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        SCREEN_INFORMATION* const si = gci.CurrentScreenBuffer;

        gci.LockConsole();
        auto Unlock = wil::ScopeExit([&] { gci.UnlockConsole(); });

        // Ensure global state is updated for our codepage.
        gci.OutputCP = dwCodePage;
        SetConsoleCPInfo(TRUE);

        PCSTR pszTestText;
        switch (dwCodePage)
        {
        case CP_USA: // US English ANSI
            pszTestText = "Test Text";
            break;
        case CP_JAPANESE: // Japanese Shift-JIS
            pszTestText = "J\x82\xa0\x82\xa2";
            break;
        case CP_UTF8:
            pszTestText = "Test \xe3\x82\xab Text";
            break;
        default:
            VERIFY_FAIL(L"Test is not ready for this codepage.");
            return;
        }
        size_t cchTestText = strlen(pszTestText);

        // Set our increment value for the loop.
        // 0 represents the special case of feeding the whole string in at at time.
        // Otherwise, we try different segment sizes to ensure preservation across calls
        // for appropriate handling of DBCS and UTF-8 sequences.
        const size_t cchIncrement = dwIncrement == 0 ? cchTestText : dwIncrement;

        for (size_t i = 0; i < cchTestText; i += cchIncrement)
        {
            Log::Comment(WEX::Common::String().Format(L"Iteration %d of loop with increment %d", i, cchIncrement));
            if (fInduceWait)
            {
                Log::Comment(L"Blocking global output state to induce waits.");
                s_AdjustOutputWait(true);
            }

            size_t cchRead = 0;
            IWaitRoutine* pWaiter = nullptr;

            // The increment is either the specified length or the remaining text in the string (if that is smaller).
            const size_t cchWriteLength = std::min(cchIncrement, cchTestText - i);

            // Run the test method
            const HRESULT hr = _pApiRoutines->WriteConsoleAImpl(si, pszTestText + i, cchWriteLength, &cchRead, &pWaiter);

            VERIFY_ARE_EQUAL(S_OK, hr, L"Successful result code from writing.");
            if (!fInduceWait)
            {
                VERIFY_IS_NULL(pWaiter, L"We should have no waiter for this case.");
                VERIFY_ARE_EQUAL(cchWriteLength, cchRead, L"We should have the same character count back as 'written' that we gave in.");
            }
            else
            {
                VERIFY_IS_NOT_NULL(pWaiter, L"We should have a waiter for this case.");
                // The cchRead is irrelevant at this point as it's not going to be returned until we're off the wait.

                Log::Comment(L"Unblocking global output state so the wait can be serviced.");
                s_AdjustOutputWait(false);
                Log::Comment(L"Dispatching the wait.");
                NTSTATUS Status = STATUS_SUCCESS;
                DWORD dwNumBytes = 0;
                DWORD dwControlKeyState = 0; // unused but matches the pattern for read.
                void* pOutputData = nullptr; // unused for writes but used for read.
                const BOOL bNotifyResult = pWaiter->Notify(WaitTerminationReason::NoReason, FALSE, &Status, &dwNumBytes, &dwControlKeyState, &pOutputData);

                VERIFY_IS_TRUE(!!bNotifyResult, L"Wait completion on notify should be successful.");
                VERIFY_ARE_EQUAL(STATUS_SUCCESS, Status, L"We should have a successful return code to pass to the caller.");

                const DWORD dwBytesExpected = (DWORD)cchWriteLength;
                VERIFY_ARE_EQUAL(dwBytesExpected, dwNumBytes, L"We should have the byte length of the string we put in as the returned value.");
            }
        }
    }

    TEST_METHOD(ApiWriteConsoleW)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:fInduceWait", L"{false, true}")
        END_TEST_METHOD_PROPERTIES();

        bool fInduceWait;
        VERIFY_SUCCEEDED(TestData::TryGetValue(L"fInduceWait", fInduceWait), L"Get whether or not we should exercise this function off a wait state.");

        CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        SCREEN_INFORMATION* const si = gci.CurrentScreenBuffer;

        gci.LockConsole();
        auto Unlock = wil::ScopeExit([&] { gci.UnlockConsole(); });

        PCWSTR pwszTestText = L"Test Text";
        const size_t cchTestText = wcslen(pwszTestText);

        if (fInduceWait)
        {
            Log::Comment(L"Blocking global output state to induce waits.");
            s_AdjustOutputWait(true);
        }

        size_t cchRead = 0;
        IWaitRoutine* pWaiter = nullptr;
        const HRESULT hr = _pApiRoutines->WriteConsoleWImpl(si, pwszTestText, cchTestText, &cchRead, &pWaiter);

        VERIFY_ARE_EQUAL(S_OK, hr, L"Successful result code from writing.");
        if (!fInduceWait)
        {
            VERIFY_IS_NULL(pWaiter, L"We should have no waiter for this case.");
            VERIFY_ARE_EQUAL(cchTestText, cchRead, L"We should have the same character count back as 'written' that we gave in.");
        }
        else
        {
            VERIFY_IS_NOT_NULL(pWaiter, L"We should have a waiter for this case.");
            // The cchRead is irrelevant at this point as it's not going to be returned until we're off the wait.

            Log::Comment(L"Unblocking global output state so the wait can be serviced.");
            s_AdjustOutputWait(false);
            Log::Comment(L"Dispatching the wait.");
            NTSTATUS Status = STATUS_SUCCESS;
            DWORD dwNumBytes = 0;
            DWORD dwControlKeyState = 0; // unused but matches the pattern for read.
            void* pOutputData = nullptr; // unused for writes but used for read.
            const BOOL bNotifyResult = pWaiter->Notify(WaitTerminationReason::NoReason, TRUE, &Status, &dwNumBytes, &dwControlKeyState, &pOutputData);

            VERIFY_IS_TRUE(!!bNotifyResult, L"Wait completion on notify should be successful.");
            VERIFY_ARE_EQUAL(STATUS_SUCCESS, Status, L"We should have a successful return code to pass to the caller.");

            const DWORD dwBytesExpected = (DWORD)(cchTestText * sizeof(wchar_t));
            VERIFY_ARE_EQUAL(dwBytesExpected, dwNumBytes, L"We should have the byte length of the string we put in as the returned value.");
        }
    }
};
