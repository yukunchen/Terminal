/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "inputBuffer.hpp"
#include "dbcs.h"
#include "stream.h"

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
    pInputInfo->ImeMode.Disable = FALSE;
    pInputInfo->ImeMode.Unavailable = FALSE;
    pInputInfo->ImeMode.Open = FALSE;
    pInputInfo->ImeMode.ReadyConversion = FALSE;
    pInputInfo->ImeMode.InComposition = FALSE;

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
