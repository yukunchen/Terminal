/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"

FocusEvent::FocusEvent(_In_ const FOCUS_EVENT_RECORD& record) :
    _focus{ !!record.bSetFocus }
{
}

FocusEvent::FocusEvent(_In_ const bool focus) :
    _focus{ focus }
{
}

FocusEvent::~FocusEvent()
{
}

INPUT_RECORD FocusEvent::ToInputRecord() const noexcept
{
    INPUT_RECORD record{ 0 };
    record.EventType = FOCUS_EVENT;
    record.Event.FocusEvent.bSetFocus = !!_focus;
    return record;
}

InputEventType FocusEvent::EventType() const noexcept
{
    return InputEventType::FocusEvent;
}

bool FocusEvent::GetFocus() const noexcept
{
    return _focus;
}

void FocusEvent::SetFocus(_In_ const bool focus) noexcept
{
    _focus = focus;
}