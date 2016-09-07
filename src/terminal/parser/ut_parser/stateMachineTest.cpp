/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>
#include <wextestclass.h>

#include "stateMachine.hpp"

#include "ascii.hpp"

using namespace Microsoft::Console::VirtualTerminal;

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class StateMachineTest;
        }
    }
}

// From VT100.net...
// 9999-10000 is the classic boundary for most parsers parameter values.
// 16383-16384 is the boundary for DECSR commands according to EK-VT520-RM section 4.3.3.2 
// 32767-32768 is our boundary SHORT_MAX for the Windows console
#define PARAM_VALUES L"{0, 1, 2, 1000, 9999, 10000, 16383, 16384, 32767, 32768, 50000, 999999999}"

class Microsoft::Console::VirtualTerminal::StateMachineTest : TermDispatch
{
    TEST_CLASS(StateMachineTest);

    virtual void Execute(_In_ wchar_t const wchControl)
    {
        wchControl;
    }

    virtual void Print(_In_ wchar_t const wchPrintable)
    {
        wchPrintable;
    }

    virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
    {
        rgwch;
        cch;
    }

    TEST_METHOD(TestEscapePath)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiTest", L"{0,1,2,3,4,5,6,7,8}") // one value for each type of state test below.
        END_TEST_METHOD_PROPERTIES()

        unsigned int uiTest;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiTest", uiTest));

        StateMachine mach(this);

        switch (uiTest)
        {
        case 0:
        {
            Log::Comment(L"Escape from Ground.");
            mach._state = StateMachine::VTStates::Ground;
            
            break;
        }
        case 1:
        {
            Log::Comment(L"Escape from Escape.");
            mach._state = StateMachine::VTStates::Escape;
            break;
        }
        case 2:
        {
            Log::Comment(L"Escape from Escape Intermediate");
            mach._state = StateMachine::VTStates::EscapeIntermediate;
            break;
        }
        case 3:
        {
            Log::Comment(L"Escape from CsiEntry");
            mach._state = StateMachine::VTStates::CsiEntry;
            break;
        }
        case 4:
        {
            Log::Comment(L"Escape from CsiIgnore");
            mach._state = StateMachine::VTStates::CsiIgnore;
            break;
        }
        case 5:
        {
            Log::Comment(L"Escape from CsiParam");
            mach._state = StateMachine::VTStates::CsiParam;
            break;
        }
        case 6:
        {
            Log::Comment(L"Escape from CsiIntermediate");
            mach._state = StateMachine::VTStates::CsiIntermediate;
            break;
        }
        case 7:
        {
            Log::Comment(L"Escape from OscParam");
            mach._state = StateMachine::VTStates::OscParam;
            break;
        }
        case 8:
        {
            Log::Comment(L"Escape from OscString");
            mach._state = StateMachine::VTStates::OscString;
            break;
        }
        }

        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
    }

    TEST_METHOD(TestEscapeImmediatePath)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'#');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::EscapeIntermediate);
        mach.ProcessCharacter(L'(');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::EscapeIntermediate);
        mach.ProcessCharacter(L')');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::EscapeIntermediate);
        mach.ProcessCharacter(L'#');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::EscapeIntermediate);
        mach.ProcessCharacter(L'6');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestGroundPrint)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(L'a');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestCsiEntry)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L'm');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestCsiImmediate)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L'$');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIntermediate);
        mach.ProcessCharacter(L'#');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIntermediate);
        mach.ProcessCharacter(L'%');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIntermediate);
        mach.ProcessCharacter(L'v');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestCsiParam)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'3');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'2');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'4');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'8');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'J');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestCsiIgnore)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L':');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'3');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'q');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);

        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L'4');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L':');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'8');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'J');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);

        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'[');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiEntry);
        mach.ProcessCharacter(L'4');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiParam);
        mach.ProcessCharacter(L'#');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIntermediate);
        mach.ProcessCharacter(L':');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'8');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::CsiIgnore);
        mach.ProcessCharacter(L'J');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(TestOscStringSimple)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L'0');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L's');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'o');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'm');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'e');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L' ');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L't');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'e');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'x');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L't');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(AsciiChars::BEL);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);

        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L'0');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L's');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'o');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'm');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'e');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L' ');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L't');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'e');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L'x');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L't');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L'\\');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }
    TEST_METHOD(TestLongOscString)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L'0');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        mach.ProcessCharacter(L';');
        for (int i = 0; i < MAX_PATH; i++) // The buffer is only 256 long, so any longer value should work :P
        {
            mach.ProcessCharacter(L's');
            VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);  
        }
        VERIFY_ARE_EQUAL(mach._sOscNextChar, mach.s_cOscStringMaxLength - 1);
        mach.ProcessCharacter(AsciiChars::BEL);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }

    TEST_METHOD(NormalTestOscParam)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        for (int i = 0; i < 5; i++) // We're only expecting to be able to keep 5 digits max
        {
            mach.ProcessCharacter((wchar_t)(L'1' + i));
            VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        }
        VERIFY_ARE_EQUAL(mach._sOscParam, 12345);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L's');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);  
        mach.ProcessCharacter(AsciiChars::BEL);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }
    TEST_METHOD(TestLongOscParam)
    {
        StateMachine mach(this);

        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        for (int i = 0; i < 6; i++) // We're only expecting to be able to keep 5 digits max
        {
            mach.ProcessCharacter((wchar_t)(L'1' + i));
            VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        }
        VERIFY_ARE_EQUAL(mach._sOscParam, SHORT_MAX);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L's');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(AsciiChars::BEL);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);

        Log::Comment(L"Make sure we cap the param value to SHORT_MAX");
        mach.ProcessCharacter(AsciiChars::ESC);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Escape);
        mach.ProcessCharacter(L']');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        for (int i = 0; i < 5; i++) // We're only expecting to be able to keep 5 digits max
        {
            mach.ProcessCharacter((wchar_t)(L'4' + i)); // 45678 > (SHORT_MAX===32767)
            VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscParam);
        }
        VERIFY_ARE_EQUAL(mach._sOscParam, SHORT_MAX);
        mach.ProcessCharacter(L';');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(L's');
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::OscString);
        mach.ProcessCharacter(AsciiChars::BEL);
        VERIFY_ARE_EQUAL(mach._state, StateMachine::VTStates::Ground);
    }
};

class StateMachineExternalTest : TermDispatch
{
    TEST_CLASS(StateMachineExternalTest);
    virtual void Execute(_In_ wchar_t const wchControl)
    {
        wchControl;
    }

    virtual void Print(_In_ wchar_t const wchPrintable)
    {
        wchPrintable;
    }

    virtual void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
    {
        rgwch;
        cch;
    }

    unsigned int _uiCursorDistance;
    unsigned int _uiLine;
    unsigned int _uiColumn;
    bool _fCursorUp;
    bool _fCursorDown;
    bool _fCursorBackward;
    bool _fCursorForward;
    bool _fCursorNextLine;
    bool _fCursorPreviousLine;
    bool _fCursorHorizontalPositionAbsolute;
    bool _fVerticalLinePositionAbsolute;
    bool _fCursorPosition;
    bool _fCursorSave;
    bool _fCursorLoad;
    bool _fCursorVisible;
    bool _fEraseDisplay;
    bool _fEraseLine;
    bool _fInsertCharacter;
    bool _fDeleteCharacter;
    EraseType _eraseType;
    bool _fSetGraphics;
    AnsiStatusType _statusReportType;
    bool _fDeviceStatusReport;
    bool _fDeviceAttributes;
    bool _fIsAltBuffer;
    bool _fCursorKeysMode;
    bool _fCursorBlinking;
    unsigned int _uiWindowWidth;

    static const size_t s_cMaxOptions = 16;
    static const unsigned int s_uiGraphicsCleared = UINT_MAX;
    GraphicsOptions _rgOptions[s_cMaxOptions];
    size_t _cOptions;

    TEST_METHOD_SETUP(SetupState)
    {
        ClearState();
        return true;
    }

    void ClearState()
    {
        _uiCursorDistance = 0;
        _uiLine = 0;
        _uiColumn = 0;
        _fCursorUp = false;
        _fCursorDown = false;
        _fCursorBackward = false;
        _fCursorForward = false;
        _fCursorNextLine = false;
        _fCursorPreviousLine = false;
        _fCursorHorizontalPositionAbsolute = false;
        _fVerticalLinePositionAbsolute = false;
        _fCursorPosition = false;
        _fCursorSave = false;
        _fCursorLoad = false;
        _fCursorVisible = true;
        _fEraseDisplay = false;
        _fEraseLine = false;
        _fInsertCharacter = false;
        _fDeleteCharacter = false;
        _eraseType = (EraseType)-1;
        _fSetGraphics = false;
        _statusReportType = (AnsiStatusType)-1;
        _fDeviceStatusReport = false;
        _fDeviceAttributes = false;
        _cOptions = 0;
        _fIsAltBuffer = false;
        _fCursorKeysMode = false;
        _fCursorBlinking = true;
        _uiWindowWidth = 80;
        memset(_rgOptions, s_uiGraphicsCleared, sizeof(_rgOptions));
    }

    virtual bool CursorUp(_In_ unsigned int const uiDistance)
    {
        _fCursorUp = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorDown(_In_ unsigned int const uiDistance)
    {
        _fCursorDown = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorBackward(_In_ unsigned int const uiDistance)
    {
        _fCursorBackward = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorForward(_In_ unsigned int const uiDistance)
    {
        _fCursorForward = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorNextLine(_In_ unsigned int const uiDistance)
    {
        _fCursorNextLine = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorPrevLine(_In_ unsigned int const uiDistance)
    {
        _fCursorPreviousLine = true;
        _uiCursorDistance = uiDistance;
        return true;
    }

    virtual bool CursorHorizontalPositionAbsolute(_In_ unsigned int const uiPosition)
    {
        _fCursorHorizontalPositionAbsolute = true;
        _uiCursorDistance = uiPosition;
        return true;
    }

    virtual bool VerticalLinePositionAbsolute(_In_ unsigned int const uiPosition)
    {
        _fVerticalLinePositionAbsolute = true;
        _uiCursorDistance = uiPosition;
        return true;
    }

    virtual bool CursorPosition(_In_ unsigned int const uiLine, _In_ unsigned int const uiColumn)
    {
        _fCursorPosition = true;
        _uiLine = uiLine;
        _uiColumn = uiColumn;
        return true;
    }

    virtual bool CursorSavePosition()
    {
        _fCursorSave = true;
        return true;
    }

    virtual bool CursorRestorePosition()
    {
        _fCursorLoad = true;
        return true;
    }

    virtual bool EraseInDisplay(_In_ EraseType const eraseType)
    { 
        _fEraseDisplay = true;
        _eraseType = eraseType;
        return true;
    }

    virtual bool EraseInLine(_In_ EraseType const eraseType)
    { 
        _fEraseLine = true;
        _eraseType = eraseType;
        return true;
    } 

    virtual bool InsertCharacter(_In_ unsigned int const uiCount)
    {
        _fInsertCharacter = true;
        _uiCursorDistance = uiCount;
        return true;
    }

    virtual bool DeleteCharacter(_In_ unsigned int const uiCount)
    {
        _fDeleteCharacter = true;
        _uiCursorDistance = uiCount;
        return true;
    }

    virtual bool CursorVisibility(_In_ bool const fIsVisible)
    {
        _fCursorVisible = fIsVisible;
        return true;
    }

    virtual bool SetGraphicsRendition(_In_reads_(cOptions) const GraphicsOptions* const rgOptions, _In_ size_t const cOptions)
    {
        size_t cCopyLength = min(cOptions, s_cMaxOptions); // whichever is smaller, our buffer size or the number given
        _cOptions = cCopyLength;
        memcpy(_rgOptions, rgOptions, _cOptions * sizeof(GraphicsOptions));

        _fSetGraphics = true;

        return true;
    }

    virtual bool DeviceStatusReport(_In_ AnsiStatusType const statusType)
    {
        _fDeviceStatusReport = true;
        _statusReportType = statusType;

        return true;
    }

    virtual bool DeviceAttributes()
    {
        _fDeviceAttributes = true;

        return true;
    }

    bool _PrivateModeParamsHelper(_In_ PrivateModeParams const param, _In_ bool const fEnable)
    {
        bool fSuccess = false;
        switch(param)
        {
        case PrivateModeParams::DECCKM_CursorKeysMode:
            // set - Enable Application Mode, reset - Numeric/normal mode
            fSuccess = SetVirtualTerminalInputMode(fEnable);
            break;
        case PrivateModeParams::DECCOLM_SetNumberOfColumns:
            fSuccess = SetColumns((unsigned int)(fEnable? s_sDECCOLMSetColumns : s_sDECCOLMResetColumns));
            break;
        case PrivateModeParams::ATT610_StartCursorBlink:
            fSuccess = EnableCursorBlinking(fEnable);
            break;
        case PrivateModeParams::DECTCEM_TextCursorEnableMode:
            fSuccess = CursorVisibility(fEnable);
            break;
        case PrivateModeParams::ASB_AlternateScreenBuffer:
            fSuccess = fEnable? UseAlternateScreenBuffer() : UseMainScreenBuffer();
            break;
        default:
            // If no functions to call, overall dispatch was a failure.
            fSuccess = false;
            break;
        }
        return fSuccess;
    }

    bool _SetResetPrivateModesHelper(_In_reads_(cParams) const PrivateModeParams* const rParams, _In_ size_t const cParams, _In_ bool const fEnable)
    {
        size_t cFailures = 0;
        for (size_t i = 0; i < cParams; i++)
        {
            cFailures += _PrivateModeParamsHelper(rParams[i], fEnable)? 0 : 1; // increment the number of failures if we fail.
        }
        return cFailures == 0;
    }

    virtual bool SetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams, _In_ size_t const cParams)
    {
        return _SetResetPrivateModesHelper(rParams, cParams, true);
    }

    virtual bool ResetPrivateModes(_In_reads_(cParams) const PrivateModeParams* const rParams, _In_ size_t const cParams)
    {
        return _SetResetPrivateModesHelper(rParams, cParams, false);
    }

    virtual bool SetColumns(_In_ unsigned int const uiColumns)
    {
        _uiWindowWidth = uiColumns;
        return true;
    }
    virtual bool SetVirtualTerminalInputMode(_In_ bool const fApplicationMode)
    {
        _fCursorKeysMode = fApplicationMode;
        return true;
    }
    virtual bool EnableCursorBlinking(_In_ bool const bEnable)
    {
        _fCursorBlinking = bEnable;
        return true;
    }
    virtual bool UseAlternateScreenBuffer()
    {
        _fIsAltBuffer = true;
        return true;
    }
    virtual bool UseMainScreenBuffer()
    {
        _fIsAltBuffer = false;
        return true;
    }

    void TestEscCursorMovement(wchar_t const wchCommand, const bool* const pfFlag)
    {
        StateMachine mach(this);

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(wchCommand);

        VERIFY_IS_TRUE(*pfFlag);
        VERIFY_ARE_EQUAL(_uiCursorDistance, 1u);
    }

    TEST_METHOD(TestEscCursorMovement)
    {
        TestEscCursorMovement(L'A', &_fCursorUp);
        ClearState();
        TestEscCursorMovement(L'B', &_fCursorDown);
        ClearState();
        TestEscCursorMovement(L'C', &_fCursorForward);
        ClearState();
        TestEscCursorMovement(L'D', &_fCursorBackward);
    }

    void InsertNumberToMachine(StateMachine* const pMachine, unsigned int uiNumber)
    {
        static const size_t cchBufferMax = 20;

        wchar_t pwszDistance[cchBufferMax];
        int cchDistance = swprintf_s(pwszDistance, cchBufferMax, L"%d", uiNumber);

        if (cchDistance > 0 && cchDistance < cchBufferMax)
        {
            for (int i = 0; i < cchDistance; i++)
            {
                pMachine->ProcessCharacter(pwszDistance[i]);
            }
        }
    }

    void ApplyParameterBoundary(unsigned int* uiExpected, unsigned int uiGiven)
    {
        // 0 and 1 should be 1. Use the preset value.
        // 1-SHORT_MAX should be what we set.
        // > SHORT_MAX should be SHORT_MAX.
        if (uiGiven <= 1)
        {
            *uiExpected = 1u;
        }
        else if (uiGiven > 1 && uiGiven <= SHORT_MAX)
        {
            *uiExpected = uiGiven;
        }
        else if (uiGiven > SHORT_MAX)
        {
            *uiExpected = SHORT_MAX; // 16383 is our max value.
        }
    }

    void TestCsiCursorMovement(wchar_t const wchCommand, unsigned int const uiDistance, const bool fUseDistance, const bool* const pfFlag)
    {
        StateMachine mach(this);
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');

        if (fUseDistance)
        {
            InsertNumberToMachine(&mach, uiDistance);
        }
       
        mach.ProcessCharacter(wchCommand);

        VERIFY_IS_TRUE(*pfFlag);
        
        unsigned int uiExpectedDistance = 1u;

        if (fUseDistance)
        {
            ApplyParameterBoundary(&uiExpectedDistance, uiDistance);
        }

        VERIFY_ARE_EQUAL(_uiCursorDistance, uiExpectedDistance);
    }

    TEST_METHOD(TestCsiCursorMovementWithValues)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiDistance", PARAM_VALUES)
        END_TEST_METHOD_PROPERTIES()

        unsigned int uiDistance;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiDistance", uiDistance));
        
        TestCsiCursorMovement(L'A', uiDistance, true, &_fCursorUp);
        ClearState();
        TestCsiCursorMovement(L'B', uiDistance, true, &_fCursorDown);
        ClearState();
        TestCsiCursorMovement(L'C', uiDistance, true, &_fCursorForward);
        ClearState();
        TestCsiCursorMovement(L'D', uiDistance, true, &_fCursorBackward);
        ClearState();
        TestCsiCursorMovement(L'E', uiDistance, true, &_fCursorNextLine);
        ClearState();
        TestCsiCursorMovement(L'F', uiDistance, true, &_fCursorPreviousLine);
        ClearState();
        TestCsiCursorMovement(L'G', uiDistance, true, &_fCursorHorizontalPositionAbsolute);
        ClearState();
        TestCsiCursorMovement(L'd', uiDistance, true, &_fVerticalLinePositionAbsolute);
        ClearState();
        TestCsiCursorMovement(L'@', uiDistance, true, &_fInsertCharacter);
        ClearState();
        TestCsiCursorMovement(L'P', uiDistance, true, &_fDeleteCharacter);
    }

    TEST_METHOD(TestCsiCursorMovementWithoutValues)
    {
        unsigned int uiDistance = 9999; // this value should be ignored with the false below.
        TestCsiCursorMovement(L'A', uiDistance, false, &_fCursorUp);
        ClearState();
        TestCsiCursorMovement(L'B', uiDistance, false, &_fCursorDown);
        ClearState();
        TestCsiCursorMovement(L'C', uiDistance, false, &_fCursorForward);
        ClearState();
        TestCsiCursorMovement(L'D', uiDistance, false, &_fCursorBackward);
        ClearState();
        TestCsiCursorMovement(L'E', uiDistance, false, &_fCursorNextLine);
        ClearState();
        TestCsiCursorMovement(L'F', uiDistance, false, &_fCursorPreviousLine);
        ClearState();
        TestCsiCursorMovement(L'G', uiDistance, false, &_fCursorHorizontalPositionAbsolute);
        ClearState();
        TestCsiCursorMovement(L'd', uiDistance, false, &_fVerticalLinePositionAbsolute);
        ClearState();
        TestCsiCursorMovement(L'@', uiDistance, false, &_fInsertCharacter);
        ClearState();
        TestCsiCursorMovement(L'P', uiDistance, false, &_fDeleteCharacter);
    }

    TEST_METHOD(TestCsiCursorPosition)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiRow", PARAM_VALUES)
            TEST_METHOD_PROPERTY(L"Data:uiCol", PARAM_VALUES)
        END_TEST_METHOD_PROPERTIES()

        unsigned int uiRow;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiRow", uiRow));
        unsigned int uiCol;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiCol", uiCol));

        StateMachine mach(this);
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        
        InsertNumberToMachine(&mach, uiRow);
        mach.ProcessCharacter(L';');
        InsertNumberToMachine(&mach, uiCol);
        mach.ProcessCharacter(L'H');

        // bound the row/col values by the max we expect
        ApplyParameterBoundary(&uiRow, uiRow);
        ApplyParameterBoundary(&uiCol, uiCol);

        VERIFY_IS_TRUE(_fCursorPosition);
        VERIFY_ARE_EQUAL(_uiLine, uiRow);
        VERIFY_ARE_EQUAL(_uiColumn, uiCol);
    }

    TEST_METHOD(TestCsiCursorPositionWithOnlyRow)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiRow", PARAM_VALUES)
        END_TEST_METHOD_PROPERTIES()

        unsigned int uiRow;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiRow", uiRow));

        StateMachine mach(this);
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        
        InsertNumberToMachine(&mach, uiRow);
        mach.ProcessCharacter(L'H');

        // bound the row/col values by the max we expect
        ApplyParameterBoundary(&uiRow, uiRow);

        VERIFY_IS_TRUE(_fCursorPosition);
        VERIFY_ARE_EQUAL(_uiLine, uiRow);
        VERIFY_ARE_EQUAL(_uiColumn, (unsigned int)1); // Without the second param, the column should always be the default
    }

    TEST_METHOD(TestCursorSaveLoad)
    {
        StateMachine mach(this);
        
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'7');
        VERIFY_IS_TRUE(_fCursorSave);

        ClearState();

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'8');
        VERIFY_IS_TRUE(_fCursorLoad);

        ClearState();

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L's');
        VERIFY_IS_TRUE(_fCursorSave);

        ClearState();

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'u');
        VERIFY_IS_TRUE(_fCursorLoad);

        ClearState();
    }

    TEST_METHOD(TestCursorKeysMode)
    {
        StateMachine mach(this);
        
        mach.ProcessString(L"\x1b[?1h", 5);
        VERIFY_IS_TRUE(_fCursorKeysMode);

        ClearState();

        mach.ProcessString(L"\x1b[?1l", 5);
        VERIFY_IS_FALSE(_fCursorKeysMode);

        ClearState();
    }

    TEST_METHOD(TestSetNumberOfColumns)
    {
        StateMachine mach(this);
        
        mach.ProcessString(L"\x1b[?3h", 5);
        VERIFY_ARE_EQUAL(_uiWindowWidth, (unsigned int)s_sDECCOLMSetColumns);

        ClearState();

        mach.ProcessString(L"\x1b[?3l", 5);
        VERIFY_ARE_EQUAL(_uiWindowWidth, (unsigned int)s_sDECCOLMResetColumns);

        ClearState();
    }

    TEST_METHOD(TestCursorBlinking)
    {
        StateMachine mach(this);
        
        mach.ProcessString(L"\x1b[?12h", 6);
        VERIFY_IS_TRUE(_fCursorBlinking);

        ClearState();

        mach.ProcessString(L"\x1b[?12l", 6);
        VERIFY_IS_FALSE(_fCursorBlinking);

        ClearState();
    }

    TEST_METHOD(TestCursorVisibility)
    {
        StateMachine mach(this);
        
        mach.ProcessString(L"\x1b[?25h", 6);
        VERIFY_IS_TRUE(_fCursorVisible);

        ClearState();

        mach.ProcessString(L"\x1b[?25l", 6);
        VERIFY_IS_FALSE(_fCursorVisible);

        ClearState();
    }

    TEST_METHOD(TestAltBufferSwapping)
    {
        StateMachine mach(this);
        
        mach.ProcessString(L"\x1b[?1049h", 8);
        VERIFY_IS_TRUE(_fIsAltBuffer);

        ClearState();

        mach.ProcessString(L"\x1b[?1049h", 8);
        VERIFY_IS_TRUE(_fIsAltBuffer);
        mach.ProcessString(L"\x1b[?1049h", 8);
        VERIFY_IS_TRUE(_fIsAltBuffer);

        ClearState();

        mach.ProcessString(L"\x1b[?1049l", 8);
        VERIFY_IS_FALSE(_fIsAltBuffer);

        ClearState();

        mach.ProcessString(L"\x1b[?1049h", 8);
        VERIFY_IS_TRUE(_fIsAltBuffer);
        mach.ProcessString(L"\x1b[?1049l", 8);
        VERIFY_IS_FALSE(_fIsAltBuffer);

        ClearState();

        mach.ProcessString(L"\x1b[?1049l", 8);
        VERIFY_IS_FALSE(_fIsAltBuffer);
        mach.ProcessString(L"\x1b[?1049l", 8);
        VERIFY_IS_FALSE(_fIsAltBuffer);

        ClearState();
    }

    TEST_METHOD(TestErase)
    {
        BEGIN_TEST_METHOD_PROPERTIES()
            TEST_METHOD_PROPERTY(L"Data:uiEraseOperation", L"{0, 1}") // for "display" and "line" type erase operations
            TEST_METHOD_PROPERTY(L"Data:uiEraseType", L"{0, 1, 2, 10}") // maps to TermDispatch::EraseType enum class options.
        END_TEST_METHOD_PROPERTIES()

        unsigned int uiEraseOperation;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiEraseOperation", uiEraseOperation));
        unsigned int uiEraseType;
        VERIFY_SUCCEEDED_RETURN(TestData::TryGetValue(L"uiEraseType", uiEraseType));

        WCHAR wchOp = L'\0';
        bool* pfOperationCallback = nullptr;

        switch (uiEraseOperation)
        {
        case 0:
            wchOp = L'J';
            pfOperationCallback = &_fEraseDisplay;
            break;
        case 1:
            wchOp = L'K';
            pfOperationCallback = &_fEraseLine;
            break;
        default:
            VERIFY_FAIL(L"Unknown erase operation permutation.");
        }

        VERIFY_IS_NOT_NULL(wchOp);
        VERIFY_IS_NOT_NULL(pfOperationCallback);

        StateMachine mach(this);
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');

        EraseType expectedEraseType;

        switch (uiEraseType)
        {
        case 0:
            expectedEraseType = EraseType::ToEnd;
            InsertNumberToMachine(&mach, uiEraseType);
            break;
        case 1:
            expectedEraseType = EraseType::FromBeginning;
            InsertNumberToMachine(&mach, uiEraseType);
            break;
        case 2:
            expectedEraseType = EraseType::All;
            InsertNumberToMachine(&mach, uiEraseType);
            break;
        case 10:
            // Do nothing. Default case of 10 should be like a 0 to the end.
            expectedEraseType = EraseType::ToEnd;
            break;
        }

        mach.ProcessCharacter(wchOp);


        VERIFY_IS_TRUE(*pfOperationCallback);
        VERIFY_ARE_EQUAL(expectedEraseType, _eraseType);        
    }

    void VerifyGraphicsOptions(_In_reads_(cExpectedOptions) const GraphicsOptions* const rgExpectedOptions, _In_ size_t const cExpectedOptions)
    {
        VERIFY_ARE_EQUAL(cExpectedOptions, _cOptions);
        bool fOptionsValid = true;

        for (size_t i = 0; i < s_cMaxOptions; i++)
        {
            GraphicsOptions expectedOption = (GraphicsOptions)s_uiGraphicsCleared;

            if (i < cExpectedOptions)
            {
                expectedOption = rgExpectedOptions[i];
            }

            fOptionsValid = expectedOption == _rgOptions[i];

            if (!fOptionsValid)
            {
                Log::Comment(NoThrowString().Format(L"Graphics option match failed. Expected: '%d' Actual: '%d'", expectedOption, _rgOptions[i]));
                break;
            }
        }

        VERIFY_IS_TRUE(fOptionsValid);
    }

    TEST_METHOD(TestSetGraphicsRendition)
    {
        StateMachine mach(this);

        GraphicsOptions rgExpected[16];

        Log::Comment(L"Test 1: Check default case.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'm');
        VERIFY_IS_TRUE(_fSetGraphics);

        rgExpected[0] = GraphicsOptions::Off;
        VerifyGraphicsOptions(rgExpected, 1);

        ClearState();

        Log::Comment(L"Test 2: Check clear/0 case.");

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'0');
        mach.ProcessCharacter(L'm');
        VERIFY_IS_TRUE(_fSetGraphics);

        rgExpected[0] = GraphicsOptions::Off;
        VerifyGraphicsOptions(rgExpected, 1);

        ClearState();

        Log::Comment(L"Test 3: Check 'handful of options' case.");

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'7');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'3');
        mach.ProcessCharacter(L'0');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L'5');
        mach.ProcessCharacter(L'm');
        VERIFY_IS_TRUE(_fSetGraphics);

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::Underline;
        rgExpected[2] = GraphicsOptions::Negative;
        rgExpected[3] = GraphicsOptions::ForegroundBlack;
        rgExpected[4] = GraphicsOptions::BackgroundMagenta;
        VerifyGraphicsOptions(rgExpected, 5);

        ClearState();

        Log::Comment(L"Test 3: Check 'too many options' (>16) case.");

        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'4');
        mach.ProcessCharacter(L';');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L'm');
        VERIFY_IS_TRUE(_fSetGraphics);

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::Underline;
        rgExpected[2] = GraphicsOptions::BoldBright;
        rgExpected[3] = GraphicsOptions::Underline;
        rgExpected[4] = GraphicsOptions::BoldBright;
        rgExpected[5] = GraphicsOptions::Underline;
        rgExpected[6] = GraphicsOptions::BoldBright;
        rgExpected[7] = GraphicsOptions::Underline;
        rgExpected[8] = GraphicsOptions::BoldBright;
        rgExpected[9] = GraphicsOptions::Underline;
        rgExpected[10] = GraphicsOptions::BoldBright;
        rgExpected[11] = GraphicsOptions::Underline;
        rgExpected[12] = GraphicsOptions::BoldBright;
        rgExpected[13] = GraphicsOptions::Underline;
        rgExpected[14] = GraphicsOptions::BoldBright;
        rgExpected[15] = GraphicsOptions::Underline;
        VerifyGraphicsOptions(rgExpected, 16);

        ClearState();

    }

    TEST_METHOD(TestDeviceStatusReport)
    {
        StateMachine mach(this);

        Log::Comment(L"Test 1: Check empty case. Should fail.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'n');

        VERIFY_IS_FALSE(_fDeviceStatusReport);

        ClearState();

        Log::Comment(L"Test 2: Check CSR (cursor position command) case 6. Should succeed.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'6');
        mach.ProcessCharacter(L'n');

        VERIFY_IS_TRUE(_fDeviceStatusReport);
        VERIFY_ARE_EQUAL(TermDispatch::AnsiStatusType::CPR_CursorPositionReport, _statusReportType);

        ClearState();

        Log::Comment(L"Test 3: Check unimplemented case 1. Should fail.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L'n');

        VERIFY_IS_FALSE(_fDeviceStatusReport);

        ClearState();
    }

    TEST_METHOD(TestDeviceAttributes)
    {
        StateMachine mach(this);

        Log::Comment(L"Test 1: Check default case, no params.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'c');

        VERIFY_IS_TRUE(_fDeviceAttributes);

        ClearState();

        Log::Comment(L"Test 2: Check default case, 0 param.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'0');
        mach.ProcessCharacter(L'c');

        VERIFY_IS_TRUE(_fDeviceAttributes);

        ClearState();

        Log::Comment(L"Test 3: Check fail case, 1 (or any other) param.");
        mach.ProcessCharacter(AsciiChars::ESC);
        mach.ProcessCharacter(L'[');
        mach.ProcessCharacter(L'1');
        mach.ProcessCharacter(L'c');

        VERIFY_IS_FALSE(_fDeviceAttributes);

        ClearState();
    }

    TEST_METHOD(TestStrings)
    {
        StateMachine mach(this);

        GraphicsOptions rgExpected[16];
        EraseType expectedEraseType;
        ///////////////////////////////////////////////////////////////////////

        Log::Comment(L"Test 1: Basic String processing. One sequence in a string.");
        mach.ProcessString(L"\x1b[0m", 4);

        VERIFY_IS_TRUE(_fSetGraphics);

        ClearState();
        
        ///////////////////////////////////////////////////////////////////////

        Log::Comment(L"Test 2: A couple of sequences all in one string");

        mach.ProcessString(L"\x1b[1;4;7;30;45m\x1b[2J", 18);
        VERIFY_IS_TRUE(_fSetGraphics);
        VERIFY_IS_TRUE(_fEraseDisplay);

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::Underline;
        rgExpected[2] = GraphicsOptions::Negative;
        rgExpected[3] = GraphicsOptions::ForegroundBlack;
        rgExpected[4] = GraphicsOptions::BackgroundMagenta;
        expectedEraseType = EraseType::All;
        VerifyGraphicsOptions(rgExpected, 5);
        VERIFY_ARE_EQUAL(expectedEraseType, _eraseType);  

        ClearState();

        ///////////////////////////////////////////////////////////////////////
        Log::Comment(L"Test 3: Two sequences seperated by a non-sequence of characters");

        mach.ProcessString(L"\x1b[1;30mHello World\x1b[2J", 22);

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::ForegroundBlack;
        expectedEraseType = EraseType::All;
        
        VERIFY_IS_TRUE(_fSetGraphics);
        VERIFY_IS_TRUE(_fEraseDisplay);

        VerifyGraphicsOptions(rgExpected, 2);
        VERIFY_ARE_EQUAL(expectedEraseType, _eraseType);   

        ClearState();
        
        ///////////////////////////////////////////////////////////////////////
        Log::Comment(L"Test 4: An entire sequence broke into multiple strings");

        mach.ProcessString(L"\x1b[1;", 4);
        VERIFY_IS_FALSE(_fSetGraphics);
        VERIFY_IS_FALSE(_fEraseDisplay);

        mach.ProcessString(L"30mHello World\x1b[2J", 18);

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::ForegroundBlack;
        expectedEraseType = EraseType::All;

        VERIFY_IS_TRUE(_fSetGraphics);
        VERIFY_IS_TRUE(_fEraseDisplay);
        
        VerifyGraphicsOptions(rgExpected, 2);
        VERIFY_ARE_EQUAL(expectedEraseType, _eraseType);   

        ClearState();
        
        ///////////////////////////////////////////////////////////////////////
        Log::Comment(L"Test 5: A sequence with mixed ProcessCharacter and ProcessString calls");

        rgExpected[0] = GraphicsOptions::BoldBright;
        rgExpected[1] = GraphicsOptions::ForegroundBlack;

        mach.ProcessString(L"\x1b[1;", 4);
        VERIFY_IS_FALSE(_fSetGraphics);
        VERIFY_IS_FALSE(_fEraseDisplay);

        mach.ProcessCharacter(L'3');
        VERIFY_IS_FALSE(_fSetGraphics);
        VERIFY_IS_FALSE(_fEraseDisplay);

        mach.ProcessCharacter(L'0');
        VERIFY_IS_FALSE(_fSetGraphics);
        VERIFY_IS_FALSE(_fEraseDisplay);

        mach.ProcessCharacter(L'm');


        VERIFY_IS_TRUE(_fSetGraphics);
        VERIFY_IS_FALSE(_fEraseDisplay);
        VerifyGraphicsOptions(rgExpected, 2);

        mach.ProcessString(L"Hello World\x1b[2J", 15);

        expectedEraseType = EraseType::All;

        VERIFY_IS_TRUE(_fEraseDisplay);
        
        VERIFY_ARE_EQUAL(expectedEraseType, _eraseType);   

        ClearState();
        
    }
};
