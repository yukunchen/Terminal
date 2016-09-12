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

NTSTATUS SrvGetConsoleMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleNumberOfInputEvents(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetLargestConsoleWindowSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

NTSTATUS SrvGetConsoleScreenBufferInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SCREENBUFFERINFO_MSG* pMsg);

NTSTATUS SrvSetScreenBufferInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetScreenBufferInfo(_In_ PSCREEN_INFORMATION const ScreenInfo, _In_ PCONSOLE_SCREENBUFFERINFO_MSG const a);

NTSTATUS SrvGetConsoleCursorInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_GETCURSORINFO_MSG* pMsg);

NTSTATUS SrvGetConsoleSelectionInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleMouseInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleFontSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleCurrentFont(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvSetConsoleCurrentFont(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvSetConsoleMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGenerateConsoleCtrlEvent(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

NTSTATUS SrvSetConsoleActiveScreenBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetConsoleActiveScreenBuffer(_In_ HANDLE hScreenBufferHandle);

NTSTATUS SrvFlushConsoleInputBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvSetConsoleScreenBufferSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

NTSTATUS SrvSetConsoleCursorPosition(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETCURSORPOSITION_MSG* pMsg);

NTSTATUS SrvSetConsoleCursorInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETCURSORINFO_MSG* pMsg);

NTSTATUS SrvSetConsoleWindowInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETWINDOWINFO_MSG* pMsg);

NTSTATUS SrvScrollConsoleScreenBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvScrollConsoleScreenBuffer(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SCROLLSCREENBUFFER_MSG* pMsg);

NTSTATUS SrvSetConsoleTextAttribute(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETTEXTATTRIBUTE_MSG* pMsg);

NTSTATUS SetScreenColors(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ WORD Attributes, _In_ WORD PopupAttributes, _In_ BOOL UpdateWholeScreen);
NTSTATUS SrvSetConsoleCP(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleCP(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleWindow(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvGetConsoleDisplayMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);
NTSTATUS SrvSetConsoleDisplayMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending);

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

NTSTATUS DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ int const iXtermTableEntry, _In_ const bool fIsForeground);
NTSTATUS DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ COLORREF const rgbColor, _In_ const bool fIsForeground);

