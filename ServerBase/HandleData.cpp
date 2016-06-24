#include "stdafx.h"
#include "HandleData.h"

//std::unordered_map<void*, PCONSOLE_OBJECT_HEADER> g_map;

// Routine Description:
// - This routine allocates an input or output handle from the process's handle table.
// - This routine initializes all non-type specific fields in the handle data structure.
// Arguments:
// - ulHandleType - Flag indicating input or output handle.
// - phOut - On return, contains allocated handle.  Handle is an index internally.  When returned to the API caller, it is translated into a handle.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.  The handle is allocated from the per-process handle table.  Holding the console
//   lock serializes both threads within the calling process and any other process that shares the console.
//NTSTATUS AllocateIoHandle(_In_ const ULONG ulHandleType,
//						  _Out_ HANDLE * const phOut,
//						  _Inout_ PCONSOLE_OBJECT_HEADER pObjHeader,
//						  _In_ const ACCESS_MASK amDesired,
//						  _In_ const ULONG ulShareMode)
//{
//	// Check the share mode.
//	BOOLEAN const ReadRequested = (amDesired & GENERIC_READ) != 0;
//	BOOLEAN const ReadShared = (ulShareMode & FILE_SHARE_READ) != 0;
//
//	BOOLEAN const WriteRequested = (amDesired & GENERIC_WRITE) != 0;
//	BOOLEAN const WriteShared = (ulShareMode & FILE_SHARE_WRITE) != 0;
//
//	if (((ReadRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->ReadShareCount)) ||
//		((ReadShared == FALSE) && (pObjHeader->ReaderCount > 0)) ||
//		((WriteRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->WriteShareCount)) || ((WriteShared == FALSE) && (pObjHeader->WriterCount > 0)))
//	{
//		return STATUS_SHARING_VIOLATION;
//	}
//
//	// Allocate all necessary state.
//	PCONSOLE_HANDLE_DATA const HandleData = new CONSOLE_HANDLE_DATA();
//	if (HandleData == nullptr)
//	{
//		return STATUS_NO_MEMORY;
//	}
//
//	//if ((ulHandleType & CONSOLE_INPUT_HANDLE) != 0)
//	//{
//	//	HandleData->pClientInput = new INPUT_READ_HANDLE_DATA();
//	//	if (HandleData->pClientInput == nullptr)
//	//	{
//	//		ConsoleHeapFree(HandleData);
//	//		return STATUS_NO_MEMORY;
//	//	}
//	//}
//
//	// Update share/open counts and store handle information.
//	pObjHeader->OpenCount += 1;
//
//	pObjHeader->ReaderCount += ReadRequested;
//	pObjHeader->ReadShareCount += ReadShared;
//
//	pObjHeader->WriterCount += WriteRequested;
//	pObjHeader->WriteShareCount += WriteShared;
//
//	HandleData->HandleType = ulHandleType;
//	HandleData->ShareAccess = ulShareMode;
//	HandleData->Access = amDesired;
//	HandleData->ClientPointer = pObjHeader;
//
//	*phOut = (HANDLE)HandleData;
//
//	return STATUS_SUCCESS;
//}

//bool FreeConsoleHandle(_In_ HANDLE hFree)
//{
//	PCONSOLE_HANDLE_DATA const HandleData = (PCONSOLE_HANDLE_DATA)hFree;
//
//	BOOLEAN const ReadRequested = (HandleData->Access & GENERIC_READ) != 0;
//	BOOLEAN const ReadShared = (HandleData->ShareAccess & FILE_SHARE_READ) != 0;
//
//	BOOLEAN const WriteRequested = (HandleData->Access & GENERIC_WRITE) != 0;
//	BOOLEAN const WriteShared = (HandleData->ShareAccess & FILE_SHARE_WRITE) != 0;
//
//	PCONSOLE_OBJECT_HEADER const Header = (PCONSOLE_OBJECT_HEADER)HandleData->ClientPointer;
//
//	delete HandleData;
//
//	Header->OpenCount -= 1;
//
//	Header->ReaderCount -= ReadRequested;
//	Header->ReadShareCount -= ReadShared;
//
//	Header->WriterCount -= WriteRequested;
//	Header->WriteShareCount -= WriteShared;
//
//	return Header->OpenCount == 0;
//}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:
//NTSTATUS DereferenceIoHandleNoCheck(_In_ HANDLE hIO, _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData)
//{
//	*ppConsoleData = (PCONSOLE_HANDLE_DATA)hIO;
//	return STATUS_SUCCESS;
//}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - ProcessData - Pointer to per process data structure.
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:
//NTSTATUS DereferenceIoHandle(_In_ HANDLE hIO,
//							 _In_ const ULONG ulHandleType,
//							 _In_ const ACCESS_MASK amRequested,
//							 _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData)
//{
//	PCONSOLE_HANDLE_DATA const Data = (PCONSOLE_HANDLE_DATA)hIO;
//
//	// Check the type and the granted access.
//	if ((hIO == nullptr) || ((Data->Access & amRequested) == 0) || ((ulHandleType != 0) && ((Data->HandleType & ulHandleType) == 0)))
//	{
//		return STATUS_INVALID_HANDLE;
//	}
//
//	*ppConsoleData = Data;
//
//	return STATUS_SUCCESS;
//}
