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

#include "..\interactivity\inc\ServiceLocator.hpp"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

class ApiRoutinesTests
{
    TEST_CLASS(ApiRoutinesTests);

    CommonState* m_state;

    ApiRoutines _Routines;
    IApiRoutines* _pApiRoutines = &_Routines;

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state = new CommonState();

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

        return true;
    }

    BOOL _fPrevInsertMode;
    void PrepVerifySetConsoleInputModeImpl(_In_ ULONG const ulOriginalInputMode)
    {
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        gci->Flags = 0;
        gci->pInputBuffer->InputMode = ulOriginalInputMode & ~(ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS);
        gci->SetInsertMode(IsFlagSet(ulOriginalInputMode, ENABLE_INSERT_MODE));
        UpdateFlag(gci->Flags, CONSOLE_QUICK_EDIT_MODE, IsFlagSet(ulOriginalInputMode, ENABLE_QUICK_EDIT_MODE));
        UpdateFlag(gci->Flags, CONSOLE_AUTO_POSITION, IsFlagSet(ulOriginalInputMode, ENABLE_AUTO_POSITION));

        // Set cursor DB to on so we can verify that it turned off when the Insert Mode changes.
        gci->CurrentScreenBuffer->SetCursorDBMode(TRUE);

        // Record the insert mode at this time to see if it changed.
        _fPrevInsertMode = gci->GetInsertMode();
    }

    void VerifySetConsoleInputModeImpl(_In_ HRESULT const hrExpected,
                                       _In_ ULONG const ulNewMode)
    {
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        InputBuffer* const pii = gci->pInputBuffer;

        // The expected mode set in the buffer is the mode given minus the flags that are stored in different fields.
        ULONG ulModeExpected = ulNewMode;
        ClearAllFlags(ulModeExpected, (ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_INSERT_MODE | ENABLE_EXTENDED_FLAGS));
        bool const fQuickEditExpected = IsFlagSet(ulNewMode, ENABLE_QUICK_EDIT_MODE);
        bool const fAutoPositionExpected = IsFlagSet(ulNewMode, ENABLE_AUTO_POSITION);
        bool const fInsertModeExpected = IsFlagSet(ulNewMode, ENABLE_INSERT_MODE);

        // If the insert mode changed, we expect the cursor to have turned off.
        BOOL const fCursorDBModeExpected = (!!_fPrevInsertMode != fInsertModeExpected) ? FALSE : TRUE;

        // Call the API
        HRESULT const hrActual = _pApiRoutines->SetConsoleInputModeImpl(pii, ulNewMode);

        // Now do verifications of final state.
        VERIFY_ARE_EQUAL(hrExpected, hrActual);
        VERIFY_ARE_EQUAL(ulModeExpected, pii->InputMode);
        VERIFY_ARE_EQUAL(fQuickEditExpected, IsFlagSet(gci->Flags, CONSOLE_QUICK_EDIT_MODE));
        VERIFY_ARE_EQUAL(fAutoPositionExpected, IsFlagSet(gci->Flags, CONSOLE_AUTO_POSITION));
        VERIFY_ARE_EQUAL(!!fInsertModeExpected, !!gci->GetInsertMode());
        VERIFY_ARE_EQUAL(!!fCursorDBModeExpected, !!gci->CurrentScreenBuffer->TextInfo->GetCursor()->IsDouble());
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
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        Log::Comment(L"Turn on insert mode with cooked read data.");
        gci->lpCookedReadData = new COOKED_READ_DATA();

        PrepVerifySetConsoleInputModeImpl(0);
        Log::Comment(L"Success code should result from setting valid flags.");
        Log::Comment(L"Flags should be set exactly as given.");
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE);
        Log::Comment(L"Turn back off and verify.");
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_EXTENDED_FLAGS);

        delete gci->lpCookedReadData;
        gci->lpCookedReadData = nullptr;
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
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        Log::Comment(L"Verify that we can set various extended flags even without the ENABLE_EXTENDED_FLAGS flag.");
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_INSERT_MODE);
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_QUICK_EDIT_MODE);
        PrepVerifySetConsoleInputModeImpl(0);
        VerifySetConsoleInputModeImpl(S_OK, ENABLE_AUTO_POSITION);

        Log::Comment(L"Verify that we cannot unset various extended flags without the ENABLE_EXTENDED_FLAGS flag.");
        PrepVerifySetConsoleInputModeImpl(ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION);
        InputBuffer* const pii = gci->pInputBuffer;
        HRESULT const hr = _pApiRoutines->SetConsoleInputModeImpl(pii, 0);

        VERIFY_ARE_EQUAL(S_OK, hr);
        VERIFY_ARE_EQUAL(true, !!gci->GetInsertMode());
        VERIFY_ARE_EQUAL(true, IsFlagSet(gci->Flags, CONSOLE_QUICK_EDIT_MODE));
        VERIFY_ARE_EQUAL(true, IsFlagSet(gci->Flags, CONSOLE_AUTO_POSITION));
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
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        gci->Title = L"Test window title.";

        int const iBytesNeeded = WideCharToMultiByte(gci->OutputCP,
                                                     0,
                                                     gci->Title,
                                                     -1,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     nullptr);

        wistd::unique_ptr<char[]> pszExpected = wil::make_unique_nothrow<char[]>(iBytesNeeded);
        VERIFY_IS_NOT_NULL(pszExpected);

        VERIFY_WIN32_BOOL_SUCCEEDED(WideCharToMultiByte(gci->OutputCP,
                                                        0,
                                                        gci->Title,
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
        VERIFY_ARE_EQUAL(wcslen(gci->Title) + 1, cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci->Title), cchNeeded);
        VERIFY_IS_TRUE(0 == strcmp(pszExpected.get(), pszTitle));
    }

    TEST_METHOD(ApiGetConsoleTitleW)
    {
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        gci->Title = L"Test window title.";

        wchar_t pwszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        VERIFY_SUCCEEDED(_pApiRoutines->GetConsoleTitleWImpl(pwszTitle, ARRAYSIZE(pwszTitle), &cchWritten, &cchNeeded));

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci->Title), cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci->Title), cchNeeded);
        VERIFY_IS_TRUE(0 == wcscmp(gci->Title, pwszTitle));
    }

    TEST_METHOD(ApiGetConsoleOriginalTitleA)
    {
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        gci->OriginalTitle = L"Test original window title.";

        int const iBytesNeeded = WideCharToMultiByte(gci->OutputCP,
                                                     0,
                                                     gci->OriginalTitle,
                                                     -1,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     nullptr);

        wistd::unique_ptr<char[]> pszExpected = wil::make_unique_nothrow<char[]>(iBytesNeeded);
        VERIFY_IS_NOT_NULL(pszExpected);

        VERIFY_WIN32_BOOL_SUCCEEDED(WideCharToMultiByte(gci->OutputCP,
                                                        0,
                                                        gci->OriginalTitle,
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
        VERIFY_ARE_EQUAL(wcslen(gci->OriginalTitle) + 1, cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci->OriginalTitle), cchNeeded);
        VERIFY_IS_TRUE(0 == strcmp(pszExpected.get(), pszTitle));
    }

    TEST_METHOD(ApiGetConsoleOriginalTitleW)
    {
        CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
        gci->OriginalTitle = L"Test original window title.";

        wchar_t pwszTitle[MAX_PATH]; // most applications use MAX_PATH
        size_t cchWritten = 0;
        size_t cchNeeded = 0;
        VERIFY_SUCCEEDED(_pApiRoutines->GetConsoleOriginalTitleWImpl(pwszTitle, ARRAYSIZE(pwszTitle), &cchWritten, &cchNeeded));

        VERIFY_ARE_NOT_EQUAL(0u, cchWritten);
        // NOTE: W version of API returns string length. A version of API returns buffer length (string + null).
        VERIFY_ARE_EQUAL(wcslen(gci->OriginalTitle), cchWritten);
        VERIFY_ARE_EQUAL(wcslen(gci->OriginalTitle), cchNeeded);
        VERIFY_IS_TRUE(0 == wcscmp(gci->OriginalTitle, pwszTitle));
    }
};
