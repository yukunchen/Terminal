/*++
Copyright (c) Microsoft Corporation

Module Name:
- misc.h

Abstract:
- This file implements the NT console server font routines.

Author:
- Therese Stowell (ThereseS) 22-Jan-1991

Revision History:
--*/

#pragma once

#include "screenInfo.hpp"
#include "../types/inc/IInputEvent.hpp"
#include <deque>
#include <memory>


void SetConsoleCPInfo(_In_ const BOOL fOutput);

BOOL CheckBisectStringW(_In_reads_bytes_(cBytes) const WCHAR * pwchBuffer,
                        _In_ DWORD cWords,
                        _In_ DWORD cBytes);
BOOL CheckBisectProcessW(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                         _In_reads_bytes_(cBytes) const WCHAR * pwchBuffer,
                         _In_ DWORD cWords,
                         _In_ DWORD cBytes,
                         _In_ SHORT sOriginalXPosition,
                         _In_ BOOL fEcho);

HRESULT SplitToOem(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events);