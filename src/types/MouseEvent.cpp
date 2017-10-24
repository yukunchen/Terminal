/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"
#include <string>

std::wostream& operator<<(std::wostream& stream, const MouseEvent* const pMouseEvent)
{
    if (pMouseEvent == nullptr)
    {
        return stream << L"nullptr";
    }

    return stream << L"MouseEvent(" <<
        L"X: " << pMouseEvent->_mousePosition.X << L", " <<
        L"Y: " << pMouseEvent->_mousePosition.Y << L", " <<
        L"buttons: " << pMouseEvent->_buttonState << L", " <<
        L"mods: " << pMouseEvent->_activeModifierKeys << L", " <<
        L"events: " << pMouseEvent->_eventFlags << L")";
}

MouseEvent::MouseEvent(_In_ const MOUSE_EVENT_RECORD& record) :
    _mousePosition{ record.dwMousePosition },
    _buttonState{ record.dwButtonState },
    _activeModifierKeys{ record.dwControlKeyState },
    _eventFlags{ record.dwEventFlags }
{
}

MouseEvent::MouseEvent(_In_ const COORD mousePosition,
                       _In_ const DWORD buttonState,
                       _In_ const DWORD activeModifierKeys,
                       _In_ const DWORD eventFlags) :
    _mousePosition{ mousePosition },
    _buttonState{ buttonState },
    _activeModifierKeys{ activeModifierKeys },
    _eventFlags{ eventFlags }
{
}

MouseEvent::~MouseEvent()
{
}

INPUT_RECORD MouseEvent::ToInputRecord() const
{
    INPUT_RECORD record{ 0 };
    record.EventType = MOUSE_EVENT;
    record.Event.MouseEvent.dwMousePosition = _mousePosition;
    record.Event.MouseEvent.dwButtonState = _buttonState;
    record.Event.MouseEvent.dwControlKeyState = _activeModifierKeys;
    record.Event.MouseEvent.dwEventFlags = _eventFlags;
    return record;
}

InputEventType MouseEvent::EventType() const
{
    return InputEventType::MouseEvent;
}
