/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"
#include "../host/cmdline.h"
#include "../interactivity/inc/ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity;

bool operator==(const KeyEvent& a, const KeyEvent& b)
{
    return (a._keyDown == b._keyDown &&
            a._repeatCount == b._repeatCount &&
            a._virtualKeyCode == b._virtualKeyCode &&
            a._virtualScanCode == b._virtualScanCode &&
            a._charData == b._charData &&
            a._activeModifierKeys == b._activeModifierKeys);
}

KeyEvent::KeyEvent(_In_ const KEY_EVENT_RECORD& record) :
    _keyDown{ !!record.bKeyDown },
    _repeatCount{ record.wRepeatCount },
    _virtualKeyCode{ record.wVirtualKeyCode },
    _virtualScanCode{ record.wVirtualScanCode },
    _charData{ record.uChar.UnicodeChar },
    _activeModifierKeys{ record.dwControlKeyState }
{
}

KeyEvent::KeyEvent() :
    _keyDown{ 0 },
    _repeatCount{ 0 },
    _virtualKeyCode{ 0 },
    _virtualScanCode{ 0 },
    _charData { 0 },
    _activeModifierKeys{ 0 }
{
}

KeyEvent::KeyEvent(_In_ const int keyDown,
                   _In_ const WORD repeatCount,
                   _In_ const WORD virtualKeyCode,
                   _In_ const WORD virtualScanCode,
                   _In_ const wchar_t charData,
                   _In_ const DWORD activeModifierKeys) :
    _keyDown{ keyDown },
    _repeatCount{ repeatCount },
    _virtualKeyCode{ virtualKeyCode },
    _virtualScanCode{ virtualScanCode },
    _charData{ charData },
    _activeModifierKeys{ activeModifierKeys }
{
}

KeyEvent::~KeyEvent()
{
}

INPUT_RECORD KeyEvent::ToInputRecord() const
{
    INPUT_RECORD record{ 0 };
    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = _keyDown;
    record.Event.KeyEvent.wRepeatCount = _repeatCount;
    record.Event.KeyEvent.wVirtualKeyCode = _virtualKeyCode;
    record.Event.KeyEvent.wVirtualScanCode = _virtualScanCode;
    record.Event.KeyEvent.uChar.UnicodeChar = _charData;
    record.Event.KeyEvent.dwControlKeyState = _activeModifierKeys;
    return record;
}

InputEventType KeyEvent::EventType() const
{
    return InputEventType::KeyEvent;
}

// Routine Description:
// - checks if the key event's data is a pause key press.
// Arguments:
// - None
// Return Value:
// - true if the key press data is a pause.
// Note:
// - The default key is Ctrl-S if extended edit keys are not specified.
bool KeyEvent::IsPauseKey() const
{
    bool isPauseKey = false;
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey())
    {
        const ExtKeySubst* const pKeySubst = GetKeySubst();
        isPauseKey = (pKeySubst != nullptr && pKeySubst->wVirKey == VK_PAUSE);
    }
    else
    {
        isPauseKey = (_virtualKeyCode == L'S' && CTRL_BUT_NOT_ALT(_activeModifierKeys));
    }
    return isPauseKey;
}

// Routine Description:
// - checks if this key event is a special key for line editing
// Arguments:
// - None
// Return Value:
// - true if this key has special relevance to line editing, false otherwise
bool KeyEvent::IsCommandLineEditingKey() const
{
    if (!IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED | CTRL_PRESSED))

    {
        switch (_virtualKeyCode)
        {
        case VK_ESCAPE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_INSERT:
        case VK_DELETE:
        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
            return true;
        default:
            break;
        }
    }
    if (IsAnyFlagSet(_activeModifierKeys, CTRL_PRESSED))
    {
        switch (_virtualKeyCode)
        {
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_RIGHT:
            return true;
        default:
            break;
        }
    }

    // Extended edit key handling
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() && GetKeySubst())
    {
        // If wUnicodeChar is specified in KeySubst,
        // the key should be handled as a normal key.
        // Basically this is for VK_BACK keys.
        return (_charData == 0);
    }

    if (IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED))
    {
        switch (_virtualKeyCode)
        {
        case VK_F7:
        case VK_F10:
            return true;
        default:
            break;
        }
    }
    return false;
}

// Routine Description:
// - checks if this key event is a special key for popups
// Arguments:
// - None
// Return Value:
// - true if this key has special relevance to popups, false otherwise
bool KeyEvent::IsCommandLinePopupKey() const
{
    if (!IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED | CTRL_PRESSED))
    {
        switch (_virtualKeyCode)
        {
        case VK_ESCAPE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_F2:
        case VK_F4:
        case VK_F7:
        case VK_F9:
            return true;
        default:
            break;
        }
    }

    // Extended key handling
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() && GetKeySubst())
    {
        return (_charData == 0);
    }

    return false;
}

// Routine Description:
// - updates key event information based on the extended key
// substitution table.
// Arguments:
// - None
// Return Value:
// - The extended key substitution object used to update the KeyEvent
// Note:
// - Modifies key event data.
const ExtKeySubst* const KeyEvent::ParseEditKeyInfo()
{
    const ExtKeySubst* const pKeySubst = GetKeySubst();

    if (pKeySubst)
    {
        // Substitute the input with ext key.
        _activeModifierKeys = pKeySubst->wMod;
        _virtualKeyCode = pKeySubst->wVirKey;
        _charData = pKeySubst->wUnicodeChar;
    }

    return pKeySubst;
}

// Routine Description:
// - Gets the extended key substitution object associated with the
// data in this KeyEvent, if applicable.
// Arguments:
// - None
// Return Value:
// - The associated extended key substitution object or nullptr if one
// doesn't exist.
const ExtKeySubst* const KeyEvent::GetKeySubst() const
{
    // If not extended mode, or Control key or Alt key is not pressed, or virtual keycode is out of range, just bail.
    if (!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() ||
        (_activeModifierKeys & (CTRL_PRESSED | ALT_PRESSED)) == 0 ||
        _virtualKeyCode < 'A' || _virtualKeyCode > 'Z')
    {
        return nullptr;
    }

    const ExtKeyDef* const pKeyDef = GetKeyDef(_virtualKeyCode);
    const ExtKeySubst* pKeySubst;

    // Get the KeySubst based on the modifier status.
    if (IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED))
    {
        if (IsAnyFlagSet(_activeModifierKeys, CTRL_PRESSED))
        {
            pKeySubst = &pKeyDef->keys[2];
        }
        else
        {
            pKeySubst = &pKeyDef->keys[1];
        }
    }
    else
    {
        assert(IsAnyFlagSet(_activeModifierKeys, CTRL_PRESSED));
        pKeySubst = &pKeyDef->keys[0];
    }

    // If the conbination is not defined, just bail.
    if (pKeySubst->wVirKey == 0)
    {
        return nullptr;
    }

    return pKeySubst;
}
