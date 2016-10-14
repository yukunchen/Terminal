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
    _ulOpenCount(0),
    _ulReaderCount(0),
    _ulWriterCount(0),
    _ulReadShareCount(0),
    _ulWriteShareCount(0)
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
                                              _Out_ ConsoleHandleData** const ppOut)
{
    try
    {
        // Allocate all necessary state.
        // TODO: WARNING. This currently relies on the ConsoleObjectHeader being the FIRST portion of the console object
        //       structure or class. If it is not the first item, the cast back to the object won't work anymore.
        std::unique_ptr<ConsoleHandleData> pHandleData = std::make_unique<ConsoleHandleData>(ulHandleType,
                                                                                                 amDesired,
                                                                                                 ulShareMode,
                                                                                                 this); 

        // Check the share mode.
        if (((pHandleData->IsReadAllowed()) && (_ulOpenCount > _ulReadShareCount)) ||
            ((!pHandleData->IsReadShared()) && (_ulReaderCount > 0)) ||
            ((pHandleData->IsWriteAllowed()) && (_ulOpenCount > _ulWriteShareCount)) ||
            ((!pHandleData->IsWriteShared()) && (_ulWriterCount > 0)))
        {
            RETURN_WIN32(ERROR_SHARING_VIOLATION);
        }

        // Update share/open counts and store handle information.
        _ulOpenCount += 1;

        if (pHandleData->IsReadAllowed())
        {
            _ulReaderCount++;
        }

        if (pHandleData->IsReadShared())
        {
            _ulReadShareCount++;
        }

        if (pHandleData->IsWriteAllowed())
        {
            _ulWriterCount++;
        }

        if (pHandleData->IsWriteShared())
        {
            _ulWriteShareCount++;
        }

        *ppOut = pHandleData.release();
    }
    CATCH_RETURN();

    return S_OK;
}

HRESULT ConsoleObjectHeader::FreeIoHandle(_In_ ConsoleHandleData* const pFree)
{
    assert(_ulOpenCount > 0);
    _ulOpenCount -= 1;

    if (pFree->IsReadAllowed())
    {
        _ulReaderCount--;
    }

    if (pFree->IsReadShared())
    {
        _ulReadShareCount--;
    }

    if (pFree->IsWriteAllowed())
    {
        _ulWriterCount--;
    }

    if (pFree->IsWriteShared())
    {
        _ulWriteShareCount--;
    }

    delete pFree;

    return S_OK;
}

bool ConsoleObjectHeader::HasAnyOpenHandles() const
{
    return _ulOpenCount != 0;
}

void ConsoleObjectHeader::IncrementOriginalScreenBuffer()
{
    _ulOpenCount++;
    _ulReaderCount++;
    _ulReadShareCount++;
    _ulWriterCount++;
    _ulWriteShareCount++;
}
