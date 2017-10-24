/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

std::wostream& operator<<(std::wostream& stream, const MenuEvent* const pMenuEvent)
{
    if (pMenuEvent == nullptr)
    {
        return stream << L"nullptr";
    }

    return stream << L"MenuEvent(" <<
        L"CommandId" << pMenuEvent->_commandId << L")";
}

MenuEvent::MenuEvent(_In_ const MENU_EVENT_RECORD& record) :
    _commandId{ record.dwCommandId }
{
}

MenuEvent::MenuEvent(_In_ const UINT commandId) :
    _commandId{ commandId }
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
