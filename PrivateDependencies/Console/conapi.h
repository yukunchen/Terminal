/*++
Copyright (c) Microsoft Corporation

Module Name:
- conapi.h

Abstract:
- This module contains the internal structures and definitions used by the console server.

Author:
- Therese Stowell (ThereseS) 12-Nov-1990

Revision History:
--*/

#pragma once

#include "conmsgl1.h"
#include "conmsgl2.h"
#include "conmsg.h"

class IConsoleObject
{

};

class IConsoleInputObject : public IConsoleObject
{

};

class IConsoleOutputObject : public IConsoleObject
{

};

#define OffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))

enum ConsoleObjectType
{
	Input = 0x1,
	Output = 0x2,
};

DEFINE_ENUM_FLAG_OPERATORS(ConsoleObjectType);

class ConsoleObjectHeader
{
public:
	~ConsoleObjectHeader()
	{
		_KnownObjects.erase(this->pObject);
	}

	static DWORD AllocateHandleToObject(_In_ ACCESS_MASK const AccessDesired,
										_In_ ULONG const ShareMode,
										_In_ ConsoleObjectType const Type,
										_In_ IConsoleObject* const pObject,
										_Out_ ConsoleObjectHeader** const ppHeader)
	{
		DWORD Result = STATUS_SUCCESS;

		// Create or retrieve the header for the given object type.
		ConsoleObjectHeader* pHeader;
		if (_KnownObjects.count(pObject) > 0)
		{
			pHeader = _KnownObjects.at(pObject);

			if (Type != pHeader->HandleType)
			{
				Result = STATUS_ASSERTION_FAILURE;
			}
		}
		else
		{
			Result =  _CreateHeader(Type, pObject, &pHeader);
		}

		if (SUCCEEDED(Result))
		{
			// If we're ok, attempt to mark the header with the handle's required access.
			Result = pHeader->_AllocateHandleRequest(AccessDesired, ShareMode);

			if (SUCCEEDED(Result))
			{
				// If the access was OK and we've been recorded, give the header back.
				*ppHeader = pHeader;
			}
		}

		return Result;
	}

	DWORD FreeHandleToObject(_In_ ACCESS_MASK const Access,
							 _In_ ULONG const ShareMode)
	{
		return _FreeHandleRequest(Access, ShareMode);
	}

	DWORD GetConsoleObject(_In_ ConsoleObjectType const Type, _Out_ IConsoleObject** const ppObject)
	{
		DWORD Result = STATUS_SUCCESS;

		if ((HandleType & Type) == 0)
		{
			Result = STATUS_INVALID_HANDLE;
		}

		if (SUCCEEDED(Result))
		{
			*ppObject = pObject;
		}

		return Result;
	}

	DWORD GetObjectType(_Out_ ConsoleObjectType* pType)
	{
		*pType = HandleType;
		return STATUS_SUCCESS;
	}

private:
	ConsoleObjectHeader(_In_ ConsoleObjectType Type, _In_ IConsoleObject* pObject) :
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

	DWORD _AllocateHandleRequest(_In_ ACCESS_MASK const AccessDesired,
								 _In_ ULONG const ShareMode)
	{
		DWORD Result = STATUS_SUCCESS;

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
			Result = STATUS_SHARING_VIOLATION;
		}

		// If valid, increment counts to mark that a handle has been given to this object
		if (SUCCEEDED(Result))
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

		return Result;
	}

	DWORD _FreeHandleRequest(_In_ ACCESS_MASK const Access,
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

	static DWORD _CreateHeader(_In_ ConsoleObjectType const Type, _In_ IConsoleObject* const pObject, _Out_ ConsoleObjectHeader** const ppHeader)
	{
		DWORD Result = STATUS_SUCCESS;

		// If we already have a header for this object, we can't make another one.
		if (_KnownObjects.count(pObject) == 1)
		{
			Result = STATUS_ASSERTION_FAILURE;
		}

		if (SUCCEEDED(Result))
		{
			ConsoleObjectHeader* const pNewHeader = new ConsoleObjectHeader(Type, pObject);

			if (pNewHeader == nullptr)
			{
				Result = STATUS_NO_MEMORY;
			}

			if (SUCCEEDED(Result))
			{
				*ppHeader = pNewHeader;
			}
		}

		return Result;
	}

	ULONG OpenCount;
	ULONG ReaderCount;
	ULONG WriterCount;
	ULONG ReadShareCount;
	ULONG WriteShareCount;

	ConsoleObjectType const HandleType;
	IConsoleObject* const pObject;

	
	static std::unordered_map<IConsoleObject*, ConsoleObjectHeader*> _KnownObjects;
};

class ConsoleHandleData {
public:
	static DWORD CreateHandle(_In_ IConsoleInputObject* const pInputObject,
							  _Out_ ConsoleHandleData** const ppHandleData,
							  _In_ ACCESS_MASK const AccessRequested = GENERIC_READ | GENERIC_WRITE,
							  _In_ ULONG const ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE)
	{
		return _CreateHandle(ConsoleObjectType::Input,
							 pInputObject,
							 AccessRequested,
							 ShareMode,
							 ppHandleData);
	}
	
	static DWORD CreateHandle(_In_ IConsoleOutputObject* const pOutputObject, 
							  _Out_ ConsoleHandleData** const ppHandleData,
							  _In_ ACCESS_MASK const AccessRequested = GENERIC_READ | GENERIC_WRITE,
							  _In_ ULONG const ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE)
	{
		return _CreateHandle(ConsoleObjectType::Output,
							 pOutputObject,
							 AccessRequested,
							 ShareMode,
							 ppHandleData);
	}

	DWORD GetConsoleObject(_In_ ConsoleObjectType Type,
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

	DWORD GetObjectType(_Out_ ConsoleObjectType* pType)
	{
		return pHeader->GetObjectType(pType);
	}

	~ConsoleHandleData()
	{
		pHeader->FreeHandleToObject(Access, ShareAccess);
	}

private:

	static DWORD _CreateHandle(_In_ ConsoleObjectType const Type,
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
		}

		return Result;
	}

	ConsoleHandleData(_In_ ACCESS_MASK AccessMask,
					  _In_ ULONG const ShareMode,
					  _In_ ConsoleObjectHeader* const pHeader) :
		Access(AccessMask),
		ShareAccess(ShareMode),
		pHeader(pHeader)
	{

	}

	ACCESS_MASK const Access;
	ULONG const ShareAccess;
	ConsoleObjectHeader* const pHeader;
	
	//PVOID ClientInputPointer;
};

typedef struct _CONSOLE_API_STATE
{
    ULONG WriteOffset;
    ULONG ReadOffset;
    ULONG InputBufferSize;
    ULONG OutputBufferSize;
    PVOID InputBuffer;
    PVOID OutputBuffer;

    PWCHAR TransBuffer;
    BOOLEAN StackBuffer;
    ULONG WriteFlags; // unused when WriteChars legacy is gone.

} CONSOLE_API_STATE, *PCONSOLE_API_STATE, *const PCCONSOLE_API_STATE;



typedef struct _CONSOLE_API_MSG
{
	// Contains the outgoing API call response
    CD_IO_COMPLETE Complete;

	// Contains state information used during the servicing of the API call
    CONSOLE_API_STATE State;

	// (probably) no longer used now that we're doing DeviceIoControl instead of NtDeviceIoControlFile
    IO_STATUS_BLOCK IoStatus;

	// Contains the incoming API call data
    CD_IO_DESCRIPTOR Descriptor;

    union
    {
        struct
        {
            CD_CREATE_OBJECT_INFORMATION CreateObject;
            CONSOLE_CREATESCREENBUFFER_MSG CreateScreenBuffer;
        };

		// Used for "user defined" IOCTL section (which is the majority of the console API surface)
        struct
        {
            CONSOLE_MSG_HEADER msgHeader;
            union
            {
                CONSOLE_MSG_BODY_L1 consoleMsgL1;
                CONSOLE_MSG_BODY_L2 consoleMsgL2;
                CONSOLE_MSG_BODY_L3 consoleMsgL3;
            } u;
        };
    };

	bool IsInputBufferAvailable() const
	{
		return State.InputBuffer != nullptr;
	}

	bool IsOutputBufferAvailable() const
	{
		return State.OutputBuffer != nullptr;
	}

	void SetCompletionStatus(_In_ DWORD const Status)
	{
		Complete.IoStatus.Status = Status;
	}

	template <typename T> void GetInputBuffer(_Outptr_ T** const ppBuffer, _Out_ ULONG* const pBufferSize)
	{
		*ppBuffer = static_cast<T*>(State.InputBuffer);
		*pBufferSize = State.InputBufferSize / sizeof(T);
	}

	template <typename T> void GetOutputBuffer(_Outptr_ T** const ppBuffer, _Out_ ULONG* const pBufferSize)
	{
		*ppBuffer = static_cast<T*>(State.OutputBuffer);
		*pBufferSize = State.OutputBufferSize / sizeof(T);
	}

	// Routine Description:
	// - This routine validates a string buffer and returns the pointers of where the strings start within the buffer.
	// Arguments:
	// - Unicode - Supplies a boolean that is TRUE if the buffer contains Unicode strings, FALSE otherwise.
	// - Buffer - Supplies the buffer to be validated.
	// - Size - Supplies the size, in bytes, of the buffer to be validated.
	// - Count - Supplies the expected number of strings in the buffer.
	// ... - Supplies a pair of arguments per expected string. The first one is the expected size, in bytes, of the string
	//       and the second one receives a pointer to where the string starts.
	// Return Value:
	// - TRUE if the buffer is valid, FALSE otherwise.
	template <typename T> DWORD UnpackInputBuffer(_In_ ULONG Count, ...)
	{
		PVOID Buffer = State.InputBuffer;
		ULONG Size = State.InputBufferSize;

		va_list Marker;
		va_start(Marker, Count);

		while (Count > 0)
		{
			ULONG* StringSize = va_arg(Marker, ULONG*);
			T** StringStart = va_arg(Marker, T**);

			// Make sure the string fits in the supplied buffer and that it is properly aligned.
			if (*StringSize > Size)
			{
				break;
			}

			if ((*StringSize % sizeof(T)) != 0)
			{
				break;
			}

			*StringStart = static_cast<T*>(Buffer);

			// Go to the next string.

			Buffer = OffsetToPointer(Buffer, *StringSize);
			Size -= *StringSize;
			Count -= 1;

			// convert bytes to characters now that we're done extracting and walking to the next string by byte count.
			*StringSize /= sizeof(T); 
		}

		va_end(Marker);

		return Count == 0 ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
	}

	DWORD _GetConsoleObject(_In_ ConsoleObjectType const Type, 
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

	DWORD GetObjectType(_Out_ ConsoleObjectType* pType)
	{
		return pHandleData->GetObjectType(pType);
	}

	DWORD GetOutputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleOutputObject** const ppObject)
	{
		IConsoleObject* pObject;

		DWORD Result = _GetConsoleObject(ConsoleObjectType::Output, AccessRequested, &pObject);

		if (SUCCEEDED(Result))
		{
			*ppObject = static_cast<IConsoleOutputObject*>(pObject);
		}

		return Result;
	}

	DWORD GetInputObject(_In_ ACCESS_MASK AccessRequested, _Out_ IConsoleInputObject** const ppObject)
	{
		IConsoleObject* pObject;

		DWORD Result = _GetConsoleObject(ConsoleObjectType::Input, AccessRequested, &pObject);

		if (SUCCEEDED(Result))
		{
			*ppObject = static_cast<IConsoleInputObject*>(pObject);
		}

		return Result;
	}

} CONSOLE_API_MSG, *PCONSOLE_API_MSG, *const PCCONSOLE_API_MSG;
