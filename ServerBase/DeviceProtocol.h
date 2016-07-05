#pragma once

#include "DeviceComm.h"

#include "ApiMessage.h"

class DeviceProtocol
{
public:
    DeviceProtocol(_In_ DeviceComm* const pComm);
    ~DeviceProtocol();

    HRESULT SetInputAvailableEvent(_In_ HANDLE Event) const;
    HRESULT SetConnectionInformation(_In_ CONSOLE_API_MSG* const pMessage,
                                   _In_ ULONG_PTR ProcessDataHandle,
                                   _In_ ULONG_PTR InputHandle,
                                   _In_ ULONG_PTR OutputHandle) const;

    HRESULT GetApiCall(_Out_ CONSOLE_API_MSG* const pMessage) const;
    HRESULT CompleteApiCall(_In_ CONSOLE_API_MSG* const pMessage) const;

    // Auto-reads input
    HRESULT GetInputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const;

    // You must write output when done filling buffer.
    HRESULT GetOutputBuffer(_In_ CONSOLE_API_MSG* const pMessage) const;

private:

    HRESULT _SetCompletionPayload(_In_ CONSOLE_API_MSG* const pMessage,
                                _In_reads_bytes_(nDataSize) LPVOID pData,
                                _In_ DWORD nDataSize) const;

    HRESULT _ReadInputOperation(_In_ CONSOLE_API_MSG* const pMessage) const;

    HRESULT _WriteOutputOperation(_In_ CONSOLE_API_MSG* const pMessage) const;

    HRESULT _SendCompletion(_In_ CONSOLE_API_MSG* const pMessage) const;

    HRESULT _FreeBuffers(_In_ CONSOLE_API_MSG* const pMessage) const;

    DeviceComm* const _pComm;


};

