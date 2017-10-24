/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"
#include <string>

std::wostream& operator<<(std::wostream& stream, const FocusEvent* const pFocusEvent)
{
    if (pFocusEvent == nullptr)
    {
        return stream << L"nullptr";
    }

    return stream << L"FocusEvent(" <<
        L"focus" << pFocusEvent->_setFocus << L")";
}

FocusEvent::FocusEvent(_In_ const FOCUS_EVENT_RECORD& record) :
    _setFocus{ record.bSetFocus }
{
}

FocusEvent::FocusEvent(_In_ const int setFocus) :
    _setFocus{ setFocus }
{
}

FocusEvent::~FocusEvent()
{
}

INPUT_RECORD FocusEvent::ToInputRecord() const
{
    INPUT_RECORD record{ 0 };
    record.EventType = FOCUS_EVENT;
    record.Event.FocusEvent.bSetFocus = _setFocus;
    return record;
}

InputEventType FocusEvent::EventType() const
{
    return InputEventType::FocusEvent;
}
