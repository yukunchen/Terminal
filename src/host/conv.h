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

void WriteConvRegionToScreen(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                             _In_ const SMALL_RECT srConvRegion);

RECT GetImeSuggestionWindowPos();

NTSTATUS ConsoleImeResizeCompStrView();
NTSTATUS ConsoleImeResizeCompStrScreenBuffer(_In_ COORD const coordNewScreenSize);

void ConsoleImePaint(_In_ ConversionAreaInfo* ConvAreaInfo);
NTSTATUS ImeControl(_In_ PCOPYDATASTRUCT pCopyDataStruct);
