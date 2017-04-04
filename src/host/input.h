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

#define UNICODE_BACKSPACE ((WCHAR)0x08)
// NOTE: This isn't actually a backspace. It's a graphical block. But I believe it's emitted by one of our ANSI/OEM --> Unicode conversions.
// We should dig further into this in the future.
#define UNICODE_BACKSPACE2 ((WCHAR)0x25d8)
#define UNICODE_CARRIAGERETURN ((WCHAR)0x0d)
#define UNICODE_LINEFEED ((WCHAR)0x0a)
#define UNICODE_BELL ((WCHAR)0x07)
#define UNICODE_TAB ((WCHAR)0x09)
#define UNICODE_SPACE ((WCHAR)0x20)
#define UNICODE_LEFT_SMARTQUOTE ((WCHAR)0x201c)
#define UNICODE_RIGHT_SMARTQUOTE ((WCHAR)0x201d)
#define UNICODE_EM_DASH ((WCHAR)0x2014)
#define UNICODE_EN_DASH ((WCHAR)0x2013)
#define UNICODE_NBSP ((WCHAR)0xa0)
#define UNICODE_NARROW_NBSP ((WCHAR)0x202f)
#define UNICODE_QUOTE L'\"'
#define UNICODE_HYPHEN L'-'

#define TAB_SIZE 8
#define TAB_MASK (TAB_SIZE - 1)
// WHY IS THIS NOT POSITION % TAB_SIZE?!
#define NUMBER_OF_SPACES_IN_TAB(POSITION) (TAB_SIZE - ((POSITION) & TAB_MASK))

// these values are related to GetKeyboardState
#define KEY_PRESSED 0x8000
#define KEY_TOGGLED 0x01

#define LoadKeyEvent(PEVENT, KEYDOWN, CHAR, KEYCODE, SCANCODE, KEYSTATE) { \
        (PEVENT)->EventType = KEY_EVENT;                                   \
        (PEVENT)->Event.KeyEvent.bKeyDown = KEYDOWN;                       \
        (PEVENT)->Event.KeyEvent.wRepeatCount = 1;                         \
        (PEVENT)->Event.KeyEvent.uChar.UnicodeChar = CHAR;                 \
        (PEVENT)->Event.KeyEvent.wVirtualKeyCode = KEYCODE;                \
        (PEVENT)->Event.KeyEvent.wVirtualScanCode = SCANCODE;              \
        (PEVENT)->Event.KeyEvent.dwControlKeyState = KEYSTATE;             \
        }

void ClearKeyInfo(_In_ const HWND hWnd);

ULONG GetControlKeyState(_In_ const LPARAM lParam);

bool IsInProcessedInputMode();
bool IsInVirtualTerminalInputMode();
bool ShouldTakeOverKeyboardShortcuts();

void HandleMenuEvent(_In_ const DWORD wParam);
void HandleFocusEvent(_In_ const BOOL fSetFocus);
void HandleCtrlEvent(_In_ const DWORD EventType);
bool HandleTerminalKeyEvent(_In_ const INPUT_RECORD* const pInputRecord);
void HandleGenericKeyEvent(INPUT_RECORD InputEvent, BOOL bGenerateBreak);

void ProcessCtrlEvents();

BOOL IsSystemKey(_In_ WORD const wVirtualKeyCode);
