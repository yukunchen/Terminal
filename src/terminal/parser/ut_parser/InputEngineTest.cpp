/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"


#include "stateMachine.hpp"
#include "InputStateMachineEngine.hpp"
#include "..\..\inc\consoletaeftemplates.hpp"

#include <vector>

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;


namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class InputEngineTest;
        };
    };
};
using namespace Microsoft::Console::VirtualTerminal;


class Microsoft::Console::VirtualTerminal::InputEngineTest
{
    TEST_CLASS(InputEngineTest);

    static void s_TestInputCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput);
    
    TEST_CLASS_SETUP(ClassSetup)
    {
        _pStateMachine = new StateMachine(new InputStateMachineEngine(s_TestInputCallback));
        return _pStateMachine != nullptr;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        delete _pStateMachine;
        return true;
    }
    TEST_METHOD_SETUP(MethodSetup)
    {
        _pStateMachine->ResetState();
        return true;
    }

    TEST_METHOD(UnmodifiedTest);
    TEST_METHOD(C0Test);
    TEST_METHOD(AlphanumericTest);
    TEST_METHOD(RoundTripTest);

    StateMachine* _pStateMachine;

};

static std::vector<INPUT_RECORD> vExpectedInput;

void InputEngineTest::s_TestInputCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    VERIFY_ARE_EQUAL(cInput, vExpectedInput.size());
    size_t numElems = min(cInput, vExpectedInput.size());
    size_t index = 0;

    for (auto expectedRecord = vExpectedInput.begin(); index < numElems; expectedRecord++)
    {
        // VERIFY_IS_TRUE(k->equals(rgInput[index]));
        Log::Comment(
            NoThrowString().Format(L"\tExpected: ") +  
            VerifyOutputTraits<INPUT_RECORD>::ToString(*expectedRecord)
        );

        Log::Comment(
            NoThrowString().Format(L"\tActual:   ") +
            VerifyOutputTraits<INPUT_RECORD>::ToString(rgInput[index])
        );

        // Log::Comment(VerifyOutputTraits<INPUT_RECORD>::ToString(rgInput[index]));

        VERIFY_ARE_EQUAL(*expectedRecord, rgInput[index]);
        index++;
    }
    vExpectedInput.clear();
}

void InputEngineTest::UnmodifiedTest()
{
    Log::Comment(L"Sending every possible VKEY at the input stream for interception during key DOWN.");
    
    DisableVerifyExceptions disable;

    for (BYTE vkey = 0; vkey < BYTE_MAX; vkey++)
    {
        std::wstring inputSeq = L"";
        wchar_t wch = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
        WORD scanCode = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
        
        DWORD dwModifierState = 0;

        bool shouldSkip = true;

        switch (vkey)
        {
        case VK_BACK:
            inputSeq = L"\x7f";
            shouldSkip = false;
            break;
        case VK_ESCAPE:
            inputSeq = L"\x1b";
            shouldSkip = false;
            // DebugBreak();
            break;
        case VK_PAUSE:
        // Why is this here? what is this key?
            inputSeq = L"\x1a";
            shouldSkip = false;
            break;
        case VK_UP:
            inputSeq = L"\x1b[A";
            shouldSkip = false;
            break;
        case VK_DOWN:
            inputSeq = L"\x1b[B";
            shouldSkip = false;
            break;
        case VK_RIGHT:
            inputSeq = L"\x1b[C";
            shouldSkip = false;
            break;
        case VK_LEFT:
            inputSeq = L"\x1b[D";
            shouldSkip = false;
            break;
        case VK_HOME:
            inputSeq = L"\x1b[H";
            shouldSkip = false;
            break;
        case VK_INSERT:
            inputSeq = L"\x1b[2~";
            shouldSkip = false;
            break;
        case VK_DELETE:
            inputSeq = L"\x1b[3~";
            shouldSkip = false;
            break;
        case VK_END:
            inputSeq = L"\x1b[F";
            shouldSkip = false;
            break;
        case VK_PRIOR:
            inputSeq = L"\x1b[5~";
            shouldSkip = false;
            break;
        case VK_NEXT:
            inputSeq = L"\x1b[6~";
            shouldSkip = false;
            break;
        case VK_F1:
            // F1-F4 are SS3 sequences, adding support for those in MSFT:13420038
            inputSeq = L"\x1bOP";
            shouldSkip = true;
            break;
        case VK_F2:
            inputSeq = L"\x1bOQ";
            shouldSkip = true;
            break;
        case VK_F3:
            inputSeq = L"\x1bOR";
            shouldSkip = true;
            break;
        case VK_F4:
            inputSeq = L"\x1bOS";
            shouldSkip = true;
            break;
        case VK_F5:
            inputSeq = L"\x1b[15~";
            shouldSkip = false;
            break;
        case VK_F6:
            inputSeq = L"\x1b[17~";
            shouldSkip = false;
            break;
        case VK_F7:
            inputSeq = L"\x1b[18~";
            shouldSkip = false;
            break;
        case VK_F8:
            inputSeq = L"\x1b[19~";
            shouldSkip = false;
            break;
        case VK_F9:
            inputSeq = L"\x1b[20~";
            shouldSkip = false;
            break;
        case VK_F10:
            inputSeq = L"\x1b[21~";
            shouldSkip = false;
            break;
        case VK_F11:
            inputSeq = L"\x1b[23~";
            shouldSkip = false;
            break;
        case VK_F12:
            inputSeq = L"\x1b[24~";
            shouldSkip = false;
            break;
        case VK_CANCEL:
            inputSeq = L"\x3";
            shouldSkip = false;
            break;
        default:
            break;
        }

        // Don't skip alphanumerics
        if (shouldSkip && (vkey >= '0' && vkey <= '9'))
        {
            shouldSkip = false;
            inputSeq = std::wstring(&wch, 1);
        }

        if (shouldSkip && (vkey >= 'A' && vkey <= 'Z'))
        {
            shouldSkip = false;
            inputSeq = std::wstring(&wch, 1);
            // 'A' - 'Z' need to have shift pressed, but numbers shouldn't
            dwModifierState = SHIFT_PRESSED;
        }

        if (shouldSkip)
        {
            Log::Comment(NoThrowString().Format(L"Skipping Key 0x%x", vkey));
            continue;
        }

        Log::Comment(NoThrowString().Format(L"Testing Key 0x%x", vkey));
        Log::Comment(NoThrowString().Format(L"Input Sequence=\"%s\"", inputSeq.c_str()));

        INPUT_RECORD rgInput[2];
        rgInput[0].EventType = KEY_EVENT;
        rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
        rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
        rgInput[0].Event.KeyEvent.wRepeatCount = 1;
        rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
        rgInput[0].Event.KeyEvent.wVirtualScanCode = scanCode;
        rgInput[0].Event.KeyEvent.uChar.UnicodeChar = wch;

        rgInput[1] = rgInput[0];
        rgInput[1].Event.KeyEvent.bKeyDown = FALSE;

        vExpectedInput.push_back(rgInput[0]);
        vExpectedInput.push_back(rgInput[1]);

        // if (wch == L'A') DebugBreak();

        _pStateMachine->ProcessString(&inputSeq[0], inputSeq.length());

    }
}



void InputEngineTest::C0Test()
{
    Log::Comment(L"Sending 0x0-0x19 to parser to make sure they're translated correctly back to C-key");
    DisableVerifyExceptions disable;
    for (wchar_t wch = '\x0'; wch < '\x20'; wch++)
    {
        std::wstring inputSeq = std::wstring(&wch, 1);
        
        wchar_t expectedWch = wch + 0x40;
        short keyscan = VkKeyScan(expectedWch);
        short vkey =  keyscan & 0xff;
        short keyscanModifiers = (keyscan >> 8) & 0xff;
        WORD scanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);

        // Because of course, these are not the same flags.
        // The expected key may also have shift pressed. 
        // Case in point: 0x3 = (Ctrl+C) = (Ctrl + Shift + vkey('c'))
        DWORD dwModifierState = LEFT_CTRL_PRESSED |
            (IsFlagSet(keyscanModifiers, 1) ? SHIFT_PRESSED : 0);
        // It won't have Alt or anything else pressed.

        Log::Comment(NoThrowString().Format(L"Testing char 0x%x", wch));
        Log::Comment(NoThrowString().Format(L"Input Sequence=\"%s\"", inputSeq.c_str()));

        INPUT_RECORD rgInput[4];
        rgInput[0].EventType = KEY_EVENT;
        rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
        rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
        rgInput[0].Event.KeyEvent.wRepeatCount = 1;
        rgInput[0].Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
        rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
        rgInput[0].Event.KeyEvent.uChar.UnicodeChar = 0;


        rgInput[1].EventType = KEY_EVENT;
        rgInput[1].Event.KeyEvent.bKeyDown = TRUE;
        rgInput[1].Event.KeyEvent.dwControlKeyState = dwModifierState;
        rgInput[1].Event.KeyEvent.wRepeatCount = 1;
        rgInput[1].Event.KeyEvent.wVirtualKeyCode = vkey;
        rgInput[1].Event.KeyEvent.wVirtualScanCode = scanCode;
        rgInput[1].Event.KeyEvent.uChar.UnicodeChar = wch;

        rgInput[2] = rgInput[1];
        rgInput[2].Event.KeyEvent.bKeyDown = FALSE;

        rgInput[3] = rgInput[0];
        rgInput[3].Event.KeyEvent.bKeyDown = FALSE;
        rgInput[3].Event.KeyEvent.dwControlKeyState = dwModifierState ^ LEFT_CTRL_PRESSED;

        vExpectedInput.push_back(rgInput[0]);
        vExpectedInput.push_back(rgInput[1]);
        vExpectedInput.push_back(rgInput[2]);
        vExpectedInput.push_back(rgInput[3]);

        _pStateMachine->ProcessString(&inputSeq[0], inputSeq.length());

    }
}

void InputEngineTest::AlphanumericTest()
{
    Log::Comment(L"Sending every printable ASCII character");
    DisableVerifyExceptions disable;
    for (wchar_t wch = '\x20'; wch < '\x7f'; wch++)
    {
        std::wstring inputSeq = std::wstring(&wch, 1);
        
        short keyscan = VkKeyScan(wch);
        short vkey =  keyscan & 0xff;
        WORD scanCode = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);

        short keyscanModifiers = (keyscan >> 8) & 0xff;
        // Because of course, these are not the same flags.
        DWORD dwModifierState = 0 |
            IsFlagSet(keyscanModifiers, 1) ? SHIFT_PRESSED : 0 |
            IsFlagSet(keyscanModifiers, 2) ? LEFT_CTRL_PRESSED : 0 |
            IsFlagSet(keyscanModifiers, 4) ? LEFT_ALT_PRESSED : 0 ;

        Log::Comment(NoThrowString().Format(L"Testing char 0x%x", wch));
        Log::Comment(NoThrowString().Format(L"Input Sequence=\"%s\"", inputSeq.c_str()));

        INPUT_RECORD rgInput[2];
        rgInput[0].EventType = KEY_EVENT;
        rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
        rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
        rgInput[0].Event.KeyEvent.wRepeatCount = 1;
        rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
        rgInput[0].Event.KeyEvent.wVirtualScanCode = scanCode;
        rgInput[0].Event.KeyEvent.uChar.UnicodeChar = wch;

        rgInput[1] = rgInput[0];
        rgInput[1].Event.KeyEvent.bKeyDown = FALSE;

        vExpectedInput.push_back(rgInput[0]);
        vExpectedInput.push_back(rgInput[1]);

        if (wch >= 'A' && wch <= 'Z')
        {
            dwModifierState = SHIFT_PRESSED;
        }

        _pStateMachine->ProcessString(&inputSeq[0], inputSeq.length());

    }
    
}


void InputEngineTest::RoundTripTest()
{
    // Send Every VKEY through the TerminalInput module, then take the uchar's 
    //   from the generated INPUT_RECORDs and put them through the InputEngine.
    // The VKEY sequence it writes out should be the same as the original.
    VERIFY_SUCCEEDED(E_FAIL);
}