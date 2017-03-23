/*++
Copyright (c) Microsoft Corporation

Module Name:
- stream.h

Abstract:
- This file implements the NT console server stream API

Author:
- Therese Stowell (ThereseS) 6-Nov-1990

Revision History:
--*/

#pragma once

#include "cmdline.h"
#include "..\server\IWaitRoutine.h"
#include "readData.hpp"

#define IS_CONTROL_CHAR(wch)  ((wch) < L' ')

// TODO MSFT:8805366 update docs
// Routine Description:
// - This routine is used in stream input.  It gets input and filters it for unicode characters.
// Arguments:
// - pInputBuffer - Pointer to input buffer.
// - Char - Unicode char input.
// - Wait - TRUE if the routine shouldn't wait for input.
// - Console - Pointer to console buffer information.
// - HandleData - Pointer to handle data structure.
// - Message - csr api message.
// - WaitRoutine - Routine to call when wait is woken up.
// - WaitParameter - Parameter to pass to wait routine.
// - WaitParameterLength - Length of wait parameter.
// - WaitBlockExists - TRUE if wait block has already been created.
// - CommandLineEditingKeys - if present, arrow keys will be returned. on output, if TRUE, Char contains virtual key code for arrow key.
// - CommandLinePopupKeys - if present, arrow keys will be returned. on output, if TRUE, Char contains virtual key code for arrow key.
// Return Value:
// - <none>
NTSTATUS GetChar(_In_ InputBuffer* pInputBuffer,
                 _Out_ PWCHAR pwchChar,
                 _In_ const BOOL fWait,
                 _Out_opt_ PBOOLEAN pfCommandLineEditingKeys,
                 _Out_opt_ PBOOLEAN pfCommandLinePopupKeys,
                 _Out_opt_ PBOOLEAN pfEnableScrollMode,
                 _Out_opt_ PDWORD pdwKeyState);

// Routine Description:
// - This routine returns the total number of screen spaces the characters up to the specified character take up.
ULONG RetrieveTotalNumberOfSpaces(_In_ const SHORT sOriginalCursorPositionX,
                                  _In_reads_(ulCurrentPosition) const WCHAR * const pwchBuffer,
                                  _In_ const ULONG ulCurrentPosition);

// Routine Description:
// - This routine returns the number of screen spaces the specified character takes up.
ULONG RetrieveNumberOfSpaces(_In_ SHORT sOriginalCursorPositionX,
                             _In_reads_(ulCurrentPosition + 1) const WCHAR * const pwchBuffer,
                             _In_ ULONG ulCurrentPosition);

VOID UnblockWriteConsole(_In_ const DWORD dwReason);
