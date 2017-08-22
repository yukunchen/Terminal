/*++
Copyright (c) Microsoft Corporation

Module Name:
- inputBuffer.hpp

Abstract:
- storage area for incoming input events.

Author:
- Therese Stowell (Thereses) 12-Nov-1990. Adapted from OS/2 subsystem server\srvpipe.c

Revision History:
- Moved from input.h/input.cpp. (AustDi, 2017)
- Refactored to class, added stl container usage (AustDi, 2017)
--*/

#pragma once

#include "inputReadHandleData.h"
#include "readData.hpp"
#include "IInputEvent.hpp"

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

    bool fInComposition;  // specifies if there's an ongoing text composition

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
                            _In_ BOOLEAN const fUnicode);

    NTSTATUS PrependInputBuffer(_In_ INPUT_RECORD* pInputRecord, _Inout_ DWORD* const pcLength);
    HRESULT PrependInputBuffer(_Inout_ std::deque<INPUT_RECORD>& inRecords, _Out_ size_t& eventsWritten);
    HRESULT PrependInputBuffer(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents,
                               _Out_ size_t& eventsWritten);

    size_t WriteInputBuffer(_Inout_ std::deque<INPUT_RECORD>& inRecords);
    size_t WriteInputBuffer(_Inout_ std::unique_ptr<IInputEvent> pInputEvent);
    size_t WriteInputBuffer(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);

private:
    std::deque<std::unique_ptr<IInputEvent>> _storage;

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

    HRESULT _WriteBuffer(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inRecords,
                         _Out_ size_t& eventsWritten,
                         _Out_ bool& setWaitEvent);

    bool _CoalesceMouseMovedEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);
    bool _CoalesceRepeatedKeyPressEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);
    HRESULT _HandleConsoleSuspensionEvents(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);

    std::deque<std::unique_ptr<IInputEvent>> _inputRecordsToInputEvents(_In_ const std::deque<INPUT_RECORD>& inRecords);

#ifdef UNIT_TESTING
    friend class InputBufferTests;
#endif
};
