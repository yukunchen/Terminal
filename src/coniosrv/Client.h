/*++
Copyright (c) Microsoft Corporation

Module Name:
- Client.h

Abstract:
- Routines to manage applications that connect to this server. Namely, these
  routines register and deregister clients, and assert whether a new client is
  eligible to be a client to this server.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

NTSTATUS
CisRegisterClient(
    _In_ PCIS_MSG Message,
    _In_ HANDLE ClientProcessHandle,
    _In_ HANDLE PipeReadHandle,
    _In_ HANDLE PipeWriteHandle,
    _In_ HANDLE AlpcSharedSectionHandle,
    _In_ SIZE_T AlpcSharedSectionSize,
    _In_ PVOID AlpcSharedSectionViewBase,
    _Outptr_ PCIS_CLIENT *ClientContext
);

VOID
CisDeregisterClient(
    _In_ PCIS_CLIENT ClientContext
);

BOOLEAN
CisCanRegisterClient(
    _In_ HANDLE ClientProcessHandle
);

BOOLEAN
CisIsClientRegistered(
    _In_ HANDLE Process
);

BOOLEAN
CisIsClientActive(
    _In_ HANDLE Process
);

BOOLEAN
CisIsClientTrusted(
    _In_ HANDLE Process
);

HANDLE
CisOpenClientProcess(
    _In_ PCIS_MSG Message
);
