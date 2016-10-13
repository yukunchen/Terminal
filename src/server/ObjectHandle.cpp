/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ObjectHandle.h"

#include "..\host\inputReadHandleData.h"

_CONSOLE_HANDLE_DATA::_CONSOLE_HANDLE_DATA(_In_ ULONG const ulHandleType,
                                           _In_ ACCESS_MASK const amAccess,
                                           _In_ ULONG const ulShareAccess,
                                           _In_ PVOID const pvClientPointer) :
    _ulHandleType(ulHandleType),
    _amAccess(amAccess),
    _ulShareAccess(ulShareAccess),
    _pvClientPointer(pvClientPointer),
    _pClientInput(nullptr)
{
    if (_IsInput())
    {
        _pClientInput = new INPUT_READ_HANDLE_DATA();
        THROW_IF_NULL_ALLOC(_pClientInput);
    }
}

bool _CONSOLE_HANDLE_DATA::_IsInput() const
{
    return IsFlagSet(_ulHandleType, CONSOLE_INPUT_HANDLE);
}

bool _CONSOLE_HANDLE_DATA::_IsOutput() const
{
    return IsFlagSet(_ulHandleType, CONSOLE_OUTPUT_HANDLE);
}

bool _CONSOLE_HANDLE_DATA::IsReadAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_READ);
}

bool _CONSOLE_HANDLE_DATA::IsReadShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_READ);
}

bool _CONSOLE_HANDLE_DATA::IsWriteAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_WRITE);
}

bool _CONSOLE_HANDLE_DATA::IsWriteShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_WRITE);
}

// Routine Description:
// - Retieves the properly typed Input Buffer from the Handle.
// Arguments:
// - HandleData - The HANDLE containing the pointer to the input buffer, typically from an API call.
// Return Value:
// - The Input Buffer that this handle points to
INPUT_INFORMATION* _CONSOLE_HANDLE_DATA::GetInputBuffer() const
{
    assert(_IsInput());
    return static_cast<INPUT_INFORMATION*>(_pvClientPointer);
}

// Routine Description:
// - Retieves the properly typed Screen Buffer from the Handle. 
// Arguments:
// - HandleData - The HANDLE containing the pointer to the screen buffer, typically from an API call.
// Return Value:
// - The Screen Buffer that this handle points to.
SCREEN_INFORMATION* _CONSOLE_HANDLE_DATA::GetScreenBuffer() const
{
    assert(_IsOutput());
    return static_cast<SCREEN_INFORMATION*>(_pvClientPointer);
}

INPUT_READ_HANDLE_DATA* _CONSOLE_HANDLE_DATA::GetClientInput() const
{
    return _pClientInput;
}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - ProcessData - Pointer to per process data structure.
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:
NTSTATUS _CONSOLE_HANDLE_DATA::DereferenceIoHandle(_In_ const ULONG ulHandleType,
                                                   _In_ const ACCESS_MASK amRequested)
{
    // Check the type and the granted access.
    if (((_amAccess & amRequested) == 0) ||
        ((ulHandleType != 0) &&
        ((_ulHandleType & ulHandleType) == 0)))
    {
        return STATUS_INVALID_HANDLE;
    }

    return STATUS_SUCCESS;
}
