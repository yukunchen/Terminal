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

void ScreenBufferSizeChange(_In_ COORD const coordNewSize);

void SetProcessForegroundRights(_In_ const HANDLE hProcess, _In_ const BOOL fForeground);

NTSTATUS ReadScreenBuffer(_In_ const SCREEN_INFORMATION * const pScreenInfo, _Inout_ PCHAR_INFO pciBuffer, _Inout_ PSMALL_RECT psrReadRegion);
NTSTATUS WriteScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo, _In_ PCHAR_INFO pciBuffer, _Inout_ PSMALL_RECT psrWriteRegion);

NTSTATUS DoCreateScreenBuffer();

NTSTATUS ReadOutputString(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                          _Inout_ PVOID pvBuffer,
                          _In_ const COORD coordRead,
                          _In_ const ULONG ulStringType,
                          _Inout_ PULONG pcRecords);

NTSTATUS ScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo,
                      _Inout_ PSMALL_RECT psrScroll,
                      _In_opt_ PSMALL_RECT psrClip,
                      _In_ COORD coordDestinationOrigin,
                      _In_ CHAR_INFO ciFill);

VOID SetConsoleWindowOwner(_In_ const HWND hwnd, _Inout_opt_ PCONSOLE_PROCESS_HANDLE pProcessData);

bool StreamScrollRegion(_Inout_ PSCREEN_INFORMATION pScreenInfo);

NTSTATUS ReadRectFromScreenBuffer(_In_ const SCREEN_INFORMATION * const pScreenInfo,
                                  _In_ COORD const coordSourcePoint,
                                  _Inout_ PCHAR_INFO pciTarget,
                                  _In_ COORD const coordTargetSize,
                                  _In_ PSMALL_RECT const psrTargetRect,
                                  _Inout_opt_ TextAttribute* pTextAttributes);

SHORT ScrollEntireScreen(_Inout_ PSCREEN_INFORMATION pScreenInfo, _In_ const SHORT sScrollValue);

// For handling process handle state, not the window state itself.
void CloseConsoleProcessState();

void GetNonBiDiKeyboardLayout(_In_ HKL * const phklActive);
