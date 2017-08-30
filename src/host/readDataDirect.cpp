/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "readDataDirect.hpp"
#include "dbcs.h"
#include "misc.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

// Routine Description:
// - Constructs direct read data class to hold context across sessions
// generally when there's not enough data to return. Also used a bit
// internally to just pass some information along the stack
// (regardless of wait necessity).
// Arguments:
// - pInputBuffer - Buffer that data will be read from.
// - pInputReadHandleData - Context stored across calls from the same
// input handle to return partial data appropriately.
// - pOutRecords - The buffer the client has presented to be filled with INPUT_RECORDs
// - cOutRecords - Max count of INPUT_RECORDs that we can stuff into
// the user's buffer (pOutRecords)
// - outEvents - storage for any IInputEvents read so far
// Return Value:
// - THROW: Throws E_INVALIDARG for invalid pointers.
DirectReadData::DirectReadData(_In_ InputBuffer* const pInputBuffer,
                                   _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                   _In_ INPUT_RECORD* pOutRecords,
                                   _In_ const size_t cOutRecords,
                                   _In_ std::deque<std::unique_ptr<IInputEvent>> partialEvents) :
    ReadData(pInputBuffer, pInputReadHandleData),
    _pOutRecords{ THROW_HR_IF_NULL(E_INVALIDARG, pOutRecords) },
    _cOutRecords{ cOutRecords },
    _partialEvents{ std::move(partialEvents) },
    _outEvents{ }
{
}

// Routine Description:
// - Destructs a read data class.
// - Decrements count of readers waiting on the given handle.
DirectReadData::~DirectReadData()
{
}

// Routine Description:
// - This routine is called to complete a direct read that blocked in
//   ReadInputBuffer. The context of the read was saved in the DirectReadData
//   structure. This routine is called when events have been written to
//   the input buffer. It is called in the context of the writing thread.
// Arguments:
// - TerminationReason - if this routine is called because a ctrl-c or
// ctrl-break was seen, this argument contains CtrlC or CtrlBreak. If
// the owning thread is exiting, it will have ThreadDying. Otherwise 0.
// - fIsUnicode - Should we return UCS-2 unicode data, or should we
// run the final data through the current Input Codepage before
// returning?
// - pReplyStatus - The status code to return to the client
// application that originally called the API (before it was queued to
// wait)
// - pNumBytes - The number of bytes of data that the server/driver
// will need to transmit back to the client process
// - pControlKeyState - For certain types of reads, this specifies
// which modifier keys were held.
// Return Value:
// - TRUE if the wait is done and result buffer/status code can be sent back to the client.
// - FALSE if we need to continue to wait until more data is available.
BOOL DirectReadData::Notify(_In_ WaitTerminationReason const TerminationReason,
                              _In_ BOOLEAN const fIsUnicode,
                              _Out_ NTSTATUS* const pReplyStatus,
                              _Out_ DWORD* const pNumBytes,
                              _Out_ DWORD* const pControlKeyState)
{
#ifdef DBG
    _pInputReadHandleData->LockReadCount();
    ASSERT(_pInputReadHandleData->GetReadCount() > 0);
    _pInputReadHandleData->UnlockReadCount();
#endif

    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    assert(gci->IsConsoleLocked());

    *pReplyStatus = STATUS_SUCCESS;
    *pControlKeyState = 0;
    bool retVal = true;

    // If ctrl-c or ctrl-break was seen, ignore it.
    if (IsAnyFlagSet(TerminationReason, (WaitTerminationReason::CtrlC | WaitTerminationReason::CtrlBreak)))
    {
        return FALSE;
    }

    // check if a partial byte is already stored that we should read
    if (!fIsUnicode &&
        _pInputBuffer->IsReadPartialByteSequenceAvailable() &&
        _cOutRecords == 1)
    {
        _partialEvents.push_back(_pInputBuffer->FetchReadPartialByteSequence(false));
    }

    // See if called by CsrDestroyProcess or CsrDestroyThread
    // via ConsoleNotifyWaitBlock. If so, just decrement the ReadCount and return.
    if (IsFlagSet(TerminationReason, WaitTerminationReason::ThreadDying))
    {
        *pReplyStatus = STATUS_THREAD_IS_TERMINATING;
    }
    // We must see if we were woken up because the handle is being
    // closed. If so, we decrement the read count. If it goes to
    // zero, we wake up the close thread. Otherwise, we wake up any
    // other thread waiting for data.
    else if (IsFlagSet(_pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::Closing))
    {
        *pReplyStatus = STATUS_ALERTED;
    }
    else
    {
        // if we get to here, this routine was called either by the input
        // thread or a write routine.  both of these callers grab the
        // current console lock.

        size_t amountToRead = _cOutRecords - (_partialEvents.size() + _outEvents.size());
        *pReplyStatus = _pInputBuffer->Read(_outEvents,
                                            amountToRead,
                                            false,
                                            false,
                                            !!fIsUnicode);

        if (*pReplyStatus == CONSOLE_STATUS_WAIT)
        {
            retVal = false;
        }
    }

    if (*pReplyStatus != CONSOLE_STATUS_WAIT)
    {
        // split key events to oem chars if necessary
        if (*pReplyStatus == STATUS_SUCCESS && !fIsUnicode)
        {
            SplitToOem(_outEvents);
        }

        // combine partial and whole events
        while (!_partialEvents.empty())
        {
            _outEvents.push_front(std::move(_partialEvents.back()));
            _partialEvents.pop_back();
        }

        // convert events to input records and add to buffer
        size_t recordCount = 0;
        for (size_t i = 0; i < _cOutRecords; ++i)
        {
            if (_outEvents.empty())
            {
                break;
            }
            _pOutRecords[i] = _outEvents.front()->ToInputRecord();
            _outEvents.pop_front();
            ++recordCount;
        }
        // store partial event if necessary
        if (!_outEvents.empty())
        {
            _pInputBuffer->StoreReadPartialByteSequence(std::move(_outEvents.front()));
            _outEvents.pop_front();
        }
        *pNumBytes = static_cast<DWORD>(recordCount * sizeof(INPUT_RECORD));
    }
    return retVal;
}
