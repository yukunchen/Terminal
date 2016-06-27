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

void CONSOLE_API_MSG::SetCompletionStatus(_In_ DWORD const Status)
{
	Complete.IoStatus.Status = Status;
}


DWORD CONSOLE_API_MSG::_GetConsoleObject(_In_ ConsoleObjectType const Type,
										 _In_ ACCESS_MASK AccessRequested,
										 _Out_ IConsoleObject** const ppObject)
{
	ConsoleHandleData* const pHandleData = (ConsoleHandleData*)Descriptor.Object;

	DWORD Result = STATUS_SUCCESS;

	if (pHandleData == nullptr)
	{
		Result = STATUS_INVALID_HANDLE;
	}

	if (SUCCEEDED(Result))
	{
		Result = pHandleData->GetConsoleObject(Type, AccessRequested, ppObject);
	}

	return Result;
}

DWORD CONSOLE_API_MSG::GetObjectType(_Out_ ConsoleObjectType* pType)
{
	ConsoleHandleData* const pHandleData = (ConsoleHandleData*)Descriptor.Object;
	return pHandleData->GetObjectType(pType);
}

DWORD CONSOLE_API_MSG::GetOutputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleOutputObject** const ppObject)
{
	IConsoleObject* pObject;

	DWORD Result = _GetConsoleObject(ConsoleObjectType::Output, AccessRequested, &pObject);

	if (SUCCEEDED(Result))
	{
		*ppObject = static_cast<IConsoleOutputObject*>(pObject);
	}

	return Result;
}

DWORD CONSOLE_API_MSG::GetInputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleInputObject** const ppObject)
{
	IConsoleObject* pObject;

	DWORD Result = _GetConsoleObject(ConsoleObjectType::Input, AccessRequested, &pObject);

	if (SUCCEEDED(Result))
	{
		*ppObject = static_cast<IConsoleInputObject*>(pObject);
	}

	return Result;
}
