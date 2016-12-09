/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "input.h"

#include "clipboard.hpp"
#include "cursor.h"
#include "dbcs.h"
#include "find.h"
#include "globals.h"
#include "handle.h"
#include "menu.h"
#include "output.h"
#include "scrolling.hpp"
#include "selection.hpp"
#include "srvinit.h"
#include "stream.h"
#include "telemetry.hpp"
#include "window.hpp"
#include "userprivapi.hpp"

#include "..\terminal\adapter\terminalInput.hpp"

#pragma hdrstop

#define CTRL_BUT_NOT_ALT(n) \
        (((n) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && \
        !((n) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))

#define MAX_CHARS_FROM_1_KEYSTROKE 6

// The following data structures are a hack to work around the fact that
// MapVirtualKey does not return the correct virtual key code in many cases.
// we store the correct info (from the keydown message) in the CONSOLE_KEY_INFO
// structure when a keydown message is translated. Then when we receive a
// wm_[sys][dead]char message, we retrieve it and clear out the record.

#define CONSOLE_FREE_KEY_INFO 0
#define CONSOLE_MAX_KEY_INFO 32

#define DEFAULT_NUMBER_OF_EVENTS 50
#define INPUT_BUFFER_SIZE_INCREMENT 10

#define KEY_ENHANCED 0x01000000
#define KEY_TRANSITION_UP 0x80000000

// indicates how much to change the opacity at each mouse/key toggle
// Opacity is defined as 0-255. 12 is therefore approximately 5% per tick.
#define OPACITY_DELTA_INTERVAL 12

NTSTATUS ReadBuffer(_In_ PINPUT_INFORMATION InputInformation,
                    _Out_writes_to_(Length, *EventsRead) PINPUT_RECORD Buffer,
                    _In_ ULONG Length,
                    _Out_ PULONG EventsRead,
                    _In_ BOOL Peek,
                    _In_ BOOL StreamRead,
                    _Out_ PBOOL ResetWaitEvent,
                    _In_ BOOLEAN Unicode);

NTSTATUS InitWindowsStuff();

bool IsInProcessedInputMode()
{
    return (g_ciConsoleInformation.pInputBuffer->InputMode & ENABLE_PROCESSED_INPUT) != 0;
}

bool IsInVirtualTerminalInputMode()
{
    return IsFlagSet(g_ciConsoleInformation.pInputBuffer->InputMode, ENABLE_VIRTUAL_TERMINAL_INPUT);
}

// Routine Description:
// - This routine creates an input buffer.  It allocates the circular buffer and initializes the information fields.
// Arguments:
// - pInputInfo - Pointer to input buffer information structure.
// Return Value:
NTSTATUS CreateInputBuffer(_In_opt_ ULONG cEvents, _Out_ PINPUT_INFORMATION pInputInfo)
{
    if (0 == cEvents)
    {
        cEvents = DEFAULT_NUMBER_OF_EVENTS;
    }

    // Allocate memory for circular buffer.
    ULONG uTemp;
    if (FAILED(DWordAdd(cEvents, 1, &uTemp)) || FAILED(DWordMult(sizeof(INPUT_RECORD), uTemp, &uTemp)))
    {
        cEvents = DEFAULT_NUMBER_OF_EVENTS;
    }

    ULONG const BufferSize = sizeof(INPUT_RECORD) * (cEvents + 1);
    pInputInfo->InputBuffer = (PINPUT_RECORD) new BYTE[BufferSize];
    if (pInputInfo->InputBuffer == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    pInputInfo->InputWaitEvent = g_hInputEvent.get();

    if (!NT_SUCCESS(Status))
    {
        delete[] pInputInfo->InputBuffer;
        return Status;
    }

    // initialize buffer header
    pInputInfo->InputBufferSize = cEvents;
    pInputInfo->InputMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
    pInputInfo->First = (ULONG_PTR) pInputInfo->InputBuffer;
    pInputInfo->In = (ULONG_PTR) pInputInfo->InputBuffer;
    pInputInfo->Out = (ULONG_PTR) pInputInfo->InputBuffer;
    pInputInfo->Last = (ULONG_PTR) pInputInfo->InputBuffer + BufferSize;
    pInputInfo->fInComposition = false;

    ZeroMemory(&pInputInfo->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
    ZeroMemory(&pInputInfo->WriteConInpDbcsLeadByte, sizeof(INPUT_RECORD));

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine resets the input buffer information fields to their initial values.
// Arguments:
// - InputBufferInformation - Pointer to input buffer information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void ReinitializeInputBuffer(_Inout_ PINPUT_INFORMATION pInputInfo)
{
    ResetEvent(pInputInfo->InputWaitEvent);

    pInputInfo->InputMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT;
    pInputInfo->In = (ULONG_PTR) pInputInfo->InputBuffer;
    pInputInfo->Out = (ULONG_PTR) pInputInfo->InputBuffer;
}

// Routine Description:
// - This routine frees the resources associated with an input buffer.
// Arguments:
// - InputBufferInformation - Pointer to input buffer information structure.
// Return Value:
void FreeInputBuffer(_In_ PINPUT_INFORMATION pInputInfo)
{
    CloseHandle(pInputInfo->InputWaitEvent);
    delete[] pInputInfo->InputBuffer;
    pInputInfo->InputBuffer = nullptr;
}

// Routine Description:
// - This routine waits for a writer to add data to the buffer.
// Arguments:
// - Console - Pointer to console buffer information.
// - pConsoleMsg - if called from dll (not InputThread), points to api
//             message.  this parameter is used for wait block processing.
//             We'll get the correct input object from this message.
// - pfnWaitRoutine - Routine to call when wait is woken up.
// - pvWaitParameter - Parameter to pass to wait routine.
// - cbWaitParameter - Length of wait parameter.
// - fWaitBlockExists - TRUE if wait block has already been created.
// Return Value:
// - STATUS_WAIT - call was from client and wait block has been created.
// - STATUS_SUCCESS - call was from server and wait has been satisfied.
NTSTATUS WaitForMoreToRead(_In_opt_ PCONSOLE_API_MSG pConsoleMsg,
                           _In_opt_ ConsoleWaitRoutine pfnWaitRoutine,
                           _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                           _In_ const ULONG cbWaitParameter,
                           _In_ const BOOLEAN fWaitBlockExists)
{
    if (!fWaitBlockExists)
    {
        PVOID const WaitParameterBuffer = new BYTE[cbWaitParameter];
        if (WaitParameterBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }
        memmove(WaitParameterBuffer, pvWaitParameter, cbWaitParameter);
        if (cbWaitParameter == sizeof(COOKED_READ_DATA) && g_ciConsoleInformation.lpCookedReadData == pvWaitParameter)
        {
            g_ciConsoleInformation.lpCookedReadData = (COOKED_READ_DATA*)WaitParameterBuffer;
        }

        HRESULT hr = ConsoleWaitQueue::s_CreateWait(pConsoleMsg, pfnWaitRoutine, WaitParameterBuffer);
        if (FAILED(hr))
        {
            delete[] WaitParameterBuffer;
            g_ciConsoleInformation.lpCookedReadData = nullptr;
            return NTSTATUS_FROM_HRESULT(hr);
        }
    }

    return CONSOLE_STATUS_WAIT;
}

// Routine Description:
// - This routine wakes up readers waiting for data to read.
// Arguments:
// - InputInformation - buffer to alert readers for
// Return Value:
// - TRUE - The operation was successful
// - FALSE/nullptr - The operation failed.
void WakeUpReadersWaitingForData(_In_ PINPUT_INFORMATION InputInformation)
{
    InputInformation->WaitQueue.NotifyWaiters(false);
}

// Routine Description:
// - This routine returns the number of events in the input buffer.
// Arguments:
// - pInputInfo - Pointer to input buffer information structure.
// - pcEvents - On output contains the number of events.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void GetNumberOfReadyEvents(_In_ const INPUT_INFORMATION * const pInputInfo, _Out_ ULONG * const pcEvents)
{
    if (pInputInfo->In < pInputInfo->Out)
    {
        *pcEvents = (ULONG)(pInputInfo->Last - pInputInfo->Out);
        *pcEvents += (ULONG)(pInputInfo->In - pInputInfo->First);
    }
    else
    {
        *pcEvents = (ULONG)(pInputInfo->In - pInputInfo->Out);
    }

    *pcEvents /= sizeof(INPUT_RECORD);
}

// Routine Description:
// - This routine removes all but the key events from the buffer.
// Arguments:
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS FlushAllButKeys()
{
    PINPUT_INFORMATION const InputInformation = g_ciConsoleInformation.pInputBuffer;

    if (InputInformation->In != InputInformation->Out)
    {
        ULONG BufferSize;
        // Allocate memory for temp buffer.
        if (FAILED(DWordMult(sizeof(INPUT_RECORD), InputInformation->InputBufferSize + 1, &BufferSize)))
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        PINPUT_RECORD TmpInputBuffer = (PINPUT_RECORD) new BYTE[BufferSize];
        if (TmpInputBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }
        PINPUT_RECORD const TmpInputBufferPtr = TmpInputBuffer;

        // copy input buffer. let ReadBuffer do any compaction work.
        ULONG NumberOfEventsRead;
        BOOL Dummy;
        NTSTATUS Status = ReadBuffer(InputInformation, TmpInputBuffer, InputInformation->InputBufferSize, &NumberOfEventsRead, TRUE, FALSE, &Dummy, TRUE);

        if (!NT_SUCCESS(Status))
        {
            delete[] TmpInputBuffer;
            return Status;
        }

        InputInformation->Out = (ULONG_PTR) InputInformation->InputBuffer;
        PINPUT_RECORD InPtr = InputInformation->InputBuffer;
        for (ULONG i = 0; i < NumberOfEventsRead; i++)
        {
            // Prevent running off the end of the buffer even though ReadBuffer will surely make this shorter than when we started.
            // We have to leave one free segment at the end for the In to point to when we're done.
            if (InPtr >= (InputInformation->InputBuffer + InputInformation->InputBufferSize - 1))
            {
                break;
            }

            if (TmpInputBuffer->EventType == KEY_EVENT)
            {
                *InPtr = *TmpInputBuffer;
                InPtr++;
            }

            TmpInputBuffer++;
        }

        InputInformation->In = (ULONG_PTR) InPtr;
        if (InputInformation->In == InputInformation->Out)
        {
            ResetEvent(InputInformation->InputWaitEvent);
        }

        delete[] TmpInputBufferPtr;
    }

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine empties the input buffer
// Arguments:
// - InputInformation - Pointer to input buffer information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void FlushInputBuffer(_Inout_ PINPUT_INFORMATION pInputInfo)
{
    pInputInfo->In = (ULONG_PTR) pInputInfo->InputBuffer;
    pInputInfo->Out = (ULONG_PTR) pInputInfo->InputBuffer;
    ResetEvent(pInputInfo->InputWaitEvent);
}

// Routine Description:
// - This routine resizes the input buffer.
// Arguments:
// - InputInformation - Pointer to input buffer information structure.
// - Size - New size in number of events.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS SetInputBufferSize(_Inout_ PINPUT_INFORMATION InputInformation, _In_ ULONG Size)
{
#if DBG
    ULONG_PTR NumberOfEvents;
    if (InputInformation->In < InputInformation->Out)
    {
        NumberOfEvents = InputInformation->Last - InputInformation->Out;
        NumberOfEvents += InputInformation->In - InputInformation->First;
    }
    else
    {
        NumberOfEvents = InputInformation->In - InputInformation->Out;
    }
    NumberOfEvents /= sizeof(INPUT_RECORD);
#endif
    ASSERT(Size > InputInformation->InputBufferSize);

    size_t BufferSize;
    // Allocate memory for new input buffer.
    if (FAILED(SizeTMult(sizeof(INPUT_RECORD), Size + 1, &BufferSize)))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    PINPUT_RECORD const InputBuffer = (PINPUT_RECORD) new BYTE[BufferSize];
    if (InputBuffer == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    // Copy old input buffer. Let the ReadBuffer do any compaction work.
    ULONG NumberOfEventsRead;
    BOOL Dummy;
    NTSTATUS Status = ReadBuffer(InputInformation, InputBuffer, Size, &NumberOfEventsRead, TRUE, FALSE, &Dummy, TRUE);

    if (!NT_SUCCESS(Status))
    {
        delete[] InputBuffer;
        return Status;
    }
    InputInformation->Out = (ULONG_PTR) InputBuffer;
    InputInformation->In = (ULONG_PTR) InputBuffer + sizeof(INPUT_RECORD) * NumberOfEventsRead;

    // adjust pointers
    InputInformation->First = (ULONG_PTR) InputBuffer;
    InputInformation->Last = (ULONG_PTR) InputBuffer + BufferSize;

    // free old input buffer
    delete[] InputInformation->InputBuffer;
    InputInformation->InputBufferSize = Size;
    InputInformation->InputBuffer = InputBuffer;

    return Status;
}

// Routine Description:
// - This routine reads from a buffer.  It does the actual circular buffer manipulation.
// Arguments:
// - InputInformation - buffer to read from
// - Buffer - buffer to read into
// - Length - length of buffer in events
// - EventsRead - where to store number of events read
// - Peek - if TRUE, don't remove data from buffer, just copy it.
// - StreamRead - if TRUE, events with repeat counts > 1 are returned as multiple events.  also, EventsRead == 1.
// - ResetWaitEvent - on exit, TRUE if buffer became empty.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS
ReadBuffer(_In_ PINPUT_INFORMATION InputInformation,
           _Out_writes_to_(Length, *EventsRead) PINPUT_RECORD Buffer,
           _In_ ULONG Length,
           _Out_ PULONG EventsRead,
           _In_ BOOL Peek,
           _In_ BOOL StreamRead,
           _Out_ PBOOL ResetWaitEvent,
           _In_ BOOLEAN Unicode)
{
    *ResetWaitEvent = FALSE;

    // If StreamRead, just return one record. If repeat count is greater than
    // one, just decrement it. The repeat count is > 1 if more than one event
    // of the same type was merged. We need to expand them back to individual
    // events here.
    if (StreamRead && ((PINPUT_RECORD) (InputInformation->Out))->EventType == KEY_EVENT)
    {

        ASSERT(Length == 1);
        ASSERT(InputInformation->In != InputInformation->Out);
        memmove((PBYTE) Buffer, (PBYTE) InputInformation->Out, sizeof(INPUT_RECORD));
        InputInformation->Out += sizeof(INPUT_RECORD);
        if (InputInformation->Last == InputInformation->Out)
        {
            InputInformation->Out = InputInformation->First;
        }

        if (InputInformation->Out == InputInformation->In)
        {
            *ResetWaitEvent = TRUE;
        }

        *EventsRead = 1;
        return STATUS_SUCCESS;
    }

    ULONG BufferLengthInBytes = Length * sizeof(INPUT_RECORD);
    ULONG TransferLength, OldTransferLength;
    ULONG Length2;
    PINPUT_RECORD BufferRecords = nullptr;
    PINPUT_RECORD QueueRecords;
    WCHAR UniChar;
    WORD EventType;

    // if in > out, buffer looks like this:
    //
    //         out     in
    //    ______ _____________
    //   |      |      |      |
    //   | free | data | free |
    //   |______|______|______|
    //
    // we transfer the requested number of events or the amount in the buffer
    if (InputInformation->In > InputInformation->Out)
    {
        if ((InputInformation->In - InputInformation->Out) > BufferLengthInBytes)
        {
            TransferLength = BufferLengthInBytes;
        }
        else
        {
            TransferLength = (ULONG) (InputInformation->In - InputInformation->Out);
        }
        if (!Unicode)
        {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            BufferRecords = (PINPUT_RECORD) Buffer;
            QueueRecords = (PINPUT_RECORD) InputInformation->Out;

            while (BufferLengthInBytes < Length && OldTransferLength)
            {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT)
                {
                    if (IsCharFullWidth(UniChar))
                    {
                        BufferLengthInBytes += 2;
                    }
                    else
                    {
                        BufferLengthInBytes++;
                    }
                }
                else
                {
                    BufferLengthInBytes++;
                }

                OldTransferLength--;
            }

            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
        {
            memmove((PBYTE) Buffer, (PBYTE) InputInformation->Out, TransferLength);
        }

        *EventsRead = TransferLength / sizeof(INPUT_RECORD);
        ASSERT(*EventsRead <= Length);

        if (!Peek)
        {
            InputInformation->Out += TransferLength;
            ASSERT(InputInformation->Out <= InputInformation->Last);
        }

        if (InputInformation->Out == InputInformation->In)
        {
            *ResetWaitEvent = TRUE;
        }

        return STATUS_SUCCESS;
    }

    // if out > in, buffer looks like this:
    //
    //         in     out
    //    ______ _____________
    //   |      |      |      |
    //   | data | free | data |
    //   |______|______|______|
    //
    // we read from the out pointer to the end of the buffer then from the
    // beginning of the buffer, until we hit the in pointer or enough bytes
    // are read.
    else
    {
        if ((InputInformation->Last - InputInformation->Out) > BufferLengthInBytes)
        {
            TransferLength = BufferLengthInBytes;
        }
        else
        {
            TransferLength = (ULONG) (InputInformation->Last - InputInformation->Out);
        }

        if (!Unicode)
        {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            BufferRecords = (PINPUT_RECORD) Buffer;
            QueueRecords = (PINPUT_RECORD) InputInformation->Out;

            while (BufferLengthInBytes < Length && OldTransferLength)
            {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT)
                {
                    if (IsCharFullWidth(UniChar))
                    {
                        BufferLengthInBytes += 2;
                    }
                    else
                    {
                        BufferLengthInBytes++;
                    }
                }
                else
                {
                    BufferLengthInBytes++;
                }

                OldTransferLength--;
            }

            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
        {
            memmove((PBYTE) Buffer, (PBYTE) InputInformation->Out, TransferLength);
        }

        *EventsRead = TransferLength / sizeof(INPUT_RECORD);
        ASSERT(*EventsRead <= Length);

        if (!Peek)
        {
            InputInformation->Out += TransferLength;
            ASSERT(InputInformation->Out <= InputInformation->Last);
            if (InputInformation->Out == InputInformation->Last)
            {
                InputInformation->Out = InputInformation->First;
            }
        }

        if (!Unicode)
        {
            if (BufferLengthInBytes >= Length)
            {
                if (InputInformation->Out == InputInformation->In)
                {
                    *ResetWaitEvent = TRUE;
                }
                return STATUS_SUCCESS;
            }
        }
        else if (*EventsRead == Length)
        {
            if (InputInformation->Out == InputInformation->In)
            {
                *ResetWaitEvent = TRUE;
            }

            return STATUS_SUCCESS;
        }

        // hit end of buffer, read from beginning
        OldTransferLength = TransferLength;
        Length2 = Length;
        if (!Unicode)
        {
            ASSERT(Length > BufferLengthInBytes);
            Length -= BufferLengthInBytes;
            if (Length == 0)
            {
                if (InputInformation->Out == InputInformation->In)
                {
                    *ResetWaitEvent = TRUE;
                }
                return STATUS_SUCCESS;
            }
            BufferLengthInBytes = Length * sizeof(INPUT_RECORD);

            if ((InputInformation->In - InputInformation->First) > BufferLengthInBytes)
            {
                TransferLength = BufferLengthInBytes;
            }
            else
            {
                TransferLength = (ULONG) (InputInformation->In - InputInformation->First);
            }
        }
        else if ((InputInformation->In - InputInformation->First) > (BufferLengthInBytes - OldTransferLength))
        {
            TransferLength = BufferLengthInBytes - OldTransferLength;
        }
        else
        {
            TransferLength = (ULONG) (InputInformation->In - InputInformation->First);
        }
        if (!Unicode)
        {
            BufferLengthInBytes = 0;
            OldTransferLength = TransferLength / sizeof(INPUT_RECORD);
            QueueRecords = (PINPUT_RECORD) InputInformation->First;

            while (BufferLengthInBytes < Length && OldTransferLength)
            {
                UniChar = QueueRecords->Event.KeyEvent.uChar.UnicodeChar;
                EventType = QueueRecords->EventType;
                *BufferRecords++ = *QueueRecords++;
                if (EventType == KEY_EVENT)
                {
                    if (IsCharFullWidth(UniChar))
                    {
                        BufferLengthInBytes += 2;
                    }
                    else
                    {
                        BufferLengthInBytes++;
                    }
                }
                else
                {
                    BufferLengthInBytes++;
                }
                OldTransferLength--;
            }

            ASSERT(TransferLength >= OldTransferLength * sizeof(INPUT_RECORD));
            TransferLength -= OldTransferLength * sizeof(INPUT_RECORD);
        }
        else
        {
            memmove((PBYTE) Buffer + OldTransferLength, (PBYTE) InputInformation->First, TransferLength);
        }

        *EventsRead += TransferLength / sizeof(INPUT_RECORD);
        ASSERT(*EventsRead <= Length2);

        if (!Peek)
        {
            InputInformation->Out = InputInformation->First + TransferLength;
        }

        if (InputInformation->Out == InputInformation->In)
        {
            *ResetWaitEvent = TRUE;
        }

        return STATUS_SUCCESS;
    }
}

// Routine Description:
// - This routine reads from the input buffer.
// Arguments:
// - pInputInfo - Pointer to input buffer information structure.
// - pInputRecord - Buffer to read into.
// - pcLength - On input, number of events to read.  On output, number of events read.
// - fPeek - If TRUE, copy events to pInputRecord but don't remove them from the input buffer.
// - fWaitForData - if TRUE, wait until an event is input.  if FALSE, return immediately
// - fStreamRead - if TRUE, events with repeat counts > 1 are returned as multiple events.  also, EventsRead == 1.
// - pHandleData - Pointer to handle data structure.  This parameter is optional if fWaitForData is false.
// - pConsoleMsg - if called from dll (not InputThread), points to api message.  this parameter is used for wait block processing.
// - pfnWaitRoutine - Routine to call when wait is woken up.
// - pvWaitParameter - Parameter to pass to wait routine.
// - cbWaitParameter - Length of wait parameter.
// - fWaitBlockExists - TRUE if wait block has already been created.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS ReadInputBuffer(_In_ PINPUT_INFORMATION const pInputInfo,
                         _Out_writes_(*pcLength) PINPUT_RECORD pInputRecord,
                         _Inout_ PDWORD pcLength,
                         _In_ BOOL const fPeek,
                         _In_ BOOL const fWaitForData,
                         _In_ BOOL const fStreamRead,
                         _In_ INPUT_READ_HANDLE_DATA* pHandleData,
                         _In_opt_ PCONSOLE_API_MSG pConsoleMsg,
                         _In_opt_ ConsoleWaitRoutine pfnWaitRoutine,
                         _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                         _In_ ULONG const cbWaitParameter,
                         _In_ BOOLEAN const fWaitBlockExists,
                         _In_ BOOLEAN const fUnicode)
{
    NTSTATUS Status;
    if (pInputInfo->In == pInputInfo->Out)
    {
        if (!fWaitForData)
        {
            *pcLength = 0;
            return STATUS_SUCCESS;
        }

        pHandleData->IncrementReadCount();
        Status = WaitForMoreToRead(pConsoleMsg, pfnWaitRoutine, pvWaitParameter, cbWaitParameter, fWaitBlockExists);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                // WaitForMoreToRead failed, restore ReadCount and bail out
                pHandleData->DecrementReadCount();
            }

            *pcLength = 0;
            return Status;
        }
    }

    // read from buffer
    ULONG EventsRead;
    BOOL ResetWaitEvent;
    Status = ReadBuffer(pInputInfo, pInputRecord, *pcLength, &EventsRead, fPeek, fStreamRead, &ResetWaitEvent, fUnicode);
    if (ResetWaitEvent)
    {
        ResetEvent(pInputInfo->InputWaitEvent);
    }

    *pcLength = EventsRead;
    return Status;
}

// Routine Description:
// - This routine writes to a buffer.  It does the actual circular buffer manipulation.
// Arguments:
// - InputInformation - buffer to write to
// - Buffer - buffer to write from
// - Length - length of buffer in events
// - EventsWritten - where to store number of events written.
// - SetWaitEvent - on exit, TRUE if buffer became non-empty.
// Return Value:
// - ERROR_BROKEN_PIPE - no more readers.
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS WriteBuffer(_Inout_ PINPUT_INFORMATION InputInformation, _In_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG EventsWritten, _Out_ PBOOL SetWaitEvent)
{
    NTSTATUS Status;
    ULONG TransferLength;
    ULONG BufferLengthInBytes;

    *SetWaitEvent = FALSE;

    // windows sends a mouse_move message each time a window is updated.
    // coalesce these.
    if (Length == 1 && InputInformation->Out != InputInformation->In)
    {
        PINPUT_RECORD InputEvent = (PINPUT_RECORD) Buffer;

        if (InputEvent->EventType == MOUSE_EVENT && InputEvent->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
        {
            PINPUT_RECORD LastInputEvent;

            if (InputInformation->In == InputInformation->First)
            {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->Last - sizeof(INPUT_RECORD));
            }
            else
            {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->In - sizeof(INPUT_RECORD));
            }
            if (LastInputEvent->EventType == MOUSE_EVENT && LastInputEvent->Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
            {
                LastInputEvent->Event.MouseEvent.dwMousePosition.X = InputEvent->Event.MouseEvent.dwMousePosition.X;
                LastInputEvent->Event.MouseEvent.dwMousePosition.Y = InputEvent->Event.MouseEvent.dwMousePosition.Y;
                *EventsWritten = 1;
                return STATUS_SUCCESS;
            }
        }
        else if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown)
        {
            PINPUT_RECORD LastInputEvent;
            if (InputInformation->In == InputInformation->First)
            {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->Last - sizeof(INPUT_RECORD));
            }
            else
            {
                LastInputEvent = (PINPUT_RECORD) (InputInformation->In - sizeof(INPUT_RECORD));
            }

            if (IsCharFullWidth(InputEvent->Event.KeyEvent.uChar.UnicodeChar))
            {
                /* do nothing */ ;
            }
            else if (InputEvent->Event.KeyEvent.dwControlKeyState & NLS_IME_CONVERSION)
            {
                if (LastInputEvent->EventType == KEY_EVENT &&
                    LastInputEvent->Event.KeyEvent.bKeyDown &&
                    (LastInputEvent->Event.KeyEvent.uChar.UnicodeChar ==
                     InputEvent->Event.KeyEvent.uChar.UnicodeChar) && (LastInputEvent->Event.KeyEvent.dwControlKeyState == InputEvent->Event.KeyEvent.dwControlKeyState))
                {
                    LastInputEvent->Event.KeyEvent.wRepeatCount += InputEvent->Event.KeyEvent.wRepeatCount;
                    *EventsWritten = 1;
                    return STATUS_SUCCESS;
                }
            }
            else if (LastInputEvent->EventType == KEY_EVENT && LastInputEvent->Event.KeyEvent.bKeyDown && (LastInputEvent->Event.KeyEvent.wVirtualScanCode ==   // scancode same
                                                                                                           InputEvent->Event.KeyEvent.wVirtualScanCode) && (LastInputEvent->Event.KeyEvent.uChar.UnicodeChar == // character same
                                                                                                                                                            InputEvent->Event.KeyEvent.uChar.UnicodeChar) && (LastInputEvent->Event.KeyEvent.dwControlKeyState ==   // ctrl/alt/shift state same
                                                                                                                                                                                                              InputEvent->
                                                                                                                                                                                                              Event.
                                                                                                                                                                                                              KeyEvent.
                                                                                                                                                                                                              dwControlKeyState))
            {
                LastInputEvent->Event.KeyEvent.wRepeatCount += InputEvent->Event.KeyEvent.wRepeatCount;
                *EventsWritten = 1;
                return STATUS_SUCCESS;
            }
        }
    }

    BufferLengthInBytes = Length * sizeof(INPUT_RECORD);
    *EventsWritten = 0;
    while (*EventsWritten < Length)
    {

        // if out > in, buffer looks like this:
        //
        //             in     out
        //        ______ _____________
        //       |      |      |      |
        //       | data | free | data |
        //       |______|______|______|
        //
        // we can write from in to out-1
        if (InputInformation->Out > InputInformation->In)
        {
            TransferLength = BufferLengthInBytes;
            if ((InputInformation->Out - InputInformation->In - sizeof(INPUT_RECORD)) < BufferLengthInBytes)
            {
                Status = SetInputBufferSize(InputInformation, InputInformation->InputBufferSize + Length + INPUT_BUFFER_SIZE_INCREMENT);
                if (!NT_SUCCESS(Status))
                {
                    RIPMSG1(RIP_WARNING, "Couldn't grow input buffer, Status == 0x%x", Status);
                    TransferLength = (ULONG) (InputInformation->Out - InputInformation->In - sizeof(INPUT_RECORD));
                    if (TransferLength == 0)
                    {
                        return Status;
                    }
                }
                else
                {
                    goto OutPath;   // after resizing, in > out
                }
            }
            memmove((PBYTE) InputInformation->In, (PBYTE) Buffer, TransferLength);
            Buffer = (PVOID) (((PBYTE) Buffer) + TransferLength);
            *EventsWritten += TransferLength / sizeof(INPUT_RECORD);
            BufferLengthInBytes -= TransferLength;
            InputInformation->In += TransferLength;
        }

        // if in >= out, buffer looks like this:
        //
        //             out     in
        //        ______ _____________
        //       |      |      |      |
        //       | free | data | free |
        //       |______|______|______|
        //
        // we write from the in pointer to the end of the buffer then from the
        // beginning of the buffer, until we hit the out pointer or enough bytes
        // are written.
        else
        {
            if (InputInformation->Out == InputInformation->In)
            {
                *SetWaitEvent = TRUE;
            }
OutPath:
            if ((InputInformation->Last - InputInformation->In) > BufferLengthInBytes)
            {
                TransferLength = BufferLengthInBytes;
            }
            else
            {
                if (InputInformation->First == InputInformation->Out && InputInformation->In == (InputInformation->Last - sizeof(INPUT_RECORD)))
                {
                    TransferLength = BufferLengthInBytes;
                    Status = SetInputBufferSize(InputInformation, InputInformation->InputBufferSize + Length + INPUT_BUFFER_SIZE_INCREMENT);
                    if (!NT_SUCCESS(Status))
                    {
                        RIPMSG1(RIP_WARNING, "Couldn't grow input buffer, Status == 0x%x", Status);
                        return Status;
                    }
                }
                else
                {
                    TransferLength = (ULONG) (InputInformation->Last - InputInformation->In);
                    if (InputInformation->First == InputInformation->Out)
                    {
                        TransferLength -= sizeof(INPUT_RECORD);
                    }
                }
            }
            memmove((PBYTE) InputInformation->In, (PBYTE) Buffer, TransferLength);
            Buffer = (PVOID) (((PBYTE) Buffer) + TransferLength);
            *EventsWritten += TransferLength / sizeof(INPUT_RECORD);
            BufferLengthInBytes -= TransferLength;
            InputInformation->In += TransferLength;
            if (InputInformation->In == InputInformation->Last)
            {
                InputInformation->In = InputInformation->First;
            }
        }
        if (TransferLength == 0)
        {
            ASSERT(FALSE);
        }
    }
    return STATUS_SUCCESS;
}

BOOL IsSystemKey(_In_ WORD const wVirtualKeyCode)
{
    switch (wVirtualKeyCode)
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_PAUSE:
        case VK_CAPITAL:
        case VK_LWIN:
        case VK_RWIN:
        case VK_NUMLOCK:
        case VK_SCROLL:
            return TRUE;
    }
    return FALSE;
}

// Routine Description:
// - This routine processes special characters in the input stream.
// Arguments:
// - Console - Pointer to console structure.
// - InputEvent - Buffer to write from.
// - nLength - Number of events to write.
// Return Value:
// - Number of events to write after special characters have been stripped.
// Note:
// - The console lock must be held when calling this routine.
DWORD PreprocessInput(_In_ PINPUT_RECORD InputEvent, _In_ DWORD nLength)
{
    for (ULONG NumEvents = nLength; NumEvents != 0; NumEvents--)
    {
        if (InputEvent->EventType == KEY_EVENT && InputEvent->Event.KeyEvent.bKeyDown)
        {
            // if output is suspended, any keyboard input releases it.
            if ((g_ciConsoleInformation.Flags & CONSOLE_SUSPENDED) && !IsSystemKey(InputEvent->Event.KeyEvent.wVirtualKeyCode))
            {

                UnblockWriteConsole(CONSOLE_OUTPUT_SUSPENDED);
                memmove(InputEvent, InputEvent + 1, (NumEvents - 1) * sizeof(INPUT_RECORD));
                nLength--;
                continue;
            }

            // intercept control-s
            if ((g_ciConsoleInformation.pInputBuffer->InputMode & ENABLE_LINE_INPUT) &&
                (InputEvent->Event.KeyEvent.wVirtualKeyCode == VK_PAUSE || IsPauseKey(&InputEvent->Event.KeyEvent)))
            {

                g_ciConsoleInformation.Flags |= CONSOLE_OUTPUT_SUSPENDED;
                memmove(InputEvent, InputEvent + 1, (NumEvents - 1) * sizeof(INPUT_RECORD));
                nLength--;
                continue;
            }
        }
        InputEvent++;
    }
    return nLength;
}

// Routine Description:
// -  This routine writes to the beginning of the input buffer.
// Arguments:
// - pInputInfo - Pointer to input buffer information structure.
// - pInputRecord - Buffer to write from.
// - cInputRecords - On input, number of events to write.  On output, number of events written.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
NTSTATUS PrependInputBuffer(_In_ PINPUT_INFORMATION pInputInfo, _In_ PINPUT_RECORD pInputRecord, _Inout_ DWORD * const pcLength)
{
    DWORD cInputRecords = *pcLength;
    cInputRecords = PreprocessInput(pInputRecord, cInputRecords);
    if (cInputRecords == 0)
    {
        return STATUS_SUCCESS;
    }

    ULONG NumExistingEvents;
    GetNumberOfReadyEvents(pInputInfo, &NumExistingEvents);

    PINPUT_RECORD pExistingEvents;
    ULONG EventsRead = 0;
    BOOL Dummy;
    if (NumExistingEvents)
    {
        ULONG NumBytes;

        if (FAILED(ULongMult(NumExistingEvents, sizeof(INPUT_RECORD), &NumBytes)))
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        pExistingEvents = (PINPUT_RECORD) new BYTE[NumBytes];
        if (pExistingEvents == nullptr)
        {
            return STATUS_NO_MEMORY;
        }

        NTSTATUS Status = ReadBuffer(pInputInfo, pExistingEvents, NumExistingEvents, &EventsRead, FALSE, FALSE, &Dummy, TRUE);
        if (!NT_SUCCESS(Status))
        {
            delete[] pExistingEvents;
            return Status;
        }
    }
    else
    {
        pExistingEvents = nullptr;
    }

    ULONG EventsWritten;
    BOOL SetWaitEvent;
    // write new info to buffer
    WriteBuffer(pInputInfo, pInputRecord, cInputRecords, &EventsWritten, &SetWaitEvent);

    // Write existing info to buffer.
    if (pExistingEvents)
    {
        WriteBuffer(pInputInfo, pExistingEvents, EventsRead, &EventsWritten, &Dummy);
        delete[] pExistingEvents;
    }

    if (SetWaitEvent)
    {
        SetEvent(pInputInfo->InputWaitEvent);
    }

    // alert any writers waiting for space
    WakeUpReadersWaitingForData(pInputInfo);

    *pcLength = cInputRecords;
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine writes to the input buffer.
// Arguments:
// - pInputInfo - Pointer to input buffer information structure.
// - pInputRecord - Buffer to write from.
// - cInputRecords - On input, number of events to write.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
DWORD WriteInputBuffer(_In_ PINPUT_INFORMATION pInputInfo, _In_ PINPUT_RECORD pInputRecord, _In_ DWORD cInputRecords)
{
    cInputRecords = PreprocessInput(pInputRecord, cInputRecords);
    if (cInputRecords == 0)
    {
        return 0;
    }

    // Write to buffer.
    ULONG EventsWritten;
    BOOL SetWaitEvent;
    WriteBuffer(pInputInfo, pInputRecord, cInputRecords, &EventsWritten, &SetWaitEvent);

    if (SetWaitEvent)
    {
        SetEvent(pInputInfo->InputWaitEvent);
    }

    // Alert any writers waiting for space.
    WakeUpReadersWaitingForData(pInputInfo);

    return EventsWritten;
}

NTSTATUS InitWindowsSubsystem()
{
    g_hInstance = GetModuleHandle(L"ConhostV2.dll");

    ConsoleProcessHandle* ProcessData = g_ciConsoleInformation.ProcessHandleList.FindProcessInList(ConsoleProcessList::ROOT_PROCESS_ID);
    ASSERT(ProcessData != nullptr && ProcessData->fRootProcess);

    // Create and activate the main window
    NTSTATUS Status = Window::CreateInstance(&g_ciConsoleInformation, g_ciConsoleInformation.ScreenBuffers, &g_ciConsoleInformation.pWindow);

    if (!NT_SUCCESS(Status))
    {
        RIPMSG2(RIP_WARNING, "CreateWindowsWindow failed with status 0x%x, gle = 0x%x", Status, GetLastError());
        return Status;
    }

    SetConsoleWindowOwner(g_ciConsoleInformation.pWindow->GetWindowHandle(), ProcessData);

    g_ciConsoleInformation.pWindow->ActivateAndShow(g_ciConsoleInformation.GetShowWindow());

    NotifyWinEvent(EVENT_CONSOLE_START_APPLICATION, g_ciConsoleInformation.hWnd, ProcessData->dwProcessId, 0);

    return STATUS_SUCCESS;
}

// Routine Description:
// - Ensures the SxS initialization for the process.
void InitSideBySide(_Out_writes_(ScratchBufferSize) PWSTR ScratchBuffer, __range(MAX_PATH, MAX_PATH) DWORD ScratchBufferSize)
{
    ACTCTXW actctx = { 0 };

    ASSERT(ScratchBufferSize >= MAX_PATH);

    // Account for the fact that sidebyside stuff happens in CreateProcess
    // but conhost is run with RtlCreateUserProcess.

    // If conhost is at some future date
    // launched with CreateProcess or SideBySide support moved
    // into the kernel and SideBySide setup moved to textmode, at which
    // time this code block will not be needed.

    // Until then, this code block is needed when activated as the default console in the OS by the loader.
    // If the console is changed to be invoked a different way (for example if we add a main method that takes
    // a parameter to a client application instead), then this code would be unnecessary but not likely harmful.

    // Having SxS not initialized is a problem when 3rd party IMEs attempt to inject into the process and then
    // make references to DLLs in the system that are in the SxS cache (ex. a 3rd party IME is loaded and asks for
    // comctl32.dll. The load will fail if SxS wasn't initialized.) This was bug# WIN7:681280.

    // We look at the first few chars without being careful about a terminal nul, so init them.
    ScratchBuffer[0] = 0;
    ScratchBuffer[1] = 0;
    ScratchBuffer[2] = 0;
    ScratchBuffer[3] = 0;
    ScratchBuffer[4] = 0;
    ScratchBuffer[5] = 0;
    ScratchBuffer[6] = 0;

    // GetModuleFileNameW truncates its result to fit in the buffer, so to detect if we fit, we have to do this.
    ScratchBuffer[ScratchBufferSize - 2] = 0;
    DWORD const dwModuleFileNameLength = GetModuleFileNameW(nullptr, ScratchBuffer, ScratchBufferSize);
    if (dwModuleFileNameLength == 0)
    {
        RIPMSG1(RIP_ERROR, "GetModuleFileNameW failed %d.\n", GetLastError());
        goto Exit;
    }
    if (ScratchBuffer[ScratchBufferSize - 2] != 0)
    {
        RIPMSG1(RIP_ERROR, "GetModuleFileNameW requires more than ScratchBufferSize(%d) - 1.\n", ScratchBufferSize);
        goto Exit;
    }

    // We get an NT path from the Win32 api. Fix it to be Win32.
    UINT NtToWin32PathOffset = 0;
    if (ScratchBuffer[0] == '\\' && ScratchBuffer[1] == '?' && ScratchBuffer[2] == '?' && ScratchBuffer[3] == '\\'
        //&& ScratchBuffer[4] == a drive letter
        && ScratchBuffer[5] == ':' && ScratchBuffer[6] == '\\')
    {
        NtToWin32PathOffset = 4;
    }

    actctx.cbSize = sizeof(actctx);
    actctx.dwFlags = (ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_SET_PROCESS_DEFAULT);
    actctx.lpResourceName = MAKEINTRESOURCE(IDR_SYSTEM_MANIFEST);
    actctx.lpSource = ScratchBuffer + NtToWin32PathOffset;

    HANDLE const hActCtx = CreateActCtxW(&actctx);

    // The error value is INVALID_HANDLE_VALUE.
    // ACTCTX_FLAG_SET_PROCESS_DEFAULT has nothing to return upon success, so it returns nullptr.
    // There is nothing to cleanup upon ACTCTX_FLAG_SET_PROCESS_DEFAULT success, the data
    // is referenced in the PEB, and lasts till process shutdown.
    ASSERT(hActCtx == nullptr || hActCtx == INVALID_HANDLE_VALUE);
    if (hActCtx == INVALID_HANDLE_VALUE)
    {
        RIPMSG1(RIP_WARNING, "InitSideBySide failed create an activation context. Error: %d", GetLastError());
        goto Exit;
    }

Exit:
    ScratchBuffer[0] = 0;
}

// Routine Description:
// - Sets the program files environment variables for the process, if missing.
void InitEnvironmentVariables()
{
    struct
    {
        LPCWSTR szRegValue;
        LPCWSTR szVariable;
    } EnvProgFiles[] =
    {
        {
        L"ProgramFilesDir", L"ProgramFiles"},
        {
        L"CommonFilesDir", L"CommonProgramFiles"},
#if BUILD_WOW64_ENABLED
        {
        L"ProgramFilesDir (x86)", L"ProgramFiles(x86)"},
        {
        L"CommonFilesDir (x86)", L"CommonProgramFiles(x86)"},
        {
        L"ProgramW6432Dir", L"ProgramW6432"},
        {
        L"CommonW6432Dir", L"CommonProgramW6432"}
#endif
    };

    WCHAR wchValue[MAX_PATH];
    for (UINT i = 0; i < ARRAYSIZE(EnvProgFiles); i++)
    {
        if (!GetEnvironmentVariable(EnvProgFiles[i].szVariable, nullptr, 0))
        {
            DWORD dwMaxBufferSize = sizeof(wchValue);
            if (RegGetValue(HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows\\CurrentVersion", EnvProgFiles[i].szRegValue, RRF_RT_REG_SZ, nullptr, (LPBYTE) wchValue, &dwMaxBufferSize) == ERROR_SUCCESS)
            {
                wchValue[(dwMaxBufferSize / sizeof(wchValue[0])) - 1] = 0;
                SetEnvironmentVariable(EnvProgFiles[i].szVariable, wchValue);
            }
        }
    }

    // Initialize SxS for the process.
    InitSideBySide(wchValue, ARRAYSIZE(wchValue));
}

DWORD ConsoleInputThread(LPVOID /*lpParameter*/)
{
    InitEnvironmentVariables();

    LockConsole();
    NTSTATUS Status = InitWindowsSubsystem();
    UnlockConsole();
    if (!NT_SUCCESS(Status))
    {
        g_ntstatusConsoleInputInitStatus = Status;
        g_hConsoleInputInitEvent.SetEvent();
        return Status;
    }

    g_hConsoleInputInitEvent.SetEvent();

    for (;;)
    {
        MSG msg;
        if (GetMessageW(&msg, nullptr, 0, 0) == 0)
        {
            break;
        }

        // special handling to forcibly differentiate between ctrl+c and ctrl+break.
        // this is to get around the fact that we currently use TranslateMessage
        // when we should be handling it ourselves. MSFT:9891752 tracks this.
        const bool controlKeyPressed = IsFlagSet(GetKeyState(VK_LCONTROL), KEY_PRESSED) || IsFlagSet(GetKeyState(VK_RCONTROL), KEY_PRESSED);
        const WORD virtualScanCode = LOBYTE(HIWORD(msg.lParam));
        if (controlKeyPressed && msg.message == WM_KEYDOWN && MapVirtualKeyW(virtualScanCode, MAPVK_VSC_TO_VK_EX) == 'C')
        {
            msg.message = WM_CHAR;
            msg.wParam = 'c';
            msg.lParam = 'C';
            DispatchMessageW(&msg);
        }
        else if (!UserPrivApi::s_TranslateMessageEx(&msg, TM_POSTCHARBREAKS))
        {
            DispatchMessageW(&msg);
        }
        // do this so that alt-tab works while journalling
        else if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_TAB && (msg.lParam & 0x20000000))
        {
            // alt is really down
            DispatchMessageW(&msg);
        }
    }

    // Free all resources used by this thread
    DeactivateTextServices();

    return 0;
}

ULONG GetControlKeyState(_In_ const LPARAM lParam)
{
    ULONG ControlKeyState = 0;

    if (GetKeyState(VK_LMENU) & KEY_PRESSED)
    {
        ControlKeyState |= LEFT_ALT_PRESSED;
    }
    if (GetKeyState(VK_RMENU) & KEY_PRESSED)
    {
        ControlKeyState |= RIGHT_ALT_PRESSED;
    }
    if (GetKeyState(VK_LCONTROL) & KEY_PRESSED)
    {
        ControlKeyState |= LEFT_CTRL_PRESSED;
    }
    if (GetKeyState(VK_RCONTROL) & KEY_PRESSED)
    {
        ControlKeyState |= RIGHT_CTRL_PRESSED;
    }
    if (GetKeyState(VK_SHIFT) & KEY_PRESSED)
    {
        ControlKeyState |= SHIFT_PRESSED;
    }
    if (GetKeyState(VK_NUMLOCK) & KEY_TOGGLED)
    {
        ControlKeyState |= NUMLOCK_ON;
    }
    if (GetKeyState(VK_SCROLL) & KEY_TOGGLED)
    {
        ControlKeyState |= SCROLLLOCK_ON;
    }
    if (GetKeyState(VK_CAPITAL) & KEY_TOGGLED)
    {
        ControlKeyState |= CAPSLOCK_ON;
    }
    if (lParam & KEY_ENHANCED)
    {
        ControlKeyState |= ENHANCED_KEY;
    }

    ControlKeyState |= (lParam & ALTNUMPAD_BIT);

    return ControlKeyState;
}

ULONG ConvertMouseButtonState(_In_ ULONG Flag, _In_ ULONG State)
{
    if (State & MK_LBUTTON)
    {
        Flag |= FROM_LEFT_1ST_BUTTON_PRESSED;
    }
    if (State & MK_MBUTTON)
    {
        Flag |= FROM_LEFT_2ND_BUTTON_PRESSED;
    }
    if (State & MK_RBUTTON)
    {
        Flag |= RIGHTMOST_BUTTON_PRESSED;
    }

    return Flag;
}

// Routine Description:
// - This routine wakes up any readers waiting for data when a ctrl-c or ctrl-break is input.
// Arguments:
// - InputInfo - pointer to input buffer
// - Flag - flag indicating whether ctrl-break or ctrl-c was input.
void TerminateRead(_Inout_ PINPUT_INFORMATION InputInfo, _In_ WaitTerminationReason Flag)
{
    InputInfo->WaitQueue.NotifyWaiters(true, Flag);
}

// Routine Description:
// - Returns TRUE if DefWindowProc should be called.
BOOL HandleSysKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole)
{
    WORD VirtualKeyCode;

    if (Message == WM_SYSCHAR || Message == WM_SYSDEADCHAR)
    {
        VirtualKeyCode = (WORD) MapVirtualKeyW(LOBYTE(HIWORD(lParam)), MAPVK_VSC_TO_VK_EX);
    }
    else
    {
        VirtualKeyCode = LOWORD(wParam);
    }

    // Log a telemetry flag saying the user interacted with the Console
    Telemetry::Instance().SetUserInteractive();

    // check for ctrl-esc
    BOOL const bCtrlDown = GetKeyState(VK_CONTROL) & KEY_PRESSED;

    if (VirtualKeyCode == VK_ESCAPE &&
        bCtrlDown && !(GetKeyState(VK_MENU) & KEY_PRESSED) && !(GetKeyState(VK_SHIFT) & KEY_PRESSED))
    {
        return TRUE;    // call DefWindowProc
    }

    // check for alt-f4
    if (VirtualKeyCode == VK_F4 && (GetKeyState(VK_MENU) & KEY_PRESSED) && IsInProcessedInputMode() && g_ciConsoleInformation.IsAltF4CloseAllowed())
    {
        return TRUE; // let DefWindowProc generate WM_CLOSE
    }

    if ((lParam & 0x20000000) == 0)
    {   // we're iconic
        // Check for ENTER while iconic (restore accelerator).
        if (VirtualKeyCode == VK_RETURN)
        {

            return TRUE;    // call DefWindowProc
        }
        else
        {
            HandleKeyEvent(hWnd, Message, wParam, lParam, pfUnlockConsole);
            return FALSE;
        }
    }

    if (VirtualKeyCode == VK_RETURN && !bCtrlDown)
    {
        // only toggle on keydown
        if (!(lParam & KEY_TRANSITION_UP))
        {
            g_ciConsoleInformation.pWindow->ToggleFullscreen();
        }

        return FALSE;
    }

    // make sure alt-space gets translated so that the system menu is displayed.
    if (!(GetKeyState(VK_CONTROL) & KEY_PRESSED))
    {
        if (VirtualKeyCode == VK_SPACE)
        {
            if (IsInVirtualTerminalInputMode())
            {
                HandleKeyEvent(hWnd, Message, wParam, lParam, pfUnlockConsole);
                return FALSE;
            }

            return TRUE;    // call DefWindowProc
        }

        if (VirtualKeyCode == VK_ESCAPE)
        {
            return TRUE;    // call DefWindowProc
        }
        if (VirtualKeyCode == VK_TAB)
        {
            return TRUE;    // call DefWindowProc
        }
    }

    HandleKeyEvent(hWnd, Message, wParam, lParam, pfUnlockConsole);

    return FALSE;
}

// Routine Description:
// - returns true if we're in a mode amenable to us taking over keyboard shortcuts
bool ShouldTakeOverKeyboardShortcuts()
{
    return !g_ciConsoleInformation.GetCtrlKeyShortcutsDisabled() && IsInProcessedInputMode();
}

void HandleKeyEvent(_In_ const HWND /*hWnd*/, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole)
{

    const BOOL bKeyDown = IsFlagClear(lParam, KEY_TRANSITION_UP);
    if (bKeyDown)
    {
        // Log a telemetry flag saying the user interacted with the Console
        // Only log when the key is a down press.  Otherwise we're getting many calls with
        // Message = WM_CHAR, VirtualKeyCode = VK_TAB, with bKeyDown = false
        // when nothing is happening, or the user has merely clicked on the title bar, and
        // this can incorrectly mark the session as being interactive.
        Telemetry::Instance().SetUserInteractive();
    }

    BOOLEAN ContinueProcessing;
    ULONG EventsWritten;
    BOOL bGenerateBreak = FALSE;

    // BOGUS for WM_CHAR/WM_DEADCHAR, in which LOWORD(lParam) is a character
    WORD VirtualKeyCode = LOWORD(wParam);
    const ULONG ControlKeyState = GetControlKeyState(lParam);
    WORD VirtualScanCode = LOBYTE(HIWORD(lParam));
    WORD repeatCount = LOWORD(lParam);

    // Make sure we retrieve the key info first, or we could chew up unneeded space in the key info table if we bail out early.
    INPUT_RECORD InputEvent;
    InputEvent.EventType = KEY_EVENT;
    InputEvent.Event.KeyEvent.bKeyDown = bKeyDown;
    InputEvent.Event.KeyEvent.wRepeatCount = repeatCount;
    InputEvent.Event.KeyEvent.wVirtualKeyCode = VirtualKeyCode;
    InputEvent.Event.KeyEvent.wVirtualScanCode = VirtualScanCode;
    InputEvent.Event.KeyEvent.dwControlKeyState = ControlKeyState;
    InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)0;

    // We need to differentiate between WM_KEYDOWN and the rest
    // because when the message is WM_KEYDOWN we need to be
    // translating the scancode and with the others we're given the
    // wchar directly.
    if (Message == WM_CHAR || Message == WM_SYSCHAR || Message == WM_DEADCHAR || Message == WM_SYSDEADCHAR)
    {
        // If this is a fake character, zero the scancode.
        if (lParam & 0x02000000)
        {
            InputEvent.Event.KeyEvent.wVirtualScanCode = 0;
        }
        InputEvent.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)wParam;
        InputEvent.Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKeyScan((WCHAR)wParam));
        VirtualKeyCode = InputEvent.Event.KeyEvent.wVirtualKeyCode;
        InputEvent.Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKeyW(InputEvent.Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
        VirtualScanCode = InputEvent.Event.KeyEvent.wVirtualScanCode;
    }
    else if (Message == WM_KEYDOWN)
    {
        // if alt-gr, ignore
        if (lParam & 0x02000000)
        {
            return;
        }
        InputEvent.Event.KeyEvent.wVirtualKeyCode = (WORD) MapVirtualKeyW(VirtualScanCode, MAPVK_VSC_TO_VK_EX);
        VirtualKeyCode = InputEvent.Event.KeyEvent.wVirtualKeyCode;
        VirtualScanCode = VirtualKeyCode;
    }

    const INPUT_KEY_INFO inputKeyInfo(VirtualKeyCode, ControlKeyState);

    // If this is a key up message, should we ignore it? We do this so that if a process reads a line from the input
    // buffer, the key up event won't get put in the buffer after the read completes.
    if (g_ciConsoleInformation.Flags & CONSOLE_IGNORE_NEXT_KEYUP)
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_IGNORE_NEXT_KEYUP;
        if (!bKeyDown)
        {
            return;
        }
    }

    Selection* pSelection = &Selection::Instance();

    if (!IsInVirtualTerminalInputMode())
    {
        // First attempt to process simple key chords (Ctrl+Key)
        if (inputKeyInfo.IsCtrlOnly() && ShouldTakeOverKeyboardShortcuts())
        {
            if (!bKeyDown)
            {
                return;
            }

            switch (VirtualKeyCode)
            {
            case 'A':
                // Set Text Selection using keyboard to true for telemetry
                Telemetry::Instance().SetKeyboardTextSelectionUsed();
                // the user is asking to select all
                pSelection->SelectAll();
                return;
            case 'F':
                // the user is asking to go to the find window
                DoFind();
                *pfUnlockConsole = FALSE;
                return;
            case 'M':
                // the user is asking for mark mode
                Clipboard::s_DoMark();
                return;
            case 'V':
                // the user is attempting to paste from the clipboard
                Telemetry::Instance().SetKeyboardTextEditingUsed();
                Clipboard::s_DoPaste();
                return;
            case VK_HOME:
            case VK_END:
            case VK_UP:
            case VK_DOWN:
                // if the user is asking for keyboard scroll, give it to them
                if (Scrolling::s_HandleKeyScrollingEvent(&inputKeyInfo))
                {
                    return;
                }
                break;
            case VK_PRIOR:
            case VK_NEXT:
                Telemetry::Instance().SetCtrlPgUpPgDnUsed();
                break;
            }
        }

        // Handle F11 fullscreen toggle
        if (VirtualKeyCode == VK_F11 &&
            bKeyDown &&
            inputKeyInfo.HasNoModifiers() &&
            ShouldTakeOverKeyboardShortcuts())
        {
            g_ciConsoleInformation.pWindow->ToggleFullscreen();
            return;
        }

        // handle shift-ins paste
        if (inputKeyInfo.IsShiftOnly() && ShouldTakeOverKeyboardShortcuts())
        {
            if (!bKeyDown)
            {
                return;
            }
            else if (VirtualKeyCode == VK_INSERT && !(pSelection->IsInSelectingState() && pSelection->IsKeyboardMarkSelection()))
            {
                Clipboard::s_DoPaste();
                return;
            }
        }

        // handle ctrl+shift+plus/minus for transparency adjustment
        if (inputKeyInfo.IsShiftAndCtrlOnly() && ShouldTakeOverKeyboardShortcuts())
        {
            if (!bKeyDown)
            {
                return;
            }
            else
            {
                //This is the only place where the window opacity is changed NOT due to the props sheet.
                short opacityDelta = 0;
                if (VirtualKeyCode == VK_OEM_PLUS || VirtualKeyCode == VK_ADD)
                {
                    opacityDelta = OPACITY_DELTA_INTERVAL;
                }
                else if (VirtualKeyCode == VK_OEM_MINUS || VirtualKeyCode == VK_SUBTRACT)
                {
                    opacityDelta = -OPACITY_DELTA_INTERVAL;
                }
                if (opacityDelta != 0)
                {
                    g_ciConsoleInformation.pWindow->ChangeWindowOpacity(opacityDelta);
                    g_ciConsoleInformation.pWindow->SetWindowHasMoved(true);

                    return;
                }

            }
        }
    }

    // Then attempt to process more complicated selection/scrolling commands that require state.
    // These selection and scrolling functions must go after the simple key-chord combinations
    // as they have the potential to modify state in a way those functions do not expect.
    if (g_ciConsoleInformation.Flags & CONSOLE_SELECTING)
    {
        if (!bKeyDown || pSelection->HandleKeySelectionEvent(&inputKeyInfo))
        {
            return;
        }
    }
    if (Scrolling::s_IsInScrollMode())
    {
        if (!bKeyDown || Scrolling::s_HandleKeyScrollingEvent(&inputKeyInfo))
        {
            return;
        }
    }
    if (pSelection->s_IsValidKeyboardLineSelection(&inputKeyInfo) && IsInProcessedInputMode() && g_ciConsoleInformation.GetExtendedEditKey())
    {
        if (!bKeyDown || pSelection->HandleKeyboardLineSelectionEvent(&inputKeyInfo))
        {
            return;
        }
    }

    // if the user is inputting chars at an inappropriate time, beep.
    if ((g_ciConsoleInformation.Flags & (CONSOLE_SELECTING | CONSOLE_SCROLLING | CONSOLE_SCROLLBAR_TRACKING)) &&
        bKeyDown &&
        !IsSystemKey(VirtualKeyCode))
    {
        g_ciConsoleInformation.pWindow->SendNotifyBeep();
        return;
    }

    // ignore key strokes that will generate CHAR messages. this is only necessary while a dialog box is up.
    if (g_uiDialogBoxCount != 0)
    {
        if (Message != WM_CHAR && Message != WM_SYSCHAR && Message != WM_DEADCHAR && Message != WM_SYSDEADCHAR)
        {
            WCHAR awch[MAX_CHARS_FROM_1_KEYSTROKE];
            BYTE KeyState[256];
            if (GetKeyboardState(KeyState))
            {
                int cwch = ToUnicodeEx((UINT) wParam, HIWORD(lParam), KeyState, awch, ARRAYSIZE(awch), TM_POSTCHARBREAKS, nullptr);
                if (cwch != 0)
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else
        {
            // remember to generate break
            if (Message == WM_CHAR)
            {
                bGenerateBreak = TRUE;
            }
        }
    }

    ContinueProcessing = TRUE;

    if (HandleTerminalKeyEvent(&InputEvent))
    {
        return;
    }

    if (CTRL_BUT_NOT_ALT(InputEvent.Event.KeyEvent.dwControlKeyState) && InputEvent.Event.KeyEvent.bKeyDown)
    {
        // check for ctrl-c, if in line input mode.
        if (InputEvent.Event.KeyEvent.wVirtualKeyCode == 'C' && IsInProcessedInputMode())
        {
            HandleCtrlEvent(CTRL_C_EVENT);
            if (g_ciConsoleInformation.PopupCount == 0)
            {
                TerminateRead(g_ciConsoleInformation.pInputBuffer, WaitTerminationReason::CtrlC);
            }

            if (!(g_ciConsoleInformation.Flags & CONSOLE_SUSPENDED))
            {
                ContinueProcessing = FALSE;
            }
        }

        // check for ctrl-break.
        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
        {
            FlushInputBuffer(g_ciConsoleInformation.pInputBuffer);
            HandleCtrlEvent(CTRL_BREAK_EVENT);
            if (g_ciConsoleInformation.PopupCount == 0)
            {
                TerminateRead(g_ciConsoleInformation.pInputBuffer, WaitTerminationReason::CtrlBreak);
            }

            if (!(g_ciConsoleInformation.Flags & CONSOLE_SUSPENDED))
            {
                ContinueProcessing = FALSE;
            }
        }

        // don't write ctrl-esc to the input buffer
        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
        {
            ContinueProcessing = FALSE;
        }
    }
    else if (InputEvent.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED) &&
             InputEvent.Event.KeyEvent.bKeyDown && InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
    {
        ContinueProcessing = FALSE;
    }

    if (ContinueProcessing)
    {
        EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
        if (EventsWritten && bGenerateBreak)
        {
            InputEvent.Event.KeyEvent.bKeyDown = FALSE;
            WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
        }
    }
}

// Routine Description:
// - Handler for detecting whether a key-press event can be appropriately converted into a terminal sequence.
//   Will only trigger when virtual terminal input mode is set via STDIN handle
// Arguments:
// - pInputRecord - Input record event from the general input event handler
// Return Value:
// - True if the modes were appropriate for converting to a terminal sequence AND there was a matching terminal sequence for this key. False otherwise.
bool HandleTerminalKeyEvent(_In_ const INPUT_RECORD* const pInputRecord)
{
    // If the modes don't align, this is unhandled by default.
    bool fWasHandled = false;

    // Virtual terminal input mode
    if (IsInVirtualTerminalInputMode())
    {
        fWasHandled = g_ciConsoleInformation.termInput.HandleKey(pInputRecord);
    }

    return fWasHandled;
}

// Routine Description:
// - Handler for detecting whether a key-press event can be appropriately converted into a terminal sequence.
//   Will only trigger when virtual terminal input mode is set via STDIN handle
// Arguments:
// - pInputRecord - Input record event from the general input event handler
// Return Value:
// - True if the modes were appropriate for converting to a terminal sequence AND there was a matching terminal sequence for this key. False otherwise.
bool HandleTerminalMouseEvent(_In_ const COORD cMousePosition, _In_ const unsigned int uiButton, _In_ const short sModifierKeystate, _In_ const short sWheelDelta)
{
    // If the modes don't align, this is unhandled by default.
    bool fWasHandled = false;

    // Virtual terminal input mode
    if (IsInVirtualTerminalInputMode())
    {
        fWasHandled = g_ciConsoleInformation.terminalMouseInput.HandleMouse(cMousePosition, uiButton, sModifierKeystate, sWheelDelta);
    }

    return fWasHandled;
}


// Routine Description:
// - Returns TRUE if DefWindowProc should be called.
BOOL HandleMouseEvent(_In_ const SCREEN_INFORMATION * const pScreenInfo, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam)
{
    if (Message != WM_MOUSEMOVE)
    {
        // Log a telemetry flag saying the user interacted with the Console
        Telemetry::Instance().SetUserInteractive();
    }

    Selection* const pSelection = &Selection::Instance();

    if (!(g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS) && !pSelection->IsMouseButtonDown())
    {
        return TRUE;
    }

    if (g_ciConsoleInformation.Flags & CONSOLE_IGNORE_NEXT_MOUSE_INPUT)
    {
        // only reset on up transition
        if (Message != WM_LBUTTONDOWN && Message != WM_MBUTTONDOWN && Message != WM_RBUTTONDOWN)
        {
            g_ciConsoleInformation.Flags &= ~CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
            return FALSE;
        }
        return TRUE;
    }

    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms645617(v=vs.85).aspx
    //  Important  Do not use the LOWORD or HIWORD macros to extract the x- and y-
    //  coordinates of the cursor position because these macros return incorrect
    //  results on systems with multiple monitors. Systems with multiple monitors
    //  can have negative x- and y- coordinates, and LOWORD and HIWORD treat the
    //  coordinates as unsigned quantities.
    short x = GET_X_LPARAM(lParam);
    short y = GET_Y_LPARAM(lParam);

    COORD MousePosition;
    // If it's a *WHEEL event, it's in screen coordinates, not window
    if (Message == WM_MOUSEWHEEL || Message == WM_MOUSEHWHEEL)
    {
        POINT coords = {x, y};
        ScreenToClient(g_ciConsoleInformation.hWnd, &coords);
        MousePosition = {(SHORT)coords.x, (SHORT)coords.y};
    }
    else
    {
        MousePosition = {x, y};
    }

    // translate mouse position into characters, if necessary.
    COORD ScreenFontSize = pScreenInfo->GetScreenFontSize();
    MousePosition.X /= ScreenFontSize.X;
    MousePosition.Y /= ScreenFontSize.Y;

    const bool fShiftPressed = IsFlagSet(GetKeyState(VK_SHIFT), KEY_PRESSED);

    // We need to try and have the virtual terminal handle the mouse's position in viewport coordinates,
    //   not in screen buffer coordinates. It expects the top left to always be 0,0
    //   (the TerminalMouseInput object will add (1,1) to convert to VT coords on it's own.)
    // Mouse events with shift pressed will ignore this and fall through to the default handler.
    //   This is in line with PuTTY's behavior and vim's own documentation:
    //   "The xterm handling of the mouse buttons can still be used by keeping the shift key pressed." - `:help 'mouse'`, vim.
    // Mouse events while we're selecting or have a selection will also skip this and fall though
    //   (so that the VT handler doesn't eat any selection region updates)
    if (!fShiftPressed && !pSelection->IsInSelectingState())
    {
        short sDelta = 0;
        if (Message == WM_MOUSEWHEEL)
        {
            sDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        }

        if (HandleTerminalMouseEvent(MousePosition, Message, GET_KEYSTATE_WPARAM(wParam), sDelta))
        {
            return FALSE;
        }
    }

    MousePosition.X += pScreenInfo->GetBufferViewport().Left;
    MousePosition.Y += pScreenInfo->GetBufferViewport().Top;

    // make sure mouse position is clipped to screen buffer
    if (MousePosition.X < 0)
    {
        MousePosition.X = 0;
    }
    else if (MousePosition.X >= pScreenInfo->ScreenBufferSize.X)
    {
        MousePosition.X = pScreenInfo->ScreenBufferSize.X - 1;
    }
    if (MousePosition.Y < 0)
    {
        MousePosition.Y = 0;
    }
    else if (MousePosition.Y >= pScreenInfo->ScreenBufferSize.Y)
    {
        MousePosition.Y = pScreenInfo->ScreenBufferSize.Y - 1;
    }

    if (pSelection->IsInSelectingState() || pSelection->IsInQuickEditMode())
    {
        if (Message == WM_LBUTTONDOWN)
        {
            // make sure message matches button state
            if (!(GetKeyState(VK_LBUTTON) & KEY_PRESSED))
            {
                return FALSE;
            }

            if (pSelection->IsInQuickEditMode() && !pSelection->IsInSelectingState())
            {
                // start a mouse selection
                pSelection->InitializeMouseSelection(MousePosition);

                pSelection->MouseDown();

                // Check for ALT-Mouse Down "use alternate selection"
                // If in box mode, use line mode. If in line mode, use box mode.
                // TODO: move into initialize?
                pSelection->CheckAndSetAlternateSelection();

                pSelection->ShowSelection();
            }
            else
            {
                bool fExtendSelection = false;

                // We now capture the mouse to our Window. We do this so that the
                // user can "scroll" the selection endpoint to an off screen
                // position by moving the mouse off the client area.
                if (pSelection->IsMouseInitiatedSelection())
                {
                    // Check for SHIFT-Mouse Down "continue previous selection" command.
                    if (fShiftPressed)
                    {
                        fExtendSelection = true;
                    }
                }

                // if we chose to extend the selection, do that.
                if (fExtendSelection)
                {
                    pSelection->MouseDown();
                    pSelection->ExtendSelection(MousePosition);
                }
                else
                {
                    // otherwise, set up a new selection from here. note that it's important to ClearSelection(true) here
                    // because ClearSelection() unblocks console output, causing us to have
                    // a line of output occur every time the user changes the selection.
                    pSelection->ClearSelection(true);
                    pSelection->InitializeMouseSelection(MousePosition);
                    pSelection->MouseDown();
                    pSelection->ShowSelection();
                }
            }
        }
        else if (Message == WM_LBUTTONUP)
        {
            if (pSelection->IsInSelectingState() && pSelection->IsMouseInitiatedSelection())
            {
                pSelection->MouseUp();
            }
        }
        else if (Message == WM_LBUTTONDBLCLK)
        {
            // on double-click, attempt to select a "word" beneath the cursor
            COORD coordSelectionAnchor;
            pSelection->GetSelectionAnchor(&coordSelectionAnchor);

            if ((MousePosition.X == coordSelectionAnchor.X) && (MousePosition.Y == coordSelectionAnchor.Y))
            {
                ROW* const Row = pScreenInfo->TextInfo->GetRowByOffset(MousePosition.Y);
                ASSERT(Row != nullptr);

                while (coordSelectionAnchor.X > 0)
                {
                    if (IS_WORD_DELIM(Row->CharRow.Chars[coordSelectionAnchor.X - 1]))
                    {
                        break;
                    }
                    coordSelectionAnchor.X--;
                }
                while (MousePosition.X < pScreenInfo->ScreenBufferSize.X)
                {
                    if (IS_WORD_DELIM(Row->CharRow.Chars[MousePosition.X]))
                    {
                        break;
                    }
                    MousePosition.X++;
                }
                if (g_ciConsoleInformation.GetTrimLeadingZeros())
                {
                    // Trim the leading zeros: 000fe12 -> fe12, except 0x and 0n.
                    // Useful for debugging
                    if (MousePosition.X > coordSelectionAnchor.X + 2 &&
                        Row->CharRow.Chars[coordSelectionAnchor.X + 1] != L'x' &&
                        Row->CharRow.Chars[coordSelectionAnchor.X + 1] != L'X' && Row->CharRow.Chars[coordSelectionAnchor.X + 1] != L'n')
                    {
                        // Don't touch the selection begins with 0x
                        while (Row->CharRow.Chars[coordSelectionAnchor.X] == L'0' && coordSelectionAnchor.X < MousePosition.X - 1)
                        {
                            coordSelectionAnchor.X++;
                        }
                    }
                }

                // update both ends of the selection since we may have adjusted the anchor in some circumstances.
                pSelection->AdjustSelection(coordSelectionAnchor, MousePosition);
            }
        }
        else if ((Message == WM_RBUTTONDOWN) || (Message == WM_RBUTTONDBLCLK))
        {
            if (!pSelection->IsMouseButtonDown())
            {
                if (pSelection->IsInSelectingState())
                {
                    Clipboard::s_DoCopy();
                }
                else if (g_ciConsoleInformation.Flags & CONSOLE_QUICK_EDIT_MODE)
                {
                    Clipboard::s_DoPaste();
                }
                g_ciConsoleInformation.Flags |= CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
            }
        }
        else if (Message == WM_MBUTTONDOWN)
        {
            UserPrivApi::s_EnterReaderModeHelper(g_ciConsoleInformation.hWnd);
        }
        else if (Message == WM_MOUSEMOVE)
        {
            if (pSelection->IsMouseButtonDown())
            {
                pSelection->ExtendSelection(MousePosition);
            }
        }
        if (Message != WM_MOUSEWHEEL && Message != WM_MOUSEHWHEEL)
        {
            // We haven't processed the mousewheel event, so don't early return in that case
            // Otherwise, all other mouse events are done being processed.
            return FALSE;
        }
    }

    if (Message == WM_MOUSEWHEEL)
    {
        const short sKeyState = GET_KEYSTATE_WPARAM(wParam);

        // ctrl+shift+scroll will adjust the transparency of the window
        if ((sKeyState & MK_SHIFT) && (sKeyState & MK_CONTROL))
        {
            const short sDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            g_ciConsoleInformation.pWindow->ChangeWindowOpacity(OPACITY_DELTA_INTERVAL * sDelta);
            g_ciConsoleInformation.pWindow->SetWindowHasMoved(true);
        }
        else
        {
            return TRUE;
        }
    }
    else if (Message == WM_MOUSEHWHEEL)
    {
        return TRUE;
    }


    if (!(g_ciConsoleInformation.pInputBuffer->InputMode & ENABLE_MOUSE_INPUT))
    {
        ReleaseCapture();
        return TRUE;
    }

    INPUT_RECORD InputEvent;
    InputEvent.Event.MouseEvent.dwControlKeyState = GetControlKeyState(0);

    ULONG ButtonFlags;
    ULONG EventFlags;
    switch (Message)
    {
        case WM_LBUTTONDOWN:
            SetCapture(g_ciConsoleInformation.hWnd);
            ButtonFlags = FROM_LEFT_1ST_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            ReleaseCapture();
            ButtonFlags = EventFlags = 0;
            break;
        case WM_RBUTTONDOWN:
            SetCapture(g_ciConsoleInformation.hWnd);
            ButtonFlags = RIGHTMOST_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_MBUTTONDOWN:
            SetCapture(g_ciConsoleInformation.hWnd);
            ButtonFlags = FROM_LEFT_2ND_BUTTON_PRESSED;
            EventFlags = 0;
            break;
        case WM_MOUSEMOVE:
            ButtonFlags = 0;
            EventFlags = MOUSE_MOVED;
            break;
        case WM_LBUTTONDBLCLK:
            ButtonFlags = FROM_LEFT_1ST_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_RBUTTONDBLCLK:
            ButtonFlags = RIGHTMOST_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_MBUTTONDBLCLK:
            ButtonFlags = FROM_LEFT_2ND_BUTTON_PRESSED;
            EventFlags = DOUBLE_CLICK;
            break;
        case WM_MOUSEWHEEL:
            ButtonFlags = ((UINT) wParam & 0xFFFF0000);
            EventFlags = MOUSE_WHEELED;
            break;
        case WM_MOUSEHWHEEL:
            ButtonFlags = ((UINT) wParam & 0xFFFF0000);
            EventFlags = MOUSE_HWHEELED;
            break;
        default:
            RIPMSG1(RIP_ERROR, "Invalid message 0x%x", Message);
            ButtonFlags = 0;
            EventFlags = 0;
            break;
    }

    InputEvent.EventType = MOUSE_EVENT;
    InputEvent.Event.MouseEvent.dwMousePosition = MousePosition;
    InputEvent.Event.MouseEvent.dwEventFlags = EventFlags;
    InputEvent.Event.MouseEvent.dwButtonState = ConvertMouseButtonState(ButtonFlags, (UINT) wParam);
    ULONG const EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
    if (EventsWritten != 1)
    {
        RIPMSG1(RIP_WARNING, "PutInputInBuffer: EventsWritten != 1 (0x%x), 1 expected", EventsWritten);
    }

    return FALSE;
}

void HandleFocusEvent(_In_ const BOOL fSetFocus)
{
    INPUT_RECORD InputEvent;
    InputEvent.EventType = FOCUS_EVENT;
    InputEvent.Event.FocusEvent.bSetFocus = fSetFocus;

#pragma prefast(suppress:28931, "EventsWritten is not unused. Used by assertions.")
    ULONG const EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
    EventsWritten; // shut the fre build up.
    ASSERT(EventsWritten == 1);
}

void HandleMenuEvent(_In_ const DWORD wParam)
{
    INPUT_RECORD InputEvent;
    InputEvent.EventType = MENU_EVENT;
    InputEvent.Event.MenuEvent.dwCommandId = wParam;

#pragma prefast(suppress:28931, "EventsWritten is not unused. Used by assertions.")
    ULONG const EventsWritten = WriteInputBuffer(g_ciConsoleInformation.pInputBuffer, &InputEvent, 1);
    EventsWritten; // shut the fre build up.
#if DBG
    if (EventsWritten != 1)
    {
        RIPMSG0(RIP_WARNING, "PutInputInBuffer: EventsWritten != 1, 1 expected");
    }
#endif
}

void HandleCtrlEvent(_In_ const DWORD EventType)
{
    switch (EventType)
    {
        case CTRL_C_EVENT:
            g_ciConsoleInformation.CtrlFlags |= CONSOLE_CTRL_C_FLAG;
            break;
        case CTRL_BREAK_EVENT:
            g_ciConsoleInformation.CtrlFlags |= CONSOLE_CTRL_BREAK_FLAG;
            break;
        case CTRL_CLOSE_EVENT:
            g_ciConsoleInformation.CtrlFlags |= CONSOLE_CTRL_CLOSE_FLAG;
            break;
        default:
            RIPMSG1(RIP_ERROR, "Invalid EventType: 0x%x", EventType);
    }
}

void ProcessCtrlEvents()
{
    if (g_ciConsoleInformation.CtrlFlags == 0)
    {
        g_ciConsoleInformation.UnlockConsole();
        return;
    }

    // Make our own copy of the console process handle list.
    DWORD const LimitingProcessId = g_ciConsoleInformation.LimitingProcessId;
    g_ciConsoleInformation.LimitingProcessId = 0;

    ConsoleProcessTerminationRecord* rgProcessHandleList;
    size_t cProcessHandleList;

    HRESULT hr = g_ciConsoleInformation.ProcessHandleList.GetTerminationRecordsByGroupId(LimitingProcessId,
                                                                                         IsFlagSet(g_ciConsoleInformation.CtrlFlags, CONSOLE_CTRL_CLOSE_FLAG),
                                                                                         &rgProcessHandleList,
                                                                                         &cProcessHandleList);

    if (FAILED(hr) || cProcessHandleList == 0)
    {
        g_ciConsoleInformation.UnlockConsole();
        return;
    }

    // Copy ctrl flags.
    ULONG CtrlFlags = g_ciConsoleInformation.CtrlFlags;
    ASSERT(!((CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG | CONSOLE_CTRL_C_FLAG)) && (CtrlFlags & (CONSOLE_CTRL_LOGOFF_FLAG | CONSOLE_CTRL_SHUTDOWN_FLAG))));

    g_ciConsoleInformation.CtrlFlags = 0;

    g_ciConsoleInformation.UnlockConsole();

    // the ctrl flags could be a combination of the following
    // values:
    //
    //        CONSOLE_CTRL_C_FLAG
    //        CONSOLE_CTRL_BREAK_FLAG
    //        CONSOLE_CTRL_CLOSE_FLAG
    //        CONSOLE_CTRL_LOGOFF_FLAG
    //        CONSOLE_CTRL_SHUTDOWN_FLAG

    DWORD EventType = (DWORD) - 1;
    switch (CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG | CONSOLE_CTRL_C_FLAG | CONSOLE_CTRL_LOGOFF_FLAG | CONSOLE_CTRL_SHUTDOWN_FLAG))
    {

        case CONSOLE_CTRL_CLOSE_FLAG:
            EventType = CTRL_CLOSE_EVENT;
            break;

        case CONSOLE_CTRL_BREAK_FLAG:
            EventType = CTRL_BREAK_EVENT;
            break;

        case CONSOLE_CTRL_C_FLAG:
            EventType = CTRL_C_EVENT;
            break;

        case CONSOLE_CTRL_LOGOFF_FLAG:
            EventType = CTRL_LOGOFF_EVENT;
            break;

        case CONSOLE_CTRL_SHUTDOWN_FLAG:
            EventType = CTRL_SHUTDOWN_EVENT;
            break;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    for (size_t i = 0; i < cProcessHandleList; i++)
    {
        /*
         * Status will be non-successful if a process attached to this console
         * vetos shutdown. In that case, we don't want to try to kill any more
         * processes, but we do need to make sure we continue looping so we
         * can close any remaining process handles. The exception is if the
         * process is inaccessible, such that we can't even open a handle for
         * query. In this case, use best effort to send the close event but
         * ignore any errors.
         */
        if (NT_SUCCESS(Status))
        {
            CONSOLEENDTASK ConsoleEndTaskParams;

            ConsoleEndTaskParams.ProcessId = (HANDLE)rgProcessHandleList[i].dwProcessID;
            ConsoleEndTaskParams.ConsoleEventCode = EventType;
            ConsoleEndTaskParams.ConsoleFlags = CtrlFlags;
            ConsoleEndTaskParams.hwnd = g_ciConsoleInformation.hWnd;

            Status = UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleEndTask, &ConsoleEndTaskParams, sizeof(ConsoleEndTaskParams));
            if (rgProcessHandleList[i].hProcess == nullptr) {
                Status = STATUS_SUCCESS;
            }
        }

        if (rgProcessHandleList[i].hProcess != nullptr)
        {
            CloseHandle(rgProcessHandleList[i].hProcess);
        }
    }

    delete[] rgProcessHandleList;
}
