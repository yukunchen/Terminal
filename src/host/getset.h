/*++
Copyright (c) Microsoft Corporation

Module Name:
- getset.h

Abstract:
- This file implements the NT console server console state API.

Author:
- Therese Stowell (ThereseS) 5-Dec-1990

Revision History:
--*/

#pragma once
#include "../inc/conattrs.hpp"
class SCREEN_INFORMATION;


void DoSrvGetConsoleScreenBufferInfo(const SCREEN_INFORMATION& screenInfo,
                                     _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvSetScreenBufferInfo(SCREEN_INFORMATION& screenInfo, const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvGetConsoleCursorInfo(const SCREEN_INFORMATION& screenInfo,
                               _Out_ ULONG* const pCursorSize,
                               _Out_ bool* const pIsVisible);

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorPosition(SCREEN_INFORMATION& screenInfo, const COORD* const pCursorPosition);
[[nodiscard]]
HRESULT DoSrvSetConsoleCursorInfo(SCREEN_INFORMATION& screenInfo, const ULONG CursorSize, const bool IsVisible);
[[nodiscard]]
HRESULT DoSrvSetConsoleWindowInfo(SCREEN_INFORMATION& screenInfo,
                                  const bool IsAbsoluteRectangle,
                                  const SMALL_RECT* const pWindowRectangle);

[[nodiscard]]
HRESULT DoSrvScrollConsoleScreenBufferW(SCREEN_INFORMATION& screenInfo,
                                        const SMALL_RECT* const pSourceRectangle,
                                        const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        const wchar_t wchFill,
                                        const WORD attrFill);

[[nodiscard]]
HRESULT DoSrvSetConsoleTextAttribute(SCREEN_INFORMATION& screenInfo, const WORD Attribute);
void DoSrvPrivateSetLegacyAttributes(SCREEN_INFORMATION& screenInfo,
                                     const WORD Attribute,
                                     const bool fForeground,
                                     const bool fBackground,
                                     const bool fMeta);
void SetScreenColors(SCREEN_INFORMATION& screenInfo,
                     _In_ WORD Attributes,
                     _In_ WORD PopupAttributes,
                     _In_ BOOL UpdateWholeScreen);

[[nodiscard]]
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode);
void DoSrvPrivateAllowCursorBlinking(SCREEN_INFORMATION& screenInfo, const bool fEnable);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetScrollingRegion(SCREEN_INFORMATION& screenInfo, const SMALL_RECT* const psrScrollMargins);
[[nodiscard]]
NTSTATUS DoSrvPrivateReverseLineFeed(SCREEN_INFORMATION& screenInfo);
[[nodiscard]]
NTSTATUS DoSrvMoveCursorVertically(SCREEN_INFORMATION& screenInfo, const short lines);

[[nodiscard]]
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(SCREEN_INFORMATION& screenInfo);
void DoSrvPrivateUseMainScreenBuffer(SCREEN_INFORMATION&  screenInfo);

[[nodiscard]]
NTSTATUS DoSrvPrivateHorizontalTabSet();
[[nodiscard]]
NTSTATUS DoSrvPrivateForwardTab(const SHORT sNumTabs);
[[nodiscard]]
NTSTATUS DoSrvPrivateBackwardsTab(const SHORT sNumTabs);
void DoSrvPrivateTabClear(const bool fClearAll);

void DoSrvPrivateEnableVT200MouseMode(const bool fEnable);
void DoSrvPrivateEnableUTF8ExtendedMouseMode(const bool fEnable);
void DoSrvPrivateEnableSGRExtendedMouseMode(const bool fEnable);
void DoSrvPrivateEnableButtonEventMouseMode(const bool fEnable);
void DoSrvPrivateEnableAnyEventMouseMode(const bool fEnable);
void DoSrvPrivateEnableAlternateScroll(const bool fEnable);

void DoSrvPrivateSetConsoleXtermTextAttribute(SCREEN_INFORMATION& screenInfo,
                                              const int iXtermTableEntry,
                                              const bool fIsForeground);
void DoSrvPrivateSetConsoleRGBTextAttribute(SCREEN_INFORMATION& screenInfo,
                                            const COLORREF rgbColor,
                                            const bool fIsForeground);

[[nodiscard]]
NTSTATUS DoSrvPrivateEraseAll(SCREEN_INFORMATION& screenInfo);

void DoSrvSetCursorStyle(SCREEN_INFORMATION& screenInfo,
                         const CursorType cursorType);
void DoSrvSetCursorColor(SCREEN_INFORMATION& screenInfo,
                         const COLORREF cursorColor);

[[nodiscard]]
NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(const SCREEN_INFORMATION& screenInfo,
                                                      _Out_ WORD* const pwAttributes);

void DoSrvPrivateRefreshWindow(const SCREEN_INFORMATION& screenInfo);

void DoSrvGetConsoleOutputCodePage(_Out_ unsigned int* const pCodePage);

[[nodiscard]]
NTSTATUS DoSrvPrivateSuppressResizeRepaint();

void DoSrvIsConsolePty(_Out_ bool* const pIsPty);
