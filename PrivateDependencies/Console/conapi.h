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

#define OffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))

typedef struct _CONSOLE_OBJECT_HEADER
{
    ULONG OpenCount;
    ULONG ReaderCount;
    ULONG WriterCount;
    ULONG ReadShareCount;
    ULONG WriteShareCount;
} CONSOLE_OBJECT_HEADER, *PCONSOLE_OBJECT_HEADER, *const PCCONSOLE_OBJECT_HEADER;

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

	bool IsInputBufferAvailable() const
	{
		return InputBuffer != nullptr;
	}

	bool IsOutputBufferAvailable() const
	{
		return OutputBuffer != nullptr;
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
	BOOLEAN UnpackInputBuffer(_In_ BOOLEAN Unicode, _In_ ULONG Count, ...)
	{
		PVOID Buffer = InputBuffer;
		ULONG Size = InputBufferSize;

		va_list Marker;
		va_start(Marker, Count);

		while (Count > 0)
		{
			ULONG const StringSize = va_arg(Marker, ULONG);
			PVOID* StringStart = va_arg(Marker, PVOID *);

			// Make sure the string fits in the supplied buffer and that it is properly aligned.
			if (StringSize > Size)
			{
				break;
			}

			if ((Unicode != FALSE) && ((StringSize % sizeof(WCHAR)) != 0))
			{
				break;
			}

			*StringStart = Buffer;

			// Go to the next string.

			Buffer = OffsetToPointer(Buffer, StringSize);
			Size -= StringSize;
			Count -= 1;
		}

		va_end(Marker);

		return Count == 0;
	}

} CONSOLE_API_STATE, *PCONSOLE_API_STATE, *const PCCONSOLE_API_STATE;

typedef struct _CONSOLE_API_MSG
{
    CD_IO_COMPLETE Complete;
    CONSOLE_API_STATE State;
    IO_STATUS_BLOCK IoStatus;
    CD_IO_DESCRIPTOR Descriptor;
    union
    {
        struct
        {
            CD_CREATE_OBJECT_INFORMATION CreateObject;
            CONSOLE_CREATESCREENBUFFER_MSG CreateScreenBuffer;
        };
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
} CONSOLE_API_MSG, *PCONSOLE_API_MSG, *const PCCONSOLE_API_MSG;
