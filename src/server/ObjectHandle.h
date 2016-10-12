/*++
Copyright (c) Microsoft Corporation

Module Name:
- ObjectHandle.h

Abstract:
- This file defines a handle associated with a console input or output buffer object.
- This is used to expose a handle to a client application via the API.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#define CONSOLE_INPUT_HANDLE           0x00000001
#define CONSOLE_OUTPUT_HANDLE          0x00000002
#define CONSOLE_GRAPHICS_OUTPUT_HANDLE 0x00000004

class INPUT_READ_HANDLE_DATA;

typedef struct _CONSOLE_HANDLE_DATA
{
    ULONG HandleType;
    ACCESS_MASK Access;
    ULONG ShareAccess;
    PVOID ClientPointer; // This will be a pointer to a SCREEN_INFORMATION or INPUT_INFORMATION object.
    INPUT_READ_HANDLE_DATA * pClientInput;
} CONSOLE_HANDLE_DATA, *PCONSOLE_HANDLE_DATA;
