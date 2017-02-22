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

#include <deque>

#define DEFAULT_NUMBER_OF_EVENTS 50
#define INPUT_BUFFER_SIZE_INCREMENT 10

class InputBuffer
{
public:
    ConsoleObjectHeader Header;
    DWORD InputMode;
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

    InputBuffer(_In_ ULONG cEvents);
    ~InputBuffer();

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
    size_t WriteInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords);

    NTSTATUS PrependInputBuffer(_In_ PINPUT_RECORD pInputRecord, _Inout_ DWORD * const pcLength);
    size_t PrependInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords);
    void ReinitializeInputBuffer();
    size_t GetNumberOfReadyEvents();
    void FlushInputBuffer();
    HRESULT FlushAllButKeys();
    void WakeUpReadersWaitingForData();

private:
    std::deque<INPUT_RECORD> _storage;

    HRESULT _HandleConsoleSuspensionEvents(_In_ std::deque<INPUT_RECORD>& records);
    bool _CoalesceMouseMovedEvents(_In_ std::deque<INPUT_RECORD>& inRecords);
    bool _CoalesceRepeatedKeyPressEvents(_In_ std::deque<INPUT_RECORD>& inRecords);
    HRESULT _WriteBuffer(_In_ std::deque<INPUT_RECORD>& inRecords,
                         _Out_ size_t& eventsWritten,
                         _Out_ bool& setWaitEvent);
    NTSTATUS _ReadBuffer(_Out_writes_to_(Length, *EventsRead) PINPUT_RECORD Buffer,
                         _In_ ULONG Length,
                         _Out_ PULONG EventsRead,
                         _In_ BOOL Peek,
                         _In_ BOOL StreamRead,
                         _Out_ PBOOL ResetWaitEvent,
                         _In_ BOOLEAN Unicode);
    HRESULT _ReadBuffer(std::deque<INPUT_RECORD>& outRecords,
                        const size_t readCount,
                        size_t& eventsRead,
                        const bool peek,
                        const bool streamRead,
                        bool& resetWaitEvent,
                        const bool unicode);

#ifdef UNIT_TESTING
    friend class InputBufferTests;
#endif
};

void TerminateRead(_Inout_ InputBuffer* InputInfo, _In_ WaitTerminationReason Flag);