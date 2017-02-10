/*++
Copyright (c) Microsoft Corporation

Module Name:
- inputBuffer.hpp

Abstract:
- This file implements the circular buffer management for input events.
- The circular buffer is described by a header, which resides in the beginning of the memory allocated when the
  buffer is created.  The header contains all of the per-buffer information, such as reader, writer, and
  reference counts, and also holds the pointers into the circular buffer proper.
- When the in and out pointers are equal, the circular buffer is empty.  When the in pointer trails the out pointer
  by 1, the buffer is full.  Thus, a 512 byte buffer can hold only 511 bytes; one byte is lost so that full and empty
  conditions can be distinguished. So that the user can put 512 bytes in a buffer that they created with a size
  of 512, we allow for this byte lost when allocating the memory.

Author:
- Therese Stowell (Thereses) 12-Nov-1990. Adapted from OS/2 subsystem server\srvpipe.c

Revision History:
- Moved from input.h/input.cpp. (AustDi, 2017)
--*/

#pragma once

#include "inputReadHandleData.h"

#include "../server/WaitBlock.h"
#include "../server/ObjectHandle.h"
#include "../server/ObjectHeader.h"


#define DEFAULT_NUMBER_OF_EVENTS 50
#define INPUT_BUFFER_SIZE_INCREMENT 10

struct INPUT_INFORMATION
{
    ConsoleObjectHeader Header;
    _Field_size_(InputBufferSize) PINPUT_RECORD InputBuffer;
    DWORD InputBufferSize;  // size in events
    DWORD InputMode;
    ULONG_PTR First;    // ptr to base of circular buffer
    ULONG_PTR In;   // ptr to next free event
    ULONG_PTR Out;  // ptr to next available event
    ULONG_PTR Last; // ptr to end + 1 of buffer
    ConsoleWaitQueue WaitQueue; // formerly ReadWaitQueue
    struct
    {
        DWORD Disable:1;    // High   : specifies input code page or enable/disable in NLS state
        DWORD Unavailable:1;    // Middle : specifies console window doing menu loop or size move
        DWORD Open:1;   // Low    : specifies open/close in NLS state or IME hot key

        DWORD ReadyConversion:1;    // if conversion mode is ready by succeed communicate to ConIME.
        // then this field is TRUE.
        DWORD InComposition:1;  // specifies if there's an ongoing text composition
        DWORD Conversion;   // conversion mode of ime (i.e IME_CMODE_xxx).
        // this field uses by GetConsoleNlsMode
    } ImeMode;

    HANDLE InputWaitEvent;
    struct _CONSOLE_INFORMATION *Console;
    INPUT_RECORD ReadConInpDbcsLeadByte;
    INPUT_RECORD WriteConInpDbcsLeadByte[2];

    INPUT_INFORMATION(_In_ ULONG cEvents);
    ~INPUT_INFORMATION();

    NTSTATUS ReadBuffer(_Out_writes_to_(Length, *EventsRead) PINPUT_RECORD Buffer,
                        _In_ ULONG Length,
                        _Out_ PULONG EventsRead,
                        _In_ BOOL Peek,
                        _In_ BOOL StreamRead,
                        _Out_ PBOOL ResetWaitEvent,
                        _In_ BOOLEAN Unicode);

    NTSTATUS ReadInputBuffer(_Out_writes_(*pcLength) PINPUT_RECORD pInputRecord,
                            _Inout_ PDWORD pcLength,
                            _In_ BOOL const fPeek,
                            _In_ BOOL const fWaitForData,
                            _In_ BOOL const fStreamRead,
                            _In_ INPUT_READ_HANDLE_DATA* pHandleData,
                            _In_opt_ PCONSOLE_API_MSG pConsoleMessage,
                            _In_opt_ ConsoleWaitRoutine pfnWaitRoutine,
                            _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                            _In_ ULONG const cbWaitParameter,
                            _In_ BOOLEAN const fWaitBlockExists,
                            _In_ BOOLEAN const fUnicode);

    DWORD WriteInputBuffer(_In_ PINPUT_RECORD pInputRecord, _In_ DWORD cInputRecords);

    NTSTATUS WriteBuffer(_In_ PVOID Buffer, _In_ ULONG Length, _Out_ PULONG EventsWritten, _Out_ PBOOL SetWaitEvent);
    NTSTATUS PrependInputBuffer(_In_ PINPUT_RECORD pInputRecord, _Inout_ DWORD * const pcLength);
    void ReinitializeInputBuffer();
    void GetNumberOfReadyEvents(_Out_ PULONG pcEvents);
    void FlushInputBuffer();
    NTSTATUS FlushAllButKeys();
    void WakeUpReadersWaitingForData();
    NTSTATUS SetInputBufferSize(_In_ ULONG Size);
    DWORD PreprocessInput(_In_ PINPUT_RECORD InputEvent, _In_ DWORD nLength);
};
