#include "stdafx.h"

#include "..\ServerBaseApi\IConsoleObjects.h"

#include "ObjectHandle.h"

NTSTATUS ConsoleHandleData::CreateHandle(_In_ IConsoleInputObject* const pInputObject,
                                         _Out_ ConsoleHandleData** const ppHandleData,
                                         _In_ ACCESS_MASK const AccessRequested /*= GENERIC_READ | GENERIC_WRITE*/,
                                         _In_ ULONG const ShareMode /*= FILE_SHARE_READ | FILE_SHARE_WRITE*/)
{
    return _CreateHandle(ConsoleObjectType::Input,
                         pInputObject,
                         AccessRequested,
                         ShareMode,
                         ppHandleData);
}

NTSTATUS ConsoleHandleData::CreateHandle(_In_ IConsoleOutputObject* const pOutputObject,
                                         _Out_ ConsoleHandleData** const ppHandleData,
                                         _In_ ACCESS_MASK const AccessRequested /*= GENERIC_READ | GENERIC_WRITE*/,
                                         _In_ ULONG const ShareMode /*= FILE_SHARE_READ | FILE_SHARE_WRITE*/)
{
    return _CreateHandle(ConsoleObjectType::Output,
                         pOutputObject,
                         AccessRequested,
                         ShareMode,
                         ppHandleData);
}

NTSTATUS ConsoleHandleData::GetConsoleObject(_In_ ConsoleObjectType Type,
                                             _In_ ACCESS_MASK AccessMask,
                                             _Out_ IConsoleObject** ppObject) const
{
    NTSTATUS Result = STATUS_SUCCESS;

    if (((Access & AccessMask) == 0))
    {
        Result = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(Result))
    {
        Result = pHeader->GetConsoleObject(Type, ppObject);
    }

    return Result;
}

ConsoleObjectType ConsoleHandleData::GetObjectType() const
{
    return pHeader->GetObjectType();
}

ConsoleHandleData::~ConsoleHandleData()
{
    pHeader->FreeHandleToObject(Access, ShareAccess);
}

NTSTATUS ConsoleHandleData::_CreateHandle(_In_ ConsoleObjectType const Type,
                                          _In_ IConsoleObject* const pObject,
                                          _In_ ACCESS_MASK const AccessRequested,
                                          _In_ ULONG const ShareMode,
                                          _Out_ ConsoleHandleData** const ppHandleData)
{
    ConsoleObjectHeader* pHeader;

    NTSTATUS Status = ConsoleObjectHeader::AllocateHandleToObject(AccessRequested,
                                                                  ShareMode,
                                                                  Type,
                                                                  pObject,
                                                                  &pHeader);

    if (NT_SUCCESS(Status))
    {
        ConsoleHandleData* const pNewHandle = new ConsoleHandleData(AccessRequested,
                                                                    ShareMode,
                                                                    pHeader);

        if (pNewHandle == nullptr)
        {
            Status = STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS(Status))
        {
            *ppHandleData = pNewHandle;
        }
        else
        {
            pHeader->FreeHandleToObject(AccessRequested, ShareMode);
            delete pHeader;
        }
    }

    return Status;
}

ConsoleHandleData::ConsoleHandleData(_In_ ACCESS_MASK AccessMask,
                                     _In_ ULONG const ShareMode,
                                     _In_ ConsoleObjectHeader* const pHeader) :
    Access(AccessMask),
    ShareAccess(ShareMode),
    pHeader(pHeader)
{

}
