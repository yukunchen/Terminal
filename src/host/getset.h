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

class SCREEN_INFORMATION;

HRESULT DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* const pScreenInfo, _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
HRESULT DoSrvSetScreenBufferInfo(_In_ SCREEN_INFORMATION* const ScreenInfo, _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo);
HRESULT DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Out_ ULONG* const pCursorSize, _Out_ BOOLEAN* const pIsVisible);

HRESULT DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const COORD* const pCursorPosition);
HRESULT DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ ULONG const CursorSize, _In_ BOOLEAN const IsVisible);
HRESULT DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ BOOLEAN const IsAbsoluteRectangle, _In_ const SMALL_RECT* const pWindowRectangle);

HRESULT DoSrvScrollConsoleScreenBufferW(_In_ SCREEN_INFORMATION* pScreenInfo,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _In_ const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        _In_ wchar_t const wchFill,
                                        _In_ WORD const attrFill);

HRESULT DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute);
HRESULT DoSrvPrivateSetLegacyAttributes(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute, _In_ bool const fForeground, _In_ const bool fBackground, _In_ const bool fMeta);
NTSTATUS SetScreenColors(_In_ SCREEN_INFORMATION* ScreenInfo, _In_ WORD Attributes, _In_ WORD PopupAttributes, _In_ BOOL UpdateWholeScreen);

NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode);
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode);
NTSTATUS DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ bool fEnable);
NTSTATUS DoSrvPrivateSetScrollingRegion(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const SMALL_RECT* const psrScrollMargins);
NTSTATUS DoSrvPrivateReverseLineFeed(_In_ SCREEN_INFORMATION* pScreenInfo);

NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);
NTSTATUS DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr);

NTSTATUS DoSrvPrivateHorizontalTabSet();
NTSTATUS DoSrvPrivateForwardTab(_In_ SHORT const sNumTabs);
NTSTATUS DoSrvPrivateBackwardsTab(_In_ SHORT const sNumTabs);
NTSTATUS DoSrvPrivateTabClear(_In_ bool const fClearAll);

NTSTATUS DoSrvPrivateEnableVT200MouseMode(_In_ bool const fEnable);
NTSTATUS DoSrvPrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnable);
NTSTATUS DoSrvPrivateEnableSGRExtendedMouseMode(_In_ bool const fEnable);
NTSTATUS DoSrvPrivateEnableButtonEventMouseMode(_In_ bool const fEnable);
NTSTATUS DoSrvPrivateEnableAnyEventMouseMode(_In_ bool const fEnable);
NTSTATUS DoSrvPrivateEnableAlternateScroll(_In_ bool const fEnable);

NTSTATUS DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ int const iXtermTableEntry, _In_ const bool fIsForeground);
NTSTATUS DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ COLORREF const rgbColor, _In_ const bool fIsForeground);

NTSTATUS DoSrvPrivateEraseAll(_In_ SCREEN_INFORMATION* const pScreenInfo);

NTSTATUS DoSrvSetCursorStyle(_In_ SCREEN_INFORMATION* const pScreenInfo, _In_ unsigned int const cursorType);

NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(_In_ SCREEN_INFORMATION* const pScreenInfo, _Out_ WORD* const pwAttributes);
