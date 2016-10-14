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
//#define CONSOLE_GRAPHICS_OUTPUT_HANDLE 0x00000004

class INPUT_READ_HANDLE_DATA;

struct _INPUT_INFORMATION;
typedef _INPUT_INFORMATION INPUT_INFORMATION;
typedef _INPUT_INFORMATION *PINPUT_INFORMATION;

class SCREEN_INFORMATION;

typedef struct _CONSOLE_HANDLE_DATA
{
    _CONSOLE_HANDLE_DATA(_In_ ULONG const ulHandleType,
                         _In_ ACCESS_MASK const amAccess,
                         _In_ ULONG const ulShareAccess,
                         _In_ PVOID const pvClientPointer);

    HRESULT GetInputBuffer(_In_ const ACCESS_MASK amRequested,
                           _Out_ INPUT_INFORMATION** const ppInputInfo) const;
    HRESULT GetScreenBuffer(_In_ const ACCESS_MASK amRequested,
                            _Out_ SCREEN_INFORMATION** const ppScreenInfo) const;

    INPUT_READ_HANDLE_DATA* GetClientInput() const;

    bool IsReadAllowed() const;
    bool IsReadShared() const;
    bool IsWriteAllowed() const;
    bool IsWriteShared() const;

    // TODO: Temporary public access to types...
    bool IsInputHandle() const
    {
        return _IsInput();
    }

    HRESULT CloseHandle();

private:
    bool _IsInput() const;
    bool _IsOutput() const;

    HRESULT _CloseInputHandle();
    HRESULT _CloseOutputHandle();

    ULONG const _ulHandleType;
    ACCESS_MASK const _amAccess;
    ULONG const _ulShareAccess;
    PVOID const _pvClientPointer; // This will be a pointer to a SCREEN_INFORMATION or INPUT_INFORMATION object.
    INPUT_READ_HANDLE_DATA* _pClientInput;

} CONSOLE_HANDLE_DATA, *PCONSOLE_HANDLE_DATA;
