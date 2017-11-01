/*++
Copyright (c) Microsoft Corporation

Module Name:
- input.h

Abstract:
- This module contains the internal structures and definitions used
  by the input (keyboard and mouse) component of the NT console subsystem.

Author:
- Therese Stowell (Thereses) 12-Nov-1990. Adapted from OS/2 subsystem server\srvpipe.c

Revision History:
--*/

#pragma once

#include "conapi.h"
#include "screenInfo.hpp"
#include "server.h" // potentially circular include reference

#include "inputReadHandleData.h"
#include "inputBuffer.hpp"

#include "../inc/unicode.hpp"

// indicates how much to change the opacity at each mouse/key toggle
// Opacity is defined as 0-255. 12 is therefore approximately 5% per tick.
#define OPACITY_DELTA_INTERVAL 12

#define MAX_CHARS_FROM_1_KEYSTROKE 6

#define KEY_TRANSITION_UP 0x80000000

class INPUT_KEY_INFO
{
public:
    INPUT_KEY_INFO(_In_ const WORD wVirtualKeyCode, _In_ const ULONG ulControlKeyState);
    ~INPUT_KEY_INFO();

    const WORD GetVirtualKey() const;

    const bool IsCtrlPressed() const;
    const bool IsAltPressed() const;
    const bool IsShiftPressed() const;

    const bool IsCtrlOnly() const;
    const bool IsShiftOnly() const;
    const bool IsShiftAndCtrlOnly() const;
    const bool IsAltOnly() const;

    const bool HasNoModifiers() const;

private:
    WORD _wVirtualKeyCode;
    ULONG _ulControlKeyState;
};

#define TAB_SIZE 8
#define TAB_MASK (TAB_SIZE - 1)
// WHY IS THIS NOT POSITION % TAB_SIZE?!
#define NUMBER_OF_SPACES_IN_TAB(POSITION) (TAB_SIZE - ((POSITION) & TAB_MASK))

// these values are related to GetKeyboardState
#define KEY_PRESSED 0x8000
#define KEY_TOGGLED 0x01

void ClearKeyInfo(_In_ const HWND hWnd);

ULONG GetControlKeyState(_In_ const LPARAM lParam);

bool IsInProcessedInputMode();
bool IsInVirtualTerminalInputMode();
bool ShouldTakeOverKeyboardShortcuts();

void HandleMenuEvent(_In_ const DWORD wParam);
void HandleFocusEvent(_In_ const BOOL fSetFocus);
void HandleCtrlEvent(_In_ const DWORD EventType);
void HandleGenericKeyEvent(_In_ KeyEvent keyEvent, _In_ const bool generateBreak);

void ProcessCtrlEvents();

BOOL IsSystemKey(_In_ WORD const wVirtualKeyCode);
