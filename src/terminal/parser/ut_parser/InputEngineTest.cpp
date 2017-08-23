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

// typedef struct _IN_KEY
// {
//     bool bKeyDown;    
//     WORD wRepeatCount;
//     WORD wVirtualKeyCode;
//     WORD wVirtualScanCode;
//     wchar_t UnicodeChar;
//     DWORD dwControlKeyState;

//     bool equals(const INPUT_RECORD& record) const
//     {
//         if (record.EventType != KEY_EVENT) return false;
//         KEY_EVENT_RECORD keyEvent = record.Event.KeyEvent;

//         return (!!keyEvent.bKeyDown == this->bKeyDown) && // bool and BOOL
//                (keyEvent.wRepeatCount == this->wRepeatCount) &&
//                (keyEvent.wVirtualKeyCode == this->wVirtualKeyCode) &&
//                (keyEvent.wVirtualScanCode == this->wVirtualScanCode) &&
//                (keyEvent.uChar.UnicodeChar == this->UnicodeChar) &&
//                (keyEvent.dwControlKeyState == this->dwControlKeyState);
//     }

// } IN_KEY;


class  Microsoft::Console::VirtualTerminal::InputEngineTest
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

    // TEST_METHOD(foo);
    TEST_METHOD(UnmodifiedTest);

    StateMachine* _pStateMachine;

};


// static int expected = 0;
// static std::vector<IN_KEY> v;
static std::vector<INPUT_RECORD> vExpectedInput;
// static IN_KEY* rExpectedKeys;


void InputEngineTest::s_TestInputCallback(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput)
{
    rgInput;
    cInput;

    VERIFY_IS_TRUE(cInput == vExpectedInput.size());
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

// void InputEngineTest::foo()
// {
//     // v.push_back({true, 1, VK_UP, (WORD)MapVirtualKey(VK_UP, MAPVK_VK_TO_VSC), 0, 0});
//     // v.push_back({false, 1, VK_UP, (WORD)MapVirtualKey(VK_UP, MAPVK_VK_TO_VSC), 0, 0});

//     std::wstring seq = L"\x1b[A";
//     _pStateMachine->ProcessString(&seq[0], seq.length());
    
// }


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
            inputSeq = L"\x1bOP";
            shouldSkip = false;
            break;
        case VK_F2:
            inputSeq = L"\x1bOQ";
            shouldSkip = false;
            break;
        case VK_F3:
            inputSeq = L"\x1bOR";
            shouldSkip = false;
            break;
        case VK_F4:
            inputSeq = L"\x1bOS";
            shouldSkip = false;
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
