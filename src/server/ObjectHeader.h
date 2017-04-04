/*++
Copyright (c) Microsoft Corporation

Module Name:
- ObjectHeader.h

Abstract:
- This file defines the header information to count handles attached to a given object

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

#include "ObjectHandle.h"

class ConsoleObjectHeader
{
public:
    ConsoleObjectHeader();

    HRESULT AllocateIoHandle(_In_ const ConsoleHandleData::HandleType ulHandleType,
                             _In_ const ACCESS_MASK amDesired,
                             _In_ const ULONG ulShareMode,
                             _Out_ ConsoleHandleData** const ppOut);

    HRESULT FreeIoHandle(_In_ ConsoleHandleData* const pFree);

    bool HasAnyOpenHandles() const;

    // TODO: MSFT 9355013 come up with a better solution than this. http://osgvsowi/9355013
    // It's currently a "load bearing" piece because things like the renderer expect there to always be a "current screen buffer"
    void IncrementOriginalScreenBuffer();

private:
    ULONG _ulOpenCount;
    ULONG _ulReaderCount;
    ULONG _ulWriterCount;
    ULONG _ulReadShareCount;
    ULONG _ulWriteShareCount;
};
