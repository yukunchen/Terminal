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
#include "../types/inc/IInputEvent.hpp"

#include "../server/ObjectHandle.h"
#include "../server/ObjectHeader.h"
#include "../terminal/adapter/terminalInput.hpp"

#include <deque>

class InputBuffer final
{
public:
    ConsoleObjectHeader Header;
    DWORD InputMode;
    ConsoleWaitQueue WaitQueue; // formerly ReadWaitQueue
    bool fInComposition;  // specifies if there's an ongoing text composition

    InputBuffer();
    ~InputBuffer();

    // storage API for partial dbcs bytes being read from the buffer
    bool IsReadPartialByteSequenceAvailable();
    std::unique_ptr<IInputEvent> FetchReadPartialByteSequence(_In_ bool peek);
    void StoreReadPartialByteSequence(std::unique_ptr<IInputEvent> event);

    // storage API for partial dbcs bytes being written to the buffer
    bool IsWritePartialByteSequenceAvailable();
    std::unique_ptr<IInputEvent> FetchWritePartialByteSequence(_In_ bool peek);
    void StoreWritePartialByteSequence(std::unique_ptr<IInputEvent> event);

    void ReinitializeInputBuffer();
    void WakeUpReadersWaitingForData();
    void TerminateRead(_In_ WaitTerminationReason Flag);
    size_t GetNumberOfReadyEvents();
    void Flush();
    void FlushAllButKeys();

    NTSTATUS Read(_Out_ std::deque<std::unique_ptr<IInputEvent>>& OutEvents,
                  _In_ const size_t AmountToRead,
                  _In_ const bool Peek,
                  _In_ const bool WaitForData,
                  _In_ const bool Unicode);

    NTSTATUS Read(_Out_ std::unique_ptr<IInputEvent>& inEvent,
                  _In_ const bool Peek,
                  _In_ const bool WaitForData,
                  _In_ const bool Unicode);


    size_t Prepend(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);

    size_t Write(_Inout_ std::unique_ptr<IInputEvent> inEvent);
    size_t Write(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);

    bool IsInVirtualTerminalInputMode() const;
    Microsoft::Console::VirtualTerminal::TerminalInput& GetTerminalInput();

private:
    std::deque<std::unique_ptr<IInputEvent>> _storage;
    std::unique_ptr<IInputEvent> _readPartialByteSequence;
    std::unique_ptr<IInputEvent> _writePartialByteSequence;
    Microsoft::Console::VirtualTerminal::TerminalInput _termInput;

    HRESULT _ReadBuffer(_Out_ std::deque<std::unique_ptr<IInputEvent>>& outEvents,
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

    void _HandleTerminalInputCallback(_In_ std::deque<std::unique_ptr<IInputEvent>>& inEvents);

#ifdef UNIT_TESTING
    friend class InputBufferTests;
#endif
};
