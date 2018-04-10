/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"

#include "..\interactivity\win32\Clipboard.hpp"
#include "..\interactivity\inc\ServiceLocator.hpp"

#include "dbcs.h"

#include <cctype>

#ifdef BUILD_ONECORE_INTERACTIVITY
#include "..\..\interactivity\inc\VtApiRedirection.hpp"
#endif

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

#include "UnicodeLiteral.hpp"
#include "../../inc/consoletaeftemplates.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

static const WORD altScanCode = 0x38;
static const WORD leftShiftScanCode = 0x2A;

class ClipboardTests
{
    TEST_CLASS(ClipboardTests);

    CommonState* m_state;

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();
        m_state->PrepareGlobalInputBuffer();

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalInputBuffer();
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();

        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        m_state->FillTextBuffer();
        return true;
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        return true;
    }

    const UINT cRectsSelected = 4;

    void SetupRetrieveFromBuffers(bool fLineSelection, SMALL_RECT** prgsrSelection, PWCHAR** prgTempRows, size_t** prgTempRowLengths)
    {
        const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.hpp for information on the buffer state per row, the row contents, etc.

        // set up and try to retrieve the first 4 rows from the buffer
        const SCREEN_INFORMATION& screenInfo = gci.GetActiveOutputBuffer();

        SMALL_RECT* const rgsrSelection = new SMALL_RECT[cRectsSelected];

        rgsrSelection[0].Top = rgsrSelection[0].Bottom = 0;
        rgsrSelection[0].Left = 0;
        rgsrSelection[0].Right = 8;
        rgsrSelection[1].Top = rgsrSelection[1].Bottom = 1;
        rgsrSelection[1].Left = 0;
        rgsrSelection[1].Right = 14;
        rgsrSelection[2].Top = rgsrSelection[2].Bottom = 2;
        rgsrSelection[2].Left = 0;
        rgsrSelection[2].Right = 14;
        rgsrSelection[3].Top = rgsrSelection[3].Bottom = 3;
        rgsrSelection[3].Left = 0;
        rgsrSelection[3].Right = 8;

        PWCHAR* rgTempRows = new PWCHAR[cRectsSelected];
        size_t* rgTempRowLengths = new size_t[cRectsSelected];

        NTSTATUS status = Clipboard::Instance().RetrieveTextFromBuffer(screenInfo,
                                                                       fLineSelection,
                                                                       cRectsSelected,
                                                                       rgsrSelection,
                                                                       rgTempRows,
                                                                       rgTempRowLengths);

        // function successful
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // verify text lengths match the rows

        VERIFY_ARE_EQUAL(wcslen(rgTempRows[0]), rgTempRowLengths[0]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[1]), rgTempRowLengths[1]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[2]), rgTempRowLengths[2]);
        VERIFY_ARE_EQUAL(wcslen(rgTempRows[3]), rgTempRowLengths[3]);

        *prgsrSelection = rgsrSelection;
        *prgTempRows = rgTempRows;
        *prgTempRowLengths = rgTempRowLengths;
    }

    void CleanupRetrieveFromBuffers(SMALL_RECT** prgsrSelection, PWCHAR** prgTempRows, size_t** prgTempRowLengths)
    {
        if (*prgsrSelection != nullptr)
        {
            delete[](*prgsrSelection);
            *prgsrSelection = nullptr;
        }

        if (*prgTempRows != nullptr)
        {
            PWCHAR* rgTempRows = *prgTempRows;

            for (UINT iRow = 0; iRow < cRectsSelected; iRow++)
            {
                PWCHAR pwszRowData = rgTempRows[iRow];
                if (pwszRowData != nullptr)
                {
                    delete[] pwszRowData;
                    rgTempRows[iRow] = nullptr;
                }
            }

            delete[] rgTempRows;
            *prgTempRows = nullptr;
        }

        if (*prgTempRowLengths != nullptr)
        {
            delete[](*prgTempRowLengths);
            *prgTempRowLengths = nullptr;
        }
    }

#pragma prefast(push)
#pragma prefast(disable:26006, "Specifically trying to check unterminated strings in this test.")
    TEST_METHOD(TestRetrieveFromBuffer)
    {
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.hpp for information on the buffer state per row, the row contents, etc.

        SMALL_RECT* rgsrSelection = nullptr;
        PWCHAR* rgTempRows = nullptr;
        size_t* rgTempRowLengths = nullptr;

        SetupRetrieveFromBuffers(false, &rgsrSelection, &rgTempRows, &rgTempRowLengths);

        // verify trailing bytes were trimmed
        // there are 2 double-byte characters in our sample string (see CommonState.hpp for sample)
        // the width is right - left
        VERIFY_ARE_EQUAL((short)wcslen(rgTempRows[0]), rgsrSelection[0].Right - rgsrSelection[0].Left + 1);

        // since we're not in line selection, the line should be \r\n terminated
        PWCHAR tempPtr = rgTempRows[0];
        tempPtr += rgTempRowLengths[0];
        tempPtr -= 2;
        VERIFY_ARE_EQUAL(String(tempPtr), String(L"\r\n"));

        // since we're not in line selection, spaces should be trimmed from the end
        tempPtr = rgTempRows[0];
        tempPtr += rgsrSelection[0].Right - rgsrSelection[0].Left - 2;
        tempPtr++;
        VERIFY_IS_NULL(wcsrchr(tempPtr, L' '));

        // final line of selection should not contain CR/LF
        tempPtr = rgTempRows[3];
        tempPtr += rgTempRowLengths[3];
        tempPtr -= 2;
        VERIFY_ARE_NOT_EQUAL(String(tempPtr), String(L"\r\n"));

        CleanupRetrieveFromBuffers(&rgsrSelection, &rgTempRows, &rgTempRowLengths);
    }
#pragma prefast(pop)

    TEST_METHOD(TestRetrieveLineSelectionFromBuffer)
    {
        // NOTE: This test requires innate knowledge of how the common buffer text is emitted in order to test all cases
        // Please see CommonState.hpp for information on the buffer state per row, the row contents, etc.

        SMALL_RECT* rgsrSelection = nullptr;
        PWCHAR* rgTempRows = nullptr;
        size_t* rgTempRowLengths = nullptr;

        SetupRetrieveFromBuffers(true, &rgsrSelection, &rgTempRows, &rgTempRowLengths);

        // row 2, no wrap
        // no wrap row before the end should have CR/LF
        PWCHAR tempPtr = rgTempRows[2];
        tempPtr += rgTempRowLengths[2];
        tempPtr -= 2;
        VERIFY_ARE_EQUAL(String(tempPtr), String(L"\r\n"));

        // no wrap row should trim spaces at the end
        tempPtr = rgTempRows[2];
        VERIFY_IS_NULL(wcsrchr(tempPtr, L' '));

        // row 1, wrap
        // wrap row before the end should *not* have CR/LF
        tempPtr = rgTempRows[1];
        tempPtr += rgTempRowLengths[1];
        tempPtr -= 2;
        VERIFY_ARE_NOT_EQUAL(String(tempPtr), String(L"\r\n"));

        // wrap row should have spaces at the end
        tempPtr = rgTempRows[1];
        wchar_t* ptr = wcsrchr(tempPtr, L' ');
        VERIFY_IS_NOT_NULL(ptr);

        CleanupRetrieveFromBuffers(&rgsrSelection, &rgTempRows, &rgTempRowLengths);
    }

    TEST_METHOD(CanConvertTextToInputEvents)
    {
        std::wstring wstr = L"hello world";
        std::deque<std::unique_ptr<IInputEvent>> events = Clipboard::Instance().TextToKeyEvents(wstr.c_str(),
                                                                                                wstr.size());
        VERIFY_ARE_EQUAL(wstr.size() * 2, events.size());
        IInputServices* pInputServices = ServiceLocator::LocateInputServices();
        for (wchar_t wch : wstr)
        {
            std::deque<bool> keydownPattern{ true, false };
            for (bool isKeyDown : keydownPattern)
            {
                VERIFY_ARE_EQUAL(InputEventType::KeyEvent, events.front()->EventType());
                std::unique_ptr<KeyEvent> keyEvent;
                keyEvent.reset(static_cast<KeyEvent* const>(events.front().release()));
                events.pop_front();

                const short keyState = pInputServices->VkKeyScanW(wch);
                VERIFY_ARE_NOT_EQUAL(-1, keyState);
                const WORD virtualScanCode = static_cast<WORD>(pInputServices->MapVirtualKeyW(wch, MAPVK_VK_TO_VSC));

                VERIFY_ARE_EQUAL(wch, keyEvent->GetCharData());
                VERIFY_ARE_EQUAL(isKeyDown, keyEvent->IsKeyDown());
                VERIFY_ARE_EQUAL(1, keyEvent->GetRepeatCount());
                VERIFY_ARE_EQUAL(static_cast<DWORD>(0), keyEvent->GetActiveModifierKeys());
                VERIFY_ARE_EQUAL(virtualScanCode, keyEvent->GetVirtualScanCode());
                VERIFY_ARE_EQUAL(LOBYTE(keyState), keyEvent->GetVirtualKeyCode());
            }
        }
    }

    TEST_METHOD(CanConvertUppercaseText)
    {
        std::wstring wstr = L"HeLlO WoRlD";
        size_t uppercaseCount = 0;
        for (wchar_t wch : wstr)
        {
            std::isupper(wch) ? ++uppercaseCount : 0;
        }
        std::deque<std::unique_ptr<IInputEvent>> events = Clipboard::Instance().TextToKeyEvents(wstr.c_str(),
                                                                                                wstr.size());


        VERIFY_ARE_EQUAL((wstr.size() + uppercaseCount) * 2, events.size());
        IInputServices* pInputServices = ServiceLocator::LocateInputServices();
        VERIFY_IS_NOT_NULL(pInputServices);
        for (wchar_t wch : wstr)
        {
            std::deque<bool> keydownPattern{ true, false };
            for (bool isKeyDown : keydownPattern)
            {
                Log::Comment(NoThrowString().Format(L"testing char: %C; keydown: %d", wch, isKeyDown));

                VERIFY_ARE_EQUAL(InputEventType::KeyEvent, events.front()->EventType());
                std::unique_ptr<KeyEvent> keyEvent;
                keyEvent.reset(static_cast<KeyEvent* const>(events.front().release()));
                events.pop_front();

                const short keyScanError = -1;
                const short keyState = pInputServices->VkKeyScanW(wch);
                VERIFY_ARE_NOT_EQUAL(keyScanError, keyState);
                const WORD virtualScanCode = static_cast<WORD>(pInputServices->MapVirtualKeyW(wch, MAPVK_VK_TO_VSC));

                if (std::isupper(wch))
                {
                    // uppercase letters have shift key events
                    // surrounding them, making two events per letter
                    // (and another two for the keyup)
                    VERIFY_IS_FALSE(events.empty());

                    VERIFY_ARE_EQUAL(InputEventType::KeyEvent, events.front()->EventType());
                    std::unique_ptr<KeyEvent> keyEvent2;
                    keyEvent2.reset(static_cast<KeyEvent* const>(events.front().release()));
                    events.pop_front();

                    const short keyState2 = pInputServices->VkKeyScanW(wch);
                    VERIFY_ARE_NOT_EQUAL(keyScanError, keyState);
                    const WORD virtualScanCode2 = static_cast<WORD>(pInputServices->MapVirtualKeyW(wch, MAPVK_VK_TO_VSC));

                    if (isKeyDown)
                    {
                        // shift then letter
                        const KeyEvent shiftDownEvent({ TRUE, 1, VK_SHIFT, leftShiftScanCode, L'\0', SHIFT_PRESSED });
                        VERIFY_ARE_EQUAL(shiftDownEvent, *keyEvent);

                        const KeyEvent expectedKeyEvent({ TRUE, 1, LOBYTE(keyState2), virtualScanCode2, wch, SHIFT_PRESSED });
                        VERIFY_ARE_EQUAL(expectedKeyEvent, *keyEvent2);
                    }
                    else
                    {
                        // letter then shift
                        const KeyEvent expectedKeyEvent({ FALSE, 1, LOBYTE(keyState), virtualScanCode, wch, SHIFT_PRESSED });
                        VERIFY_ARE_EQUAL(expectedKeyEvent, *keyEvent);

                        const KeyEvent shiftUpEvent({ FALSE, 1, VK_SHIFT, leftShiftScanCode, L'\0', 0 });
                        VERIFY_ARE_EQUAL(shiftUpEvent, *keyEvent2);
                    }
                }
                else
                {
                    const KeyEvent expectedKeyEvent({ !!isKeyDown, 1, LOBYTE(keyState), virtualScanCode, wch, 0 });
                    VERIFY_ARE_EQUAL(expectedKeyEvent, *keyEvent);
                }
            }
        }
    }

#ifdef __INSIDE_WINDOWS
    TEST_METHOD(CanConvertCharsRequiringAltGr)
    {
        const std::wstring wstr = L"\x20ac"; // € char U+20AC
        std::deque<std::unique_ptr<IInputEvent>> events = Clipboard::Instance().TextToKeyEvents(wstr.c_str(),
                                                                                                wstr.size());

        // should be converted to:
        // 1. AltGr keydown
        // 2. € keydown
        // 3. € keyup
        // 4. AltGr keyup
        const size_t convertedSize = 4;
        VERIFY_ARE_EQUAL(convertedSize, events.size());

        const short keyState = VkKeyScanW(wstr[0]);
        const WORD virtualKeyCode = LOBYTE(keyState);

        std::deque<KeyEvent> expectedEvents;
        expectedEvents.push_back({ TRUE, 1, VK_MENU, altScanCode, L'\0', (ENHANCED_KEY | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED) });
        expectedEvents.push_back({ TRUE, 1, virtualKeyCode, 0, wstr[0], (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED) });
        expectedEvents.push_back({ FALSE, 1, virtualKeyCode, 0, wstr[0], (LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED) });
        expectedEvents.push_back({ FALSE, 1, VK_MENU, altScanCode, L'\0', ENHANCED_KEY });

        for (size_t i = 0; i < events.size(); ++i)
        {
            const KeyEvent currentKeyEvent = *reinterpret_cast<const KeyEvent* const>(events[i].get());
            VERIFY_ARE_EQUAL(expectedEvents[i], currentKeyEvent, NoThrowString().Format(L"i == %d", i));
        }
    }
#endif

    TEST_METHOD(CanConvertCharsOutsideKeyboardLayout)
    {
        const std::wstring wstr = L"\xbc"; // ¼ char U+00BC
        const UINT outputCodepage = CP_JAPANESE;
        ServiceLocator::LocateGlobals().getConsoleInformation().OutputCP = outputCodepage;
        std::deque<std::unique_ptr<IInputEvent>> events = Clipboard::Instance().TextToKeyEvents(wstr.c_str(),
                                                                                                wstr.size());

        // should be converted to:
        // 1. left alt keydown
        // 2. 1st numpad keydown
        // 3. 1st numpad keyup
        // 4. 2nd numpad keydown
        // 5. 2nd numpad keyup
        // 6. left alt keyup
        const size_t convertedSize = 6;
        VERIFY_ARE_EQUAL(convertedSize, events.size());

        std::deque<KeyEvent> expectedEvents;
        expectedEvents.push_back({ TRUE, 1,  VK_MENU, altScanCode, L'\0', LEFT_ALT_PRESSED });
        expectedEvents.push_back({ TRUE, 1,  0x66, 0x4D, L'\0', LEFT_ALT_PRESSED });
        expectedEvents.push_back({ FALSE, 1,  0x66, 0x4D, L'\0', LEFT_ALT_PRESSED });
        expectedEvents.push_back({ TRUE, 1,  0x63, 0x51, L'\0', LEFT_ALT_PRESSED });
        expectedEvents.push_back({ FALSE, 1,  0x63, 0x51, L'\0', LEFT_ALT_PRESSED });
        expectedEvents.push_back({ FALSE, 1, VK_MENU, altScanCode, wstr[0], 0 });

        for (size_t i = 0; i < events.size(); ++i)
        {
            const KeyEvent currentKeyEvent = *reinterpret_cast<const KeyEvent* const>(events[i].get());
            VERIFY_ARE_EQUAL(expectedEvents[i], currentKeyEvent, NoThrowString().Format(L"i == %d", i));
        }
    }
};
