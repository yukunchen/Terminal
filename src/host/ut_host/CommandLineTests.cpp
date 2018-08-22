/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "../../interactivity/inc/ServiceLocator.hpp"

#include "../cmdline.h"


using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

constexpr size_t PROMPT_SIZE = 512;

class CommandLineTests
{
    std::unique_ptr<CommonState> m_state;
    CommandHistory* m_pHistory;

    TEST_CLASS(CommandLineTests);

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
        m_state->PrepareCookedReadData();
        m_pHistory = CommandHistory::s_Allocate(L"cmd.exe", (HANDLE)0);
        if (!m_pHistory)
        {
            return false;
        }
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        CommandHistory::s_Free((HANDLE)0);
        m_pHistory = nullptr;
        m_state->CleanupCookedReadData();
        m_state->CleanupGlobalScreenBuffer();
        return true;
    }

    void VerifyPromptText(COOKED_READ_DATA& cookedReadData, const std::wstring wstr)
    {
        const auto span = cookedReadData.SpanWholeBuffer();
        VERIFY_ARE_EQUAL(cookedReadData._BytesRead, wstr.size() * sizeof(wchar_t));
        for (size_t i = 0; i < wstr.size(); ++i)
        {
            VERIFY_ARE_EQUAL(span.at(i), wstr.at(i));
        }
    }

    void InitCookedReadData(COOKED_READ_DATA& cookedReadData,
                            CommandHistory* pHistory,
                            wchar_t* pBuffer,
                            const size_t cchBuffer)
    {
        cookedReadData._commandHistory = pHistory;
        cookedReadData._UserBuffer = pBuffer;
        cookedReadData._UserBufferSize = cchBuffer * sizeof(wchar_t);
        cookedReadData._BufferSize = cchBuffer * sizeof(wchar_t);
        cookedReadData._BackupLimit = pBuffer;
        cookedReadData._BufPtr = pBuffer;
        cookedReadData._exeName = L"cmd.exe";
        cookedReadData.OriginalCursorPosition() = { 0, 0 };
    }

    void SetPrompt(COOKED_READ_DATA& cookedReadData, const std::wstring text)
    {
        std::copy(text.begin(), text.end(), cookedReadData._BackupLimit);
        cookedReadData._BytesRead = text.size() * sizeof(wchar_t);
        cookedReadData._CurrentPosition = text.size();
        cookedReadData._BufPtr += text.size();
        cookedReadData._visibleCharCount = text.size();
    }

    void MoveCursor(COOKED_READ_DATA& cookedReadData, const size_t column)
    {
        cookedReadData._CurrentPosition = column;
        cookedReadData._BufPtr = cookedReadData._BackupLimit + column;
    }

    TEST_METHOD(CanCycleCommandHistory)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 1", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 2", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 3", false));

        auto& commandLine = CommandLine::Instance();
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
        // should not have anything on the prompt
        VERIFY_ARE_EQUAL(cookedReadData._BytesRead, 0u);

        // go back one history item
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        VerifyPromptText(cookedReadData, L"echo 3");

        // try to go to the next history item, prompt shouldn't change
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
        VerifyPromptText(cookedReadData, L"echo 3");

        // go back another
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        VerifyPromptText(cookedReadData, L"echo 2");

        // go forward
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
        VerifyPromptText(cookedReadData, L"echo 3");

        // go back two
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        VerifyPromptText(cookedReadData, L"echo 1");

        // make sure we can't go back further
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        VerifyPromptText(cookedReadData, L"echo 1");

        // can still go forward
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
        VerifyPromptText(cookedReadData, L"echo 2");
    }

    TEST_METHOD(CanSetPromptToOldestHistory)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 1", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 2", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 3", false));

        auto& commandLine = CommandLine::Instance();
        commandLine._setPromptToOldestCommand(cookedReadData);
        VerifyPromptText(cookedReadData, L"echo 1");

        // change prompt and go back to oldest
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Next);
        commandLine._setPromptToOldestCommand(cookedReadData);
        VerifyPromptText(cookedReadData, L"echo 1");
    }

    TEST_METHOD(CanSetPromptToNewestHistory)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 1", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 2", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 3", false));

        auto& commandLine = CommandLine::Instance();
        commandLine._setPromptToNewestCommand(cookedReadData);
        VerifyPromptText(cookedReadData, L"echo 3");

        // change prompt and go back to newest
        commandLine._processHistoryCycling(cookedReadData, CommandHistory::SearchDirection::Previous);
        commandLine._setPromptToNewestCommand(cookedReadData);
        VerifyPromptText(cookedReadData, L"echo 3");
    }

    TEST_METHOD(CanDeletePromptAfterCursor)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        auto& commandLine = CommandLine::Instance();
        // set current cursor position somewhere in the middle of the prompt
        MoveCursor(cookedReadData, 4);
        commandLine._deletePromptAfterCursor(cookedReadData);
        VerifyPromptText(cookedReadData, L"test");
    }

    TEST_METHOD(CanDeletePromptBeforeCursor)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        // set current cursor position somewhere in the middle of the prompt
        MoveCursor(cookedReadData, 5);
        auto& commandLine = CommandLine::Instance();
        const COORD cursorPos = commandLine._deletePromptBeforeCursor(cookedReadData);
        cookedReadData._CurrentPosition = cursorPos.X;
        VerifyPromptText(cookedReadData, L"word blah");
    }

    TEST_METHOD(CanMoveCursorToEndOfPrompt)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        // make sure the cursor is not at the start of the prompt
        VERIFY_ARE_NOT_EQUAL(cookedReadData._CurrentPosition, 0u);
        VERIFY_ARE_NOT_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit);

        // save current position for later checking
        const auto expectedCursorPos = cookedReadData._CurrentPosition;
        const auto expectedBufferPos = cookedReadData._BufPtr;

        MoveCursor(cookedReadData, 0);

        auto& commandLine = CommandLine::Instance();
        const COORD cursorPos = commandLine._moveCursorToEndOfPrompt(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, gsl::narrow<short>(expectedCursorPos));
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, expectedCursorPos);
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, expectedBufferPos);

    }

    TEST_METHOD(CanMoveCursorToStartOfPrompt)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        // make sure the cursor is not at the start of the prompt
        VERIFY_ARE_NOT_EQUAL(cookedReadData._CurrentPosition, 0u);
        VERIFY_ARE_NOT_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit);

        auto& commandLine = CommandLine::Instance();
        const COORD cursorPos = commandLine._moveCursorToStartOfPrompt(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, 0);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, 0u);
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit);
    }

    TEST_METHOD(CanMoveCursorLeftByWord)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        auto& commandLine = CommandLine::Instance();
        // cursor position at beginning of "blah"
        short expectedPos = 10;
        COORD cursorPos = commandLine._moveCursorLeftByWord(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, gsl::narrow<size_t>(expectedPos));
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit + expectedPos);

        // move again
        expectedPos = 5; // before "word"
        cursorPos = commandLine._moveCursorLeftByWord(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, gsl::narrow<size_t>(expectedPos));
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit + expectedPos);

        // move again
        expectedPos = 0; // before "test"
        cursorPos = commandLine._moveCursorLeftByWord(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, gsl::narrow<size_t>(expectedPos));
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit + expectedPos);

        // try to move again, nothing should happen
        cursorPos = commandLine._moveCursorLeftByWord(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, gsl::narrow<size_t>(expectedPos));
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit + expectedPos);
    }

    TEST_METHOD(CanMoveCursorLeft)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        const std::wstring expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        // move left from end of prompt text to the beginning of the prompt
        auto& commandLine = CommandLine::Instance();
        for (auto it = expected.crbegin(); it != expected.crend(); ++it)
        {
            const COORD cursorPos = commandLine._moveCursorLeft(cookedReadData);
            VERIFY_ARE_EQUAL(*cookedReadData._BufPtr, *it);
        }
        // should now be at the start of the prompt
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, 0u);
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit);

        // try to move left a final time, nothing should change
        const COORD cursorPos = commandLine._moveCursorLeft(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, 0);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, 0u);
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, cookedReadData._BackupLimit);
    }

    /*
      // TODO MSFT:11285829 tcome back and turn these on once the system cursor isn't needed
    TEST_METHOD(CanMoveCursorRightByWord)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto expected = L"test word blah";
        SetPrompt(cookedReadData, expected);
        VerifyPromptText(cookedReadData, expected);

        // save current position for later checking
        const auto endCursorPos = cookedReadData._CurrentPosition;
        const auto endBufferPos = cookedReadData._BufPtr;
        // NOTE: need to initialize the actualy cursor and keep it up to date with the changes here. remove
        once functions are fixed
        // try to move right, nothing should happen
        short expectedPos = gsl::narrow<short>(endCursorPos);
        COORD cursorPos = MoveCursorRightByWord(cookedReadData);
        VERIFY_ARE_EQUAL(cursorPos.X, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._CurrentPosition, expectedPos);
        VERIFY_ARE_EQUAL(cookedReadData._BufPtr, endBufferPos);

        // move to beginning of prompt and walk to the right by word
    }

    TEST_METHOD(CanMoveCursorRight)
    {
    }

    TEST_METHOD(CanDeleteFromRightOfCursor)
    {
    }

    */

    TEST_METHOD(CanInsertCtrlZ)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, nullptr, buffer.get(), PROMPT_SIZE);

        auto& commandLine = CommandLine::Instance();
        commandLine._insertCtrlZ(cookedReadData);
        VerifyPromptText(cookedReadData, L"\x1a"); // ctrl-z
    }

    TEST_METHOD(CanDeleteCommandHistory)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 1", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 2", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"echo 3", false));

        auto& commandLine = CommandLine::Instance();
        commandLine._deleteCommandHistory(cookedReadData);
        VERIFY_ARE_EQUAL(m_pHistory->GetNumberOfCommands(), 0u);
    }

    TEST_METHOD(CanFillPromptWithPreviousCommandFragment)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"I'm a little teapot", false));
        SetPrompt(cookedReadData, L"short and stout");

        auto& commandLine = CommandLine::Instance();
        commandLine._fillPromptWithPreviousCommandFragment(cookedReadData);
        VerifyPromptText(cookedReadData, L"short and stoutapot");
    }

    TEST_METHOD(CanCycleMatchingCommandHistory)
    {
        auto buffer = std::make_unique<wchar_t[]>(PROMPT_SIZE);
        VERIFY_IS_NOT_NULL(buffer.get());

        auto& cookedReadData = ServiceLocator::LocateGlobals().getConsoleInformation().CookedReadData();
        InitCookedReadData(cookedReadData, m_pHistory, buffer.get(), PROMPT_SIZE);

        VERIFY_SUCCEEDED(m_pHistory->Add(L"I'm a little teapot", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"short and stout", false));
        VERIFY_SUCCEEDED(m_pHistory->Add(L"inflammable", false));

        SetPrompt(cookedReadData, L"i");

        auto& commandLine = CommandLine::Instance();
        commandLine._cycleMatchingCommandHistoryToPrompt(cookedReadData);
        VerifyPromptText(cookedReadData, L"inflammable");

        // make sure we skip to the next "i" history item
        commandLine._cycleMatchingCommandHistoryToPrompt(cookedReadData);
        VerifyPromptText(cookedReadData, L"I'm a little teapot");

        // should cycle back to the start of the command history
        commandLine._cycleMatchingCommandHistoryToPrompt(cookedReadData);
        VerifyPromptText(cookedReadData, L"inflammable");
    }
};
