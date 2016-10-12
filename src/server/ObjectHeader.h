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
    ULONG OpenCount;
    ULONG ReaderCount;
    ULONG WriterCount;
    ULONG ReadShareCount;
    ULONG WriteShareCount;

    static NTSTATUS s_AllocateIoHandle(_In_ const ULONG ulHandleType,
                              _Out_ HANDLE * const hOut,
                              _Inout_ ConsoleObjectHeader* pObjHeader,
                              _In_ const ACCESS_MASK amDesired,
                              _In_ const ULONG ulShareMode);

    static void s_InitializeObjectHeader(_Out_ ConsoleObjectHeader* pObjHeader);
};
