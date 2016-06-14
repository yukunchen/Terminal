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
