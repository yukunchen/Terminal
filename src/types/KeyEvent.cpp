/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

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

void KeyEvent::SetKeyDown(_In_ const bool keyDown) noexcept
{
    _keyDown = keyDown;
}


void KeyEvent::SetRepeatCount(_In_ const WORD repeatCount) noexcept
{
    _repeatCount = repeatCount;
}

void KeyEvent::SetVirtualKeyCode(_In_ const WORD virtualKeyCode) noexcept
{
    _virtualKeyCode = virtualKeyCode;
}

void KeyEvent::SetVirtualScanCode(_In_ const WORD virtualScanCode) noexcept
{
    _virtualScanCode = virtualScanCode;
}

void KeyEvent::SetCharData(_In_ const wchar_t character) noexcept
{
    _charData = character;
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

bool KeyEvent::DoActiveModifierKeysMatch(_In_ const std::unordered_set<ModifierKeyState>& consoleModifiers) const noexcept
{
    DWORD consoleBits = 0;
    for (const ModifierKeyState& mod : consoleModifiers)
    {
        SetAllFlags(consoleBits, ToConsoleControlKeyFlag(mod));
    }
    return consoleBits == _activeModifierKeys;
}
