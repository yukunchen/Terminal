/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inputBuffer.hpp"
#include "dbcs.h"
#include "stream.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#define INPUT_BUFFER_DEFAULT_INPUT_MODE (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT)

// Routine Description:
// - This method creates an input buffer.
// Arguments:
// - None
// Return Value:
// - A new instance of InputBuffer
InputBuffer::InputBuffer() :
    InputMode{ INPUT_BUFFER_DEFAULT_INPUT_MODE },
    WaitQueue{}
{
    InputWaitEvent = ServiceLocator::LocateGlobals()->hInputEvent.get();
    // initialize buffer header
    fInComposition = false;

    ZeroMemory(&ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
    ZeroMemory(&WriteConInpDbcsLeadByte, sizeof(INPUT_RECORD) * ARRAYSIZE(WriteConInpDbcsLeadByte));
}

// Routine Description:
// - This routine frees the resources associated with an input buffer.
// Arguments:
// - None
// Return Value:
InputBuffer::~InputBuffer()
{
    // TODO: MSFT:8805366 check for null before trying to close this
    // and check that it needs to be closing it in the first place.
    CloseHandle(InputWaitEvent);
}

// Routine Description:
// - This routine resets the input buffer information fields to their initial values.
// Arguments:
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void InputBuffer::ReinitializeInputBuffer()
{
    ResetEvent(InputWaitEvent);
    InputMode = INPUT_BUFFER_DEFAULT_INPUT_MODE;
    _storage.clear();
}

// Routine Description:
// - Wakes up readers waiting for data to read.
// Arguments:
// - None
// Return Value:
// - None
void InputBuffer::WakeUpReadersWaitingForData()
{
    WaitQueue.NotifyWaiters(false);
}

// Routine Description:
// - Wakes up any readers waiting for data when a ctrl-c or ctrl-break is input.
// Arguments:
// - Flag - Indicates reason to terminate the readers.
// Return Value:
// - None
void InputBuffer::TerminateRead(_In_ WaitTerminationReason Flag)
{
    WaitQueue.NotifyWaiters(true, Flag);
}

// Routine Description:
// - Returns the number of events in the input buffer.
// Arguments:
// - None
// Return Value:
// - The number of events currently in the input buffer.
// Note:
// - The console lock must be held when calling this routine.
size_t InputBuffer::GetNumberOfReadyEvents()
{
    return _storage.size();
}

// Routine Description:
// - This routine empties the input buffer
// Arguments:
// - None
// Return Value:
// - None
// Note:
// - The console lock must be held when calling this routine.
void InputBuffer::Flush()
{
    _storage.clear();
    ResetEvent(InputWaitEvent);
}

// Routine Description:
// - This routine removes all but the key events from the buffer.
// Arguments:
// - None
// Return Value:
// - S_OK on success, other HRESULTS otherwise.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::FlushAllButKeys()
{
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> keyEvents;
        for (auto it = _storage.begin(); it != _storage.end(); ++it)
        {
            if ((*it)->EventType() == InputEventType::KeyEvent)
            {
                std::unique_ptr<IInputEvent> tempPtr((*it).release());
                keyEvents.push_back(std::move(tempPtr));
            }
        }
        _storage.swap(keyEvents);
        return S_OK;
    }
    CATCH_RETURN();
}

// Routine Description:
// - This routine reads from the input buffer.
// - It can convert returned data to through the currently set Input CP, it can optionally return a wait condition
//   if there isn't enough data in the buffer, and it can be set to not remove records as it reads them out.
// Note:
// - The console lock must be held when calling this routine.
// Arguments:
// - OutEvents - deque to store the read events
// - AmountToRead - the amount of events to try to read
// - Peek - If true, copy events to pInputRecord but don't remove them from the input buffer.
// - WaitForData - if true, wait until an event is input (if there aren't enough to fill client buffer). if false, return immediately
// - Unicode - true if the data in key events should be treated as unicode. false if they should be converted by the current input CP.
// Return Value:
// - STATUS_SUCCESS if records were read into the client buffer and everything is OK.
// - CONSOLE_STATUS_WAIT if there weren't enough records to satisfy the request (and waits are allowed)
// - otherwise a suitable memory/math/string error in NTSTATUS form.
NTSTATUS InputBuffer::Read(_Out_ std::deque<std::unique_ptr<IInputEvent>>& OutEvents,
                           _In_ const size_t AmountToRead,
                           _In_ const bool Peek,
                           _In_ const bool WaitForData,
                           _In_ const bool Unicode)
{
    try
    {
        NTSTATUS Status;
        if (_storage.empty())
        {
            if (!WaitForData)
            {
                return STATUS_SUCCESS;
            }
            return CONSOLE_STATUS_WAIT;
        }

        // read from buffer
        std::deque<std::unique_ptr<IInputEvent>> events;
        size_t eventsRead;
        bool resetWaitEvent;
        Status = _ReadBuffer(events,
                             AmountToRead,
                             eventsRead,
                             Peek,
                             resetWaitEvent,
                             Unicode);

        // copy events to outEvents
        while (!events.empty())
        {
            OutEvents.push_back(std::move(events.front()));
            events.pop_front();
        }

        if (resetWaitEvent)
        {
            ResetEvent(InputWaitEvent);
        }
        return Status;
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
}

// Routine Description:
// - This routine reads a single event from the input buffer.
// - It can convert returned data to through the currently set Input CP, it can optionally return a wait condition
//   if there isn't enough data in the buffer, and it can be set to not remove records as it reads them out.
// Note:
// - The console lock must be held when calling this routine.
// Arguments:
// - outEvent - where the read event is stored
// - Peek - If true, copy events to pInputRecord but don't remove them from the input buffer.
// - WaitForData - if true, wait until an event is input (if there aren't enough to fill client buffer). if false, return immediately
// - Unicode - true if the data in key events should be treated as unicode. false if they should be converted by the current input CP.
// Return Value:
// - STATUS_SUCCESS if records were read into the client buffer and everything is OK.
// - CONSOLE_STATUS_WAIT if there weren't enough records to satisfy the request (and waits are allowed)
// - otherwise a suitable memory/math/string error in NTSTATUS form.
NTSTATUS InputBuffer::Read(_Out_ std::unique_ptr<IInputEvent>& outEvent,
                           _In_ const bool Peek,
                           _In_ const bool WaitForData,
                           _In_ const bool Unicode)
{
    NTSTATUS Status;
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> outEvents;
        Status = Read(outEvents,
                      1,
                      Peek,
                      WaitForData,
                      Unicode);
        if (!outEvents.empty())
        {
            outEvent.swap(outEvents.front());
        }
    }
    catch (...)
    {
        Status = NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

    return Status;
}

// Routine Description:
// - This routine reads from a buffer. It does the buffer manipulation.
// Arguments:
// - outEvents - where read events are placed
// - readCount - amount of events to read
// - eventsRead - where to store number of events read
// - peek - if true , don't remove data from buffer, just copy it.
// - resetWaitEvent - on exit, true if buffer became empty.
// - unicode - true if read should be done in unicode mode
// Return Value:
// - S_OK on success.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::_ReadBuffer(_Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
                                 _In_ const size_t readCount,
                                 _Out_ size_t& eventsRead,
                                 _In_ const bool peek,
                                 _Out_ bool& resetWaitEvent,
                                 _In_ const bool unicode)
{
    try
    {
        resetWaitEvent = false;

        std::deque<std::unique_ptr<IInputEvent>> readEvents;
        // we need another var to keep track of how many we've read
        // because dbcs records count for two when we aren't doing a
        // unicode read but the eventsRead count should return the number
        // of events actually put into outRecords.
        size_t virtualReadCount = 0;
        while (!_storage.empty() && virtualReadCount < readCount)
        {
            readEvents.push_back(std::move(_storage.front()));
            _storage.pop_front();
            ++virtualReadCount;
            if (!unicode)
            {
                if (readEvents.back()->EventType() == InputEventType::KeyEvent)
                {
                    const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(readEvents.back().get());
                    if (IsCharFullWidth(pKeyEvent->_charData))
                    {
                        ++virtualReadCount;
                    }
                }
            }
        }

        // the amount of events that were actually read
        eventsRead = readEvents.size();

        // copy the events back if we were supposed to peek
        if (peek)
        {
            for (auto it = readEvents.crbegin(); it != readEvents.crend(); ++it)
            {
                _storage.push_front(IInputEvent::Create((*it)->ToInputRecord()));
            }
        }

        // move events read to proper deque
        while (!readEvents.empty())
        {
            outEvents.push_back(std::move(readEvents.front()));
            readEvents.pop_front();
        }

        // signal if we emptied the buffer
        if (_storage.empty())
        {
            resetWaitEvent = true;
        }

        return S_OK;
    }
    CATCH_RETURN();
}

// Routine Description:
// -  Writes events to the beginning of the input buffer.
// Arguments:
// - inEvents - events to write to buffer.
// - eventsWritten - The number of events written to the buffer on exit.
// Return Value:
// S_OK if successful.
// Note:
// - The console lock must be held when calling this routine.
size_t InputBuffer::Prepend(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    try
    {
        THROW_IF_FAILED(_HandleConsoleSuspensionEvents(inEvents));
        if (inEvents.empty())
        {
            return STATUS_SUCCESS;
        }
        // read all of the records out of the buffer, then write the
        // prepend ones, then write the original set. We need to do it
        // this way to handle any coalescing that might occur.

        // get all of the existing records, "emptying" the buffer
        std::deque<std::unique_ptr<IInputEvent>> existingStorage;
        existingStorage.swap(_storage);

        // We will need this variable to pass to _WriteBuffer so it can attempt to determine wait status.
        // However, because we swapped the storage out from under it with an empty deque, it will always
        // return true after the first one (as it is filling the newly emptied backing deque.)
        // Then after the second one, because we've inserted some input, it will always say false.
        bool unusedWaitStatus = false;

        // write the prepend records
        size_t prependEventsWritten;
        _WriteBuffer(inEvents, prependEventsWritten, unusedWaitStatus);
        assert(unusedWaitStatus);

        // write all previously existing records
        size_t existingEventsWritten;
        _WriteBuffer(existingStorage, existingEventsWritten, unusedWaitStatus);
        assert(!unusedWaitStatus);

        // We need to set the wait event if there were 0 events in the
        // input queue when we started.
        // Because we did interesting manipulation of the wait queue
        // in order to prepend, we can't trust what _WriteBuffer said
        // and instead need to set the event if the original backing
        // buffer (the one we swapped out at the top) was empty
        // when this whole thing started.
        if (existingStorage.empty())
        {
            SetEvent(InputWaitEvent);
        }
        WakeUpReadersWaitingForData();

        return prependEventsWritten;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
        return 0;
    }
}

// Routine Description:
// - Writes event to the input buffer. Wakes up any readers that are
// waiting for additional input events.
// Arguments:
// - inEvent - input event to store in the buffer.
// Return Value:
// - The number of events that were written to input buffer.
// Note:
// - The console lock must be held when calling this routine.
// - any outside references to inEvent will ben invalidated after
// calling this method.
size_t InputBuffer::Write(_Inout_ std::unique_ptr<IInputEvent> inEvent)
{
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> inEvents;
        inEvents.push_back(std::move(inEvent));
        return Write(inEvents);
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
        return 0;
    }
}

// Routine Description:
// - Writes events to the input buffer. Wakes up any readers that are
// waiting for additional input events.
// Arguments:
// - inEvents - input events to store in the buffer.
// Return Value:
// - The number of events that were written to input buffer.
// Note:
// - The console lock must be held when calling this routine.
size_t InputBuffer::Write(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    try
    {
        THROW_IF_FAILED(_HandleConsoleSuspensionEvents(inEvents));
        if (inEvents.empty())
        {
            return 0;
        }

        // Write to buffer.
        size_t EventsWritten;
        bool SetWaitEvent;
        THROW_IF_FAILED(_WriteBuffer(inEvents, EventsWritten, SetWaitEvent));

        if (SetWaitEvent)
        {
            SetEvent(InputWaitEvent);
        }

        // Alert any writers waiting for space.
        WakeUpReadersWaitingForData();
        return EventsWritten;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
        return 0;
    }
}

// Routine Description:
// - Coalesces input events and transfers them to storage queue.
// Arguments:
// - inRecords - The events to store.
// - eventsWritten - The number of events written since this function
// was called.
// - setWaitEvent - on exit, true if buffer became non-empty.
// Return Value:
// - S_OK on success.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::_WriteBuffer(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents,
                                  _Out_ size_t& eventsWritten,
                                  _Out_ bool& setWaitEvent)
{
    eventsWritten = 0;
    setWaitEvent = false;
    const bool initiallyEmptyQueue = _storage.empty();

    try
    {
        // we only check for possible coalescing when storing one
        // record at a time because this is the original behavior of
        // the input buffer. Changing this behavior may break stuff
        // that was depending on it.
        if (inEvents.size() == 1 && !_storage.empty())
        {
            bool coalesced = false;
            // this looks kinda weird but we don't want to coalesce a
            // mouse event and then try to coalesce a key event right after.
            //
            // we also pass the whole deque to the coalescing methods
            // even though they only want one event because it should
            // be their responsibility to maintain the correct state
            // of the deque if they process any records in it.
            if (_CoalesceMouseMovedEvents(inEvents))
            {
                coalesced = true;
            }
            else if (_CoalesceRepeatedKeyPressEvents(inEvents))
            {
                coalesced = true;
            }
            if (coalesced)
            {
                eventsWritten = 1;
                return S_OK;
            }
        }
        // add all input events to the storage queue
        while (!inEvents.empty())
        {
            _storage.push_back(std::move(inEvents.front()));
            inEvents.pop_front();
            ++eventsWritten;
        }
        if (initiallyEmptyQueue && !_storage.empty())
        {
            setWaitEvent = true;
        }
        return S_OK;
    }
    CATCH_RETURN();
}

// Routine Description:
// - Checks if the last saved event and the first event of inRecords are
// both MOUSE_MOVED events. If they are, the last saved event is
// updated with the new mouse position and the first event of inRecords is
// dropped.
// Arguments:
// - inRecords - The incoming records to process.
// Return Value:
// true if events were coalesced, false if they were not.
// Note:
// - The size of inRecords must be 1.
// - Coalescing here means updating a record that already exists in
// the buffer with updated values from an incoming event, instead of
// storing the incoming event (which would make the original one
// redundant/out of date with the most current state).
bool InputBuffer::_CoalesceMouseMovedEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    assert(inEvents.size() == 1);
    assert(!_storage.empty());
    const IInputEvent* const pFirstInEvent = inEvents.front().get();
    const IInputEvent* const pLastStoredEvent = _storage.back().get();
    if (pFirstInEvent->EventType() == InputEventType::MouseEvent &&
        pLastStoredEvent->EventType() == InputEventType::MouseEvent)
    {
        const MouseEvent* const pInMouseEvent = static_cast<const MouseEvent* const>(pFirstInEvent);
        const MouseEvent* const pLastMouseEvent = static_cast<const MouseEvent* const>(pLastStoredEvent);

        if (pInMouseEvent->_eventFlags == MOUSE_MOVED &&
            pLastMouseEvent->_eventFlags == MOUSE_MOVED)
        {
            // update mouse moved position
            MouseEvent* const pMouseEvent = static_cast<MouseEvent* const>(_storage.back().release());
            pMouseEvent->_mousePosition.X = pInMouseEvent->_mousePosition.X;
            pMouseEvent->_mousePosition.Y = pInMouseEvent->_mousePosition.Y;
            std::unique_ptr<IInputEvent> tempPtr(pMouseEvent);
            tempPtr.swap(_storage.back());

            inEvents.pop_front();
            return true;
        }
    }
    return false;
}

// Routine Description::
// - If the last input event saved and the first input event in inRecords
// are both a keypress down event for the same key, update the repeat
// count of the saved event and drop the first from inRecords.
// Arguments:
// - inRecords - The incoming records to process.
// Return Value:
// true if events were coalesced, false if they were not.
// Note:
// - The size of inRecords must be 1.
// - Coalescing here means updating a record that already exists in
// the buffer with updated values from an incoming event, instead of
// storing the incoming event (which would make the original one
// redundant/out of date with the most current state).
bool InputBuffer::_CoalesceRepeatedKeyPressEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    assert(inEvents.size() == 1);
    assert(!_storage.empty());
    const IInputEvent* const pFirstInEvent = inEvents.front().get();
    const IInputEvent* const pLastStoredEvent = _storage.back().get();
    if (pFirstInEvent->EventType() == InputEventType::KeyEvent &&
        pLastStoredEvent->EventType() == InputEventType::KeyEvent)
    {
        const KeyEvent* const pInKeyEvent = static_cast<const KeyEvent* const>(pFirstInEvent);
        const KeyEvent* const pLastKeyEvent = static_cast<const KeyEvent* const>(pLastStoredEvent);

        if (pInKeyEvent->_keyDown &&
            pLastKeyEvent->_keyDown &&
            !IsCharFullWidth(pInKeyEvent->_charData))
        {
            bool sameKey = false;
            if (IsFlagSet(pInKeyEvent->_activeModifierKeys, NLS_IME_CONVERSION) &&
                pInKeyEvent->_charData == pLastKeyEvent->_charData &&
                pInKeyEvent->_activeModifierKeys == pLastKeyEvent->_activeModifierKeys)
            {
                sameKey = true;
            }
            // other key events check
            else if (pInKeyEvent->_virtualScanCode == pLastKeyEvent->_virtualScanCode &&
                    pInKeyEvent->_charData == pLastKeyEvent->_charData &&
                    pInKeyEvent->_activeModifierKeys == pLastKeyEvent->_activeModifierKeys)
            {
                sameKey = true;
            }
            if (sameKey)
            {
                // increment repeat count
                KeyEvent* const pKeyEvent = static_cast<KeyEvent* const>(_storage.back().release());
                pKeyEvent->_repeatCount += pInKeyEvent->_repeatCount;
                std::unique_ptr<IInputEvent> tempPtr(pKeyEvent);
                tempPtr.swap(_storage.back());

                inEvents.pop_front();
                return true;
            }
        }
    }
    return false;
}

// Routine Description:
// - Handles records that suspend/resume the console.
// Arguments:
// - records - records to check for pause/unpause events
// Return Value:
// - S_OK on success.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::_HandleConsoleSuspensionEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    try
    {
        std::deque<std::unique_ptr<IInputEvent>> outEvents;
        while (!inEvents.empty())
        {
            std::unique_ptr<IInputEvent> currEvent = std::move(inEvents.front());
            inEvents.pop_front();
            if (currEvent->EventType() == InputEventType::KeyEvent)
            {
                const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(currEvent.get());
                if (pKeyEvent->_keyDown)
                {
                    if (IsFlagSet(gci->Flags, CONSOLE_SUSPENDED) &&
                        !IsSystemKey(pKeyEvent->_virtualKeyCode))
                    {
                        UnblockWriteConsole(CONSOLE_OUTPUT_SUSPENDED);
                        continue;
                    }
                    else if (IsFlagSet(InputMode, ENABLE_LINE_INPUT) &&
                             (pKeyEvent->_virtualKeyCode == VK_PAUSE || IsPauseKey(pKeyEvent)))
                    {
                        SetFlag(gci->Flags, CONSOLE_SUSPENDED);
                        continue;
                    }
                }
            }
            outEvents.push_back(std::move(currEvent));
        }
        inEvents.swap(outEvents);
        return S_OK;
    }
    CATCH_RETURN();
}

// Routine Description:
// - Converts std::deque<INPUT_RECORD> to std::deque<std::unique_ptr<IInputEvent>>
// Arguments:
// - inRecords - records to convert
// Return Value:
// - std::deque of IINputEvents on success. Will throw exception on failure.
std::deque<std::unique_ptr<IInputEvent>> InputBuffer::_inputRecordsToInputEvents(_In_ const std::deque<INPUT_RECORD>& inRecords)
{
    std::deque<std::unique_ptr<IInputEvent>> outEvents;
    for (size_t i = 0; i < inRecords.size(); ++i)
    {
        std::unique_ptr<IInputEvent> event = IInputEvent::Create(inRecords[i]);
        outEvents.push_back(std::move(event));
    }
    return outEvents;
}