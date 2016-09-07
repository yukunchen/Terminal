/*++
Copyright (c) Microsoft Corporation

Module Name:
- conv.h

Abstract:
- This module contains the internal structures and definitions used by the conversion area.
- "Conversion area" refers to either the in-line area where the text color changes and suggests options in IME-based languages
  or to the reserved line at the bottom of the screen offering suggestions and the current IME mode.

Author:
- KazuM March 8, 1993

Revision History:
--*/

#pragma once

#include "server.h"

void FreeConvAreaScreenBuffer(_Inout_ PSCREEN_INFORMATION pScreenInfo);

void WriteConvRegionToScreen(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                             _In_opt_ PCONVERSIONAREA_INFORMATION pConvAreaInfo,
                             _In_ const SMALL_RECT * const psrConvRegion);

RECT GetImeSuggestionWindowPos();

NTSTATUS ConsoleImeResizeCompStrView();
NTSTATUS ConsoleImeResizeCompStrScreenBuffer(_In_ COORD const coordNewScreenSize);

void ConsoleImePaint(_In_ PCONVERSIONAREA_INFORMATION ConvAreaInfo);
NTSTATUS ImeControl(_In_ PCOPYDATASTRUCT pCopyDataStruct);

void SetUndetermineAttribute();
