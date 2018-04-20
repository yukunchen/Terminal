/*++
Copyright (c) Microsoft Corporation

Module Name:
- output.h

Abstract:
- This module contains the internal structures and definitions used
  by the output (screen) component of the NT console subsystem.

Author:
- Therese Stowell (ThereseS) 12-Nov-1990

Revision History:
--*/

#pragma once

#include "screenInfo.hpp"
#include "server.h"
#include "../buffer/out/OutputCell.hpp"

void ScreenBufferSizeChange(const COORD coordNewSize);

[[nodiscard]]
NTSTATUS ReadScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                          _Inout_ std::vector<std::vector<OutputCell>>& outputCells,
                          _Inout_ PSMALL_RECT psrReadRegion);
[[nodiscard]]
NTSTATUS WriteScreenBuffer(SCREEN_INFORMATION& screenInfo,
                           _In_ PCHAR_INFO pciBuffer,
                           _Inout_ PSMALL_RECT psrWriteRegion);

[[nodiscard]]
NTSTATUS DoCreateScreenBuffer();

[[nodiscard]]
NTSTATUS ReadOutputString(const SCREEN_INFORMATION& screenInfo,
                          _Inout_ PVOID pvBuffer,
                          const COORD coordRead,
                          const ULONG ulStringType,
                          _Inout_ PULONG pcRecords);

void ScrollRegion(SCREEN_INFORMATION& screenInfo,
                  const SMALL_RECT scrollRect,
                  const std::optional<SMALL_RECT> clipRect,
                  const COORD destinationOrigin,
                  const CHAR_INFO fill);

VOID SetConsoleWindowOwner(const HWND hwnd, _Inout_opt_ ConsoleProcessHandle* pProcessData);

bool StreamScrollRegion(SCREEN_INFORMATION& screenInfo);

std::vector<std::vector<OutputCell>> ReadRectFromScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                                                              const COORD coordSourcePoint,
                                                              const Microsoft::Console::Types::Viewport viewport);

SHORT ScrollEntireScreen(SCREEN_INFORMATION& screenInfo, const SHORT sScrollValue);

// For handling process handle state, not the window state itself.
void CloseConsoleProcessState();

void GetNonBiDiKeyboardLayout(_In_ HKL * const phklActive);
