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

class INPUT_READ_HANDLE_DATA;

struct _INPUT_INFORMATION;
typedef _INPUT_INFORMATION INPUT_INFORMATION;
typedef _INPUT_INFORMATION *PINPUT_INFORMATION;

class SCREEN_INFORMATION;

class ConsoleHandleData
{
public:
    ConsoleHandleData(_In_ ULONG const ulHandleType,
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

    // TODO: MSFT 9355178 Temporary public access to types... http://osgvsowi/9355178
    bool IsInputHandle() const
    {
        return _IsInput();
    }

    HRESULT CloseHandle();

    enum HandleType
    {
        Input = 0x1,
        Output = 0x2
    };

private:
    bool _IsInput() const;
    bool _IsOutput() const;

    HRESULT _CloseInputHandle();
    HRESULT _CloseOutputHandle();

    ULONG const _ulHandleType;
    ACCESS_MASK const _amAccess;
    ULONG const _ulShareAccess;
    PVOID _pvClientPointer; // This will be a pointer to a SCREEN_INFORMATION or INPUT_INFORMATION object.
    INPUT_READ_HANDLE_DATA* _pClientInput;
};

DEFINE_ENUM_FLAG_OPERATORS(ConsoleHandleData::HandleType);
