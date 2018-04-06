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


void DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                     _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvSetScreenBufferInfo(_In_ SCREEN_INFORMATION* const ScreenInfo, const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                               _Out_ ULONG* const pCursorSize,
                               _Out_ bool* const pIsVisible);

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, const COORD* const pCursorPosition);
[[nodiscard]]
HRESULT DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, const ULONG CursorSize, const bool IsVisible);
[[nodiscard]]
HRESULT DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo, const bool IsAbsoluteRectangle, const SMALL_RECT* const pWindowRectangle);

[[nodiscard]]
HRESULT DoSrvScrollConsoleScreenBufferW(_In_ SCREEN_INFORMATION* pScreenInfo,
                                        const SMALL_RECT* const pSourceRectangle,
                                        const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        const wchar_t wchFill,
                                        const WORD attrFill);

[[nodiscard]]
HRESULT DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, const WORD Attribute);
void DoSrvPrivateSetLegacyAttributes(_In_ SCREEN_INFORMATION* pScreenInfo,
                                     const WORD Attribute,
                                     const bool fForeground,
                                     const bool fBackground,
                                     const bool fMeta);
void SetScreenColors(_In_ SCREEN_INFORMATION* ScreenInfo,
                     _In_ WORD Attributes,
                     _In_ WORD PopupAttributes,
                     _In_ BOOL UpdateWholeScreen);

[[nodiscard]]
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode);
void DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* const pScreenInfo, const bool fEnable);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetScrollingRegion(_In_ SCREEN_INFORMATION* pScreenInfo, const SMALL_RECT* const psrScrollMargins);
[[nodiscard]]
NTSTATUS DoSrvPrivateReverseLineFeed(_In_ SCREEN_INFORMATION* pScreenInfo);

[[nodiscard]]
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);
void DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);

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

void DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                              const int iXtermTableEntry,
                                              const bool fIsForeground);
void DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                            const COLORREF rgbColor,
                                            const bool fIsForeground);

[[nodiscard]]
NTSTATUS DoSrvPrivateEraseAll(_In_ SCREEN_INFORMATION* const pScreenInfo);

void DoSrvSetCursorStyle(const SCREEN_INFORMATION* const pScreenInfo,
                         const CursorType cursorType);
void DoSrvSetCursorColor(const SCREEN_INFORMATION* const pScreenInfo,
                         const COLORREF cursorColor);

[[nodiscard]]
NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(_In_ SCREEN_INFORMATION* const pScreenInfo, _Out_ WORD* const pwAttributes);

void DoSrvPrivateRefreshWindow(_In_ SCREEN_INFORMATION* const pScreenInfo);

void DoSrvGetConsoleOutputCodePage(_Out_ unsigned int* const pCodePage);

[[nodiscard]]
NTSTATUS DoSrvPrivateSuppressResizeRepaint();
