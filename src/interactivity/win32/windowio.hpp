#pragma once

#include "precomp.h"

#include "..\..\server\ProcessHandle.h"

#pragma hdrstop

void HandleKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole);
BOOL HandleSysKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole);
BOOL HandleMouseEvent(_In_ const SCREEN_INFORMATION * const pScreenInfo, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam);

VOID SetConsoleWindowOwner(_In_ const HWND hwnd, _Inout_opt_ ConsoleProcessHandle* pProcessData);
DWORD ConsoleInputThreadProcWin32(LPVOID lpParameter);
