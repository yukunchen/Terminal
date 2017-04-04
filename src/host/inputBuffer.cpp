/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inputBuffer.hpp"
#include "dbcs.h"
#include "stream.h"

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
    InputWaitEvent = g_hInputEvent.get();
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
        std::deque<INPUT_RECORD> keyRecords;
        for (auto it = _storage.cbegin(); it != _storage.cend(); ++it)
        {
            if (it->EventType == KEY_EVENT)
            {
                keyRecords.push_back(*it);
            }
        }
        _storage.swap(keyRecords);
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
// - pInputRecord - Client provided buffer to read input events into.
// - pcLength - On input, number of events to read.  On output, number of events read.
// - fPeek - If TRUE, copy events to pInputRecord but don't remove them from the input buffer.
// - fWaitForData - if TRUE, wait until an event is input (if there aren't enough to fill client buffer).  if FALSE, return immediately
// - fUnicode - TRUE if the data in key events should be treated as unicode. FALSE if they should be converted by the current input CP.
// Return Value:
// - STATUS_SUCCESS if records were read into the client buffer and everything is OK.
// - CONSOLE_STATUS_WAIT if there weren't enough records to satisfy the request (and waits are allowed)
// - otherwise a suitable memory/math/string error in NTSTATUS form.
NTSTATUS InputBuffer::ReadInputBuffer(_Out_writes_(*pcLength) INPUT_RECORD* pInputRecord,
                                      _Inout_ PDWORD pcLength,
                                      _In_ BOOL const fPeek,
                                      _In_ BOOL const fWaitForData,
                                      _In_ BOOLEAN const fUnicode)
{
    try
    {
        NTSTATUS Status;
        if (_storage.empty())
        {
            *pcLength = 0;
            if (!fWaitForData)
            {
                return STATUS_SUCCESS;
            }
            return CONSOLE_STATUS_WAIT;
        }

        // read from buffer
        ULONG EventsRead;
        BOOL ResetWaitEvent;
        Status = _ReadBuffer(pInputRecord, *pcLength, &EventsRead, fPeek, &ResetWaitEvent, fUnicode);
        if (ResetWaitEvent)
        {
            ResetEvent(InputWaitEvent);
        }

        *pcLength = EventsRead;
        return Status;
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
}

// Routine Description:
// - This routine reads from a buffer. It does the buffer manipulation.
// Arguments:
// - Buffer - buffer to read into
// - Length - length of buffer in events
// - EventsRead - where to store number of events read
// - Peek - if TRUE, don't remove data from buffer, just copy it.
// - ResetWaitEvent - on exit, TRUE if buffer became empty.
// Return Value:
// - STATUS_SUCCESS on success.
// Note:
// - The console lock must be held when calling this routine.
// - This method is mainly a wrapper to allow an array to be used to
// read into.
NTSTATUS InputBuffer::_ReadBuffer(_Out_writes_to_(Length, *EventsRead) INPUT_RECORD* Buffer,
                                  _In_ ULONG Length,
                                  _Out_ PULONG EventsRead,
                                  _In_ BOOL Peek,
                                  _Out_ PBOOL ResetWaitEvent,
                                  _In_ BOOLEAN Unicode)
{
    try
    {
        std::deque<INPUT_RECORD> outRecords;
        size_t eventsRead;
        bool resetWaitEvent;

        // call inner func
        THROW_IF_FAILED(_ReadBuffer(outRecords,
                                    Length,
                                    eventsRead,
                                    !!Peek,
                                    resetWaitEvent,
                                    !!Unicode));

        // move data back to original vars
        *ResetWaitEvent = !!resetWaitEvent;
        THROW_IF_FAILED(SizeTToULong(eventsRead, EventsRead));

        if (outRecords.size() > Length)
        {
            throw std::out_of_range("_ReadBuffer read more records than can fit in the output buffer");
        };
        size_t currentIndex = 0;
        while (!outRecords.empty())
        {
            Buffer[currentIndex] = outRecords.front();
            outRecords.pop_front();
            ++currentIndex;
        }

        return STATUS_SUCCESS;
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

}

// Routine Description:
// - This routine reads from a buffer. It does the buffer manipulation.
// Arguments:
// - outRecords - where read records are placed
// - readCount - amount of records to read
// - eventsRead - where to store number of events read
// - peek - if TRUE, don't remove data from buffer, just copy it.
// - resetWaitEvent - on exit, TRUE if buffer became empty.
// Return Value:
// - S_OK on success.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::_ReadBuffer(_Out_ std::deque<INPUT_RECORD>& outRecords,
                                 _In_ const size_t readCount,
                                 _Out_ size_t& eventsRead,
                                 _In_ const bool peek,
                                 _Out_ bool& resetWaitEvent,
                                 _In_ const bool unicode)
{
    try
    {
        resetWaitEvent = false;

        // we need another var to keep track of how many we've read
        // because dbcs records count for two when we aren't doing a
        // unicode read but the eventsRead count should return the number
        // of events actually put into outRecords.
        size_t virtualReadCount = 0;
        while (!_storage.empty() && virtualReadCount < readCount)
        {
            outRecords.push_back(_storage.front());
            ++virtualReadCount;
            if (!unicode)
            {
                if (_storage.front().EventType == KEY_EVENT &&
                    IsCharFullWidth(_storage.front().Event.KeyEvent.uChar.UnicodeChar))
                {
                    ++virtualReadCount;
                }
            }
            _storage.pop_front();
        }

        // the amount of events that were actually read
        eventsRead = outRecords.size();

        // copy the events back if we were supposed to peek
        if (peek)
        {
            for (auto it = outRecords.crbegin(); it != outRecords.crend(); ++it)
            {
                _storage.push_front(*it);
            }
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
// -  Writes records to the beginning of the input buffer.
// Arguments:
// - pInputRecord - Buffer to write from.
// - pcLength - On input, number of events to write.  On output, number of events written.
// Return Value:
// - STATUS_SUCCESS on success.
// Note:
// - The console lock must be held when calling this routine.
// - This method is mainly a wrapper to allow an array to be used to
// read into.
NTSTATUS InputBuffer::PrependInputBuffer(_In_ INPUT_RECORD* pInputRecord, _Inout_ DWORD* const pcLength)
{
    try
    {
        // change to a deque
        std::deque<INPUT_RECORD> inRecords;
        for (size_t i = 0; i < *pcLength; ++i)
        {
            inRecords.push_back(pInputRecord[i]);
        }
        size_t eventsWritten;
        THROW_IF_FAILED(PrependInputBuffer(inRecords, eventsWritten));

        THROW_IF_FAILED(SizeTToDWord(eventsWritten, pcLength));
        return STATUS_SUCCESS;
    }
    catch (...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }
}

// Routine Description:
// -  Writes records to the beginning of the input buffer.
// Arguments:
// - inRecords - Records to write to buffer.
// - eventsWritten - The number of events written to the buffer on exit.
// Return Value:
// S_OK if successful.
// Note:
// - The console lock must be held when calling this routine.
HRESULT InputBuffer::PrependInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords, _Out_ size_t& eventsWritten)
{
    try
    {
        THROW_IF_FAILED(_HandleConsoleSuspensionEvents(inRecords));
        if (inRecords.empty())
        {
            return STATUS_SUCCESS;
        }
        // read all of the records out of the buffer, then write the
        // prepend ones, then write the original set. We need to do it
        // this way to handle any coalescing that might occur.

        // get all of the existing records, "emptying" the buffer
        std::deque<INPUT_RECORD> existingStorage;
        existingStorage.swap(_storage);

        // write the prepend records
        size_t prependEventsWritten;
        bool setWaitEvent;
        _WriteBuffer(inRecords, prependEventsWritten, setWaitEvent);

        // write all previously existing records
        size_t existingEventsWritten;
        _WriteBuffer(existingStorage, existingEventsWritten, setWaitEvent);
        if (setWaitEvent)
        {
            SetEvent(InputWaitEvent);
        }
        WakeUpReadersWaitingForData();
        eventsWritten = prependEventsWritten;
        return S_OK;
    }
    CATCH_RETURN();
}

// Routine Description:
// - Writes records to the input buffer. Wakes up any readers that are
// waiting for additional input events.
// Arguments:
// - pInputRecord - Buffer to write from.
// - cInputRecords - On input, number of events to write.
// Return Value:
// - The number of events that were written to input buffer.
// Note:
// - The console lock must be held when calling this routine.
// - This method is mainly a wrapper to allow an array to be used to
// read into.
DWORD InputBuffer::WriteInputBuffer(_In_ INPUT_RECORD* pInputRecord, _In_ DWORD cInputRecords)
{
    try
    {
        // change to a deque
        std::deque<INPUT_RECORD> inRecords;
        for (size_t i = 0; i < cInputRecords; ++i)
        {
            inRecords.push_back(pInputRecord[i]);
        }
        size_t EventsWritten = WriteInputBuffer(inRecords);
        DWORD result;
        THROW_IF_FAILED(SIZETToDWord(EventsWritten, &result));
        return result;
    }
    catch (...)
    {
        LOG_HR(wil::ResultFromCaughtException());
        return 0;
    }
}

// Routine Description:
// - Writes records to the input buffer. Wakes up any readers that are
// waiting for additional input events.
// Arguments:
// - inRecords - records to store in the buffer.
// Return Value:
// - The number of events that were written to input buffer.
// Note:
// - The console lock must be held when calling this routine.
size_t InputBuffer::WriteInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords)
{
    try
    {
        THROW_IF_FAILED(_HandleConsoleSuspensionEvents(inRecords));
        if (inRecords.empty())
        {
            return 0;
        }

        // Write to buffer.
        size_t EventsWritten;
        bool SetWaitEvent;
        THROW_IF_FAILED(_WriteBuffer(inRecords, EventsWritten, SetWaitEvent));

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
HRESULT InputBuffer::_WriteBuffer(_In_ std::deque<INPUT_RECORD>& inRecords,
                                  _Out_ size_t& eventsWritten,
                                  _Out_ bool& setWaitEvent)
{
    try
    {
        eventsWritten = 0;
        setWaitEvent = false;
        bool initiallyEmptyQueue = _storage.empty();
        // we only check for possible coalescing when storing one
        // record at a time because this is the original behavior of
        // the input buffer. Changing this behavior may break stuff
        // that was depending on it.
        if (inRecords.size() == 1 && !_storage.empty())
        {
            bool coalesced = false;
            // this looks kinda weird but we don't want to coalesce a
            // mouse event and then try to coalesce a key event right after.
            //
            // we also pass the whole deque to the coalescing methods
            // even though they only want one event because it should
            // be their responsibility to maintain the correct state
            // of the deque if they process any records in it.
            if (_CoalesceMouseMovedEvents(inRecords))
            {
                coalesced = true;
            }
            else if (_CoalesceRepeatedKeyPressEvents(inRecords))
            {
                coalesced = true;
            }
            if (coalesced)
            {
                eventsWritten = 1;
                return S_OK;
            }
        }
        // add all input records to the storage queue
        while (!inRecords.empty())
        {
            _storage.push_back(inRecords.front());
            inRecords.pop_front();
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
bool InputBuffer::_CoalesceMouseMovedEvents(_In_ std::deque<INPUT_RECORD>& inRecords)
{
    _ASSERT(inRecords.size() == 1);
    const INPUT_RECORD& firstInRecord = inRecords.front();
    INPUT_RECORD& lastStoredRecord = _storage.back();
    if (firstInRecord.EventType == MOUSE_EVENT &&
        firstInRecord.Event.MouseEvent.dwEventFlags == MOUSE_MOVED &&
        lastStoredRecord.EventType == MOUSE_EVENT &&
        lastStoredRecord.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
    {
        lastStoredRecord.Event.MouseEvent.dwMousePosition.X = firstInRecord.Event.MouseEvent.dwMousePosition.X;
        lastStoredRecord.Event.MouseEvent.dwMousePosition.Y = firstInRecord.Event.MouseEvent.dwMousePosition.Y;
        inRecords.pop_front();
        return true;
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
bool InputBuffer::_CoalesceRepeatedKeyPressEvents(_In_ std::deque<INPUT_RECORD>& inRecords)
{
    _ASSERT(inRecords.size() == 1);
    const INPUT_RECORD& firstInRecord = inRecords.front();
    INPUT_RECORD& lastStoredRecord = _storage.back();
    // both records need to be key down events
    if (firstInRecord.EventType == KEY_EVENT &&
        firstInRecord.Event.KeyEvent.bKeyDown &&
        lastStoredRecord.EventType == KEY_EVENT &&
        lastStoredRecord.Event.KeyEvent.bKeyDown &&
        !IsCharFullWidth(firstInRecord.Event.KeyEvent.uChar.UnicodeChar))
    {
        const KEY_EVENT_RECORD inKeyEventRecord = firstInRecord.Event.KeyEvent;
        const KEY_EVENT_RECORD lastKeyEventRecord = lastStoredRecord.Event.KeyEvent;
        bool sameKey = false;
        // ime key check
        if (IsFlagSet(inKeyEventRecord.dwControlKeyState, NLS_IME_CONVERSION) &&
            inKeyEventRecord.uChar.UnicodeChar == lastKeyEventRecord.uChar.UnicodeChar &&
            inKeyEventRecord.dwControlKeyState == lastKeyEventRecord.dwControlKeyState)
        {
            sameKey = true;
        }
        // other key events check
        else if (inKeyEventRecord.wVirtualScanCode == lastKeyEventRecord.wVirtualScanCode &&
                 inKeyEventRecord.uChar.UnicodeChar == lastKeyEventRecord.uChar.UnicodeChar &&
                 inKeyEventRecord.dwControlKeyState == lastKeyEventRecord.dwControlKeyState)
        {
            sameKey = true;
        }
        if (sameKey)
        {
            lastStoredRecord.Event.KeyEvent.wRepeatCount += firstInRecord.Event.KeyEvent.wRepeatCount;
            inRecords.pop_front();
            return true;
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
HRESULT InputBuffer::_HandleConsoleSuspensionEvents(_In_ std::deque<INPUT_RECORD>& records)
{
    try
    {
        std::deque<INPUT_RECORD> outRecords;
        for (size_t i = 0; i < records.size(); ++i)
        {
            INPUT_RECORD currEvent = records[i];
            if (currEvent.EventType == KEY_EVENT && currEvent.Event.KeyEvent.bKeyDown)
            {
                if (IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_SUSPENDED) &&
                    !IsSystemKey(currEvent.Event.KeyEvent.wVirtualKeyCode))
                {
                    UnblockWriteConsole(CONSOLE_OUTPUT_SUSPENDED);
                    continue;
                }
                else if (IsFlagSet(InputMode, ENABLE_LINE_INPUT) &&
                         currEvent.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE || IsPauseKey(&currEvent.Event.KeyEvent))
                {
                    SetFlag(g_ciConsoleInformation.Flags, CONSOLE_SUSPENDED);
                    continue;
                }
            }
            outRecords.push_back(currEvent);
        }
        records.swap(outRecords);
        return S_OK;
    }
    CATCH_RETURN();
}
