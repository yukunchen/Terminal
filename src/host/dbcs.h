/*++
Copyright (c) Microsoft Corporation

Module Name:
- dbcs.h

Abstract:
- Provides helpers to manage double-byte (double-width) characters for CJK languages within the text buffer
- Some items historically referred to as "FE" or "Far East" (geopol sensitive) and converted to "East Asian".
  Refers to Chinese, Japanese, and Korean languages that require significantly different handling from legacy ASCII/Latin1 characters.

Author:
- KazuM (suspected)

Revision History:
--*/

#pragma once

#include "screenInfo.hpp"

#define CP_USA                 437
#define CP_KOREAN              949
#define CP_JAPANESE            932
#define CP_CHINESE_SIMPLIFIED  936
#define CP_CHINESE_TRADITIONAL 950

#define IsBilingualCP(cp) ((cp)==CP_JAPANESE || (cp)==CP_KOREAN)
#define IsEastAsianCP(cp) ((cp)==CP_JAPANESE || (cp)==CP_KOREAN || (cp)==CP_CHINESE_TRADITIONAL || (cp)==CP_CHINESE_SIMPLIFIED)

void SetLineChar(SCREEN_INFORMATION& screenInfo);
bool CheckBisectStringA(_In_reads_bytes_(cbBuf) PCHAR pchBuf, _In_ DWORD cbBuf, const CPINFO * const pCPInfo);
void CleanupDbcsEdgesForWrite(const size_t stringLen,
                              const COORD coordTarget,
                              SCREEN_INFORMATION& screenInfo);

DWORD RemoveDbcsMarkCell(_Out_writes_(cch) PCHAR_INFO pciDst, _In_reads_(cch) const CHAR_INFO * pciSrc, _In_ DWORD cch);

bool IsDBCSLeadByteConsole(const CHAR ch, const CPINFO * const pCPInfo);

BOOL IsCharFullWidth(_In_ WCHAR wch);

BYTE CodePageToCharSet(const UINT uiCodePage);

BOOL IsAvailableEastAsianCodePage(const UINT uiCodePage);

_Ret_range_(0, cbAnsi)
ULONG TranslateUnicodeToOem(_In_reads_(cchUnicode) PCWCHAR pwchUnicode,
                            const ULONG cchUnicode,
                            _Out_writes_bytes_(cbAnsi) PCHAR pchAnsi,
                            const ULONG cbAnsi,
                            _Out_ std::unique_ptr<IInputEvent>& partialEvent);
