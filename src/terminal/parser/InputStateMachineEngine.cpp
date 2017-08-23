/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "stateMachine.hpp"
#include "InputStateMachineEngine.hpp"

#include "ascii.hpp"
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
    if (wch == '\x7f') _WriteSingleKey('\x8', VK_BACK, 0);
    else ActionPrint(wch);
}

void InputStateMachineEngine::_WriteSingleKey(wchar_t wch, short vkey, DWORD dwModifierState)
{
    INPUT_RECORD rgInput[2];
    rgInput[0].EventType = KEY_EVENT;
    rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
    rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
    rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
    rgInput[0].Event.KeyEvent.uChar.UnicodeChar = wch;

    rgInput[1] = rgInput[0];
    rgInput[1].EventType = KEY_EVENT;
    rgInput[1].Event.KeyEvent.bKeyDown = FALSE;

    _pfnWriteEvents(rgInput, 2);

}

void InputStateMachineEngine::_WriteSingleKey(wchar_t wch, DWORD dwModifierState)
{
    short vkey = VkKeyScan(wch);
    _WriteSingleKey(wch, vkey, dwModifierState);
}

void InputStateMachineEngine::_WriteSingleKey(short vkey, DWORD dwModifierState)
{
    wchar_t wch = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);
    _WriteSingleKey(wch, vkey, dwModifierState);
}

void InputStateMachineEngine::ActionPrint(_In_ wchar_t const wch)
{
    wch;
    short vkey = VkKeyScan(wch);
    
    // Something's fishy... Filter these out for now
    if (vkey == 0x332) return;
    
    if (wch == '\x7f') 
    {
        _WriteSingleKey('\x8', VK_BACK, 0);
        return;
    }
    _WriteSingleKey(wch, 0);
}

void InputStateMachineEngine::ActionPrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    rgwch;
    cch;
    for(int i = 0; i < cch; i++)
    {
        ActionPrint(rgwch[i]);
    }
}

void InputStateMachineEngine::ActionEscDispatch(_In_ wchar_t const wch, _In_ const unsigned short cIntermediate, _In_ const wchar_t wchIntermediate)
{

    wch;
    cIntermediate;
    wchIntermediate;
}

void InputStateMachineEngine::ActionCsiDispatch(_In_ wchar_t const wch, 
                       _In_ const unsigned short cIntermediate,
                       _In_ const wchar_t wchIntermediate,
                       _In_ const unsigned short* const rgusParams,
                       _In_ const unsigned short cParams)
{
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

    // INPUT_RECORD rgInput[2];
    // rgInput[0].EventType = KEY_EVENT;
    // rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    // rgInput[0].Event.KeyEvent.dwControlKeyState = dwModifierState;
    // rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    // rgInput[0].Event.KeyEvent.wVirtualKeyCode = vkey;
    // rgInput[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(vkey, MAPVK_VK_TO_VSC);
    // rgInput[0].Event.KeyEvent.uChar.UnicodeChar = (wchar_t)MapVirtualKey(vkey, MAPVK_VK_TO_CHAR);

    // rgInput[1] = rgInput[0];
    // rgInput[1].EventType = KEY_EVENT;
    // rgInput[1].Event.KeyEvent.bKeyDown = FALSE;
    
    // _pfnWriteEvents(rgInput, 2);

    wch;
    cIntermediate;
    wchIntermediate;
    rgusParams;
    cParams;
}

void InputStateMachineEngine::ActionClear()
{

}

void InputStateMachineEngine::ActionIgnore()
{

}

void InputStateMachineEngine::ActionOscDispatch(_In_ wchar_t const wch, _In_ const unsigned short sOscParam, _In_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString)
{

    wch;
    sOscParam;
    pwchOscStringBuffer;
    cchOscString;
}


DWORD InputStateMachineEngine::_GetCursorKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    DWORD dwModifiers = 0;
    if (_IsModified(cParams))
    {
        dwModifiers = _GetModifier(rgusParams[0]);
    }
    return dwModifiers;
}

DWORD InputStateMachineEngine::_GetGenericKeysModifierState(_In_ const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    DWORD dwModifiers = 0;
    if (_IsModified(cParams))
    {
        dwModifiers = _GetModifier(rgusParams[1]);
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
    unsigned short vtParam = modifierParam;
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

