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
#include "OutputCell.hpp"

void ScreenBufferSizeChange(const COORD coordNewSize);

[[nodiscard]]
NTSTATUS ReadScreenBuffer(const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ std::vector<std::vector<OutputCell>>& outputCells,
                          _Inout_ PSMALL_RECT psrReadRegion);
[[nodiscard]]
NTSTATUS WriteScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo,
                           _In_ PCHAR_INFO pciBuffer,
                           _Inout_ PSMALL_RECT psrWriteRegion);

[[nodiscard]]
NTSTATUS DoCreateScreenBuffer();

[[nodiscard]]
NTSTATUS ReadOutputString(const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ PVOID pvBuffer,
                          const COORD coordRead,
                          const ULONG ulStringType,
                          _Inout_ PULONG pcRecords);

[[nodiscard]]
NTSTATUS ScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo,
                      _Inout_ PSMALL_RECT psrScroll,
                      _In_opt_ PSMALL_RECT psrClip,
                      _In_ COORD coordDestinationOrigin,
                      _In_ CHAR_INFO ciFill);

VOID SetConsoleWindowOwner(const HWND hwnd, _Inout_opt_ ConsoleProcessHandle* pProcessData);

bool StreamScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo);

std::vector<std::vector<OutputCell>> ReadRectFromScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                                                              const COORD coordSourcePoint,
                                                              const Microsoft::Console::Types::Viewport viewport);

SHORT ScrollEntireScreen(_Inout_ PSCREEN_INFORMATION pScreenInfo, const SHORT sScrollValue);

// For handling process handle state, not the window state itself.
void CloseConsoleProcessState();

void GetNonBiDiKeyboardLayout(_In_ HKL * const phklActive);
