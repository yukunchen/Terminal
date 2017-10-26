/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

bool operator==(const KeyEvent& a, const KeyEvent& b) noexcept
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

KeyEvent::KeyEvent(_In_ const bool keyDown,
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

INPUT_RECORD KeyEvent::ToInputRecord() const noexcept
{
    INPUT_RECORD record{ 0 };
    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = !!_keyDown;
    record.Event.KeyEvent.wRepeatCount = _repeatCount;
    record.Event.KeyEvent.wVirtualKeyCode = _virtualKeyCode;
    record.Event.KeyEvent.wVirtualScanCode = _virtualScanCode;
    record.Event.KeyEvent.uChar.UnicodeChar = _charData;
    record.Event.KeyEvent.dwControlKeyState = _activeModifierKeys;
    return record;
}

InputEventType KeyEvent::EventType() const noexcept
{
    return InputEventType::KeyEvent;
}

bool KeyEvent::IsShiftPressed() const noexcept
{
    return IsFlagSet(_activeModifierKeys, SHIFT_PRESSED);
}

bool KeyEvent::IsAltPressed() const noexcept
{
    return IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED);
}

bool KeyEvent::IsCtrlPressed() const noexcept
{
    return IsAnyFlagSet(_activeModifierKeys, CTRL_PRESSED);
}

bool KeyEvent::IsAltGrPressed() const noexcept
{
    return AreAllFlagsSet(_activeModifierKeys, LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
}

bool KeyEvent::IsModifierPressed() const noexcept
{
    return IsAnyFlagSet(_activeModifierKeys, MOD_PRESSED);
}

bool KeyEvent::IsCursorKey() const noexcept
{
    // true iff vk in [End, Home, Left, Up, Right, Down]
    return (_virtualKeyCode >= VK_END) && (_virtualKeyCode <= VK_DOWN);
}

bool KeyEvent::IsAltNumpadSet() const noexcept
{
    return IsFlagSet(_activeModifierKeys, ALTNUMPAD_BIT);
}

bool KeyEvent::IsKeyDown() const noexcept
{
    return _keyDown;
}

void KeyEvent::SetKeyDown(_In_ const bool keyDown) noexcept
{
    _keyDown = keyDown;
}

WORD KeyEvent::GetRepeatCount() const noexcept
{
    return _repeatCount;
}

void KeyEvent::SetRepeatCount(_In_ const WORD repeatCount) noexcept
{
    _repeatCount = repeatCount;
}

WORD KeyEvent::GetVirtualKeyCode() const noexcept
{
    return _virtualKeyCode;
}

void KeyEvent::SetVirtualKeyCode(_In_ const WORD virtualKeyCode) noexcept
{
    _virtualKeyCode = virtualKeyCode;
}

WORD KeyEvent::GetVirtualScanCode() const noexcept
{
    return _virtualScanCode;
}

void KeyEvent::SetVirtualScanCode(_In_ const WORD virtualScanCode) noexcept
{
    _virtualScanCode = virtualScanCode;
}

wchar_t KeyEvent::GetCharData() const noexcept
{
    return _charData;
}

void KeyEvent::SetCharData(_In_ const wchar_t character) noexcept
{
    _charData = character;
}

DWORD KeyEvent::GetActiveModifierKeys() const noexcept
{
    return _activeModifierKeys;
}

void KeyEvent::SetActiveModifierKeys(_In_ const DWORD activeModifierKeys) noexcept
{
    _activeModifierKeys = activeModifierKeys;
}

void KeyEvent::DeactivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept
{
    DWORD bitFlag = ToConsoleControlKeyFlag(modifierKey);
    ClearAllFlags(_activeModifierKeys, bitFlag);
}

void KeyEvent::ActivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept
{
    DWORD bitFlag = ToConsoleControlKeyFlag(modifierKey);
    SetAllFlags(_activeModifierKeys, bitFlag);
}

bool KeyEvent::DoActiveModifierKeysMatch(_In_ const std::unordered_set<ModifierKeyState>& consoleModifiers) noexcept
{
    DWORD consoleBits = 0;
    for (const ModifierKeyState& mod : consoleModifiers)
    {
        SetAllFlags(consoleBits, ToConsoleControlKeyFlag(mod));
    }
    return consoleBits == _activeModifierKeys;
}
