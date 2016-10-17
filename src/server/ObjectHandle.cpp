/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ObjectHandle.h"

#include "..\host\globals.h"
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

// Routine Description:
// - Checks if this handle represents an input type object.
// Arguments:
// - <none>
// Return Value:
// - True if this handle is for an input object. False otherwise.
bool ConsoleHandleData::_IsInput() const
{
    return IsFlagSet(_ulHandleType, HandleType::Input);
}

// Routine Description:
// - Checks if this handle represents an output type object.
// Arguments:
// - <none>
// Return Value:
// - True if this handle is for an output object. False otherwise.
bool ConsoleHandleData::_IsOutput() const
{
    return IsFlagSet(_ulHandleType, HandleType::Output);
}

// Routine Description:
// - Indicates whether this handle is allowed to be used for reading the underlying object data.
// Arguments:
// - <none>
// Return Value:
// - True if read is permitted. False otherwise.
bool ConsoleHandleData::IsReadAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_READ);
}

// Routine Description:
// - Indicates whether this handle allows multiple customers to share reading of the underlying object data.
// Arguments:
// - <none>
// Return Value:
// - True if sharing read access is permitted. False otherwise.
bool ConsoleHandleData::IsReadShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_READ);
}

// Routine Description:
// - Indicates whether this handle is allowed to be used for writing the underlying object data.
// Arguments:
// - <none>
// Return Value:
// - True if write is permitted. False otherwise.
bool ConsoleHandleData::IsWriteAllowed() const
{
    return IsFlagSet(_amAccess, GENERIC_WRITE);
}

// Routine Description:
// - Indicates whether this handle allows multiple customers to share writing of the underlying object data.
// Arguments:
// - <none>
// Return Value:
// - True if sharing write access is permitted. False otherwise.
bool ConsoleHandleData::IsWriteShared() const
{
    return IsFlagSet(_ulShareAccess, FILE_SHARE_WRITE);
}

// Routine Description:
// - Retieves the properly typed Input Buffer from the Handle.
// Arguments:
// - amRequested - Access that the client would like for manipulating the buffer
// - ppInputInfo - On success, filled with the referenced Input Buffer object
// Return Value:
// - HRESULT S_OK or suitable error.
HRESULT ConsoleHandleData::GetInputBuffer(_In_ const ACCESS_MASK amRequested,
                                          _Out_ INPUT_INFORMATION** const ppInputInfo) const
{
    *ppInputInfo = nullptr;

    RETURN_HR_IF(E_ACCESSDENIED, IsAnyFlagClear(_amAccess, amRequested));
    RETURN_HR_IF(E_HANDLE, IsAnyFlagClear(_ulHandleType, HandleType::Input));

    *ppInputInfo = static_cast<INPUT_INFORMATION*>(_pvClientPointer);

    return S_OK;
}

// Routine Description:
// - Retieves the properly typed Screen Buffer from the Handle.
// Arguments:
// - amRequested - Access that the client would like for manipulating the buffer
// - ppInputInfo - On success, filled with the referenced Screen Buffer object
// Return Value:
// - HRESULT S_OK or suitable error.
HRESULT ConsoleHandleData::GetScreenBuffer(_In_ const ACCESS_MASK amRequested,
                                           _Out_ SCREEN_INFORMATION** const ppScreenInfo) const
{
    *ppScreenInfo = nullptr;

    RETURN_HR_IF(E_ACCESSDENIED, IsAnyFlagClear(_amAccess, amRequested));
    RETURN_HR_IF(E_HANDLE, IsAnyFlagClear(_ulHandleType, HandleType::Output));

    *ppScreenInfo = static_cast<SCREEN_INFORMATION*>(_pvClientPointer);

    return S_OK;
}

// Routine Description:
// - Retrieves the wait queue associated with the given object held by this handle.
// Arguments:
// - ppWaitQueue - On success, filled with a pointer to the desired queue
// Return Value:
// - HRESULT S_OK or suitable error.
HRESULT ConsoleHandleData::GetWaitQueue(_Out_ ConsoleWaitQueue** const ppWaitQueue) const
{
    if (_IsInput())
    {
        INPUT_INFORMATION* const pObj = static_cast<INPUT_INFORMATION*>(_pvClientPointer);
        *ppWaitQueue = &pObj->WaitQueue;
        return S_OK;
    }
    else if (_IsOutput())
    {
        // TODO: shouldn't the output queue be per output object target, not global?
        *ppWaitQueue = &g_ciConsoleInformation.OutputQueue;
        return S_OK;
    }
    else
    {
        return E_UNEXPECTED;
    }
}

// Routine Description:
// - For input buffers only, retrieves an extra handle data structure used to save some information
//   across multiple reads from the same handle.
// Arguments:
// - <none>
// Return Value:
// - Pointer to the input read handle data structure with the aforementioned extra info.
INPUT_READ_HANDLE_DATA* ConsoleHandleData::GetClientInput() const
{
    return _pClientInput;
}

// Routine Description:
// - Closes this handle destroying memory as appropriate and freeing ref counts. 
//   Do not use this handle after closing.
// Arguments:
// - <none>
// Return Value:
// - HRESULT S_OK or suitable error code.
// TODO: MSFT: 9358923 Consider making this a part of the destructor. http://osgvsowi/9358923
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
        return E_UNEXPECTED;
    }
}

// Routine Description:
// - This routine closes an input handle.  It decrements the input buffer's
//   reference count.  If it goes to zero, the buffer is reinitialized.
//   Otherwise, the handle is removed from sharing.
// Arguments:
// - <none>
// Return Value:
// - HRESULT S_OK or suitable error code.
// Note:
// - The console lock must be held when calling this routine.
HRESULT ConsoleHandleData::_CloseInputHandle()
{
    assert(_IsInput());
    INPUT_INFORMATION* pInputInfo = static_cast<INPUT_INFORMATION*>(_pvClientPointer);
    INPUT_READ_HANDLE_DATA* pReadHandleData = GetClientInput();

    if (IsFlagSet(pReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending))
    {
        ClearFlag(pReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending);
        delete[] pReadHandleData->BufPtr;
        pReadHandleData->BufPtr = nullptr;
    }

    // see if there are any reads waiting for data via this handle.  if
    // there are, wake them up.  there aren't any other outstanding i/o
    // operations via this handle because the console lock is held.

    pReadHandleData->LockReadCount();
    if (pReadHandleData->GetReadCount() != 0)
    {
        pReadHandleData->UnlockReadCount();
        SetFlag(pReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::Closing);

        pInputInfo->WaitQueue.ConsoleNotifyWait(TRUE, nullptr);

        pReadHandleData->LockReadCount();
    }

    assert(pReadHandleData->GetReadCount() == 0);
    pReadHandleData->UnlockReadCount();

    delete pReadHandleData;
    pReadHandleData = nullptr;

    LOG_IF_FAILED(pInputInfo->Header.FreeIoHandle(this));

    if (!pInputInfo->Header.HasAnyOpenHandles())
    {
        ReinitializeInputBuffer(pInputInfo);
    }

    pInputInfo = nullptr;
    _pvClientPointer = nullptr;

    return S_OK;
}

// Routine Description:
// - This routine closes an output handle.  It decrements the screen buffer's
//   reference count.  If it goes to zero, the buffer is freed.  Otherwise,
//   the handle is removed from sharing.
// Arguments:
// - <none>
// Return Value:
// - HRESULT S_OK or suitable error code.
// Note:
// - The console lock must be held when calling this routine.
HRESULT ConsoleHandleData::_CloseOutputHandle()
{
    assert(_IsOutput());
    SCREEN_INFORMATION* pScreenInfo = static_cast<SCREEN_INFORMATION*>(_pvClientPointer);

    LOG_IF_FAILED(pScreenInfo->Header.FreeIoHandle(this));
    if (!pScreenInfo->Header.HasAnyOpenHandles())
    {
        SCREEN_INFORMATION::s_RemoveScreenBuffer(pScreenInfo);
    }

    pScreenInfo = nullptr;
    _pvClientPointer = nullptr;

    return S_OK;
}
