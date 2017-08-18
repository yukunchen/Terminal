/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "IInputEvent.hpp"

FocusEvent::FocusEvent(_In_ const FOCUS_EVENT_RECORD& record) :
    _setFocus{ record.bSetFocus }
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
