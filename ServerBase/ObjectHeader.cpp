#include "stdafx.h"
#include "ObjectHeader.h"
#include "..\ServerBaseApi\IConsoleObjects.h"

std::unordered_map<IConsoleObject*, ConsoleObjectHeader*> ConsoleObjectHeader::_KnownObjects;

ConsoleObjectHeader::ConsoleObjectHeader(_In_ ConsoleObjectType Type, _In_ IConsoleObject* pObject) :
    OpenCount(0),
    ReaderCount(0),
    WriterCount(0),
    ReadShareCount(0),
    WriteShareCount(0),
    HandleType(Type),
    pObject(pObject)
{
    _KnownObjects.insert(std::make_pair(pObject, this));
}

ConsoleObjectHeader::~ConsoleObjectHeader()
{
    _KnownObjects.erase(this->pObject);
}

NTSTATUS ConsoleObjectHeader::AllocateHandleToObject(_In_ ACCESS_MASK const AccessDesired,
                                                  _In_ ULONG const ShareMode,
                                                  _In_ ConsoleObjectType const Type,
                                                  _In_ IConsoleObject* const pObject,
                                                  _Out_ ConsoleObjectHeader** const ppHeader)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Create or retrieve the header for the given object type.
    ConsoleObjectHeader* pHeader;
    if (_KnownObjects.count(pObject) > 0)
    {
        pHeader = _KnownObjects.at(pObject);

        if (Type != pHeader->HandleType)
        {
            Status = STATUS_ASSERTION_FAILURE;
        }
    }
    else
    {
        Status = _CreateHeader(Type, pObject, &pHeader);
    }

    if (NT_SUCCESS(Status))
    {
        // If we're ok, attempt to mark the header with the handle's required access.
        Status = pHeader->_AllocateHandleRequest(AccessDesired, ShareMode);

        if (NT_SUCCESS(Status))
        {
            // If the access was OK and we've been recorded, give the header back.
            *ppHeader = pHeader;
        }
    }

    return Status;
}

NTSTATUS ConsoleObjectHeader::FreeHandleToObject(_In_ ACCESS_MASK const Access,
                                              _In_ ULONG const ShareMode)
{
    return _FreeHandleRequest(Access, ShareMode);
}

NTSTATUS ConsoleObjectHeader::GetConsoleObject(_In_ ConsoleObjectType const Type, _Out_ IConsoleObject** const ppObject) const
{
    NTSTATUS Result = STATUS_SUCCESS;

    if ((HandleType & Type) == 0)
    {
        Result = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(Result))
    {
        *ppObject = pObject;
    }

    return Result;
}

ConsoleObjectType ConsoleObjectHeader::GetObjectType() const
{
    return HandleType;
}

NTSTATUS ConsoleObjectHeader::_AllocateHandleRequest(_In_ ACCESS_MASK const AccessDesired,
                                                  _In_ ULONG const ShareMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Determine modes requested
    BOOLEAN const ReadRequested = (AccessDesired & GENERIC_READ) != 0;
    BOOLEAN const ReadShared = (ShareMode & FILE_SHARE_READ) != 0;

    BOOLEAN const WriteRequested = (AccessDesired & GENERIC_WRITE) != 0;
    BOOLEAN const WriteShared = (ShareMode & FILE_SHARE_WRITE) != 0;

    // Validate that request is valid
    if (((ReadRequested != FALSE) && (OpenCount > ReadShareCount)) ||
        ((ReadShared == FALSE) && (ReaderCount > 0)) ||
        ((WriteRequested != FALSE) && (OpenCount > WriteShareCount)) ||
        ((WriteShared == FALSE) && (WriterCount > 0)))
    {
        Status = STATUS_SHARING_VIOLATION;
    }

    // If valid, increment counts to mark that a handle has been given to this object
    if (NT_SUCCESS(Status))
    {
        OpenCount++;

        if (ReadRequested)
        {
            ReaderCount++;
        }

        if (ReadShared)
        {
            ReadShareCount++;
        }

        if (WriteRequested)
        {
            WriterCount++;
        }

        if (WriteShared)
        {
            WriteShareCount++;
        }
    }

    return Status;
}

NTSTATUS ConsoleObjectHeader::_FreeHandleRequest(_In_ ACCESS_MASK const Access,
                                                 _In_ ULONG const ShareMode)
{
    BOOLEAN const ReadRequested = (Access & GENERIC_READ) != 0;
    BOOLEAN const ReadShared = (ShareMode & FILE_SHARE_READ) != 0;

    BOOLEAN const WriteRequested = (Access & GENERIC_WRITE) != 0;
    BOOLEAN const WriteShared = (ShareMode & FILE_SHARE_WRITE) != 0;

    OpenCount -= 1;

    if (ReadRequested)
    {
        ReaderCount--;
    }

    if (ReadShared)
    {
        ReadShareCount--;
    }

    if (WriteRequested)
    {
        WriterCount--;
    }

    if (WriteShared)
    {
        WriteShareCount--;
    }

    return STATUS_SUCCESS;
}

NTSTATUS ConsoleObjectHeader::_CreateHeader(_In_ ConsoleObjectType const Type, _In_ IConsoleObject* const pObject, _Out_ ConsoleObjectHeader** const ppHeader)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // If we already have a header for this object, we can't make another one.
    if (_KnownObjects.count(pObject) == 1)
    {
        Status = STATUS_ASSERTION_FAILURE;
    }

    if (NT_SUCCESS(Status))
    {
        ConsoleObjectHeader* const pNewHeader = new ConsoleObjectHeader(Type, pObject);

        if (pNewHeader == nullptr)
        {
            Status = STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS(Status))
        {
            *ppHeader = pNewHeader;
        }
    }

    return Status;
}
