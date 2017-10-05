/*++
Copyright (c) Microsoft Corporation

Module Name:
- readDataDirect.hpp

Abstract:
- This file defines the read data structure for INPUT_RECORD-reading console APIs.
- A direct read specifically means that we are returning multiplexed input data stored
  in the internal console buffers that could have originated from any type of input device.
  This is not strictly string/text information but could also be mouse moves, screen changes, etc.
- Specifically this refers to ReadConsoleInput A/W and PeekConsoleInput A/W

Author:
- Austin Diviness (AustDi) 1-Mar-2017
- Michael Niksa (MiNiksa) 1-Mar-2017

Revision History:
- Pulled from original code by - KazuM Apr.19.1996
- Separated from directio.h/directio.cpp (AustDi, 2017)
--*/

#pragma once

#include "readData.hpp"
#include "../types/inc/IInputEvent.hpp"
#include <deque>
#include <memory>


class DirectReadData final : public ReadData
{
public:
    DirectReadData(_In_ InputBuffer* const pInputBuffer,
                   _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                   _In_ const size_t eventReadCount,
                   _In_ std::deque<std::unique_ptr<IInputEvent>> partialEvents);

    DirectReadData(DirectReadData&&) = default;

    ~DirectReadData() override;

    BOOL Notify(_In_ WaitTerminationReason const TerminationReason,
                _In_ BOOLEAN const fIsUnicode,
                _Out_ NTSTATUS* const pReplyStatus,
                _Out_ DWORD* const pNumBytes,
                _Out_ DWORD* const pControlKeyState,
                _Out_ void* const pOutputData) override;

private:
    const size_t _eventReadCount;
    std::deque<std::unique_ptr<IInputEvent>> _partialEvents;
    std::deque<std::unique_ptr<IInputEvent>> _outEvents;
};
