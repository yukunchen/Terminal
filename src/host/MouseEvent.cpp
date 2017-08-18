/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "IInputEvent.hpp"

MouseEvent::MouseEvent(_In_ const MOUSE_EVENT_RECORD& record) :
    _mousePosition{ record.dwMousePosition },
    _buttonState{ record.dwButtonState },
    _activeModifierKeys{ record.dwControlKeyState },
    _eventFlags{ record.dwEventFlags }
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
