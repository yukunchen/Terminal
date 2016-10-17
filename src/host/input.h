/*++
Copyright (c) Microsoft Corporation

Module Name:
- input.h

Abstract:
- This module contains the internal structures and definitions used
  by the input (keyboard and mouse) component of the NT console subsystem.
- This file implements the circular buffer management for input events.
- The circular buffer is described by a header, which resides in the beginning of the memory allocated when the
  buffer is created.  The header contains all of the per-buffer information, such as reader, writer, and
  reference counts, and also holds the pointers into the circular buffer proper.
- When the in and out pointers are equal, the circular buffer is empty.  When the in pointer trails the out pointer
  by 1, the buffer is full.  Thus, a 512 byte buffer can hold only 511 bytes; one byte is lost so that full and empty
  conditions can be distinguished. So that the user can put 512 bytes in a buffer that they created with a size
  of 512, we allow for this byte lost when allocating the memory.

Author:
- Therese Stowell (Thereses) 12-Nov-1990. Adapted from OS/2 subsystem server\srvpipe.c

Revision History:
--*/

#pragma once

#include "conapi.h"
#include "screenInfo.hpp"
#include "server.h" // potentially circular include reference

#include "inputReadHandleData.h"

typedef struct _INPUT_INFORMATION
{
    ConsoleObjectHeader Header;
    _Field_size_(InputBufferSize) PINPUT_RECORD InputBuffer;
    DWORD InputBufferSize;  // size in events
    DWORD InputMode;
    ULONG_PTR First;    // ptr to base of circular buffer
    ULONG_PTR In;   // ptr to next free event
    ULONG_PTR Out;  // ptr to next available event
    ULONG_PTR Last; // ptr to end + 1 of buffer
    ConsoleWaitQueue WaitQueue; // formerly ReadWaitQueue
    struct
    {
        DWORD Disable:1;    // High   : specifies input code page or enable/disable in NLS state
        DWORD Unavailable:1;    // Middle : specifies console window doing menu loop or size move
        DWORD Open:1;   // Low    : specifies open/close in NLS state or IME hot key

        DWORD ReadyConversion:1;    // if conversion mode is ready by succeed communicate to ConIME.
        // then this field is TRUE.
        DWORD InComposition:1;  // specifies if there's an ongoing text composition
        DWORD Conversion;   // conversion mode of ime (i.e IME_CMODE_xxx).
        // this field uses by GetConsoleNlsMode
    } ImeMode;

    HANDLE InputWaitEvent;
    struct _CONSOLE_INFORMATION *Console;
    INPUT_RECORD ReadConInpDbcsLeadByte;
    INPUT_RECORD WriteConInpDbcsLeadByte[2];
} INPUT_INFORMATION, *PINPUT_INFORMATION;

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
#define UNICODE_LONG_HYPHEN ((WCHAR)0x2013)
#define UNICODE_QUOTE L'\"'
#define UNICODE_HYPHEN L'-'

#define TAB_SIZE 8
#define TAB_MASK (TAB_SIZE - 1)
// WHY IS THIS NOT POSITION % TAB_SIZE?!
#define NUMBER_OF_SPACES_IN_TAB(POSITION) (TAB_SIZE - ((POSITION) & TAB_MASK))

#define AT_EOL(COOKEDREADDATA) ((COOKEDREADDATA)->BytesRead == ((COOKEDREADDATA)->CurrentPosition*2))

#define KEY_PRESSED 0x8000
#define KEY_TOGGLED 0x01

#define CONSOLE_CTRL_C_SEEN  1
#define CONSOLE_CTRL_BREAK_SEEN 2

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

NTSTATUS ReadInputBuffer(_In_ PINPUT_INFORMATION const pInputInfo,
                         _Out_writes_(*pcLength) PINPUT_RECORD pInputRecord,
                         _Inout_ PDWORD pcLength,
                         _In_ BOOL const fPeek,
                         _In_ BOOL const fWaitForData,
                         _In_ BOOL const fStreamRead,
                         _In_ INPUT_READ_HANDLE_DATA* pHandleData,
                         _In_opt_ PCONSOLE_API_MSG pConsoleMessage,
                         _In_opt_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                         _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                         _In_ ULONG const cbWaitParameter,
                         _In_ BOOLEAN const fWaitBlockExists,
                         _In_ BOOLEAN const fUnicode);
DWORD WriteInputBuffer(_In_ PINPUT_INFORMATION pInputInfo, _In_ PINPUT_RECORD pInputRecord, _In_ DWORD cInputRecords);
NTSTATUS PrependInputBuffer(_In_ PINPUT_INFORMATION pInputInfo, _In_ PINPUT_RECORD pInputRecord, _Inout_ DWORD * const pcLength);
NTSTATUS CreateInputBuffer(_In_opt_ ULONG cEvents, _Out_ PINPUT_INFORMATION pInputInfo);
void ReinitializeInputBuffer(_Inout_ PINPUT_INFORMATION pInputInfo);
void FreeInputBuffer(_In_ PINPUT_INFORMATION pInputInfo);

NTSTATUS WaitForMoreToRead(_In_ PINPUT_INFORMATION pInputInfo,
                           _In_opt_ PCONSOLE_API_MSG pConsoleMsg,
                           _In_opt_ CONSOLE_WAIT_ROUTINE pfnWaitRoutine,
                           _In_reads_bytes_opt_(cbWaitParameter) PVOID pvWaitParameter,
                           _In_ const ULONG cbWaitParameter,
                           _In_ const BOOLEAN fWaitBlockExists);

ULONG GetControlKeyState(_In_ const LPARAM lParam);

void GetNumberOfReadyEvents(_In_ const INPUT_INFORMATION * const pInputInfo, _Out_ PULONG pcEvents);
void FlushInputBuffer(_Inout_ PINPUT_INFORMATION pInputInfo);

NTSTATUS FlushAllButKeys();

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
