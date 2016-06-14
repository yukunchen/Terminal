#pragma once

#include "DeviceComm.h"

class DeviceProtocol
{
public:
    DeviceProtocol(_In_ DeviceComm* const pComm);
    ~DeviceProtocol();

    DWORD SetInputAvailableEvent(_In_ HANDLE Event) const;
    DWORD SetConnectionInformation(_In_ const CD_IO_DESCRIPTOR* const pAssociatedMessage,
                                   _In_ ULONG_PTR ProcessDataHandle,
                                   _In_ ULONG_PTR InputHandle,
                                   _In_ ULONG_PTR OutputHandle) const;

    DWORD GetReadIo(_Out_ CONSOLE_API_MSG* const pMessage) const;

    DWORD SendCompletion(_In_ const CD_IO_DESCRIPTOR* const pAssociatedMessage,
                         _In_ NTSTATUS const status,
                         _In_reads_bytes_(nDataSize) LPVOID pData,
                         _In_ DWORD nDataSize) const;

    DeviceComm* const _pComm;
};

