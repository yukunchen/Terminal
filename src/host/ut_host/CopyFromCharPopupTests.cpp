/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "../../interactivity/inc/ServiceLocator.hpp"

#include "../CopyFromCharPopup.hpp"


using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

static constexpr size_t BUFFER_SIZE = 256;

class CopyFromCharPopupTests
{
    TEST_CLASS(CopyFromCharPopupTests);

    std::unique_ptr<CommonState> m_state;

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = std::make_unique<CommonState>();
        m_state->PrepareGlobalFont();
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalFont();
        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state->PrepareGlobalScreenBuffer();
        m_state->PrepareGlobalInputBuffer();
        m_state->PrepareReadHandle();
        m_state->PrepareCookedReadData();
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        m_state->CleanupCookedReadData();
        m_state->CleanupReadHandle();
        m_state->CleanupGlobalInputBuffer();
        m_state->CleanupGlobalScreenBuffer();
        return true;
    }

    void InitReadData(COOKED_READ_DATA& cookedReadData,
                      wchar_t* const pBuffer,
                      const size_t cchBuffer,
                      const size_t cursorPosition)
    {
        cookedReadData._BufferSize = cchBuffer * sizeof(wchar_t);
        cookedReadData._BufPtr = pBuffer + cursorPosition;
        cookedReadData._BackupLimit = pBuffer;
        cookedReadData.OriginalCursorPosition() = { 0, 0 };
        cookedReadData._BytesRead = cursorPosition * sizeof(wchar_t);
        cookedReadData._CurrentPosition = cursorPosition;
        cookedReadData.VisibleCharCount() = cursorPosition;
    }

    TEST_METHOD(CanDismiss)
    {
        // function to simulate user pressing escape key
        Popup::UserInputFunction fn = [](COOKED_READ_DATA& /*cookedReadData*/, bool& popupKey, wchar_t& wch)
        {
            popupKey = true;
            wch = VK_ESCAPE;
            return STATUS_SUCCESS;
        };

        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        // prepare popup
        CopyFromCharPopup popup{ gci.GetActiveOutputBuffer() };
        popup.SetUserInputFunction(fn);

        // prepare cookedReadData
        std::wstring testString = L"hello world";
        wchar_t buffer[BUFFER_SIZE];
        std::fill(std::begin(buffer), std::end(buffer), UNICODE_SPACE);
        std::copy(testString.begin(), testString.end(), std::begin(buffer));
        auto& cookedReadData = gci.CookedReadData();
        InitReadData(cookedReadData, buffer, ARRAYSIZE(buffer), testString.size());

        VERIFY_ARE_EQUAL(popup.Process(cookedReadData), static_cast<NTSTATUS>(CONSOLE_STATUS_WAIT_NO_BLOCK));

        // the buffer should not be changed
        std::wstring resultString(buffer, buffer + testString.size());
        VERIFY_ARE_EQUAL(testString, resultString);
        VERIFY_ARE_EQUAL(cookedReadData._BytesRead, testString.size() * sizeof(wchar_t));

    }

    TEST_METHOD(DeleteAllWhenCharNotFound)
    {
        Popup::UserInputFunction fn = [](COOKED_READ_DATA& /*cookedReadData*/, bool& popupKey, wchar_t& wch)
        {
            popupKey = false;
            wch = L'x';
            return STATUS_SUCCESS;
        };

        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        // prepare popup
        CopyFromCharPopup popup{ gci.GetActiveOutputBuffer() };
        popup.SetUserInputFunction(fn);

        // prepare cookedReadData
        std::wstring testString = L"hello world";
        wchar_t buffer[BUFFER_SIZE];
        std::fill(std::begin(buffer), std::end(buffer), UNICODE_SPACE);
        std::copy(testString.begin(), testString.end(), std::begin(buffer));
        auto& cookedReadData = gci.CookedReadData();
        InitReadData(cookedReadData, buffer, ARRAYSIZE(buffer), testString.size());
        // move cursor to beginning of prompt text
        cookedReadData._CurrentPosition = 0;

        VERIFY_ARE_EQUAL(popup.Process(cookedReadData), static_cast<NTSTATUS>(CONSOLE_STATUS_WAIT_NO_BLOCK));

        // all text to the right of the cursor should be gone
        VERIFY_ARE_EQUAL(cookedReadData._BytesRead, 0u);
    }

    TEST_METHOD(CanDeletePartialLine)
    {
        Popup::UserInputFunction fn = [](COOKED_READ_DATA& /*cookedReadData*/, bool& popupKey, wchar_t& wch)
        {
            popupKey = false;
            wch = L'f';
            return STATUS_SUCCESS;
        };

        auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        // prepare popup
        CopyFromCharPopup popup{ gci.GetActiveOutputBuffer() };
        popup.SetUserInputFunction(fn);

        // prepare cookedReadData
        std::wstring testString = L"By the rude bridge that arched the flood";
        wchar_t buffer[BUFFER_SIZE];
        std::fill(std::begin(buffer), std::end(buffer), UNICODE_SPACE);
        std::copy(testString.begin(), testString.end(), std::begin(buffer));
        auto& cookedReadData = gci.CookedReadData();
        InitReadData(cookedReadData, buffer, ARRAYSIZE(buffer), testString.size());
        // move cursor to index 12
        const size_t index = 12;
        cookedReadData._BufPtr = buffer + index;
        cookedReadData._CurrentPosition = index;

        VERIFY_ARE_EQUAL(popup.Process(cookedReadData), static_cast<NTSTATUS>(CONSOLE_STATUS_WAIT_NO_BLOCK));

        std::wstring expectedText = L"By the rude flood";
        VERIFY_ARE_EQUAL(cookedReadData._BytesRead, expectedText.size() * sizeof(wchar_t));
        std::wstring resultText(buffer, buffer + expectedText.size());
        VERIFY_ARE_EQUAL(resultText, expectedText);
    }
};
