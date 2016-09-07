/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#pragma hdrstop

_Success_(return == TRUE)
static BOOL ConsoleInitializeWait(_In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                                  _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                                  _In_ PVOID pvWaitParameter,
                                  _Outptr_ PCONSOLE_WAIT_BLOCK * ppWaitBlock)
{
    PCONSOLE_WAIT_BLOCK const WaitBlock = (PCONSOLE_WAIT_BLOCK) ConsoleHeapAlloc(WAITBLOCK_TAG, sizeof(CONSOLE_WAIT_BLOCK));
    if (WaitBlock == nullptr)
    {
        SetReplyStatus(pWaitReplyMessage, STATUS_NO_MEMORY);
        return FALSE;
    }

    WaitBlock->WaitParameter = pvWaitParameter;
    WaitBlock->WaitRoutine = pfnWaitRoutine;
    memmove(&WaitBlock->WaitReplyMessage, pWaitReplyMessage, sizeof(CONSOLE_API_MSG));

    if (pWaitReplyMessage->Complete.Write.Data != nullptr)
    {
        WaitBlock->WaitReplyMessage.Complete.Write.Data = &WaitBlock->WaitReplyMessage.u;
    }

    *ppWaitBlock = WaitBlock;

    return TRUE;
}

BOOL ConsoleCreateWait(_In_ PLIST_ENTRY pWaitQueue,
                       _In_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                       _Inout_ PCONSOLE_API_MSG pWaitReplyMessage,
                       _In_ PVOID pvWaitParameter)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    PCONSOLE_PROCESS_HANDLE const ProcessData = GetMessageProcess(pWaitReplyMessage);
    ASSERT(ProcessData != nullptr);

    PCONSOLE_WAIT_BLOCK WaitBlock;
    if (!ConsoleInitializeWait(pfnWaitRoutine, pWaitReplyMessage, pvWaitParameter, &WaitBlock))
    {
        return FALSE;
    }

    InsertTailList(pWaitQueue, &WaitBlock->Link);
    InsertHeadList(&ProcessData->WaitBlockQueue, &WaitBlock->ProcessLink);

    return TRUE;
}

BOOL ConsoleNotifyWaitBlock(_In_ PCONSOLE_WAIT_BLOCK pWaitBlock, _In_ PLIST_ENTRY pWaitQueue, _In_ PVOID pvSatisfyParameter, _In_ BOOL fThreadDying)
{
    if ((*pWaitBlock->WaitRoutine)(pWaitQueue, &pWaitBlock->WaitReplyMessage, pWaitBlock->WaitParameter, pvSatisfyParameter, fThreadDying))
    {
        RemoveEntryList(&pWaitBlock->ProcessLink);

        ReleaseMessageBuffers(&pWaitBlock->WaitReplyMessage);

        ConsoleComplete(g_ciConsoleInformation.Server, &pWaitBlock->WaitReplyMessage.Complete);

        RemoveEntryList(&pWaitBlock->Link);

        ConsoleHeapFree(pWaitBlock);

        return TRUE;
    }
    else
    {
        // If fThreadDying is TRUE we need to make sure that we removed the pWaitBlock from the list (which we don't do on this branch).
        ASSERT(!fThreadDying);
        return FALSE;
    }
}

BOOL ConsoleNotifyWait(_In_ PLIST_ENTRY pWaitQueue, _In_ const BOOL fSatisfyAll, _In_ PVOID pvSatisfyParameter)
{
    BOOL Result = FALSE;

    PLIST_ENTRY const ListHead = pWaitQueue;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCONSOLE_WAIT_BLOCK const WaitBlock = CONTAINING_RECORD(ListNext, CONSOLE_WAIT_BLOCK, Link);
        ListNext = ListNext->Flink;
        if (WaitBlock->WaitRoutine)
        {
            Result |= ConsoleNotifyWaitBlock(WaitBlock, pWaitQueue, pvSatisfyParameter, FALSE);
            if (!fSatisfyAll)
            {
                break;
            }
        }
    }

    return Result;
}

// Routine Description:
// - This routine completes a message with the given completion descriptor.
// Arguments:
// - Object - Supplies the server object whose message will be completed.
// - Complete - Supplies the completion descriptor.
// Return Value:
// - NTSTATUS indicating if the message was successfully completed.
NTSTATUS ConsoleComplete(_In_ HANDLE hObject, _In_ PCD_IO_COMPLETE pComplete)
{
    return IoControlFile(hObject,
                         IOCTL_CONDRV_COMPLETE_IO,
                         pComplete,
                         sizeof(*pComplete),
                         nullptr,
                         0);
}

// Routine Description:
// - This routine reads [part of] the input payload of the given message.
// Arguments:
// - Message - Supplies the message whose input will be read.
// - Offset - Supplies the offset from which to start reading the payload.
// - Buffer - Receives the payload.
// - Size - Supplies the number of bytes to be read into the buffer.
// Return Value:
// - NTSTATUS indicating if the payload was successfully read.
NTSTATUS ReadMessageInput(_In_ PCONSOLE_API_MSG pMessage,
                          _In_ const ULONG ulOffset,
                          _Out_writes_bytes_(cbSize) PVOID pvBuffer,
                          _In_ const ULONG cbSize)
{
    CD_IO_OPERATION IoOperation;
    IoOperation.Identifier = pMessage->Descriptor.Identifier;
    IoOperation.Buffer.Offset = pMessage->State.ReadOffset + ulOffset;
    IoOperation.Buffer.Data = pvBuffer;
    IoOperation.Buffer.Size = cbSize;

    NTSTATUS Status = IoControlFile(g_ciConsoleInformation.Server,
                                   IOCTL_CONDRV_READ_INPUT,
                                   &IoOperation,
                                   sizeof IoOperation,
                                   nullptr,
                                   0);

    return Status;
}

// Routine Description:
// - This routine retrieves the input buffer associated with this message. It will allocate one if needed.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Message - Supplies the message whose input buffer will be retrieved.
// - Buffer - Receives a pointer to the input buffer.
// - Size - Receives the size, in bytes, of the input buffer.
// Return Value:
// -  NTSTATUS indicating if the input buffer was successfully retrieved.
NTSTATUS GetInputBuffer(_In_ PCONSOLE_API_MSG pMessage, _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer, _Out_ ULONG * const pcbSize)
{
    // Initialize the buffer if it hasn't been initialized yet.
    if (pMessage->State.InputBuffer == nullptr)
    {

        if (pMessage->State.ReadOffset > pMessage->Descriptor.InputSize)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ULONG const ReadSize = pMessage->Descriptor.InputSize - pMessage->State.ReadOffset;

        PVOID const Payload = ConsoleHeapAlloc(PAYLOAD_TAG, ReadSize);
        if (Payload == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS Status = ReadMessageInput(pMessage, 0, Payload, ReadSize);
        if (!NT_SUCCESS(Status))
        {
            ConsoleHeapFree(Payload);
            return Status;
        }

        pMessage->State.InputBuffer = Payload;
        pMessage->State.InputBufferSize = ReadSize;
    }

    // Return the buffer.
    *ppvBuffer = pMessage->State.InputBuffer;
    *pcbSize = pMessage->State.InputBufferSize;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine retrieves the output buffer associated with this message. It will allocate one if needed.
//   The allocated will be bigger than the actual output size by the requested factor.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Message - Supplies the message whose output buffer will be retrieved.
// - Factor - Supplies the factor to multiply the allocated buffer by.
// - Buffer - Receives a pointer to the output buffer.
// - Size - Receives the size, in bytes, of the output buffer.
//  Return Value:
// - NTSTATUS indicating if the output buffer was successfully retrieved.
NTSTATUS GetAugmentedOutputBuffer(_Inout_ PCONSOLE_API_MSG pMessage,
                                  _In_ const ULONG ulFactor,
                                  _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer,
                                  _Out_ PULONG pcbSize)
{
    // Initialize the buffer if it hasn't been initialized yet.
    if (pMessage->State.OutputBuffer == nullptr)
    {
        if (pMessage->State.WriteOffset > pMessage->Descriptor.OutputSize)
        {
            return STATUS_UNSUCCESSFUL;
        }

        pMessage->State.OutputBufferSize = pMessage->Descriptor.OutputSize - pMessage->State.WriteOffset;

        if (pMessage->State.OutputBufferSize > ULONG_MAX / ulFactor)
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        pMessage->State.OutputBufferSize *= ulFactor;
        pMessage->State.OutputBuffer = ConsoleHeapAlloc(PAYLOAD_TAG, pMessage->State.OutputBufferSize);

        if (pMessage->State.OutputBuffer == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // Return the buffer.
    *ppvBuffer = pMessage->State.OutputBuffer;
    *pcbSize = pMessage->State.OutputBufferSize;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine retrieves the output buffer associated with this message. It will allocate one if needed.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Message - Supplies the message whose output buffer will be retrieved.
// - Buffer - Receives a pointer to the output buffer.
// - Size - Receives the size, in bytes, of the output buffer.
// Return Value:
// - NTSTATUS indicating if the output buffer was successfully retrieved.
NTSTATUS GetOutputBuffer(_Inout_ PCONSOLE_API_MSG pMessage, _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer, _Out_ ULONG * const pcbSize)
{
    return GetAugmentedOutputBuffer(pMessage, 1, ppvBuffer, pcbSize);
}

// Routine Description:
// - This routine releases output or input buffers that might have been allocated
//   during the processing of the given message. If the current completion status
//   of the message indicates success, this routine also writes the output buffer
//   (if any) to the message.
// Arguments:
// - Message - Supplies the message to have its buffers released.
// Return Value:
// - <None>
void ReleaseMessageBuffers(_Inout_ PCONSOLE_API_MSG pMessage)
{
    if (pMessage->State.InputBuffer != nullptr)
    {
        ConsoleHeapFree(pMessage->State.InputBuffer);
        pMessage->State.InputBuffer = nullptr;
    }

    if (pMessage->State.OutputBuffer != nullptr)
    {
        if (NT_SUCCESS(pMessage->Complete.IoStatus.Status))
        {
            CD_IO_OPERATION IoOperation;
            IoOperation.Identifier = pMessage->Descriptor.Identifier;
            IoOperation.Buffer.Offset = pMessage->State.WriteOffset;
            IoOperation.Buffer.Data = pMessage->State.OutputBuffer;
            IoOperation.Buffer.Size = (ULONG) pMessage->Complete.IoStatus.Information;

            IoControlFile(g_ciConsoleInformation.Server,
                          IOCTL_CONDRV_WRITE_OUTPUT,
                          &IoOperation,
                          sizeof(IoOperation),
                          nullptr,
                          0);
        }

        ConsoleHeapFree(pMessage->State.OutputBuffer);
        pMessage->State.OutputBuffer = nullptr;
    }
}
