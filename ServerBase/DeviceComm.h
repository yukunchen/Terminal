#pragma once
class DeviceComm
{
public:
    DeviceComm(_In_ HANDLE server);
    ~DeviceComm();

    DWORD SetServerInformation(_In_ CD_IO_SERVER_INFORMATION* const pServerInfo) const;
    DWORD ReadIo(_In_opt_ CD_IO_COMPLETE* const pCompletion,
                 _Out_ CONSOLE_API_MSG* const pMessage) const;
    DWORD CompleteIo(_In_ CD_IO_COMPLETE* const pCompletion) const;

	DWORD ReadInput(_In_ CD_IO_OPERATION* const pIoOperation) const;

private:

    DWORD _CallIoctl(_In_ DWORD dwIoControlCode,
                     _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
                     _In_ DWORD nInBufferSize,
                     _Out_writes_bytes_opt_(nOutBufferSize) LPVOID lpOutBuffer,
                     _In_ DWORD nOutBufferSize) const;
    
    HANDLE _server;

};

