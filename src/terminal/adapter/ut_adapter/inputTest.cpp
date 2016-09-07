/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>
#include <wextestclass.h>
#include "ConsoleTAEFTemplates.hpp"

#include "terminalInput.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class InputTest;
        };
    };
};
using namespace Microsoft::Console::VirtualTerminal;

// For magic reasons, this has to live outside the class. Something wonderful about TAEF macros makes it
// invisible to the linker when inside the class.
static PCWSTR s_pwszInputExpected;

class InputTest
{
public:

    TEST_CLASS(InputTest);

    static void s_TerminalInputTestCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
    {
        size_t cInputExpected = 0;
        VERIFY_SUCCEEDED(StringCchLengthW(s_pwszInputExpected, STRSAFE_MAX_CCH, &cInputExpected));

        if (VERIFY_ARE_EQUAL(cInputExpected, cInput, L"Verify expected and actual input array lengths matched."))
        {
            Log::Comment(L"We are expecting always key events and always key down. All other properties should not be written by simulated keys.");

            INPUT_RECORD irExpected = { 0 };
            irExpected.EventType = KEY_EVENT;
            irExpected.Event.KeyEvent.bKeyDown = TRUE;
            irExpected.Event.KeyEvent.wRepeatCount = 1;

            Log::Comment(L"Verifying individual array members...");
            for (size_t i = 0; i < cInput; i++)
            {
                irExpected.Event.KeyEvent.uChar.UnicodeChar = s_pwszInputExpected[i];
                VERIFY_ARE_EQUAL(irExpected, rgInput[i]);
            }
        }
    }
  
    TEST_METHOD(TerminalInputTests)
    {
        Log::Comment(L"Starting test...");

        const TerminalInput* const pInput = new TerminalInput(s_TerminalInputTestCallback);

        Log::Comment(L"Sending every possible VKEY at the input stream for interception during key DOWN.");
        for (BYTE vkey = 0; vkey < BYTE_MAX; vkey++)
        {
            Log::Comment(NoThrowString().Format(L"Testing Key 0x%x", vkey));

            bool fExpectedKeyHandled = true;

            INPUT_RECORD irTest = { 0 };
            irTest.EventType = KEY_EVENT;
            irTest.Event.KeyEvent.wRepeatCount = 1;
            irTest.Event.KeyEvent.wVirtualKeyCode = vkey;
            irTest.Event.KeyEvent.bKeyDown = TRUE;

            // Set up expected result
            switch (vkey)
            {
            case VK_BACK:
                s_pwszInputExpected = L"\x7f";
                break;
            case VK_ESCAPE:
                s_pwszInputExpected = L"\x1b";
                break;
            case VK_PAUSE:
                s_pwszInputExpected = L"\x1a";
                break;
            case VK_UP:
                s_pwszInputExpected = L"\x1b[A";
                break;
            case VK_DOWN:
                s_pwszInputExpected = L"\x1b[B";
                break;
            case VK_RIGHT:
                s_pwszInputExpected = L"\x1b[C";
                break;
            case VK_LEFT:
                s_pwszInputExpected = L"\x1b[D";
                break;
            case VK_HOME:
                s_pwszInputExpected = L"\x1b[H";
                break;
            case VK_INSERT:
                s_pwszInputExpected = L"\x1b[2~";
                break;
            case VK_DELETE:
                s_pwszInputExpected = L"\x1b[3~";
                break;
            case VK_END:
                s_pwszInputExpected = L"\x1b[F";
                break;
            case VK_PRIOR:
                s_pwszInputExpected = L"\x1b[5~";
                break;
            case VK_NEXT:
                s_pwszInputExpected = L"\x1b[6~";
                break;
            case VK_F1:
                s_pwszInputExpected = L"\x1bOP";
                break;
            case VK_F2:
                s_pwszInputExpected = L"\x1bOQ";
                break;
            case VK_F3:
                s_pwszInputExpected = L"\x1bOR";
                break;
            case VK_F4:
                s_pwszInputExpected = L"\x1bOS";
                break;
            case VK_F5:
                s_pwszInputExpected = L"\x1b[15~";
                break;
            case VK_F6:
                s_pwszInputExpected = L"\x1b[17~";
                break;
            case VK_F7:
                s_pwszInputExpected = L"\x1b[18~";
                break;
            case VK_F8:
                s_pwszInputExpected = L"\x1b[19~";
                break;
            case VK_F9:
                s_pwszInputExpected = L"\x1b[20~";
                break;
            case VK_F10:
                s_pwszInputExpected = L"\x1b[21~";
                break;
            case VK_F11:
                s_pwszInputExpected = L"\x1b[23~";
                break;
            case VK_F12:
                s_pwszInputExpected = L"\x1b[24~";
                break;
            default:
                fExpectedKeyHandled = false;
                break;
            }
            if (!fExpectedKeyHandled && (vkey >= '0' && vkey <= 'Z'))
            {
                fExpectedKeyHandled = true;
            }

            // Send key into object (will trigger callback and verification)
            VERIFY_ARE_EQUAL(fExpectedKeyHandled, pInput->HandleKey(&irTest), L"Verify key was handled if it should have been.");
        }

        Log::Comment(L"Sending every possible VKEY at the input stream for interception during key UP.");
        for (BYTE vkey = 0; vkey < BYTE_MAX; vkey++)
        {
            Log::Comment(NoThrowString().Format(L"Testing Key 0x%x", vkey));

            INPUT_RECORD irTest = { 0 };
            irTest.EventType = KEY_EVENT;
            irTest.Event.KeyEvent.wRepeatCount = 1;
            irTest.Event.KeyEvent.wVirtualKeyCode = vkey;
            irTest.Event.KeyEvent.bKeyDown = FALSE;

            // Send key into object (will trigger callback and verification)
            VERIFY_ARE_EQUAL(false, pInput->HandleKey(&irTest), L"Verify key was NOT handled.");
        }

        Log::Comment(L"Verify other types of events are not handled/intercepted.");

        INPUT_RECORD irUnhandled = { 0 };

        Log::Comment(L"Testing MOUSE_EVENT");
        irUnhandled.EventType = MOUSE_EVENT;
        VERIFY_ARE_EQUAL(false, pInput->HandleKey(&irUnhandled), L"Verify MOUSE_EVENT was NOT handled.");

        Log::Comment(L"Testing WINDOW_BUFFER_SIZE_EVENT");
        irUnhandled.EventType = WINDOW_BUFFER_SIZE_EVENT;
        VERIFY_ARE_EQUAL(false, pInput->HandleKey(&irUnhandled), L"Verify WINDOW_BUFFER_SIZE_EVENT was NOT handled.");

        Log::Comment(L"Testing MENU_EVENT");
        irUnhandled.EventType = MENU_EVENT;
        VERIFY_ARE_EQUAL(false, pInput->HandleKey(&irUnhandled), L"Verify MENU_EVENT was NOT handled.");

        Log::Comment(L"Testing FOCUS_EVENT");
        irUnhandled.EventType = FOCUS_EVENT;
        VERIFY_ARE_EQUAL(false, pInput->HandleKey(&irUnhandled), L"Verify FOCUS_EVENT was NOT handled.");
    }
};

