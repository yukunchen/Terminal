/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "stateMachine.hpp"
#include "InputStateMachineEngine.hpp"
#include "../adapter/terminalInput.hpp"
#include "../../inc/consoletaeftemplates.hpp"
#include "../../inc/unicode.hpp"
#include "../../types/inc/convert.hpp"

#include <vector>
#include <functional>
#include <sstream>
#include <string>

#ifdef BUILD_ONECORE_INTERACTIVITY
#include "../../../interactivity/inc/VtApiRedirection.hpp"
#endif

// From dbcs.h:
#define CP_USA                 437
// From utf8ToWideCharParser.hpp
#define CP_UTF8 65001

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
            class TestInteractDispatch;
        };
    };
};
using namespace Microsoft::Console::VirtualTerminal;

class Microsoft::Console::VirtualTerminal::InputEngineTest
{
    TEST_CLASS(InputEngineTest);

    void RoundtripTerminalInputCallback(std::deque<std::unique_ptr<IInputEvent>>& inEvents);
    void TestInputCallback(std::deque<std::unique_ptr<IInputEvent>>& inEvents);
    void TestInputStringCallback(std::deque<std::unique_ptr<IInputEvent>>& inEvents);
    
    TEST_CLASS_SETUP(ClassSetup)
    {
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        return true;
    }

    TEST_METHOD_SETUP(MethodSetup)
    {
        vExpectedInput.clear();
        return true;
    }

    TEST_METHOD(C0Test);
    TEST_METHOD(AlphanumericTest);
    TEST_METHOD(RoundTripTest);
    TEST_METHOD(WindowManipulationTest);
    TEST_METHOD(UTF8Test);

    StateMachine* _pStateMachine;

    std::deque<INPUT_RECORD> vExpectedInput;

    bool _expectedToCallWindowManipulation;
    bool _expectSendCtrlC;
    DispatchCommon::WindowManipulationType _expectedWindowManipulation;
    unsigned short _expectedParams[16];
    size_t _expectedCParams;

    friend class TestInteractDispatch;
};


class Microsoft::Console::VirtualTerminal::TestInteractDispatch : public IInteractDispatch
{
public:
    TestInteractDispatch(_In_ std::function<void(std::deque<std::unique_ptr<IInputEvent>>&)> pfn, InputEngineTest* testInstance);
    virtual bool WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents) override;
    virtual bool WriteCtrlC() override;
    virtual bool WindowManipulation(_In_ const DispatchCommon::WindowManipulationType uiFunction,
                                _In_reads_(cParams) const unsigned short* const rgusParams,
                                _In_ size_t const cParams) override; // DTTERM_WindowManipulation
    virtual bool WriteString(_In_reads_(cch) const wchar_t* const pws,
                             const size_t cch) override;
private:
    std::function<void(std::deque<std::unique_ptr<IInputEvent>>&)> _pfnWriteInputCallback;
    InputEngineTest* _testInstance;
};

TestInteractDispatch::TestInteractDispatch(_In_ std::function<void(std::deque<std::unique_ptr<IInputEvent>>&)> pfn, InputEngineTest* testInstance) :
    _pfnWriteInputCallback(pfn),
    _testInstance(testInstance)
{

}

bool TestInteractDispatch::WriteInput(_In_ std::deque<std::unique_ptr<IInputEvent>>& inputEvents)
{
    _pfnWriteInputCallback(inputEvents);
    return true;
}

bool TestInteractDispatch::WriteCtrlC()
{
    VERIFY_IS_TRUE(_testInstance->_expectSendCtrlC);
    KeyEvent key = KeyEvent(true, 1, 'C', 0, UNICODE_ETX, LEFT_CTRL_PRESSED);
    std::deque<std::unique_ptr<IInputEvent>> inputEvents;
    inputEvents.push_back(std::make_unique<KeyEvent>(key));
    return WriteInput(inputEvents);
}

bool TestInteractDispatch::WindowManipulation(_In_ const DispatchCommon::WindowManipulationType uiFunction,
                                              _In_reads_(cParams) const unsigned short* const rgusParams,
                                              _In_ size_t const cParams)
{

    VERIFY_ARE_EQUAL(true, _testInstance->_expectedToCallWindowManipulation);
    VERIFY_ARE_EQUAL(_testInstance->_expectedWindowManipulation, uiFunction);
    for(size_t i = 0; i < cParams; i++)
    {
        VERIFY_ARE_EQUAL(_testInstance->_expectedParams[i], rgusParams[i]);
    }
    return true;
}

bool TestInteractDispatch::WriteString(_In_reads_(cch) const wchar_t* const pws,
                                       const size_t cch)
{
    // DebugBreak();
    std::deque<std::unique_ptr<IInputEvent>> keyEvents;
    
    for (int i = 0; i < cch; ++i)
    {
        const wchar_t wch = pws[i];
        // We're forcing the translation to CP_USA, so that it'll be constant 
        //  regardless of the CP the test is running in
        std::deque<std::unique_ptr<KeyEvent>> convertedEvents = CharToKeyEvents(wch, CP_USA);
        while (!convertedEvents.empty())
        {
            keyEvents.push_back(std::move(convertedEvents.front()));
            convertedEvents.pop_front();
        }
    }
    
    return WriteInput(keyEvents);
}


bool IsShiftPressed(const DWORD modifierState)
{
    return IsFlagSet(modifierState, SHIFT_PRESSED);
}

bool IsAltPressed(const DWORD modifierState)
{
    return IsAnyFlagSet(modifierState, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
}

bool IsCtrlPressed(const DWORD modifierState)
{
    return IsAnyFlagSet(modifierState, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
}

bool ModifiersEquivalent(DWORD a, DWORD b)
{
    bool fShift = IsShiftPressed(a) == IsShiftPressed(b);
    bool fAlt = IsAltPressed(a) == IsAltPressed(b);
    bool fCtrl = IsCtrlPressed(a) == IsCtrlPressed(b);
    return fShift && fCtrl && fAlt;
}

void InputEngineTest::RoundtripTerminalInputCallback(_In_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    // Take all the characters out of the input records here, and put them into 
    //  the input state machine.
    size_t cInput = inEvents.size();
    INPUT_RECORD* rgInput = new INPUT_RECORD[cInput];
    VERIFY_SUCCEEDED(IInputEvent::ToInputRecords(inEvents, rgInput, cInput));
    VERIFY_IS_NOT_NULL(rgInput);
    auto cleanup = wil::ScopeExit([&]{delete[] rgInput;});

    std::wstring vtseq = L"";
    
    for (size_t i = 0; i < cInput; i++)
    {
        INPUT_RECORD inRec = rgInput[i];
        VERIFY_ARE_EQUAL(KEY_EVENT, inRec.EventType);
        if(inRec.Event.KeyEvent.bKeyDown)
        {
            vtseq += &inRec.Event.KeyEvent.uChar.UnicodeChar;
        }
    }
    Log::Comment(
        NoThrowString().Format(L"\tvtseq: \"%s\"(%zu)", vtseq.c_str(), vtseq.length())
    );

    _pStateMachine->ProcessString(&vtseq[0], vtseq.length());
    Log::Comment(L"String processed");
}

void InputEngineTest::TestInputCallback(std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    size_t cInput = inEvents.size();
    INPUT_RECORD* rgInput = new INPUT_RECORD[cInput];
    VERIFY_SUCCEEDED(IInputEvent::ToInputRecords(inEvents, rgInput, cInput));
    VERIFY_IS_NOT_NULL(rgInput);
    VERIFY_ARE_EQUAL((size_t)1, vExpectedInput.size());
    auto cleanup = wil::ScopeExit([&]{delete[] rgInput;});

    bool foundEqual = false;
    INPUT_RECORD irExpected = vExpectedInput.back();

    Log::Comment(
        NoThrowString().Format(L"\texpected:\t") +
        VerifyOutputTraits<INPUT_RECORD>::ToString(irExpected)
    );

    // Look for an equivalent input record.
    // Differences between left and right modifiers are ignored, as long as one is pressed.
    // There may be other keypresses, eg. modifier keypresses, those are ignored.
    for (size_t i = 0; i < cInput; i++)
    {
        INPUT_RECORD inRec = rgInput[i];
        Log::Comment(
            NoThrowString().Format(L"\tActual  :\t") +
            VerifyOutputTraits<INPUT_RECORD>::ToString(inRec)
        );

        bool areEqual = 
            (irExpected.EventType == inRec.EventType) &&
            (irExpected.Event.KeyEvent.bKeyDown == inRec.Event.KeyEvent.bKeyDown) &&
            (irExpected.Event.KeyEvent.wRepeatCount == inRec.Event.KeyEvent.wRepeatCount) &&
            (irExpected.Event.KeyEvent.uChar.UnicodeChar == inRec.Event.KeyEvent.uChar.UnicodeChar) &&
            ModifiersEquivalent(irExpected.Event.KeyEvent.dwControlKeyState, inRec.Event.KeyEvent.dwControlKeyState);

        foundEqual |= areEqual;
        if (areEqual)
        {
            Log::Comment(L"\t\tFound Match");
        }
    }

    VERIFY_IS_TRUE(foundEqual);
    vExpectedInput.clear();
}

void InputEngineTest::TestInputStringCallback(std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    size_t cInput = inEvents.size();
    INPUT_RECORD* rgInput = new INPUT_RECORD[cInput];
    VERIFY_SUCCEEDED(IInputEvent::ToInputRecords(inEvents, rgInput, cInput));
    VERIFY_IS_NOT_NULL(rgInput);
    // VERIFY_ARE_EQUAL((size_t)1, vExpectedInput.size());
    auto cleanup = wil::ScopeExit([&]{delete[] rgInput;});

    bool foundEqual = false;

    for (auto expected : vExpectedInput)
    {
        Log::Comment(
            NoThrowString().Format(L"\texpected:\t") +
            VerifyOutputTraits<INPUT_RECORD>::ToString(expected)
        );
        
    }

    INPUT_RECORD irExpected = vExpectedInput.front();
    Log::Comment(
        NoThrowString().Format(L"\tLooking for:\t") +
        VerifyOutputTraits<INPUT_RECORD>::ToString(irExpected)
    );


    // Look for an equivalent input record.
    // Differences between left and right modifiers are ignored, as long as one is pressed.
    // There may be other keypresses, eg. modifier keypresses, those are ignored.
    for (size_t i = 0; i < cInput; i++)
    {
        INPUT_RECORD inRec = rgInput[i];
        Log::Comment(
            NoThrowString().Format(L"\tActual  :\t") +
            VerifyOutputTraits<INPUT_RECORD>::ToString(inRec)
        );

        bool areEqual = 
            (irExpected.EventType == inRec.EventType) &&
            (irExpected.Event.KeyEvent.bKeyDown == inRec.Event.KeyEvent.bKeyDown) &&
            (irExpected.Event.KeyEvent.wRepeatCount == inRec.Event.KeyEvent.wRepeatCount) &&
            (irExpected.Event.KeyEvent.uChar.UnicodeChar == inRec.Event.KeyEvent.uChar.UnicodeChar) &&
            ModifiersEquivalent(irExpected.Event.KeyEvent.dwControlKeyState, inRec.Event.KeyEvent.dwControlKeyState);

        // foundEqual |= areEqual;
        if (areEqual)
        {
            Log::Comment(L"\t\tFound Match");
            vExpectedInput.pop_front();
            irExpected = vExpectedInput.front();
            Log::Comment(
                NoThrowString().Format(L"\tLooking for:\t") +
                VerifyOutputTraits<INPUT_RECORD>::ToString(irExpected)
            );
        }
    }

    VERIFY_IS_TRUE(foundEqual);
    vExpectedInput.clear();
}

void InputEngineTest::C0Test()
{
    auto pfn = std::bind(&InputEngineTest::TestInputCallback, this, std::placeholders::_1);
    _pStateMachine = new StateMachine(
            std::make_unique<InputStateMachineEngine>(
                std::make_unique<TestInteractDispatch>(pfn, this)
            )
    );
    VERIFY_IS_NOT_NULL(_pStateMachine);
    
    Log::Comment(L"Sending 0x0-0x19 to parser to make sure they're translated correctly back to C-key");
    DisableVerifyExceptions disable;
    for (wchar_t wch = '\x0'; wch < '\x20'; wch++)
    {
        std::wstring inputSeq = std::wstring(&wch, 1);
        // In general, he actual key that we're going to generate for a C0 char 
        //      is char+0x40 and with ctrl pressed.
        wchar_t expectedWch = wch + 0x40;
        bool writeCtrl = true;
        // These two are weird exceptional cases.
        switch(wch)
        {
            case L'\r': // Enter
                expectedWch = wch;
                writeCtrl = false;
                break;
            case L'\x1b': // Escape
                expectedWch = wch;
                writeCtrl = false;
                break;
            case L'\t': // Tab
                writeCtrl = false;
                break;
        }

        short keyscan = VkKeyScan(expectedWch);
        short vkey =  keyscan & 0xff;
        short keyscanModifiers = (keyscan >> 8) & 0xff;
        WORD scanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
        
        DWORD dwModifierState = 0;
        if (writeCtrl)
        {
            dwModifierState = SetFlag(dwModifierState, LEFT_CTRL_PRESSED);
        }
        // If we need to press shift for this key, but not on alphabetical chars
        //  Eg simulating C-z, not C-S-z.
        if (IsFlagSet(keyscanModifiers, 1) && (expectedWch < L'A' || expectedWch > L'Z' ))
        {
            dwModifierState = SetFlag(dwModifierState, SHIFT_PRESSED);
        }

        // Just make sure we write the same thing telnetd did:
        if (wch == UNICODE_ETX)
        {
            Log::Comment(NoThrowString().Format(
                L"We used to expect 0x%x, 0x%x, 0x%x, 0x%x here",
                vkey, scanCode, wch, dwModifierState
            ));
            vkey = 'C';
            scanCode = 0;
            wch = UNICODE_ETX;
            dwModifierState = LEFT_CTRL_PRESSED;
            Log::Comment(NoThrowString().Format(
                L"Now we expect 0x%x, 0x%x, 0x%x, 0x%x here",
                vkey, scanCode, wch, dwModifierState
            ));
            _expectSendCtrlC = true;
        }
        else
        {
            _expectSendCtrlC = false;
        }

        Log::Comment(NoThrowString().Format(L"Testing char 0x%x", wch));
        Log::Comment(NoThrowString().Format(L"Input Sequence=\"%s\"", inputSeq.c_str()));

        INPUT_RECORD inputRec;

        inputRec.EventType = KEY_EVENT;
        inputRec.Event.KeyEvent.bKeyDown = TRUE;
        inputRec.Event.KeyEvent.dwControlKeyState = dwModifierState;
        inputRec.Event.KeyEvent.wRepeatCount = 1;
        inputRec.Event.KeyEvent.wVirtualKeyCode = vkey;
        inputRec.Event.KeyEvent.wVirtualScanCode = scanCode;
        inputRec.Event.KeyEvent.uChar.UnicodeChar = wch;

        vExpectedInput.push_back(inputRec);

        _pStateMachine->ProcessString(&inputSeq[0], inputSeq.length());

    }
}

void InputEngineTest::AlphanumericTest()
{
    auto pfn = std::bind(&InputEngineTest::TestInputCallback, this, std::placeholders::_1);
    _pStateMachine = new StateMachine(
            std::make_unique<InputStateMachineEngine>(
                std::make_unique<TestInteractDispatch>(pfn, this)
            )
    );
    VERIFY_IS_NOT_NULL(_pStateMachine);
    
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
            (IsFlagSet(keyscanModifiers, 1) ? SHIFT_PRESSED : 0) |
            (IsFlagSet(keyscanModifiers, 2) ? LEFT_CTRL_PRESSED : 0) |
            (IsFlagSet(keyscanModifiers, 4) ? LEFT_ALT_PRESSED : 0) ;

        Log::Comment(NoThrowString().Format(L"Testing char 0x%x", wch));
        Log::Comment(NoThrowString().Format(L"Input Sequence=\"%s\"", inputSeq.c_str()));

        INPUT_RECORD inputRec;
        inputRec.EventType = KEY_EVENT;
        inputRec.Event.KeyEvent.bKeyDown = TRUE;
        inputRec.Event.KeyEvent.dwControlKeyState = dwModifierState;
        inputRec.Event.KeyEvent.wRepeatCount = 1;
        inputRec.Event.KeyEvent.wVirtualKeyCode = vkey;
        inputRec.Event.KeyEvent.wVirtualScanCode = scanCode;
        inputRec.Event.KeyEvent.uChar.UnicodeChar = wch;

        vExpectedInput.push_back(inputRec);

        _pStateMachine->ProcessString(&inputSeq[0], inputSeq.length());
    }
    
}

void InputEngineTest::RoundTripTest()
{
    auto pfn = std::bind(&InputEngineTest::TestInputCallback, this, std::placeholders::_1);
    _pStateMachine = new StateMachine(
            std::make_unique<InputStateMachineEngine>(
                std::make_unique<TestInteractDispatch>(pfn, this)
            )
    );
    VERIFY_IS_NOT_NULL(_pStateMachine);
    
    // Send Every VKEY through the TerminalInput module, then take the char's 
    //   from the generated INPUT_RECORDs and put them through the InputEngine.
    // The VKEY sequence it writes out should be the same as the original.

    auto pfn2 = std::bind(&InputEngineTest::RoundtripTerminalInputCallback, this, std::placeholders::_1);
    const TerminalInput* const pInput = new TerminalInput(pfn2);
    
    for (BYTE vkey = 0; vkey < BYTE_MAX; vkey++)
    {
        wchar_t wch = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
        WORD scanCode = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);

        unsigned int uiActualKeystate = 0;

        // Couple of exceptional cases here:
        if (vkey >= 'A' && vkey <= 'Z')
        {
            // A-Z need shift pressed in addition to the 'a'-'z' chars.
            uiActualKeystate = SetFlag(uiActualKeystate, SHIFT_PRESSED);
        }
        else if (vkey == VK_CANCEL  || vkey == VK_PAUSE)
        {
            uiActualKeystate = SetFlag(uiActualKeystate, LEFT_CTRL_PRESSED);
        }

        if (vkey == UNICODE_ETX)
        {
            _expectSendCtrlC = true;
        }

        INPUT_RECORD irTest = { 0 };
        irTest.EventType = KEY_EVENT;
        irTest.Event.KeyEvent.dwControlKeyState = uiActualKeystate;
        irTest.Event.KeyEvent.wRepeatCount = 1;
        irTest.Event.KeyEvent.wVirtualKeyCode = vkey;
        irTest.Event.KeyEvent.bKeyDown = TRUE;
        irTest.Event.KeyEvent.uChar.UnicodeChar = wch;
        irTest.Event.KeyEvent.wVirtualScanCode = scanCode;
        
        Log::Comment(
            NoThrowString().Format(L"Expecting::   ") +
            VerifyOutputTraits<INPUT_RECORD>::ToString(irTest)
        );

        vExpectedInput.clear();
        vExpectedInput.push_back(irTest);

        auto inputKey = IInputEvent::Create(irTest);
        pInput->HandleKey(inputKey.get());
    }

}

void InputEngineTest::WindowManipulationTest()
{
    auto pfn = std::bind(&InputEngineTest::TestInputCallback, this, std::placeholders::_1);
    _pStateMachine = new StateMachine(
            std::make_unique<InputStateMachineEngine>(
                std::make_unique<TestInteractDispatch>(pfn, this)
            )
    );
    VERIFY_IS_NOT_NULL(_pStateMachine);

    Log::Comment(NoThrowString().Format(
        L"Try sending a bunch of Window Manipulation sequences. "
        L"Only the valid ones should call the "
        L"TestInteractDispatch::WindowManipulation callback."
    ));

    bool fValidType = false;

    const unsigned short param1 = 123;
    const unsigned short param2 = 456;
    const wchar_t* const wszParam1 = L"123";
    const wchar_t* const wszParam2 = L"456";

    for(unsigned int i = 0; i < static_cast<unsigned int>(BYTE_MAX); i++)
    {
        if (i == DispatchCommon::WindowManipulationType::ResizeWindowInCharacters)
        {
            fValidType = true;
        }

        std::wstringstream seqBuilder;
        seqBuilder << L"\x1b[" << i;


        if (i == DispatchCommon::WindowManipulationType::ResizeWindowInCharacters)
        {
            // We need to build the string with the params as strings for some reason - 
            //      x86 would implicitly convert them to chars (eg 123 -> '{') 
            //      before appending them to the string 
            seqBuilder << L";" << wszParam1 << L";" << wszParam2;

            _expectedToCallWindowManipulation = true;
            _expectedCParams = 2;
            _expectedParams[0] = param1;
            _expectedParams[1] = param2;
            _expectedWindowManipulation = static_cast<DispatchCommon::WindowManipulationType>(i);
        }
        else if (i == DispatchCommon::WindowManipulationType::RefreshWindow)
        {
            // refresh window doesn't expect any params.

            _expectedToCallWindowManipulation = true;
            _expectedCParams = 0;
            _expectedWindowManipulation = static_cast<DispatchCommon::WindowManipulationType>(i);
        }
        else
        {
            _expectedToCallWindowManipulation = false;
            _expectedCParams = 0;
            _expectedWindowManipulation = DispatchCommon::WindowManipulationType::Invalid;
        }
        seqBuilder << L"t";
        std::wstring seq = seqBuilder.str();
        Log::Comment(NoThrowString().Format(
            L"Processing \"%s\"", seq.c_str()
        ));
        _pStateMachine->ProcessString(&seq[0], seq.length());
    }
}

void InputEngineTest::UTF8Test()
{
    auto pfn = std::bind(&InputEngineTest::TestInputStringCallback, this, std::placeholders::_1);
    _pStateMachine = new StateMachine(
            std::make_unique<InputStateMachineEngine>(
                std::make_unique<TestInteractDispatch>(pfn, this)
            )
    );
    VERIFY_IS_NOT_NULL(_pStateMachine);
    DisableVerifyExceptions disable;
    Log::Comment(L"Sending various utf-8 strings, and seeing what we get out");
    std::wstring utf8Input = L"\x041B"; // "Л", UTF-16: 041B,  utf8: "\xd09b"
    // std::string utf8Input = "Л"; //UTF-16: 041B

    INPUT_RECORD proto = {0};
    proto.EventType = KEY_EVENT;
    proto.Event.KeyEvent.dwControlKeyState = LEFT_ALT_PRESSED;
    proto.Event.KeyEvent.wRepeatCount = 1;
    proto.Event.KeyEvent.uChar.UnicodeChar = UNICODE_NULL;
    // Fill these in for each char
    proto.Event.KeyEvent.wVirtualKeyCode = 0;
    proto.Event.KeyEvent.bKeyDown = TRUE;
    proto.Event.KeyEvent.wVirtualScanCode = 0;

    INPUT_RECORD altDown = proto;
    altDown.Event.KeyEvent.wVirtualKeyCode = VK_MENU;
    altDown.Event.KeyEvent.wVirtualScanCode = altScanCode;
    INPUT_RECORD altUp = altDown;
    altUp.Event.KeyEvent.bKeyDown = FALSE;
    altUp.Event.KeyEvent.uChar.UnicodeChar = L'\x041B';


    // So this whole thing might be fucked all the way from VtInputThread::_HandleRunInput
    // VtInputThread::_HandleRunInput takes in the utf8 buffer, 
    // then calls ConvertToW
    INPUT_RECORD four = proto;
    four.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD4;
    four.Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(MapVirtualKeyW(four.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC));

    INPUT_RECORD one = proto;
    one.Event.KeyEvent.wVirtualKeyCode = VK_NUMPAD1;
    one.Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(MapVirtualKeyW(one.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC));

    INPUT_RECORD b = proto;
    b.Event.KeyEvent.wVirtualKeyCode = static_cast<WORD>('B');
    b.Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(MapVirtualKeyW(b.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC));

    // I think for some reason the synthesis just makes this a null-input 
    //      record with the uchar set to the actual char value.
    // I don't care if that's not what I though would happen, I just want it to 
    //      work, and that seems to work

    vExpectedInput.clear();
    vExpectedInput.push_back(altDown);
    vExpectedInput.push_back(four);
    vExpectedInput.push_back(one);
    vExpectedInput.push_back(b);
    vExpectedInput.push_back(altUp);

    Log::Comment(NoThrowString().Format(
        L"Processing \"%s\"", utf8Input.c_str()
    ));
    _pStateMachine->ProcessString(&utf8Input[0], utf8Input.length());

    utf8Input = L"旅";
    INPUT_RECORD test = {0};
    test.Event.KeyEvent.uChar.UnicodeChar = L'旅';
    Log::Comment(NoThrowString().Format(
        L"Processing \"%s\"", utf8Input.c_str()
    ));
    _pStateMachine->ProcessString(&utf8Input[0], utf8Input.length());



}
