/*++
Copyright (c) Microsoft Corporation

Module Name:
- DeviceComm.h

Abstract:
- This module assists in communicating via IOCTL messages to and from a Device server handle.

Author:
- Michael Niksa (MiNiksa) 14-Sept-2016

Revision History:
--*/

#pragma once

#include "..\host\conapi.h"

#include <wil\resource.h>

class DeviceComm
{
public:
    DeviceComm(_In_ HANDLE Server);
    ~DeviceComm();

    HRESULT SetServerInformation(_In_ CD_IO_SERVER_INFORMATION* const pServerInfo) const;
    HRESULT ReadIo(_In_opt_ CD_IO_COMPLETE* const pCompletion,
                   _Out_ CONSOLE_API_MSG* const pMessage) const;
    HRESULT CompleteIo(_In_ CD_IO_COMPLETE* const pCompletion) const;

    HRESULT ReadInput(_In_ CD_IO_OPERATION* const pIoOperation) const;
    HRESULT WriteOutput(_In_ CD_IO_OPERATION* const pIoOperation) const;

    HRESULT AllowUIAccess() const;

private:

    HRESULT _CallIoctl(_In_ DWORD dwIoControlCode,
                       _In_reads_bytes_opt_(cbInBufferSize) PVOID pInBuffer,
                       _In_ DWORD cbInBufferSize,
                       _Out_writes_bytes_opt_(cbOutBufferSize) PVOID pOutBuffer,
                       _In_ DWORD cbOutBufferSize) const;

    wil::unique_handle _Server;

};
