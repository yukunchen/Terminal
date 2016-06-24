#pragma once

typedef struct _CONSOLE_OBJECT_HEADER {
	ULONG OpenCount;
	ULONG ReaderCount;
	ULONG WriterCount;
	ULONG ReadShareCount;
	ULONG WriteShareCount;
} CONSOLE_OBJECT_HEADER, *PCONSOLE_OBJECT_HEADER;


NTSTATUS AllocateIoHandle(_In_ const ULONG ulHandleType,
						  _Out_ HANDLE * const phOut,
						  _Inout_ PCONSOLE_OBJECT_HEADER pObjHeader,
						  _In_ const ACCESS_MASK amDesired,
						  _In_ const ULONG ulShareMode);

bool FreeConsoleHandle(_In_ HANDLE hFree);

NTSTATUS DereferenceIoHandleNoCheck(_In_ HANDLE hIO, _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData);

NTSTATUS DereferenceIoHandle(_In_ HANDLE hIO,
							 _In_ const ULONG ulHandleType,
							 _In_ const ACCESS_MASK amRequested,
							 _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData);
