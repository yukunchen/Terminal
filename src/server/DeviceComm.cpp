/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "DeviceComm.h"

DeviceComm::DeviceComm(_In_ HANDLE Server) :
    _Server(Server)
{
    THROW_IF_HANDLE_INVALID(Server);
}


DeviceComm::~DeviceComm()
{
}

HRESULT DeviceComm::SetServerInformation(_In_ CD_IO_SERVER_INFORMATION* const pServerInfo) const
{
    return _CallIoctl(IOCTL_CONDRV_SET_SERVER_INFORMATION,
                      pServerInfo,
                      sizeof(*pServerInfo),
                      nullptr,
                      0);
}

HRESULT DeviceComm::ReadIo(_In_opt_ CD_IO_COMPLETE* const pCompletion,
                           _Out_ CONSOLE_API_MSG* const pMessage) const
{
    HRESULT Result = _CallIoctl(IOCTL_CONDRV_READ_IO,
                                pCompletion,
                                pCompletion == nullptr ? 0 : sizeof(*pCompletion),
                                &pMessage->Descriptor,
                                sizeof(CONSOLE_API_MSG) - FIELD_OFFSET(CONSOLE_API_MSG, Descriptor));

    if (Result == ERROR_IO_PENDING)
    {
        WaitForSingleObjectEx(_Server.get(), 0, FALSE);
        Result = pMessage->IoStatus.Status;
    }

    return Result;
}

HRESULT DeviceComm::CompleteIo(_In_ CD_IO_COMPLETE* const pCompletion) const
{
    return _CallIoctl(IOCTL_CONDRV_COMPLETE_IO,
                      pCompletion,
                      sizeof(*pCompletion),
                      nullptr,
                      0);
}

HRESULT DeviceComm::ReadInput(_In_ CD_IO_OPERATION* const pIoOperation) const
{
    return _CallIoctl(IOCTL_CONDRV_READ_INPUT,
                      pIoOperation,
                      sizeof(*pIoOperation),
                      nullptr,
                      0);
}

HRESULT DeviceComm::WriteOutput(_In_ CD_IO_OPERATION* const pIoOperation) const
{
    return _CallIoctl(IOCTL_CONDRV_WRITE_OUTPUT,
                      pIoOperation,
                      sizeof(*pIoOperation),
                      nullptr,
                      0);
}

HRESULT DeviceComm::AllowUIAccess() const
{
    return _CallIoctl(IOCTL_CONDRV_ALLOW_VIA_UIACCESS,
                      nullptr,
                      0,
                      nullptr,
                      0);
}

HRESULT DeviceComm::_CallIoctl(_In_ DWORD dwIoControlCode,
                               _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
                               _In_ DWORD nInBufferSize,
                               _Out_writes_bytes_opt_(nOutBufferSize) LPVOID lpOutBuffer,
                               _In_ DWORD nOutBufferSize) const
{
    DWORD written = 0;
    RETURN_IF_WIN32_BOOL_FALSE(DeviceIoControl(_Server.get(),
                                               dwIoControlCode,
                                               lpInBuffer,
                                               nInBufferSize,
                                               lpOutBuffer,
                                               nOutBufferSize,
                                               &written,
                                               nullptr));

    return S_OK;
}
