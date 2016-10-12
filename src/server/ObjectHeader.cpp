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
NTSTATUS ConsoleObjectHeader::s_AllocateIoHandle(_In_ const ULONG ulHandleType,
                          _Out_ HANDLE * const phOut,
                          _Inout_ ConsoleObjectHeader* pObjHeader,
                          _In_ const ACCESS_MASK amDesired,
                          _In_ const ULONG ulShareMode)
{
    // Check the share mode.
    BOOLEAN const ReadRequested = (amDesired & GENERIC_READ) != 0;
    BOOLEAN const ReadShared = (ulShareMode & FILE_SHARE_READ) != 0;

    BOOLEAN const WriteRequested = (amDesired & GENERIC_WRITE) != 0;
    BOOLEAN const WriteShared = (ulShareMode & FILE_SHARE_WRITE) != 0;

    if (((ReadRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->ReadShareCount)) ||
        ((ReadShared == FALSE) && (pObjHeader->ReaderCount > 0)) ||
        ((WriteRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->WriteShareCount)) || ((WriteShared == FALSE) && (pObjHeader->WriterCount > 0)))
    {
        return STATUS_SHARING_VIOLATION;
    }

    // Allocate all necessary state.
    PCONSOLE_HANDLE_DATA const HandleData = new CONSOLE_HANDLE_DATA();
    if (HandleData == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    if ((ulHandleType & CONSOLE_INPUT_HANDLE) != 0)
    {
        // TODO: resolve reference
        HandleData->pClientInput = new INPUT_READ_HANDLE_DATA();
        if (HandleData->pClientInput == nullptr)
        {
            delete HandleData;
            return STATUS_NO_MEMORY;
        }
    }

    // Update share/open counts and store handle information.
    pObjHeader->OpenCount += 1;

    pObjHeader->ReaderCount += ReadRequested;
    pObjHeader->ReadShareCount += ReadShared;

    pObjHeader->WriterCount += WriteRequested;
    pObjHeader->WriteShareCount += WriteShared;

    HandleData->HandleType = ulHandleType;
    HandleData->ShareAccess = ulShareMode;
    HandleData->Access = amDesired;
    HandleData->ClientPointer = pObjHeader;

    *phOut = (HANDLE)HandleData;

    return STATUS_SUCCESS;
}
