/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "stateMachine.hpp"
#include "InputStateMachineEngine.hpp"

#include "ascii.hpp"
#include <assert.h>
using namespace Microsoft::Console::VirtualTerminal;

const InputStateMachineEngine::CSI_TO_VKEY InputStateMachineEngine::s_rgCsiMap[]
{
    { CsiActionCodes::ArrowUp, VK_UP },
    { CsiActionCodes::ArrowDown, VK_DOWN },
    { CsiActionCodes::ArrowRight, VK_RIGHT },
    { CsiActionCodes::ArrowLeft, VK_LEFT },
    { CsiActionCodes::Home, VK_HOME },
    { CsiActionCodes::End, VK_END },
    { CsiActionCodes::F1, VK_F1 },
    { CsiActionCodes::F2, VK_F2 },
    { CsiActionCodes::F3, VK_F3 },
    { CsiActionCodes::F4, VK_F4 },
};

const InputStateMachineEngine::GENERIC_TO_VKEY InputStateMachineEngine::s_rgGenericMap[]
{
    { GenericKeyIdentifiers::Insert, VK_INSERT },
    { GenericKeyIdentifiers::Delete, VK_DELETE },
    { GenericKeyIdentifiers::Prior, VK_PRIOR },
    { GenericKeyIdentifiers::Next, VK_NEXT },
    { GenericKeyIdentifiers::F5, VK_F5 },
    { GenericKeyIdentifiers::F6, VK_F6 },
    { GenericKeyIdentifiers::F7, VK_F7 },
    { GenericKeyIdentifiers::F8, VK_F8 },
    { GenericKeyIdentifiers::F9, VK_F9 },
    { GenericKeyIdentifiers::F10, VK_F10 },
    { GenericKeyIdentifiers::F11, VK_F11 },
    { GenericKeyIdentifiers::F12, VK_F12 },
};


InputStateMachineEngine::InputStateMachineEngine(_In_ WriteInputEvents const pfnWriteEvents)
    : _pfnWriteEvents(pfnWriteEvents)
{
    _trace = Microsoft::Console::VirtualTerminal::ParserTracing();
}

InputStateMachineEngine::~InputStateMachineEngine()
{

}

void InputStateMachineEngine::ActionExecute(_In_ wchar_t const wch)
{
    if (wch >= '\x0' && wch < '\x20')
    {
        // This is a C0 Control Character.
        // This should be translated as Ctrl+(wch+x40)
        wchar_t actualChar = wch;
        actualChar = wch+0x40;
        bool writeCtrl = true;

        ////////////////////////////////////////////////////////////////////////
        // This is a hack to help debug why powershell on windows doesn't work.
        // This segment will fix it, but break emacs on wsl again.
        bool tryPowershellFix = false;
        if (tryPowershellFix)
        {
            actualChar = wch;
            switch(wch)
            {
                case '\0': //0x0 NUL
                case '\x03': //0x03 ETX
                case '\b': // x08 BS
                case '\t': // x09 HT
                case '\n': // x0a LF
                case '\r': // x0d CR
                case '\x1b': // x1b ESC
                    writeCtrl = false;
                    break;
                default:
                    actualChar = wch+0x40;
                    break;
            }
        }
        ////////////////////////////////////////////////////////////////////////

        short vkey = 0;
        DWORD dwModifierState = 0;
        _GenerateKeyFromChar(actualChar, &vkey, &dwModifierState);
        
        if (writeCtrl)
        {
            dwModifierState = dwModifierState | LEFT_CTRL_PRESSED;
            // _WriteSingleKey(actualChar, vkey, dwModifierState);
            // _WriteControlAndKey(actualChar, vkey, dwModifierState);
            _WriteControlAndKey(wch, vkey, dwModifierState);
        }
        else
        {
            _WriteSingleKey(actualChar, vkey, dwModifierState);
        }
    }
    else if (wch == '\x7f')
    {
        _WriteSingleKey('\x8', VK_BACK, 0);
    } 
    else ActionPrint(wch);
}

void InputStateMachineEngine::_WriteControlAndKey(wchar_t wch, short vkey, DWORD dwModifierState)
{
    INPUT_RECORD rgInput[4];
    rgInput[0].EventType = KEY_EVENT;
    rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
    rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    rgInput[0].Event.KeyEvent.wVirtualKeyCode = VK_CONTROL;
    rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC);
    rgInput[0].Event.KeyEvent.uChar.UnicodeChar = 0x0;

    _GetSingleKeypress(wch, vkey, dwModifierState, &rgInput[1], 2);

    rgInput[3] = rgInput[0];
    rgInput[3].Event.KeyEvent.bKeyDown = FALSE;
    rgInput[3].Event.KeyEvent.dwControlKeyState = dwModifierState ^ LEFT_CTRL_PRESSED;


    // _pfnWriteEvents(&rgInput[0], 1);
    // _WriteSingleKey(wch, vkey, dwModifierState);
    // _pfnWriteEvents(&rgInput[1], 1);
    _pfnWriteEvents(rgInput, 4);
}

void InputStateMachineEngine::_WriteSingleKey(wchar_t wch, short vkey, DWORD dwModifierState)
{
    INPUT_RECORD rgInput[2];
    _GetSingleKeypress(wch, vkey, dwModifierState, rgInput, 2);

    // rgInput[0].EventType = KEY_EVENT;
    // rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    // rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
    // rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    // rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
    // rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
    // rgInput[0].Event.KeyEvent.uChar.UnicodeChar = wch;

    // rgInput[1] = rgInput[0];
    // rgInput[1].Event.KeyEvent.bKeyDown = FALSE;

    _pfnWriteEvents(rgInput, 2);
}

void InputStateMachineEngine::_GetSingleKeypress(wchar_t wch, short vkey, DWORD dwModifierState, _Inout_ INPUT_RECORD* const rgInput, _In_ size_t cRecords)
{
    assert(cRecords >= 2);
    rgInput[0].EventType = KEY_EVENT;
    rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
    rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
    rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
    rgInput[0].Event.KeyEvent.uChar.UnicodeChar = wch;

    rgInput[1] = rgInput[0];
    rgInput[1].Event.KeyEvent.bKeyDown = FALSE;

}

void InputStateMachineEngine::_WriteSingleKey(short vkey, DWORD dwModifierState)
{
    wchar_t wch = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
    _WriteSingleKey(wch, vkey, dwModifierState);
}

void InputStateMachineEngine::ActionPrint(_In_ wchar_t const wch)
{
    short vkey = 0;
    DWORD dwModifierState = 0;
    _GenerateKeyFromChar(wch, &vkey, &dwModifierState);

    _WriteSingleKey(wch, vkey, dwModifierState);
}

void InputStateMachineEngine::ActionPrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    for(size_t i = 0; i < cch; i++)
    {
        ActionPrint(rgwch[i]);
    }
}

void InputStateMachineEngine::ActionEscDispatch(_In_ wchar_t const wch, _In_ const unsigned short cIntermediate, _In_ const wchar_t wchIntermediate)
{
    UNREFERENCED_PARAMETER(cIntermediate);
    UNREFERENCED_PARAMETER(wchIntermediate);

    DWORD dwModifierState = 0;
    _GenerateKeyFromChar(wch, nullptr, &dwModifierState);

    // Alt is definitely pressed in the esc+key case.
    dwModifierState = dwModifierState | LEFT_ALT_PRESSED;
    
    _WriteSingleKey(wch, dwModifierState);

}

void InputStateMachineEngine::ActionCsiDispatch(_In_ wchar_t const wch, 
                       _In_ const unsigned short cIntermediate,
                       _In_ const wchar_t wchIntermediate,
                       _In_ const unsigned short* const rgusParams,
                       _In_ const unsigned short cParams)
{
    UNREFERENCED_PARAMETER(cIntermediate);
    UNREFERENCED_PARAMETER(wchIntermediate);
    _trace.TraceOnAction(L"Input.CsiDispatch");

    DWORD dwModifierState = 0;
    short vkey = 0;

    if (wch == CsiActionCodes::Generic)
    {
        dwModifierState = _GetGenericKeysModifierState(rgusParams, cParams);
        vkey = _GetGenericVkey(rgusParams, cParams);
    }
    else
    {
        dwModifierState = _GetCursorKeysModifierState(rgusParams, cParams);
        vkey = _GetCursorKeysVkey(wch);
    }

    _WriteSingleKey(vkey, dwModifierState);

}

void InputStateMachineEngine::ActionClear()
{

}

void InputStateMachineEngine::ActionIgnore()
{

}

void InputStateMachineEngine::ActionOscDispatch(_In_ wchar_t const wch, _In_ const unsigned short sOscParam, _In_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString)
{
    UNREFERENCED_PARAMETER(wch);
    UNREFERENCED_PARAMETER(sOscParam);
    UNREFERENCED_PARAMETER(pwchOscStringBuffer);
    UNREFERENCED_PARAMETER(cchOscString);
}


DWORD InputStateMachineEngine::_GetCursorKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    DWORD dwModifiers = 0;
    if (_IsModified(cParams))
    {
        dwModifiers = _GetModifier(rgusParams[1]);
    }
    return dwModifiers;
}

DWORD InputStateMachineEngine::_GetGenericKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    DWORD dwModifiers = 0;
    if (_IsModified(cParams))
    {
        dwModifiers = _GetModifier(rgusParams[0]);
    }
    return dwModifiers;
}

bool InputStateMachineEngine::_IsModified(_In_ const unsigned short cParams)
{
    return cParams == 2;
}

DWORD InputStateMachineEngine::_GetModifier(_In_ const unsigned short modifierParam)
{
    // Something something off by one
    unsigned short vtParam = modifierParam-1;
    DWORD modifierState = modifierParam > 0 ? ENHANCED_KEY : 0;

    bool fShift = IsFlagSet(vtParam, 0x1);
    bool fAlt = IsFlagSet(vtParam, 0x2);
    bool fCtrl = IsFlagSet(vtParam, 0x4);
    return modifierState | (fShift? SHIFT_PRESSED : 0) | (fAlt? LEFT_ALT_PRESSED : 0) | (fCtrl? LEFT_CTRL_PRESSED : 0);
}


short InputStateMachineEngine::_GetGenericVkey(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    cParams;
    unsigned short identifier = rgusParams[0];
    for(int i = 0; i < ARRAYSIZE(s_rgGenericMap); i++)
    {
        GENERIC_TO_VKEY mapping = s_rgGenericMap[i];
        if (mapping.Identifier == identifier)
        {
            return mapping.vkey;
        }
    }
    return 0;
}

short InputStateMachineEngine::_GetCursorKeysVkey(_In_ const wchar_t wch)
{
    for(int i = 0; i < ARRAYSIZE(s_rgCsiMap); i++)
    {
        CSI_TO_VKEY mapping = s_rgCsiMap[i];
        if (mapping.Action == wch)
        {
            return mapping.vkey;
        }
    }

    return 0;
}

void InputStateMachineEngine::_GenerateKeyFromChar(_In_ const wchar_t wch, _Out_ short* const pVkey, _Out_ DWORD* const pdwModifierState)
{
    // Low order byte is key, high order is modifiers
    short keyscan = VkKeyScan(wch);
    
    short vkey =  keyscan & 0xff;

    short keyscanModifiers = (keyscan >> 8) & 0xff;
    // Because of course, these are not the same flags.
    short dwModifierState = 0 |
        IsFlagSet(keyscanModifiers, 1) ? SHIFT_PRESSED : 0 |
        IsFlagSet(keyscanModifiers, 2) ? LEFT_CTRL_PRESSED : 0 |
        IsFlagSet(keyscanModifiers, 4) ? LEFT_ALT_PRESSED : 0 ;

    if (pVkey != nullptr) 
    {
        *pVkey = vkey;
    }
    if (pdwModifierState != nullptr)
    {
        *pdwModifierState = dwModifierState;
    } 
}