/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ObjectHandle.h"

#include "..\host\inputReadHandleData.h"
#include "..\host\input.h"
#include "..\host\screenInfo.hpp"

ConsoleHandleData::ConsoleHandleData(_In_ ULONG const ulHandleType,
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

bool ConsoleHandleData::_IsInput() const
{
    return IsFlagSet(_ulHandleType, CONSOLE_INPUT_HANDLE);
}

bool ConsoleHandleData::_IsOutput() const
{
    return IsFlagSet(_ulHandleType, CONSOLE_OUTPUT_HANDLE);
}

bool ConsoleHandleData::IsReadAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_READ);
}

bool ConsoleHandleData::IsReadShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_READ);
}

bool ConsoleHandleData::IsWriteAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_WRITE);
}

bool ConsoleHandleData::IsWriteShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_WRITE);
}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - ProcessData - Pointer to per process data structure.
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:

// Routine Description:
// - Retieves the properly typed Input Buffer from the Handle.
// Arguments:
// - HandleData - The HANDLE containing the pointer to the input buffer, typically from an API call.
// Return Value:
// - The Input Buffer that this handle points to
HRESULT ConsoleHandleData::GetInputBuffer(_In_ const ACCESS_MASK amRequested,
                                             _Out_ INPUT_INFORMATION** const ppInputInfo) const
{
    *ppInputInfo = nullptr;

    RETURN_HR_IF(E_ACCESSDENIED, IsAnyFlagClear(_amAccess, amRequested));
    RETURN_HR_IF(E_HANDLE, IsAnyFlagClear(_ulHandleType, CONSOLE_INPUT_HANDLE));

    *ppInputInfo = static_cast<INPUT_INFORMATION*>(_pvClientPointer);

    return S_OK;
}

// Routine Description:
// - Retieves the properly typed Screen Buffer from the Handle. 
// Arguments:
// - HandleData - The HANDLE containing the pointer to the screen buffer, typically from an API call.
// Return Value:
// - The Screen Buffer that this handle points to.
HRESULT ConsoleHandleData::GetScreenBuffer(_In_ const ACCESS_MASK amRequested,
                                              _Out_ SCREEN_INFORMATION** const ppScreenInfo) const
{
    *ppScreenInfo = nullptr;

    RETURN_HR_IF(E_ACCESSDENIED, IsAnyFlagClear(_amAccess, amRequested));
    RETURN_HR_IF(E_HANDLE, IsAnyFlagClear(_ulHandleType, CONSOLE_OUTPUT_HANDLE));

    *ppScreenInfo = static_cast<SCREEN_INFORMATION*>(_pvClientPointer);

    return S_OK;
}

INPUT_READ_HANDLE_DATA* ConsoleHandleData::GetClientInput() const
{
    return _pClientInput;
}

// TODO: Consider making this a part of the destructor.
HRESULT ConsoleHandleData::CloseHandle()
{
    if (_IsInput())
    {
        return _CloseInputHandle();
    }
    else if (_IsOutput())
    {
        return _CloseOutputHandle();
    }
    else
    {
        return E_NOTIMPL;
    }
}

// Routine Description:
// - This routine closes an input handle.  It decrements the input buffer's
//   reference count.  If it goes to zero, the buffer is reinitialized.
//   Otherwise, the handle is removed from sharing.
// Arguments:
// - ProcessData - Pointer to per process data.
// - HandleData - Pointer to handle data structure.
// - Handle - Handle to close.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
HRESULT ConsoleHandleData::_CloseInputHandle()
{
    INPUT_INFORMATION* pInputInfo = static_cast<INPUT_INFORMATION*>(_pvClientPointer);
    INPUT_READ_HANDLE_DATA* pReadHandleData = GetClientInput();

    if (pReadHandleData->InputHandleFlags & HANDLE_INPUT_PENDING)
    {
        pReadHandleData->InputHandleFlags &= ~HANDLE_INPUT_PENDING;
        delete[] pReadHandleData->BufPtr;
    }

    // see if there are any reads waiting for data via this handle.  if
    // there are, wake them up.  there aren't any other outstanding i/o
    // operations via this handle because the console lock is held.

    pReadHandleData->LockReadCount();
    if (pReadHandleData->GetReadCount() != 0)
    {
        pReadHandleData->UnlockReadCount();
        pReadHandleData->InputHandleFlags |= HANDLE_CLOSING;

        ConsoleNotifyWait(&pInputInfo->ReadWaitQueue, TRUE, nullptr);

        pReadHandleData->LockReadCount();
    }

    assert(pReadHandleData->GetReadCount() == 0);
    pReadHandleData->UnlockReadCount();

    delete pReadHandleData;

    LOG_IF_FAILED(pInputInfo->Header.FreeIoHandle(this));

    if (!pInputInfo->Header.HasAnyOpenHandles())
    {
        ReinitializeInputBuffer(pInputInfo);
    }

    return S_OK;
}

// Routine Description:
// - This routine closes an output handle.  It decrements the screen buffer's
//   reference count.  If it goes to zero, the buffer is freed.  Otherwise,
//   the handle is removed from sharing.
// Arguments:
// - ProcessData - Pointer to per process data.
// - Console - Pointer to console information structure.
// - HandleData - Pointer to handle data structure.
// - Handle - Handle to close.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
HRESULT ConsoleHandleData::_CloseOutputHandle()
{
    SCREEN_INFORMATION* pScreenInfo = static_cast<SCREEN_INFORMATION*>(_pvClientPointer);
    
    LOG_IF_FAILED(pScreenInfo->Header.FreeIoHandle(this));
    if (!pScreenInfo->Header.HasAnyOpenHandles())
    {
        SCREEN_INFORMATION::s_RemoveScreenBuffer(pScreenInfo);
    }

    return S_OK;

}
