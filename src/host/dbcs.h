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

#define CP_USA                 1252
#define CP_KOREAN              949
#define CP_JAPANESE            932
#define CP_CHINESE_SIMPLIFIED  936
#define CP_CHINESE_TRADITIONAL 950

#define IsBilingualCP(cp) ((cp)==CP_JAPANESE || (cp)==CP_KOREAN)
#define IsEastAsianCP(cp) ((cp)==CP_JAPANESE || (cp)==CP_KOREAN || (cp)==CP_CHINESE_TRADITIONAL || (cp)==CP_CHINESE_SIMPLIFIED)

void SetLineChar(_In_ SCREEN_INFORMATION * const pScreenInfo);
bool CheckBisectStringA(_In_reads_bytes_(cbBuf) PCHAR pchBuf, _In_ DWORD cbBuf, _In_ const CPINFO * const pCPInfo);
void BisectWrite(_In_ const SHORT sStringLen, _In_ const COORD coordTarget, _In_ PSCREEN_INFORMATION pScreenInfo);

DWORD RemoveDbcsMarkCell(_Out_writes_(cch) PCHAR_INFO pciDst, _In_reads_(cch) const CHAR_INFO * pciSrc, _In_ DWORD cch);
DWORD RemoveDbcsMarkAll(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                        _In_ ROW* pRow,
                        _In_ SHORT * const psLeftChar,
                        _In_ PRECT prcText,
                        _Inout_opt_ int * const piTextLeft,
                        _Inout_updates_(cchBuf) PWCHAR pwchBuf,
                        _In_ const SHORT cchBuf);

bool IsDBCSLeadByteConsole(_In_ const CHAR ch, _In_ const CPINFO * const pCPInfo);

BOOL IsCharFullWidth(_In_ WCHAR wch);

BYTE CodePageToCharSet(_In_ UINT const uiCodePage);

BOOL IsAvailableEastAsianCodePage(_In_ UINT const uiCodePage);

NTSTATUS ConsoleImeMessagePump(_In_ const UINT msg, _In_ const WPARAM wParam, _In_ const LPARAM lParam);

_Ret_range_(0, cbAnsi)
ULONG TranslateUnicodeToOem(_In_reads_(cchUnicode) PCWCHAR pwchUnicode,
                            _In_ const ULONG cchUnicode,
                            _Out_writes_bytes_(cbAnsi) PCHAR pchAnsi,
                            _In_ const ULONG cbAnsi,
                            _Out_opt_ PINPUT_RECORD pDbcsInputRecord);
