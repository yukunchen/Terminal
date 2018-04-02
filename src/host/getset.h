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
void DoSrvSetScreenBufferInfo(_In_ SCREEN_INFORMATION* const ScreenInfo, _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
void DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                               _Out_ ULONG* const pCursorSize,
                               _Out_ BOOLEAN* const pIsVisible);

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const COORD* const pCursorPosition);
[[nodiscard]]
HRESULT DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ ULONG const CursorSize, _In_ BOOLEAN const IsVisible);
[[nodiscard]]
HRESULT DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ BOOLEAN const IsAbsoluteRectangle, _In_ const SMALL_RECT* const pWindowRectangle);

[[nodiscard]]
HRESULT DoSrvScrollConsoleScreenBufferW(_In_ SCREEN_INFORMATION* pScreenInfo,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _In_ const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        _In_ wchar_t const wchFill,
                                        _In_ WORD const attrFill);

[[nodiscard]]
HRESULT DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute);
void DoSrvPrivateSetLegacyAttributes(_In_ SCREEN_INFORMATION* pScreenInfo,
                                     _In_ WORD const Attribute,
                                     _In_ bool const fForeground,
                                     _In_ const bool fBackground,
                                     _In_ const bool fMeta);
void SetScreenColors(_In_ SCREEN_INFORMATION* ScreenInfo,
                     _In_ WORD Attributes,
                     _In_ WORD PopupAttributes,
                     _In_ BOOL UpdateWholeScreen);

[[nodiscard]]
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode);
void DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* const pScreenInfo, _In_ const bool fEnable);
[[nodiscard]]
NTSTATUS DoSrvPrivateSetScrollingRegion(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const SMALL_RECT* const psrScrollMargins);
[[nodiscard]]
NTSTATUS DoSrvPrivateReverseLineFeed(_In_ SCREEN_INFORMATION* pScreenInfo);

[[nodiscard]]
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);
void DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);

[[nodiscard]]
NTSTATUS DoSrvPrivateHorizontalTabSet();
[[nodiscard]]
NTSTATUS DoSrvPrivateForwardTab(_In_ SHORT const sNumTabs);
[[nodiscard]]
NTSTATUS DoSrvPrivateBackwardsTab(_In_ SHORT const sNumTabs);
void DoSrvPrivateTabClear(_In_ bool const fClearAll);

void DoSrvPrivateEnableVT200MouseMode(_In_ bool const fEnable);
void DoSrvPrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnable);
void DoSrvPrivateEnableSGRExtendedMouseMode(_In_ bool const fEnable);
void DoSrvPrivateEnableButtonEventMouseMode(_In_ bool const fEnable);
void DoSrvPrivateEnableAnyEventMouseMode(_In_ bool const fEnable);
void DoSrvPrivateEnableAlternateScroll(_In_ bool const fEnable);

void DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                              _In_ const int iXtermTableEntry,
                                              _In_ const bool fIsForeground);
void DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                            _In_ const COLORREF rgbColor,
                                            _In_ const bool fIsForeground);

[[nodiscard]]
NTSTATUS DoSrvPrivateEraseAll(_In_ SCREEN_INFORMATION* const pScreenInfo);

void DoSrvSetCursorStyle(_In_ const SCREEN_INFORMATION* const pScreenInfo,
                         _In_ CursorType const cursorType);
void DoSrvSetCursorColor(_In_ const SCREEN_INFORMATION* const pScreenInfo,
                         _In_ COLORREF const cursorColor);

[[nodiscard]]
NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(_In_ SCREEN_INFORMATION* const pScreenInfo, _Out_ WORD* const pwAttributes);

void DoSrvPrivateRefreshWindow(_In_ SCREEN_INFORMATION* const pScreenInfo);

void DoSrvGetConsoleOutputCodePage(_Out_ unsigned int* const pCodePage);

[[nodiscard]]
NTSTATUS DoSrvPrivateSuppressResizeRepaint();

void DoSrvIsConsolePty(_Out_ bool* const pIsPty);
