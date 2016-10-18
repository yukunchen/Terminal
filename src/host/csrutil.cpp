/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#pragma hdrstop



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

    return g_pDeviceComm->ReadInput(&IoOperation);
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

        PVOID const Payload = new BYTE[ReadSize];
        if (Payload == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS Status = ReadMessageInput(pMessage, 0, Payload, ReadSize);
        if (!NT_SUCCESS(Status))
        {
            delete[] Payload;
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
        pMessage->State.OutputBuffer = new BYTE[pMessage->State.OutputBufferSize];

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
        delete[] pMessage->State.InputBuffer;
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

            LOG_IF_FAILED(g_pDeviceComm->WriteOutput(&IoOperation));
        }

        delete[] pMessage->State.OutputBuffer;
        pMessage->State.OutputBuffer = nullptr;
    }
}
