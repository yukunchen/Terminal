/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

MenuEvent::MenuEvent(_In_ const MENU_EVENT_RECORD& record) :
    _commandId{ record.dwCommandId }
{
}

MenuEvent::~MenuEvent()
{
}

INPUT_RECORD MenuEvent::ToInputRecord() const
{
    INPUT_RECORD record{ 0 };
    record.EventType = MENU_EVENT;
    record.Event.MenuEvent.dwCommandId = _commandId;
    return record;
}

InputEventType MenuEvent::EventType() const
{
    return InputEventType::MenuEvent;
}
