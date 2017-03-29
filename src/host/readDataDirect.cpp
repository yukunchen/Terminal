/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "readDataDirect.hpp"
#include "dbcs.h"
#include "misc.h"

// Routine Description:
// - Constructs direct read data class to hold context across sessions generally when there's not enough data to return
//   Also used a bit internally to just pass some information along the stack (regardless of wait necessity).
// Arguments:
// - pInputBuffer - Buffer that data will be read from.
// - pInputReadHandleData - Context stored across calls from the same input handle to return partial data appropriately.
// - pUserBuffer - The buffer the client has presented to be filled with input information
// - cUserBufferNumRecords - Max count of INPUT_RECORDs that we can stuff into the user's buffer
// - fIsPeek - Is this a peek operation? Meaning should we leave the records behind for the next consumer to read?
// Return Value:
// - THROW: Throws E_INVALIDARG for invalid pointers.
DIRECT_READ_DATA::DIRECT_READ_DATA(_In_ InputBuffer* const pInputBuffer,
                                   _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                   _In_ INPUT_RECORD* pUserBuffer,
                                   _In_ ULONG const cUserBufferNumRecords,
                                   _In_ bool const fIsPeek) :
    ReadData(pInputBuffer, pInputReadHandleData),
    _pUserBuffer{ THROW_HR_IF_NULL(E_INVALIDARG, pUserBuffer) },
    _cUserBufferNumRecords{  cUserBufferNumRecords },
    _fIsPeek{ fIsPeek }
{
}

// Routine Description:
// - Destructs a read data class.
// - Decrements count of readers waiting on the given handle.
DIRECT_READ_DATA::~DIRECT_READ_DATA()
{

}

// Routine Description:
// - This routine is called to complete a direct read that blocked in
//   ReadInputBuffer.  The context of the read was saved in the DirectReadData
//   structure.  This routine is called when events have been written to
//   the input buffer.  It is called in the context of the writing thread.
// Arguments:
// - TerminationReason - if this routine is called because a ctrl-c or ctrl-break was seen, this argument
//                      contains CtrlC or CtrlBreak. If the owning thread is exiting, it will have ThreadDying. Otherwise 0.
// - fIsUnicode - Should we return UCS-2 unicode data, or should we run the final data through the current Input Codepage before returning?
// - pReplyStatus - The status code to return to the client application that originally called the API (before it was queued to wait)
// - pNumBytes - The number of bytes of data that the server/driver will need to transmit back to the client process
// - pControlKeyState - For certain types of reads, this specifies which modifier keys were held.
// Return Value:
// - TRUE if the wait is done and result buffer/status code can be sent back to the client.
// - FALSE if we need to continue to wait until more data is available.
BOOL DIRECT_READ_DATA::Notify(_In_ WaitTerminationReason const TerminationReason,
                              _In_ BOOLEAN const fIsUnicode,
                              _Out_ NTSTATUS* const pReplyStatus,
                              _Out_ DWORD* const pNumBytes,
                              _Out_ DWORD* const pControlKeyState)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    BOOLEAN RetVal = TRUE;
    *pReplyStatus = STATUS_SUCCESS;
    *pControlKeyState = 0;

    ULONG NumRecords = _cUserBufferNumRecords;

    // If ctrl-c or ctrl-break was seen, ignore it.
    if (IsAnyFlagSet(TerminationReason, (WaitTerminationReason::CtrlC | WaitTerminationReason::CtrlBreak)))
    {
        return FALSE;
    }

    PINPUT_RECORD Buffer = nullptr;
    if (!fIsUnicode)
    {
        if (_pInputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
        {
            if (NumRecords == 1)
            {
                Buffer = _pUserBuffer;
                *Buffer = _pInputBuffer->ReadConInpDbcsLeadByte;
                if (!_fIsPeek)
                {
                    ZeroMemory(&_pInputBuffer->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
                }

                *pReplyStatus = STATUS_SUCCESS;
                *pNumBytes = sizeof(INPUT_RECORD);
            }
        }
    }

    BOOLEAN fAddDbcsLead = FALSE;
    // this routine should be called by a thread owning the same lock on the same console as we're reading from.
#ifdef DBG
    _pInputReadHandleData->LockReadCount();
    ASSERT(_pInputReadHandleData->GetReadCount() > 0);
    _pInputReadHandleData->UnlockReadCount();
#endif

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

        // this routine should be called by a thread owning the same
        // lock on the same console as we're reading from.

        ASSERT(g_ciConsoleInformation.IsConsoleLocked());

        Buffer = _pUserBuffer;

        PDWORD nLength = nullptr;

        g_ciConsoleInformation.ReadConInpNumBytesUnicode = NumRecords;
        if (!fIsUnicode)
        {
            // ASCII : a->NumRecords is ASCII byte count
            if (_pInputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
            {
                // Saved DBCS Traling byte
                if (g_ciConsoleInformation.ReadConInpNumBytesUnicode != 1)
                {
                    g_ciConsoleInformation.ReadConInpNumBytesUnicode--;
                    Buffer++;
                    fAddDbcsLead = TRUE;
                    nLength = &g_ciConsoleInformation.ReadConInpNumBytesUnicode;
                }
                else
                {
                    ASSERT(g_ciConsoleInformation.ReadConInpNumBytesUnicode == 1);
                }
            }
            else
            {
                nLength = &g_ciConsoleInformation.ReadConInpNumBytesUnicode;
            }
        }
        else
        {
            nLength = &NumRecords;
        }

        *pReplyStatus = _pInputBuffer->ReadInputBuffer(Buffer,
                                                       nLength,
                                                       _fIsPeek,
                                                       _fIsPeek,
                                                       fIsUnicode);
        if (*pReplyStatus == CONSOLE_STATUS_WAIT)
        {
            RetVal = FALSE;
        }
    }

    // If the read was completed (status != wait), free the direct read data.
    if (*pReplyStatus != CONSOLE_STATUS_WAIT)
    {
        if (*pReplyStatus == STATUS_SUCCESS && !fIsUnicode)
        {
            if (fAddDbcsLead && _pInputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
            {
                NumRecords--;
            }

            NumRecords = TranslateInputToOem(Buffer,
                                             NumRecords,
                                             g_ciConsoleInformation.ReadConInpNumBytesUnicode,
                                             _fIsPeek ? nullptr : &_pInputBuffer->ReadConInpDbcsLeadByte);
            if (fAddDbcsLead && _pInputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
            {
                *(Buffer - 1) = _pInputBuffer->ReadConInpDbcsLeadByte;
                if (!_fIsPeek)
                {
                    ZeroMemory(&_pInputBuffer->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
                }
                NumRecords++;
                Buffer--;
            }
        }

        *pNumBytes = NumRecords * sizeof(INPUT_RECORD);
    }

    return RetVal;
}
