#pragma once

#include "..\ServerBaseApi\IConsoleObjects.h"

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

    static NTSTATUS AllocateHandleToObject(_In_ ACCESS_MASK const AccessDesired,
                                        _In_ ULONG const ShareMode,
                                        _In_ ConsoleObjectType const Type,
                                        _In_ IConsoleObject* const pObject,
                                        _Out_ ConsoleObjectHeader** const ppHeader);

    NTSTATUS FreeHandleToObject(_In_ ACCESS_MASK const Access,
                             _In_ ULONG const ShareMode);

    NTSTATUS GetConsoleObject(_In_ ConsoleObjectType const Type, _Out_ IConsoleObject** const ppObject) const;

    ConsoleObjectType GetObjectType() const;

private:
    ConsoleObjectHeader(_In_ ConsoleObjectType Type, _In_ IConsoleObject* pObject);

    NTSTATUS _AllocateHandleRequest(_In_ ACCESS_MASK const AccessDesired,
                                 _In_ ULONG const ShareMode);

    NTSTATUS _FreeHandleRequest(_In_ ACCESS_MASK const Access,
                             _In_ ULONG const ShareMode);

    static NTSTATUS _CreateHeader(_In_ ConsoleObjectType const Type,
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
