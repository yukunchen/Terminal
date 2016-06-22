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

DWORD DeviceProtocol::SetConnectionInformation(_In_ const CD_IO_DESCRIPTOR* const pAssociatedMessage,
                                               _In_ ULONG_PTR ProcessDataHandle,
                                               _In_ ULONG_PTR InputHandle,
                                               _In_ ULONG_PTR OutputHandle) const
{
    CD_CONNECTION_INFORMATION ConnectionInfo;
    ConnectionInfo.Process = ProcessDataHandle;
    ConnectionInfo.Input = InputHandle;
    ConnectionInfo.Output = OutputHandle;

    return SendCompletion(pAssociatedMessage,
                          STATUS_SUCCESS,
                          &ConnectionInfo,
                          sizeof(ConnectionInfo));
}


DWORD DeviceProtocol::GetReadIo(_Out_ CONSOLE_API_MSG* const pMessage) const
{
    // Clear out receive message
    ZeroMemory(pMessage, sizeof(*pMessage));

    // For optimization, we could be returning completions here on the next read.
    return _pComm->ReadIo(nullptr,
                          pMessage);
}

DWORD DeviceProtocol::GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage, 
									 _Outptr_result_bytebuffer_(*pBufferSize) void** const ppBuffer, 
									 _Out_ ULONG* const pBufferSize)
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
				Status = _ReadInputOperation(pMessage, 0, Payload, PayloadSize);
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

	if (SUCCEEDED(Status))
	{
		// Return the buffer.
		*ppBuffer = pMessage->State.InputBuffer;
		*pBufferSize = pMessage->State.InputBufferSize;
	}

	return Status;
}

DWORD DeviceProtocol::GetOutputBuffer(_In_ CONSOLE_API_MSG* const pMessage, 
									  _Outptr_result_bytebuffer_(*pBufferSize) void** const ppBuffer, 
									  _Out_ ULONG* const pBufferSize)
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

	if (SUCCEEDED(Status))
	{
		// Return the buffer.
		*ppBuffer = pMessage->State.OutputBuffer;
		*pBufferSize = pMessage->State.OutputBufferSize;
	}

	return Status;
}

DWORD DeviceProtocol::SendCompletion(_In_ const CD_IO_DESCRIPTOR* const pAssociatedMessage,
                                     _In_ NTSTATUS const status,
                                     _In_reads_bytes_(nDataSize) LPVOID pData,
                                     _In_ DWORD nDataSize) const
{
    CD_IO_COMPLETE Completion;
    Completion.Identifier = pAssociatedMessage->Identifier;
    Completion.IoStatus.Status = status;
    Completion.IoStatus.Information = nDataSize;
    Completion.Write.Data = pData;
    Completion.Write.Offset = 0;
    Completion.Write.Size = nDataSize;

    return _pComm->CompleteIo(&Completion);
}

DWORD DeviceProtocol::WriteOutputOperation(_In_ const CONSOLE_API_MSG* const pAssociatedMessage,
										   _In_ ULONG const Offset,
										   _In_reads_bytes_(BufferSize) void* const pBuffer,
										   _In_ ULONG const BufferSize) const
{
	CD_IO_OPERATION Op;
	Op.Identifier = pAssociatedMessage->Descriptor.Identifier;
	Op.Buffer.Offset = pAssociatedMessage->State.WriteOffset + Offset;
	Op.Buffer.Data = pBuffer;
	Op.Buffer.Size = BufferSize;

	return _pComm->WriteOutput(&Op);
}

DWORD DeviceProtocol::_ReadInputOperation(_In_ const CONSOLE_API_MSG* const pAssociatedMessage,
										 _In_ ULONG const Offset,
										 _Out_writes_bytes_(BufferSize) void* const pBuffer,
										 _In_ ULONG const BufferSize) const
{
	CD_IO_OPERATION Op;
	Op.Identifier = pAssociatedMessage->Descriptor.Identifier;
	Op.Buffer.Offset = pAssociatedMessage->State.ReadOffset + Offset;
	Op.Buffer.Data = pBuffer;
	Op.Buffer.Size = BufferSize;

	return _pComm->ReadInput(&Op);
}