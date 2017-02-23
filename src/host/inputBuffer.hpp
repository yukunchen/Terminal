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
- Refactored to class, added stl container usage (AustDi, 2017)
--*/

#pragma once

#include "inputReadHandleData.h"

#include "../server/WaitBlock.h"
#include "../server/ObjectHandle.h"
#include "../server/ObjectHeader.h"

#include <deque>

class InputBuffer final
{
public:
    ConsoleObjectHeader Header;
    DWORD InputMode;
    ConsoleWaitQueue WaitQueue; // formerly ReadWaitQueue
    HANDLE InputWaitEvent;
    INPUT_RECORD ReadConInpDbcsLeadByte;
    INPUT_RECORD WriteConInpDbcsLeadByte[2];

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

    InputBuffer();
    ~InputBuffer();
    void ReinitializeInputBuffer();
    void WakeUpReadersWaitingForData();
    void TerminateRead(_In_ WaitTerminationReason Flag);
    size_t GetNumberOfReadyEvents();
    void Flush();
    HRESULT FlushAllButKeys();

    NTSTATUS ReadInputBuffer(_Out_writes_(*pcLength) INPUT_RECORD* pInputRecord,
                            _Inout_ PDWORD pcLength,
                            _In_ BOOL const fPeek,
                            _In_ BOOL const fWaitForData,
                            _In_ INPUT_READ_HANDLE_DATA* pHandleData,
                            _In_opt_ PCONSOLE_API_MSG pConsoleMessage,
                            _In_opt_ ConsoleWaitRoutine pfnWaitRoutine,
                            _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                            _In_ ULONG const cbWaitParameter,
                            _In_ BOOLEAN const fWaitBlockExists,
                            _In_ BOOLEAN const fUnicode);

    NTSTATUS PrependInputBuffer(_In_ INPUT_RECORD* pInputRecord, _Inout_ DWORD* const pcLength);
    size_t PrependInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords);

    DWORD WriteInputBuffer(_In_ INPUT_RECORD* pInputRecord, _In_ DWORD cInputRecords);
    size_t WriteInputBuffer(_In_ std::deque<INPUT_RECORD>& inRecords);

private:
    std::deque<INPUT_RECORD> _storage;

    NTSTATUS _ReadBuffer(_Out_writes_to_(Length, *EventsRead) INPUT_RECORD* Buffer,
                         _In_ ULONG Length,
                         _Out_ PULONG EventsRead,
                         _In_ BOOL Peek,
                         _Out_ PBOOL ResetWaitEvent,
                         _In_ BOOLEAN Unicode);

    HRESULT _ReadBuffer(_Out_ std::deque<INPUT_RECORD>& outRecords,
                        _In_ const size_t readCount,
                        _Out_ size_t& eventsRead,
                        _In_ const bool peek,
                        _Out_ bool& resetWaitEvent,
                        _In_ const bool unicode);

    HRESULT _WriteBuffer(_In_ std::deque<INPUT_RECORD>& inRecords,
                         _Out_ size_t& eventsWritten,
                         _Out_ bool& setWaitEvent);

    bool _CoalesceMouseMovedEvents(_In_ std::deque<INPUT_RECORD>& inRecords);
    bool _CoalesceRepeatedKeyPressEvents(_In_ std::deque<INPUT_RECORD>& inRecords);
    HRESULT _HandleConsoleSuspensionEvents(_In_ std::deque<INPUT_RECORD>& records);

#ifdef UNIT_TESTING
    friend class InputBufferTests;
#endif
};
