#pragma once

#include "ObjectHeader.h"

class ConsoleHandleData {
public:
    static NTSTATUS CreateHandle(_In_ IConsoleInputObject* const pInputObject,
                                 _Out_ ConsoleHandleData** const ppHandleData,
                                 _In_ ACCESS_MASK const AccessRequested = GENERIC_READ | GENERIC_WRITE,
                                 _In_ ULONG const ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE);

    static NTSTATUS CreateHandle(_In_ IConsoleOutputObject* const pOutputObject,
                                 _Out_ ConsoleHandleData** const ppHandleData,
                                 _In_ ACCESS_MASK const AccessRequested = GENERIC_READ | GENERIC_WRITE,
                                 _In_ ULONG const ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE);

    NTSTATUS GetConsoleObject(_In_ ConsoleObjectType Type,
                              _In_ ACCESS_MASK AccessMask,
                              _Out_ IConsoleObject** ppObject) const;

    ConsoleObjectType GetObjectType() const;

    ~ConsoleHandleData();

private:

    static NTSTATUS _CreateHandle(_In_ ConsoleObjectType const Type,
                                  _In_ IConsoleObject* const pObject,
                                  _In_ ACCESS_MASK const AccessRequested,
                                  _In_ ULONG const ShareMode,
                                  _Out_ ConsoleHandleData** const ppHandleData);

    ConsoleHandleData(_In_ ACCESS_MASK AccessMask,
                      _In_ ULONG const ShareMode,
                      _In_ ConsoleObjectHeader* const pHeader);

    ACCESS_MASK const Access;
    ULONG const ShareAccess;
    ConsoleObjectHeader* const pHeader;

    //PVOID ClientInputPointer;
};


