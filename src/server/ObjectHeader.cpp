/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ObjectHeader.h"
#include "ObjectHandle.h"

#include "..\host\inputReadHandleData.h"

ConsoleObjectHeader::ConsoleObjectHeader() :
    OpenCount(0),
    ReaderCount(0),
    WriterCount(0),
    ReadShareCount(0),
    WriteShareCount(0)
{

}

// Routine Description:
// - This routine allocates an input or output handle from the process's handle table.
// - This routine initializes all non-type specific fields in the handle data structure.
// Arguments:
// - ulHandleType - Flag indicating input or output handle.
// - phOut - On return, contains allocated handle.  Handle is an index internally.  When returned to the API caller, it is translated into a handle.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.  The handle is allocated from the per-process handle table.  Holding the console
//   lock serializes both threads within the calling process and any other process that shares the console.
HRESULT ConsoleObjectHeader::AllocateIoHandle(_In_ const ULONG ulHandleType,
                                              _In_ const ACCESS_MASK amDesired,
                                              _In_ const ULONG ulShareMode,
                                              _Out_ HANDLE* const phOut)
{
    // Check the share mode.
    bool const ReadRequested = IsFlagSet(amDesired, GENERIC_READ);
    bool const ReadShared = IsFlagSet(ulShareMode, FILE_SHARE_READ);

    bool const WriteRequested = IsFlagSet(amDesired, GENERIC_WRITE);
    bool const WriteShared = IsFlagSet(ulShareMode, FILE_SHARE_WRITE);

    if (((ReadRequested) && (OpenCount > ReadShareCount)) ||
        ((!ReadShared) && (ReaderCount > 0)) ||
        ((WriteRequested) && (OpenCount > WriteShareCount)) ||
        ((!WriteShared) && (WriterCount > 0)))
    {
        RETURN_WIN32(ERROR_SHARING_VIOLATION);
    }

    // Allocate all necessary state.
    wistd::unique_ptr<CONSOLE_HANDLE_DATA> HandleData = wil::make_unique_nothrow<CONSOLE_HANDLE_DATA>();
    RETURN_IF_NULL_ALLOC(HandleData);

    if (IsFlagSet(ulHandleType, CONSOLE_INPUT_HANDLE))
    {
        // TODO: resolve reference
        HandleData->pClientInput = new INPUT_READ_HANDLE_DATA();
        RETURN_IF_NULL_ALLOC(HandleData);
    }

    // Update share/open counts and store handle information.
    OpenCount += 1;

    ReaderCount += ReadRequested;
    ReadShareCount += ReadShared;

    WriterCount += WriteRequested;
    WriteShareCount += WriteShared;

    HandleData->HandleType = ulHandleType;
    HandleData->ShareAccess = ulShareMode;
    HandleData->Access = amDesired;
    HandleData->ClientPointer = this;

    *phOut = static_cast<HANDLE>(HandleData.release());

    return S_OK;
}
