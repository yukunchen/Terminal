#include "stdafx.h"
#include "DeviceProtocol.h"


DeviceProtocol::DeviceProtocol(_In_ DeviceComm* const pComm) :
    _pComm(pComm)
{
}

DeviceProtocol::~DeviceProtocol()
{
}

DWORD DeviceProtocol::SetInputAvailableEvent(_In_ HANDLE Event) const
{
    CD_IO_SERVER_INFORMATION serverInfo;
    serverInfo.InputAvailableEvent = Event;
    return _pComm->SetServerInformation(&serverInfo);
}

DWORD DeviceProtocol::SetConnectionInformation(_In_ CONSOLE_API_MSG* const pMessage,
                                               _In_ ULONG_PTR ProcessDataHandle,
                                               _In_ ULONG_PTR InputHandle,
                                               _In_ ULONG_PTR OutputHandle) const
{
	DWORD Result = STATUS_SUCCESS;

	CD_CONNECTION_INFORMATION* const pConnectionInfo = new CD_CONNECTION_INFORMATION();

	if (pConnectionInfo == nullptr)
	{
		Result = STATUS_NO_MEMORY;
	}
	
	if (NT_SUCCESS(Result))
	{
		pConnectionInfo->Process = ProcessDataHandle;
		pConnectionInfo->Input = InputHandle;
		pConnectionInfo->Output = OutputHandle;
		_SetCompletionPayload(pMessage, pConnectionInfo, sizeof(*pConnectionInfo));
	}

	return Result;
}

DWORD DeviceProtocol::GetApiCall(_Out_ CONSOLE_API_MSG* const pMessage) const
{
    // Clear out receive message
    ZeroMemory(pMessage, sizeof(*pMessage));

    // For optimization, we could be returning completions here on the next read.
    DWORD Status = _pComm->ReadIo(nullptr, pMessage);

	// Prepare completion structure by matching the identifiers.
	// Various calls might fill the rest of this structure if they would like to say something while replying.
	ZeroMemory(&pMessage->Complete, sizeof(pMessage->Complete));
	pMessage->Complete.Identifier = pMessage->Descriptor.Identifier;
	pMessage->Complete.IoStatus.Status = STATUS_SUCCESS;

	return Status;
}

DWORD DeviceProtocol::CompleteApiCall(_In_ CONSOLE_API_MSG* const pMessage) const
{
	// Send output buffer back to caller if one exists.
	_WriteOutputOperation(pMessage);

	// Reply to caller that we're done servicing the API call.
	DWORD Result = _SendCompletion(pMessage);

	// Clear all buffers that were allocated for message processing
	_FreeBuffers(pMessage);

	return Result;
}

DWORD DeviceProtocol::GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const
{
	DWORD Status = STATUS_SUCCESS;

	// Initialize the buffer if it hasn't been initialized yet.
	if (pMessage->State.InputBuffer == nullptr)
	{
		if (pMessage->State.ReadOffset > pMessage->Descriptor.InputSize)
		{
			Status = STATUS_UNSUCCESSFUL;
		}

		if (SUCCEEDED(Status))
		{
			ULONG const PayloadSize = pMessage->Descriptor.InputSize - pMessage->State.ReadOffset;

			BYTE* const Payload = new BYTE[PayloadSize];
			if (Payload == nullptr)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}

			if (SUCCEEDED(Status))
			{
				Status = _ReadInputOperation(pMessage);
				if (SUCCEEDED(Status))
				{
					pMessage->State.InputBuffer = Payload;
					pMessage->State.InputBufferSize = PayloadSize;
				}
				else
				{
					delete[] Payload;
				}
			}
		}
	}

	return Status;
}

DWORD DeviceProtocol::GetOutputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const
{
	DWORD Status = STATUS_SUCCESS;

	// Initialize the buffer if it hasn't been initialized yet.
	if (pMessage->State.OutputBuffer == nullptr)
	{
		if (pMessage->State.WriteOffset > pMessage->Descriptor.OutputSize)
		{
			Status = STATUS_UNSUCCESSFUL;
		}

		if (SUCCEEDED(Status))
		{
			ULONG const PayloadSize = pMessage->Descriptor.OutputSize - pMessage->State.WriteOffset;

			BYTE* const Payload = new BYTE[PayloadSize];
			if (Payload == nullptr)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}

			if (SUCCEEDED(Status))
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

	return Status;
}

DWORD DeviceProtocol::_SendCompletion(_In_ CONSOLE_API_MSG* const pMessage) const
{
	if (ERROR_IO_PENDING != pMessage->Complete.IoStatus.Status)
	{
		return _pComm->CompleteIo(&pMessage->Complete);
	}
	else
	{
		return ERROR_IO_PENDING;
	}
}

DWORD DeviceProtocol::SetCompletionStatus(_In_ CONSOLE_API_MSG* const pMessage,
										  _In_ DWORD const Status) const
{
	pMessage->Complete.IoStatus.Status = Status;

	return STATUS_SUCCESS;
}

DWORD DeviceProtocol::_SetCompletionPayload(_In_ CONSOLE_API_MSG* const pMessage,
										    _In_reads_bytes_(nDataSize) LPVOID pData,
											_In_ DWORD nDataSize) const
{
	// Not necessary, this is set up when we receive the packet initially.
	//pMessage->Complete.Identifier = pMessage->Descriptor.Identifier;

    pMessage->Complete.IoStatus.Information = nDataSize;
    pMessage->Complete.Write.Data = pData;
    pMessage->Complete.Write.Offset = 0;
    pMessage->Complete.Write.Size = nDataSize;

	return STATUS_SUCCESS;
}

DWORD DeviceProtocol::_ReadInputOperation(_In_ CONSOLE_API_MSG* const pMessage) const
{
	CD_IO_OPERATION Op;
	Op.Identifier = pMessage->Descriptor.Identifier;
	Op.Buffer.Offset = pMessage->State.ReadOffset;
	Op.Buffer.Data = pMessage->State.InputBuffer;
	Op.Buffer.Size = pMessage->State.InputBufferSize;

	return _pComm->ReadInput(&Op);
}

DWORD DeviceProtocol::_WriteOutputOperation(_In_ CONSOLE_API_MSG* const pMessage) const
{
	CD_IO_OPERATION Op;
	Op.Identifier = pMessage->Descriptor.Identifier;
	Op.Buffer.Offset = pMessage->State.WriteOffset;
	Op.Buffer.Data = pMessage->State.OutputBuffer;
	Op.Buffer.Size = pMessage->State.OutputBufferSize;

	return _pComm->WriteOutput(&Op);
}

DWORD DeviceProtocol::_FreeBuffers(_In_ CONSOLE_API_MSG* const pMessage) const
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

	return STATUS_SUCCESS;
}
