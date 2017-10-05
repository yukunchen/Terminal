/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "writeData.hpp"

#include "_stream.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

// Routine Description:
// - Creates a new write data object for used in servicing write console requests
// Arguments:
// - psiContext - The output buffer to write text data to
// - pwchContext - The string information that the client application sent us to be written
// - cbContext - Byte count of the string above
// Return Value:
// - THROW: Throws if space cannot be allocated to copy the given string
WriteData::WriteData(_In_ SCREEN_INFORMATION* const psiContext,
                     _In_reads_bytes_(cbContext) wchar_t* const pwchContext,
                     _In_ ULONG const cbContext) :
    IWaitRoutine(ReplyDataType::Write),
    _psiContext(psiContext),
    _pwchContext(THROW_IF_NULL_ALLOC(reinterpret_cast<wchar_t*>(new byte[cbContext]))),
    _cbContext(cbContext)
{
    memmove(_pwchContext, pwchContext, _cbContext);
}

// Routine Description:
// - Destroys the write data object
// - Frees the string copy we made on creation
WriteData::~WriteData()
{
    if (nullptr != _pwchContext)
    {
        delete[] _pwchContext;
    }
}

// Routine Description:
// - Called back at a later time to resume the writing operation when the output object becomes unblocked.
// Arguments:
// - TerminationReason - if this routine is called because a ctrl-c or ctrl-break was seen, this argument
//                      contains CtrlC or CtrlBreak. If the owning thread is exiting, it will have ThreadDying. Otherwise 0.
// - fIsUnicode - Input data was in UCS-2 unicode or it needs to be converted with the current Output Codepage
// - pReplyStatus - The status code to return to the client application that originally called the API (before it was queued to wait)
// - pNumBytes - The number of bytes of data that the server/driver will need to transmit back to the client process
// - pControlKeyState - Unused for write operations. Set to 0.
// - pOutputData - not used.
// - TRUE if the wait is done and result buffer/status code can be sent back to the client.
// - FALSE if we need to continue to wait because the output object blocked again
BOOL WriteData::Notify(_In_ WaitTerminationReason const TerminationReason,
                       _In_ BOOLEAN const fIsUnicode,
                       _Out_ NTSTATUS* const pReplyStatus,
                       _Out_ DWORD* const pNumBytes,
                       _Out_ DWORD* const pControlKeyState,
                       _Out_ void* const pOutputData)
{
    UNREFERENCED_PARAMETER(fIsUnicode);
    UNREFERENCED_PARAMETER(pOutputData);
    *pNumBytes = _cbContext;
    *pControlKeyState = 0;

    if (IsFlagSet(TerminationReason, WaitTerminationReason::ThreadDying))
    {
        *pReplyStatus = STATUS_THREAD_IS_TERMINATING;
        return TRUE;
    }

    // if we get to here, this routine was called by the input
    // thread, which grabs the current console lock.

    // This routine should be called by a thread owning the same lock on the
    // same console as we're reading from.

    assert(ServiceLocator::LocateGlobals()->getConsoleInformation()->IsConsoleLocked());

    IWaitRoutine* pWaiter = nullptr;
    ULONG cbContext = _cbContext;
    NTSTATUS Status = DoWriteConsole(_pwchContext,
                                     &cbContext,
                                     _psiContext,
                                     &pWaiter);

    if (Status == CONSOLE_STATUS_WAIT)
    {
        // an extra waiter will be created by DoWriteConsole, but we're already a waiter so discard it.
        delete pWaiter;
        return FALSE;
    }

    *pNumBytes = cbContext;
    *pReplyStatus = Status;
    return TRUE;
}
