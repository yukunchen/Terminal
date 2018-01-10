/*++
Copyright (c) Microsoft Corporation

Module Name:
- readDataRaw.hpp

Abstract:
- This file defines the read data structure for char-reading console APIs.
- Raw reads specifically refer to when the client application just wants text data with no edit line.
- The data struct will help store context across multiple calls or in the case of a wait condition.
- Wait conditions happen pretty much only when we don't have enough text (keyboard) data to return to the client.
- This can be triggered via ReadConsole A/W and ReadFile A/W calls.

Author:
- Austin Diviness (AustDi) 1-Mar-2017
- Michael Niksa (MiNiksa) 1-Mar-2017

Revision History:
- Pulled from original authoring by Therese Stowell (ThereseS) 6-Nov-1990
- Separated from stream.h/stream.cpp (AustDi, 2017)
--*/

#pragma once

#include "readData.hpp"

class RAW_READ_DATA final : public ReadData
{
public:
    RAW_READ_DATA(_In_ InputBuffer* const pInputBuffer,
                  _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                  _In_ ULONG BufferSize,
                  _Inout_updates_bytes_(BufferSize) WCHAR* BufPtr);

    ~RAW_READ_DATA() override;

    RAW_READ_DATA(RAW_READ_DATA&&) = default;

    BOOL Notify(_In_ WaitTerminationReason const TerminationReason,
                _In_ BOOLEAN const fIsUnicode,
                _Out_ NTSTATUS* const pReplyStatus,
                _Out_ DWORD* const pNumBytes,
                _Out_ DWORD* const pControlKeyState,
                _Out_ void* const pOutputData) override;

private:
    ULONG _BufferSize;
    _Field_size_(_BufferSize) PWCHAR _BufPtr;
};
