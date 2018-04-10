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


void DoSrvGetConsoleScreenBufferInfo(_In_ const SCREEN_INFORMATION& screenInfo,
                                     _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvSetScreenBufferInfo(_Inout_ SCREEN_INFORMATION& screenInfo, const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvGetConsoleCursorInfo(_In_ const SCREEN_INFORMATION& screenInfo,
                               _Out_ ULONG* const pCursorSize,
                               _Out_ bool* const pIsVisible);

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorPosition(_Inout_ SCREEN_INFORMATION& screenInfo, const COORD* const pCursorPosition);
[[nodiscard]]
HRESULT DoSrvSetConsoleCursorInfo(_Inout_ SCREEN_INFORMATION& screenInfo, const ULONG CursorSize, const bool IsVisible);
[[nodiscard]]
HRESULT DoSrvSetConsoleWindowInfo(_Inout_ SCREEN_INFORMATION& screenInfo,
                                  const bool IsAbsoluteRectangle,
                                  const SMALL_RECT* const pWindowRectangle);

[[nodiscard]]
HRESULT DoSrvScrollConsoleScreenBufferW(_Inout_ SCREEN_INFORMATION& screenInfo,
                                        const SMALL_RECT* const pSourceRectangle,
                                        const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        const wchar_t wchFill,
                                        const WORD attrFill);

[[nodiscard]]
HRESULT DoSrvSetConsoleTextAttribute(_Inout_ SCREEN_INFORMATION& screenInfo, const WORD Attribute);
void DoSrvPrivateSetLegacyAttributes(_Inout_ SCREEN_INFORMATION& screenInfo,
                                     const WORD Attribute,
                                     const bool fForeground,
                                     const bool fBackground,
                                     const bool fMeta);
void SetScreenColors(_Inout_ SCREEN_INFORMATION& screenInfo,
                     _In_ WORD Attributes,
                     _In_ WORD PopupAttributes,
                     _In_ BOOL UpdateWholeScreen);

[[nodiscard]]
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode);
void DoSrvPrivateAllowCursorBlinking(_Inout_ SCREEN_INFORMATION& screenInfo, const bool fEnable);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetScrollingRegion(_Inout_ SCREEN_INFORMATION& screenInfo, const SMALL_RECT* const psrScrollMargins);
[[nodiscard]]
NTSTATUS DoSrvPrivateReverseLineFeed(_Inout_ SCREEN_INFORMATION& screenInfo);

[[nodiscard]]
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_Inout_ SCREEN_INFORMATION& screenInfo);
void DoSrvPrivateUseMainScreenBuffer(_Inout_ SCREEN_INFORMATION&  screenInfo);

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

void DoSrvPrivateSetConsoleXtermTextAttribute(_Inout_ SCREEN_INFORMATION& screenInfo,
                                              const int iXtermTableEntry,
                                              const bool fIsForeground);
void DoSrvPrivateSetConsoleRGBTextAttribute(_Inout_ SCREEN_INFORMATION& screenInfo,
                                            const COLORREF rgbColor,
                                            const bool fIsForeground);

[[nodiscard]]
NTSTATUS DoSrvPrivateEraseAll(_Inout_ SCREEN_INFORMATION& screenInfo);

void DoSrvSetCursorStyle(SCREEN_INFORMATION& screenInfo,
                         const CursorType cursorType);
void DoSrvSetCursorColor(SCREEN_INFORMATION& screenInfo,
                         const COLORREF cursorColor);

[[nodiscard]]
NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(_In_ const SCREEN_INFORMATION& screenInfo,
                                                      _Out_ WORD* const pwAttributes);

void DoSrvPrivateRefreshWindow(_In_ const SCREEN_INFORMATION& screenInfo);

void DoSrvGetConsoleOutputCodePage(_Out_ unsigned int* const pCodePage);

[[nodiscard]]
NTSTATUS DoSrvPrivateSuppressResizeRepaint();
