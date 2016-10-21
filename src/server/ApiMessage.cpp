/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include <intsafe.h>

#include "ApiMessage.h"
#include "DeviceComm.h"

ConsoleProcessHandle* _CONSOLE_API_MSG::GetProcessHandle() const
{
    return reinterpret_cast<ConsoleProcessHandle*>(Descriptor.Process);
}

ConsoleHandleData* _CONSOLE_API_MSG::GetObjectHandle() const
{
    return reinterpret_cast<ConsoleHandleData*>(Descriptor.Object);
}

// Routine Description:
// - This routine reads [part of] the input payload of the given message.
// Arguments:
// - Offset - Supplies the offset from which to start reading the payload.
// - Buffer - Receives the payload.
// - Size - Supplies the number of bytes to be read into the buffer.
// Return Value:
// - HRESULT indicating if the payload was successfully read.
HRESULT _CONSOLE_API_MSG::ReadMessageInput(_In_ const ULONG cbOffset,
                                           _Out_writes_bytes_(cbSize) PVOID pvBuffer,
                                           _In_ const ULONG cbSize)
{
    CD_IO_OPERATION IoOperation;
    IoOperation.Identifier = Descriptor.Identifier;
    IoOperation.Buffer.Offset = State.ReadOffset + cbOffset;
    IoOperation.Buffer.Data = pvBuffer;
    IoOperation.Buffer.Size = cbSize;

    return _pDeviceComm->ReadInput(&IoOperation);
}

// Routine Description:
// - This routine retrieves the input buffer associated with this message. It will allocate one if needed.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Message - Supplies the message whose input buffer will be retrieved.
// - Buffer - Receives a pointer to the input buffer.
// - Size - Receives the size, in bytes, of the input buffer.
// Return Value:
// -  HRESULT indicating if the input buffer was successfully retrieved.
HRESULT _CONSOLE_API_MSG::GetInputBuffer(_Outptr_result_bytebuffer_(*pcbSize) void** const ppvBuffer,
                                         _Out_ ULONG* const pcbSize)
{
    // Initialize the buffer if it hasn't been initialized yet.
    if (State.InputBuffer == nullptr)
    {
        RETURN_HR_IF(E_FAIL, State.ReadOffset > Descriptor.InputSize);

        ULONG const cbReadSize = Descriptor.InputSize - State.ReadOffset;

        wistd::unique_ptr<BYTE[]> pPayload = wil::make_unique_nothrow<BYTE[]>(cbReadSize);
        RETURN_IF_NULL_ALLOC(pPayload);

        RETURN_IF_FAILED(ReadMessageInput(0, pPayload.get(), cbReadSize));

        State.InputBuffer = pPayload.release(); // TODO: don't release, maintain as smart pointer.
        State.InputBufferSize = cbReadSize;
    }

    // Return the buffer.
    *ppvBuffer = State.InputBuffer;
    *pcbSize = State.InputBufferSize;

    return S_OK;
}

// Routine Description:
// - This routine retrieves the output buffer associated with this message. It will allocate one if needed.
//   The allocated will be bigger than the actual output size by the requested factor.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Factor - Supplies the factor to multiply the allocated buffer by.
// - Buffer - Receives a pointer to the output buffer.
// - Size - Receives the size, in bytes, of the output buffer.
//  Return Value:
// - HRESULT indicating if the output buffer was successfully retrieved.
HRESULT _CONSOLE_API_MSG::GetAugmentedOutputBuffer(_In_ const ULONG cbFactor,
                                                   _Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer,
                                                   _Out_ PULONG pcbSize)
{
    // Initialize the buffer if it hasn't been initialized yet.
    if (State.OutputBuffer == nullptr)
    {
        RETURN_HR_IF(E_FAIL, State.WriteOffset > Descriptor.OutputSize);

        ULONG cbWriteSize = Descriptor.OutputSize - State.WriteOffset;
        RETURN_IF_FAILED(ULongMult(cbWriteSize, cbFactor, &cbWriteSize));

        BYTE* pPayload = new BYTE[cbWriteSize];
        RETURN_IF_NULL_ALLOC(pPayload);

        State.OutputBuffer = pPayload; // TODO: maintain as smart pointer.
        State.OutputBufferSize = cbWriteSize;
    }

    // Return the buffer.
    *ppvBuffer = State.OutputBuffer;
    *pcbSize = State.OutputBufferSize;

    return S_OK;
}

// Routine Description:
// - This routine retrieves the output buffer associated with this message. It will allocate one if needed.
// - Before completing the message, ReleaseMessageBuffers must be called to free any allocation performed by this routine.
// Arguments:
// - Message - Supplies the message whose output buffer will be retrieved.
// - Buffer - Receives a pointer to the output buffer.
// - Size - Receives the size, in bytes, of the output buffer.
// Return Value:
// - HRESULT indicating if the output buffer was successfully retrieved.
HRESULT _CONSOLE_API_MSG::GetOutputBuffer(_Outptr_result_bytebuffer_(*pcbSize) PVOID * const ppvBuffer,
                                          _Out_ ULONG * const pcbSize)
{
    return GetAugmentedOutputBuffer(1, ppvBuffer, pcbSize);
}

// Routine Description:
// - This routine releases output or input buffers that might have been allocated
//   during the processing of the given message. If the current completion status
//   of the message indicates success, this routine also writes the output buffer
//   (if any) to the message.
// Arguments:
// - <none>
// Return Value:
// - HRESULT indicating if the payload was successfully written if applicable.
HRESULT _CONSOLE_API_MSG::ReleaseMessageBuffers()
{
    HRESULT hr = S_OK;

    if (State.InputBuffer != nullptr)
    {
        delete[] State.InputBuffer;
        State.InputBuffer = nullptr;
    }

    if (State.OutputBuffer != nullptr)
    {
        if (NT_SUCCESS(Complete.IoStatus.Status))
        {
            CD_IO_OPERATION IoOperation;
            IoOperation.Identifier = Descriptor.Identifier;
            IoOperation.Buffer.Offset = State.WriteOffset;
            IoOperation.Buffer.Data = State.OutputBuffer;
            IoOperation.Buffer.Size = (ULONG)Complete.IoStatus.Information;

            LOG_IF_FAILED(_pDeviceComm->WriteOutput(&IoOperation));
        }

        delete[] State.OutputBuffer;
        State.OutputBuffer = nullptr;
    }

    return hr;
}

