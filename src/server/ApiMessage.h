/*++
Copyright (c) Microsoft Corporation

Module Name:
- ApiMessage.h

Abstract:
- This file extends the published structure of an API message to provide encapsulation and helper methods

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in util.cpp & conapi.h & csrutil.cpp
--*/

#pragma once

#include "ApiMessageState.h"

class ConsoleProcessHandle;
class ConsoleHandleData;

class DeviceComm;

typedef struct _CONSOLE_API_MSG
{
    CD_IO_COMPLETE Complete;
    CONSOLE_API_STATE State;

    DeviceComm* _pDeviceComm;

    // From here down is the actual packet data sent/received.
    CD_IO_DESCRIPTOR Descriptor;
    union
    {
        struct
        {
            CD_CREATE_OBJECT_INFORMATION CreateObject;
            CONSOLE_CREATESCREENBUFFER_MSG CreateScreenBuffer;
        };
        struct
        {
            CONSOLE_MSG_HEADER msgHeader;
            union
            {
                CONSOLE_MSG_BODY_L1 consoleMsgL1;
                CONSOLE_MSG_BODY_L2 consoleMsgL2;
                CONSOLE_MSG_BODY_L3 consoleMsgL3;
            } u;
        };
    };
    // End packet data

public:
    ConsoleProcessHandle* GetProcessHandle() const;
    ConsoleHandleData* GetObjectHandle() const;

    HRESULT ReadMessageInput(_In_ const ULONG cbOffset, _Out_writes_bytes_(cbSize) PVOID pvBuffer, _In_ const ULONG cbSize);
    HRESULT GetAugmentedOutputBuffer(_In_ const ULONG cbFactor,
                                      _Outptr_result_bytebuffer_(*pcbSize) PVOID * ppvBuffer,
                                      _Out_ PULONG pcbSize);
    HRESULT GetOutputBuffer(_Outptr_result_bytebuffer_(*pcbSize) void** const ppvBuffer, _Out_ ULONG* const pcbSize);
    HRESULT GetInputBuffer(_Outptr_result_bytebuffer_(*pcbSize) void** const ppvBuffer, _Out_ ULONG* const pcbSize);

    HRESULT ReleaseMessageBuffers();

} CONSOLE_API_MSG, *PCONSOLE_API_MSG, *const PCCONSOLE_API_MSG;
