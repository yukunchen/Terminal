#pragma once

#include "DeviceComm.h"

#include "ApiMessage.h"

class DeviceProtocol
{
public:
    DeviceProtocol(_In_ DeviceComm* const pComm);
    ~DeviceProtocol();

    DWORD SetInputAvailableEvent(_In_ HANDLE Event) const;
    DWORD SetConnectionInformation(_In_ CONSOLE_API_MSG* const pMessage,
                                   _In_ ULONG_PTR ProcessDataHandle,
                                   _In_ ULONG_PTR InputHandle,
                                   _In_ ULONG_PTR OutputHandle) const;

    DWORD GetApiCall(_Out_ CONSOLE_API_MSG* const pMessage) const;
    DWORD CompleteApiCall(_In_ CONSOLE_API_MSG* const pMessage) const;

    // Auto-reads input
    DWORD GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const;

    // You must write output when done filling buffer.
    DWORD GetOutputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const;

private:

    DWORD _SetCompletionPayload(_In_ CONSOLE_API_MSG* const pMessage,
                                _In_reads_bytes_(nDataSize) LPVOID pData,
                                _In_ DWORD nDataSize) const;

    DWORD _ReadInputOperation(_In_ CONSOLE_API_MSG* const pMessage) const;

    DWORD _WriteOutputOperation(_In_ CONSOLE_API_MSG* const pMessage) const;

    DWORD _SendCompletion(_In_ CONSOLE_API_MSG* const pMessage) const;

    DWORD _FreeBuffers(_In_ CONSOLE_API_MSG* const pMessage) const;

    DeviceComm* const _pComm;


};

