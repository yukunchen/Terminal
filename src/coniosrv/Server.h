/*++
Copyright (c) Microsoft Corporation

Module Name:
- Server.h

Abstract:
- Server lifecycle management routines (initialization only since this server
  never quits).

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

NTSTATUS
CisInitializeServer(
    VOID
);

NTSTATUS
CisInitializeServerGlobals(
    VOID
);
