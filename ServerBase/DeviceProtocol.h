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
	DWORD GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage, _Outptr_result_bytebuffer_(*pBufferSize) void** const ppBuffer, _Out_ ULONG* const pBufferSize);

    DWORD SendCompletion(_In_ const CD_IO_DESCRIPTOR* const pAssociatedMessage,
                         _In_ NTSTATUS const status,
                         _In_reads_bytes_(nDataSize) LPVOID pData,
                         _In_ DWORD nDataSize) const;

private:

	DWORD _GetInputOperation(_In_ CONSOLE_API_MSG* const pAssociatedMessage,
							_In_ ULONG const Offset,
							_Out_writes_bytes_(BufferSize) void* const pBuffer,
							_In_ ULONG const BufferSize) const;

    DeviceComm* const _pComm;


};

