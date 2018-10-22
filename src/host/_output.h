/*++
Copyright (c) Microsoft Corporation

Module Name:
- _output.h

Abstract:
- These methods provide processing of the text buffer into the final screen rendering state
- For all languages (CJK and Western)
- Most GDI work is processed in these classes

Author(s):
- KazuM Jun.11.1997

Revision History:
- Remove FE/Non-FE separation in preparation for refactoring. (MiNiksa, 2014)
--*/

#pragma once

#include "screenInfo.hpp"
#include "../buffer/out/OutputCell.hpp"
#include "../buffer/out/OutputCellRect.hpp"

void StreamWriteToScreenBuffer(SCREEN_INFORMATION& screenInfo,
                               const std::wstring& wstr,
                               const bool fWasLineWrapped);

void WriteRectToScreenBufferOC(SCREEN_INFORMATION& screenInfo,
                               const OutputCellRect& cells,
                               const COORD dest);

void WriteRectToScreenBuffer(SCREEN_INFORMATION& screenInfo,
                             const std::vector<std::vector<OutputCell>>& cells,
                             const COORD coordDest);

void WriteRegionToScreen(SCREEN_INFORMATION& screenInfo, _In_ PSMALL_RECT psrRegion);

void WriteToScreen(SCREEN_INFORMATION& screenInfo, const SMALL_RECT srRegion);

size_t FillOutputAttributes(SCREEN_INFORMATION& screenInfo,
                            const TextAttribute attr,
                            const COORD target,
                            const size_t amountToWrite);

size_t FillOutputW(SCREEN_INFORMATION& screenInfo,
                   const wchar_t wch,
                   const COORD target,
                   const size_t amountToWrite);

size_t FillOutputA(SCREEN_INFORMATION& screenInfo,
                   const char ch,
                   const COORD target,
                   const size_t amountToWrite);

void FillRectangle(SCREEN_INFORMATION& screenInfo,
                   const OutputCell& cell,
                   const Microsoft::Console::Types::Viewport rect);

size_t WriteOutputAttributes(SCREEN_INFORMATION& screenInfo,
                             const std::vector<WORD>& attrs,
                             const COORD target);

size_t WriteOutputStringW(SCREEN_INFORMATION& screenInfo,
                          const std::vector<wchar_t>& chars,
                          const COORD target);

size_t WriteOutputStringA(SCREEN_INFORMATION& screenInfo,
                          const std::vector<char>& chars,
                          const COORD target);
