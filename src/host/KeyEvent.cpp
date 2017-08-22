/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "IInputEvent.hpp"

KeyEvent::KeyEvent(_In_ const KEY_EVENT_RECORD& record) :
    _keyDown{ !!record.bKeyDown },
    _repeatCount{ record.wRepeatCount },
    _virtualKeyCode{ record.wVirtualKeyCode },
    _virtualScanCode{ record.wVirtualScanCode },
    _charData{ record.uChar.UnicodeChar },
    _activeModifierKeys{ record.dwControlKeyState }
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