/*++
Copyright (c) Microsoft Corporation

Module Name:
- conapi.h

Abstract:
- This module contains the internal structures and definitions used by the console server.

Author:
- Therese Stowell (ThereseS) 12-Nov-1990

Revision History:
--*/

#pragma once

#include <condrv.h>
#include <conmsgl3.h>

#include "..\server\ObjectHeader.h"

typedef struct _CONSOLE_API_STATE
{
    ULONG WriteOffset;
    ULONG ReadOffset;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PVOID InputBuffer;
    PVOID OutputBuffer;

    PWCHAR TransBuffer;
    BOOLEAN StackBuffer;
    ULONG WriteFlags; // unused when WriteChars legacy is gone.
} CONSOLE_API_STATE, *PCONSOLE_API_STATE, * const PCCONSOLE_API_STATE;

typedef struct _CONSOLE_API_MSG
{
    CD_IO_COMPLETE Complete;
    CONSOLE_API_STATE State;
    IO_STATUS_BLOCK IoStatus;
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
} CONSOLE_API_MSG, *PCONSOLE_API_MSG, *const PCCONSOLE_API_MSG;
