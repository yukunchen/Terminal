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

class ConsoleObjectHeader
{
public:
    ConsoleObjectHeader();

    ULONG OpenCount;
    ULONG ReaderCount;
    ULONG WriterCount;
    ULONG ReadShareCount;
    ULONG WriteShareCount;

    HRESULT AllocateIoHandle(_In_ const ULONG ulHandleType,
                             _In_ const ACCESS_MASK amDesired,
                             _In_ const ULONG ulShareMode,
                             _Out_ HANDLE * const hOut);
};
