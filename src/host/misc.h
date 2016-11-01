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

int ConvertToOem(_In_ const UINT uiCodePage,
                 _In_reads_(cchSource) const WCHAR * const pwchSource,
                 _In_ const UINT cchSource,
                 _Out_writes_(cchTarget) CHAR * const pchTarget,
                 _In_ const UINT cchTarget);

int ConvertInputToUnicode(_In_ const UINT uiCodePage,
                          _In_reads_(cchSource) const CHAR * const pchSource,
                          _In_ const UINT cchSource,
                          _Out_writes_(cchTarget) WCHAR * const pwchTarget,
                          _In_ const UINT cchTarget);

WCHAR CharToWchar(_In_reads_(cch) const char * const pch, _In_ const UINT cch);

int ConvertOutputToUnicode(_In_ UINT uiCodePage,
                           _In_reads_(cchSource) const CHAR * const pchSource,
                           _In_ UINT cchSource,
                           _Out_writes_(cchTarget) WCHAR *pwchTarget,
                           _In_ UINT cchTarget);

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
