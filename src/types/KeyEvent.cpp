/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"
#include <string>

bool operator==(const KeyEvent& a, const KeyEvent& b)
{
    return (a._keyDown == b._keyDown &&
            a._repeatCount == b._repeatCount &&
            a._virtualKeyCode == b._virtualKeyCode &&
            a._virtualScanCode == b._virtualScanCode &&
            a._charData == b._charData &&
            a._activeModifierKeys == b._activeModifierKeys);
}

std::wostream& operator<<(std::wostream& stream, const KeyEvent* const pKeyEvent)
{
    if (pKeyEvent == nullptr)
    {
        return stream << L"nullptr";
    }

    std::wstring keyMotion = pKeyEvent->_keyDown ? L"keyDown" : L"keyUp";
    std::wstring charData = { pKeyEvent->_charData };
    if (pKeyEvent->_charData == L'\0')
    {
        charData = L"null";
    }

    return stream << L"KeyEvent(" <<
        keyMotion << L", " <<
        L"repeat: " << pKeyEvent->_repeatCount << L", " <<
        L"keyCode: " << pKeyEvent->_virtualKeyCode << L", " <<
        L"scanCode: " << pKeyEvent->_virtualScanCode << L", " <<
        L"char: " << charData << L", " <<
        L"mods: " << pKeyEvent->_activeModifierKeys << L")";
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
