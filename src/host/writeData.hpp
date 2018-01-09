/*++
Copyright (c) Microsoft Corporation

Module Name:
- writeData.hpp

Abstract:
- This file defines the interface for write data structures.
- This is used not only within the write call, but also to hold context in case a wait condition is required
  because writing to the buffer is blocked for some reason.

Author:
- Michael Niksa (MiNiksa) 9-Mar-2017

Revision History:
--*/

#pragma once

#include "../server/IWaitRoutine.h"
#include "../server/WaitTerminationReason.h"

class WriteData : public IWaitRoutine
{
public:
    WriteData(_In_ SCREEN_INFORMATION* const psiContext,
              _In_reads_bytes_(cbContext) wchar_t* const pwchContext,
              _In_ ULONG const cbContext,
              _In_ UINT const uiOutputCodepage);
    ~WriteData();

    void SetLeadByteAdjustmentStatus(_In_ bool const fLeadByteCaptured,
                                     _In_ bool const fLeadByteConsumed);

    void SetUtf8ConsumedCharacters(_In_ size_t const cchUtf8Consumed);

    BOOL Notify(_In_ WaitTerminationReason const TerminationReason,
                _In_ BOOLEAN const fIsUnicode,
                _Out_ NTSTATUS* const pReplyStatus,
                _Out_ DWORD* const pNumBytes,
                _Out_ DWORD* const pControlKeyState,
                _Out_ void* const pOutputData);

private:
    SCREEN_INFORMATION* const _psiContext;
    wchar_t* const _pwchContext;
    ULONG const _cbContext;
    UINT const _uiOutputCodepage;
    bool _fLeadByteCaptured;
    bool _fLeadByteConsumed;
    size_t _cchUtf8Consumed;
};
