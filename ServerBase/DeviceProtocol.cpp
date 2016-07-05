#include "stdafx.h"
#include "DeviceProtocol.h"


DeviceProtocol::DeviceProtocol(_In_ DeviceComm* const pComm) :
    _pComm(pComm)
{
}

DeviceProtocol::~DeviceProtocol()
{
}

HRESULT DeviceProtocol::SetInputAvailableEvent(_In_ HANDLE Event) const
{
    CD_IO_SERVER_INFORMATION serverInfo;
    serverInfo.InputAvailableEvent = Event;
    return _pComm->SetServerInformation(&serverInfo);
}

HRESULT DeviceProtocol::SetConnectionInformation(_In_ CONSOLE_API_MSG* const pMessage,
                                               _In_ ULONG_PTR ProcessDataHandle,
                                               _In_ ULONG_PTR InputHandle,
                                               _In_ ULONG_PTR OutputHandle) const
{
    HRESULT Result = S_OK;

    CD_CONNECTION_INFORMATION* const pConnectionInfo = new CD_CONNECTION_INFORMATION();

    if (pConnectionInfo == nullptr)
    {
        Result = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(Result))
    {
        pConnectionInfo->Process = ProcessDataHandle;
        pConnectionInfo->Input = InputHandle;
        pConnectionInfo->Output = OutputHandle;
        Result = _SetCompletionPayload(pMessage, pConnectionInfo, sizeof(*pConnectionInfo));
    }

    return Result;
}

HRESULT DeviceProtocol::GetApiCall(_Out_ CONSOLE_API_MSG* const pMessage) const
{
    // Clear out receive message
    ZeroMemory(pMessage, sizeof(*pMessage));

    // For optimization, we could be returning completions here on the next read.
    HRESULT Result = _pComm->ReadIo(nullptr, pMessage);

    // Prepare completion structure by matching the identifiers.
    // Various calls might fill the rest of this structure if they would like to say something while replying.
    ZeroMemory(&pMessage->Complete, sizeof(pMessage->Complete));
    pMessage->Complete.Identifier = pMessage->Descriptor.Identifier;
    pMessage->Complete.IoStatus.Status = STATUS_SUCCESS;

    return Result;
}

HRESULT DeviceProtocol::CompleteApiCall(_In_ CONSOLE_API_MSG* const pMessage) const
{
    HRESULT Result = S_OK;

    // Send output buffer back to caller if one exists.
    if (pMessage->IsOutputBufferAvailable())
    {
        Result = _WriteOutputOperation(pMessage);

        // If our write back succeeded, set the final information with the amount of data written.
        if (SUCCEEDED(Result))
        {
            pMessage->IoStatus.Information = pMessage->State.OutputBufferSize;
        }
    }

    // Reply to caller that we're done servicing the API call.
    Result = _SendCompletion(pMessage);

    // Clear all buffers that were allocated for message processing
    _FreeBuffers(pMessage);

    return Result;
}

HRESULT DeviceProtocol::GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const
{
    HRESULT Result = S_OK;

    // Initialize the buffer if it hasn't been initialized yet.
    if (pMessage->State.InputBuffer == nullptr)
    {
        if (pMessage->State.ReadOffset > pMessage->Descriptor.InputSize)
        {
            Result = E_INVALIDARG;
        }

        if (SUCCEEDED(Result))
        {
            ULONG const PayloadSize = pMessage->Descriptor.InputSize - pMessage->State.ReadOffset;

            BYTE* const Payload = new BYTE[PayloadSize];
            if (Payload == nullptr)
            {
                Result = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(Result))
            {
                pMessage->State.InputBuffer = Payload;
                pMessage->State.InputBufferSize = PayloadSize;

                Result = _ReadInputOperation(pMessage);
            }
        }
    }

    return Result;
}

HRESULT DeviceProtocol::GetOutputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const
{
    HRESULT Result = S_OK;

    // Initialize the buffer if it hasn't been initialized yet.
    if (pMessage->State.OutputBuffer == nullptr)
    {
        if (pMessage->State.WriteOffset > pMessage->Descriptor.OutputSize)
        {
            Result = E_INVALIDARG;
        }

        if (SUCCEEDED(Result))
        {
            ULONG const PayloadSize = pMessage->Descriptor.OutputSize - pMessage->State.WriteOffset;

            BYTE* const Payload = new BYTE[PayloadSize];
            if (Payload == nullptr)
            {
                Result = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(Result))
            {
                pMessage->State.OutputBuffer = Payload;
                pMessage->State.OutputBufferSize = PayloadSize;
            }
            else
            {
                delete[] Payload;
            }
        }
    }

    return Result;
}

HRESULT DeviceProtocol::_SendCompletion(_In_ CONSOLE_API_MSG* const pMessage) const
{
    if (STATUS_PENDING == pMessage->Complete.IoStatus.Status)
    {
        return ERROR_IO_PENDING;
    }
    else
    {
        return _pComm->CompleteIo(&pMessage->Complete);
    }
}

HRESULT DeviceProtocol::_SetCompletionPayload(_In_ CONSOLE_API_MSG* const pMessage,
                                            _In_reads_bytes_(nDataSize) LPVOID pData,
                                            _In_ DWORD nDataSize) const
{
    // Not necessary, this is set up when we receive the packet initially.
    //pMessage->Complete.Identifier = pMessage->Descriptor.Identifier;

    pMessage->Complete.IoStatus.Information = nDataSize;
    pMessage->Complete.Write.Data = pData;
    pMessage->Complete.Write.Offset = 0;
    pMessage->Complete.Write.Size = nDataSize;

    return S_OK;
}

HRESULT DeviceProtocol::_ReadInputOperation(_In_ CONSOLE_API_MSG* const pMessage) const
{
    CD_IO_OPERATION Op;
    Op.Identifier = pMessage->Descriptor.Identifier;
    Op.Buffer.Offset = pMessage->State.ReadOffset;
    Op.Buffer.Data = pMessage->State.InputBuffer;
    Op.Buffer.Size = pMessage->State.InputBufferSize;

    return _pComm->ReadInput(&Op);
}

HRESULT DeviceProtocol::_WriteOutputOperation(_In_ CONSOLE_API_MSG* const pMessage) const
{
    CD_IO_OPERATION Op;
    Op.Identifier = pMessage->Descriptor.Identifier;
    Op.Buffer.Offset = pMessage->State.WriteOffset;
    Op.Buffer.Data = pMessage->State.OutputBuffer;
    Op.Buffer.Size = pMessage->State.OutputBufferSize;

    return _pComm->WriteOutput(&Op);
}

HRESULT DeviceProtocol::_FreeBuffers(_In_ CONSOLE_API_MSG* const pMessage) const
{
    if (nullptr != pMessage->Complete.Write.Data)
    {
        delete pMessage->Complete.Write.Data;
        pMessage->Complete.Write.Data = nullptr;
    }

    if (nullptr != pMessage->State.InputBuffer)
    {
        delete[] pMessage->State.InputBuffer;
        pMessage->State.InputBuffer = nullptr;
    }

    if (nullptr != pMessage->State.OutputBuffer)
    {
        delete[] pMessage->State.OutputBuffer;
        pMessage->State.OutputBuffer = nullptr;
    }

    return S_OK;
}
