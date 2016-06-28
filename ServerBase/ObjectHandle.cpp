#include "stdafx.h"
#include "ObjectHandle.h"

DWORD ConsoleHandleData::CreateHandle(_In_ IConsoleInputObject* const pInputObject,
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

DWORD ConsoleHandleData::CreateHandle(_In_ IConsoleOutputObject* const pOutputObject,
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

DWORD ConsoleHandleData::GetConsoleObject(_In_ ConsoleObjectType Type,
                                          _In_ ACCESS_MASK AccessMask,
                                          _Out_ IConsoleObject** ppObject)
{
    DWORD Result = STATUS_SUCCESS;

    if (((Access & AccessMask) == 0))
    {
        Result = STATUS_INVALID_HANDLE;
    }

    if (SUCCEEDED(Result))
    {
        Result = pHeader->GetConsoleObject(Type, ppObject);
    }

    return Result;
}

DWORD ConsoleHandleData::GetObjectType(_Out_ ConsoleObjectType* pType)
{
    return pHeader->GetObjectType(pType);
}

ConsoleHandleData::~ConsoleHandleData()
{
    pHeader->FreeHandleToObject(Access, ShareAccess);
}

DWORD ConsoleHandleData::_CreateHandle(_In_ ConsoleObjectType const Type,
                                       _In_ IConsoleObject* const pObject,
                                       _In_ ACCESS_MASK const AccessRequested,
                                       _In_ ULONG const ShareMode,
                                       _Out_ ConsoleHandleData** const ppHandleData)
{
    DWORD Result = STATUS_SUCCESS;

    ConsoleObjectHeader* pHeader;

    Result = ConsoleObjectHeader::AllocateHandleToObject(AccessRequested,
                                                         ShareMode,
                                                         Type,
                                                         pObject,
                                                         &pHeader);

    if (SUCCEEDED(Result))
    {
        ConsoleHandleData* const pNewHandle = new ConsoleHandleData(AccessRequested,
                                                                    ShareMode,
                                                                    pHeader);

        if (pNewHandle == nullptr)
        {
            Result = STATUS_NO_MEMORY;
        }

        if (SUCCEEDED(Result))
        {
            *ppHandleData = pNewHandle;
        }
        else
        {
            pHeader->FreeHandleToObject(AccessRequested, ShareMode);
            delete pHeader;
        }
    }

    return Result;
}

ConsoleHandleData::ConsoleHandleData(_In_ ACCESS_MASK AccessMask,
                                     _In_ ULONG const ShareMode,
                                     _In_ ConsoleObjectHeader* const pHeader) :
    Access(AccessMask),
    ShareAccess(ShareMode),
    pHeader(pHeader)
{

}
