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

void ScreenBufferSizeChange(_In_ COORD const coordNewSize);

[[nodiscard]]
NTSTATUS ReadScreenBuffer(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ PCHAR_INFO pciBuffer,
                          _Inout_ PSMALL_RECT psrReadRegion);
[[nodiscard]]
NTSTATUS WriteScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo,
                           _In_ PCHAR_INFO pciBuffer,
                           _Inout_ PSMALL_RECT psrWriteRegion);

[[nodiscard]]
NTSTATUS DoCreateScreenBuffer();

[[nodiscard]]
NTSTATUS ReadOutputString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ PVOID pvBuffer,
                          _In_ const COORD coordRead,
                          _In_ const ULONG ulStringType,
                          _Inout_ PULONG pcRecords);

[[nodiscard]]
NTSTATUS ScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo,
                      _Inout_ PSMALL_RECT psrScroll,
                      _In_opt_ PSMALL_RECT psrClip,
                      _In_ COORD coordDestinationOrigin,
                      _In_ CHAR_INFO ciFill);

VOID SetConsoleWindowOwner(_In_ const HWND hwnd, _Inout_opt_ ConsoleProcessHandle* pProcessData);

bool StreamScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo);

std::vector<std::vector<OutputCell>> ReadRectFromScreenBuffer(const SCREEN_INFORMATION& screenInfo,
                                                              const COORD coordSourcePoint,
                                                              const SMALL_RECT targetRect);

SHORT ScrollEntireScreen(_Inout_ PSCREEN_INFORMATION pScreenInfo, _In_ const SHORT sScrollValue);

// For handling process handle state, not the window state itself.
void CloseConsoleProcessState();

void GetNonBiDiKeyboardLayout(_In_ HKL * const phklActive);
