#include "stdafx.h"
#include "DeviceComm.h"


DeviceComm::DeviceComm(_In_ HANDLE server) :
    _server(server)
{
}


DeviceComm::~DeviceComm()
{
}

DWORD DeviceComm::SetServerInformation(_In_ CD_IO_SERVER_INFORMATION* const pServerInfo) const
{
    return _CallIoctl(IOCTL_CONDRV_SET_SERVER_INFORMATION,
                      pServerInfo,
                      sizeof(*pServerInfo),
                      nullptr,
                      0);
}

DWORD DeviceComm::ReadIo(_In_opt_ CD_IO_COMPLETE* const pCompletion,
                         _Out_ CONSOLE_API_MSG* const pMessage) const
{
    DWORD result = _CallIoctl(IOCTL_CONDRV_READ_IO,
                              pCompletion,
                              pCompletion == nullptr ? 0 : sizeof(*pCompletion),
                              &pMessage->Descriptor,
                              sizeof(CONSOLE_API_MSG) - FIELD_OFFSET(CONSOLE_API_MSG, Descriptor));

    if (result == ERROR_IO_PENDING)
    {
        WaitForSingleObjectEx(_server, 0, FALSE);
        result = pMessage->IoStatus.Status;
    }

    if (result == STATUS_PIPE_DISCONNECTED)
    {
        result = ERROR_PIPE_NOT_CONNECTED;
    }

    return result;
}

DWORD DeviceComm::CompleteIo(_In_ CD_IO_COMPLETE* const pCompletion) const
{
    return _CallIoctl(IOCTL_CONDRV_COMPLETE_IO,
                      pCompletion,
                      sizeof(*pCompletion),
                      nullptr,
                      0);
}

DWORD DeviceComm::ReadInput(_In_ CD_IO_OPERATION* const pIoOperation) const
{
    return _CallIoctl(IOCTL_CONDRV_READ_INPUT,
                      pIoOperation,
                      sizeof(*pIoOperation),
                      nullptr,
                      0);
}

DWORD DeviceComm::WriteOutput(_In_ CD_IO_OPERATION* const pIoOperation) const
{
    return _CallIoctl(IOCTL_CONDRV_WRITE_OUTPUT,
                      pIoOperation,
                      sizeof(*pIoOperation),
                      nullptr,
                      0);
}

DWORD DeviceComm::AllowUIAccess() const
{
    return _CallIoctl(IOCTL_CONDRV_ALLOW_VIA_UIACCESS,
                      nullptr,
                      0,
                      nullptr,
                      0);
}

DWORD DeviceComm::_CallIoctl(_In_ DWORD dwIoControlCode,
                             _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
                             _In_ DWORD nInBufferSize,
                             _Out_writes_bytes_opt_(nOutBufferSize) LPVOID lpOutBuffer,
                             _In_ DWORD nOutBufferSize) const
{
    DWORD result = S_OK;

    DWORD written = 0;

    BOOL const succeeded = DeviceIoControl(_server,
                                           dwIoControlCode,
                                           lpInBuffer,
                                           nInBufferSize,
                                           lpOutBuffer,
                                           nOutBufferSize,
                                           &written,
                                           nullptr);

    if (FALSE == succeeded)
    {
        result = GetLastError();
    }

    return result;
}