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

void StreamWriteToScreenBuffer(_Inout_updates_(cchBuffer) PWCHAR pwchBuffer,
                               _In_ SHORT cchBuffer,
                               _In_ PSCREEN_INFORMATION pScreenInfo,
                               _Inout_updates_(cchBuffer) PCHAR const pchBuffer,
                               _In_ const bool fWasLineWrapped);

NTSTATUS WriteRectToScreenBuffer(_In_reads_(coordSrcDimensions.X * coordSrcDimensions.Y * sizeof(CHAR_INFO)) PBYTE const prgbSrc,
                             _In_ const COORD coordSrcDimensions,
                             _In_ const SMALL_RECT * const psrSrc,
                             _In_ PSCREEN_INFORMATION pScreenInfo,
                             _In_ const COORD coordDest,
                             _In_reads_opt_(coordSrcDimensions.X * coordSrcDimensions.Y) TextAttribute* const pTextAttributes);

void WriteRegionToScreen(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ PSMALL_RECT psrRegion);

void WriteToScreen(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT srRegion);

NTSTATUS WriteOutputString(_In_ PSCREEN_INFORMATION pScreenInfo,
                           _In_reads_(*pcRecords) const VOID * pvBuffer,
                           _In_ const COORD coordWrite,
                           _In_ const ULONG ulStringType,
                           _Inout_ PULONG pcRecords,    // this value is valid even for error cases
                           _Out_opt_ PULONG pcColumns);

NTSTATUS FillOutput(_In_ PSCREEN_INFORMATION pScreenInfo,
                    _In_ WORD wElement,
                    _In_ const COORD coordWrite,
                    _In_ const ULONG ulElementType,
                    _Inout_ PULONG pcElements); // this value is valid even for error cases

void FillRectangle(_In_ const CHAR_INFO * const pciFill, _In_ PSCREEN_INFORMATION pScreenInfo, _In_ const SMALL_RECT * const psrTarget);
