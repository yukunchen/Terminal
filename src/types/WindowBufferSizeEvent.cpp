/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inc/IInputEvent.hpp"
#include <string>

std::wostream& operator<<(std::wostream& stream, const WindowBufferSizeEvent* const pEvent)
{
    if (pEvent == nullptr)
    {
        return stream << L"nullptr";
    }

    return stream << L"WindowbufferSizeEvent(" <<
        L"X: " << pEvent->_size.X << L", " <<
        "Y: " << pEvent->_size.Y << L")";
}

WindowBufferSizeEvent::WindowBufferSizeEvent(_In_ const WINDOW_BUFFER_SIZE_RECORD& record) :
    _size{ record.dwSize }
{
}

WindowBufferSizeEvent::WindowBufferSizeEvent(_In_ const COORD size) :
    _size{ size }
{
}

WindowBufferSizeEvent::~WindowBufferSizeEvent()
{
}

INPUT_RECORD WindowBufferSizeEvent::ToInputRecord() const
{
    INPUT_RECORD record{ 0 };
    record.EventType = WINDOW_BUFFER_SIZE_EVENT;
    record.Event.WindowBufferSizeEvent.dwSize = _size;
    return record;
}

InputEventType WindowBufferSizeEvent::EventType() const
{
    return InputEventType::WindowBufferSizeEvent;
}
