/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

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

INPUT_RECORD KeyEvent::ToInputRecord() const
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

InputEventType KeyEvent::EventType() const
{
    return InputEventType::KeyEvent;
}

bool KeyEvent::IsCursorKey() const
{
    // true iff vk in [End, Home, Left, Up, Right, Down]
    return (_virtualKeyCode >= VK_END) && (_virtualKeyCode <= VK_DOWN);
}

bool KeyEvent::IsKeyDown() const
{
    return _keyDown;
}

void KeyEvent::SetKeyDown(_In_ const bool keyDown)
{
    _keyDown = keyDown;
}

/*
size_t KeyEvent::GetRepeatCount() const
{
    return repeatCount;
}

void KeyEvent::SetRepeatCount(_In_ const size_t repeatCount)
{
    _repeatCount = repeatCount;
}
*/

WORD KeyEvent::GetVirtualKeyCode() const
{
    return _virtualKeyCode;
}

void KeyEvent::SetVirtualKeyCode(_In_ const WORD virtualKeyCode)
{
    _virtualKeyCode = virtualKeyCode;
}