#pragma once

#include "IConsoleObjects.h"

enum ConsoleObjectType
{
    Input = 0x1,
    Output = 0x2,
};

DEFINE_ENUM_FLAG_OPERATORS(ConsoleObjectType);

class ConsoleObjectHeader
{
public:
    ~ConsoleObjectHeader();

    static DWORD AllocateHandleToObject(_In_ ACCESS_MASK const AccessDesired,
                                        _In_ ULONG const ShareMode,
                                        _In_ ConsoleObjectType const Type,
                                        _In_ IConsoleObject* const pObject,
                                        _Out_ ConsoleObjectHeader** const ppHeader);

    DWORD FreeHandleToObject(_In_ ACCESS_MASK const Access,
                             _In_ ULONG const ShareMode);

    DWORD GetConsoleObject(_In_ ConsoleObjectType const Type, _Out_ IConsoleObject** const ppObject);

    DWORD GetObjectType(_Out_ ConsoleObjectType* pType);

private:
    ConsoleObjectHeader(_In_ ConsoleObjectType Type, _In_ IConsoleObject* pObject);

    DWORD _AllocateHandleRequest(_In_ ACCESS_MASK const AccessDesired,
                                 _In_ ULONG const ShareMode);

    DWORD _FreeHandleRequest(_In_ ACCESS_MASK const Access,
                             _In_ ULONG const ShareMode);

    static DWORD _CreateHeader(_In_ ConsoleObjectType const Type,
                               _In_ IConsoleObject* const pObject,
                               _Out_ ConsoleObjectHeader** const ppHeader);

    ULONG OpenCount;
    ULONG ReaderCount;
    ULONG WriterCount;
    ULONG ReadShareCount;
    ULONG WriteShareCount;

    ConsoleObjectType const HandleType;
    IConsoleObject* const pObject;

    static std::unordered_map<IConsoleObject*, ConsoleObjectHeader*> _KnownObjects;
};
