#include "stdafx.h"
#include "ApiMessage.h"

#include "ObjectHandle.h"

bool CONSOLE_API_MSG::IsInputBufferAvailable() const
{
    return State.InputBuffer != nullptr;
}

bool CONSOLE_API_MSG::IsOutputBufferAvailable() const
{
    return State.OutputBuffer != nullptr;
}

void CONSOLE_API_MSG::SetCompletionStatus(_In_ NTSTATUS const Status)
{
    Complete.IoStatus.Status = Status;
}

NTSTATUS CONSOLE_API_MSG::_GetConsoleObject(_In_ ConsoleObjectType const Type,
                                            _In_ ACCESS_MASK AccessRequested,
                                            _Out_ IConsoleObject** const ppObject) const
{
    ConsoleHandleData* const pHandleData = (ConsoleHandleData*)Descriptor.Object;

    NTSTATUS Result = STATUS_SUCCESS;

    if (pHandleData == nullptr)
    {
        Result = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(Result))
    {
        Result = pHandleData->GetConsoleObject(Type, AccessRequested, ppObject);
    }

    return Result;
}

ConsoleObjectType CONSOLE_API_MSG::GetObjectType() const
{
    const ConsoleHandleData* const pHandleData = reinterpret_cast<ConsoleHandleData*>(Descriptor.Object);
    return pHandleData->GetObjectType();
}

NTSTATUS CONSOLE_API_MSG::GetOutputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleOutputObject** const ppObject) const
{
    IConsoleObject* pObject;

    NTSTATUS const Result = _GetConsoleObject(ConsoleObjectType::Output, AccessRequested, &pObject);

    if (SUCCEEDED(Result))
    {
        *ppObject = static_cast<IConsoleOutputObject*>(pObject);
    }

    return Result;
}

NTSTATUS CONSOLE_API_MSG::GetInputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleInputObject** const ppObject) const
{
    IConsoleObject* pObject;

    NTSTATUS const Result = _GetConsoleObject(ConsoleObjectType::Input, AccessRequested, &pObject);

    if (SUCCEEDED(Result))
    {
        *ppObject = static_cast<IConsoleInputObject*>(pObject);
    }

    return Result;
}

ConsoleProcessHandle* CONSOLE_API_MSG::GetProcessHandle() const
{
    return reinterpret_cast<ConsoleProcessHandle*>(Descriptor.Process);
}
