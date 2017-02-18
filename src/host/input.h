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

#define AT_EOL(COOKEDREADDATA) ((COOKEDREADDATA)->BytesRead == ((COOKEDREADDATA)->CurrentPosition*2))

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


NTSTATUS WaitForMoreToRead(_In_opt_ PCONSOLE_API_MSG pConsoleMsg,
                           _In_opt_ ConsoleWaitRoutine pfnWaitRoutine,
                           _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                           _In_ const ULONG cbWaitParameter,
                           _In_ const BOOLEAN fWaitBlockExists);

ULONG GetControlKeyState(_In_ const LPARAM lParam);


BOOL HandleSysKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole);
void HandleKeyEvent(_In_ const HWND hWnd, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam, _Inout_opt_ PBOOL pfUnlockConsole);
BOOL HandleMouseEvent(_In_ const SCREEN_INFORMATION * const pScreenInfo, _In_ const UINT Message, _In_ const WPARAM wParam, _In_ const LPARAM lParam);
void HandleMenuEvent(_In_ const DWORD wParam);
void HandleFocusEvent(_In_ const BOOL fSetFocus);
void HandleCtrlEvent(_In_ const DWORD EventType);
void ProcessCtrlEvents();

bool HandleTerminalKeyEvent(_In_ const INPUT_RECORD* const pInputRecord);

BOOL IsPrintableKey(WORD const wVirtualKeyCode);
BOOL IsSystemKey(_In_ WORD const wVirtualKeyCode);
