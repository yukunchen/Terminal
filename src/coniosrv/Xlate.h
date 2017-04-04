/*++
Copyright (c) Microsoft Corporation

Module Name:
- Xlate.h

Abstract:
- Implements part of the input stack and keyboard-related Windows API's that are
  otherwise missing on OneCore.
- Code comes from \minkernel\minwin\console\conkbd\xlate.c.
- Originally, the code comes from NtUser:
  The Windows input stack is implemented in NtUser. As such, the vast majority
  of the code in this module first came from \windows\Core\ntuser\client and
  \windows\Core\ntuser\kernel. Most function, macro, and global variable names
  have been preserved, so merely searching for them in the directories listed
  above will reveal their true source.

Author:
- RichP Mar.10.2004

Revision History:
- Added input API implementations exposed by this server (see ConIoSrv.h). (HeGatta, 2017)
--*/

#pragma once

//
// from userk.h
//

/*
 * Key Event (KE) structure
 * Stores a Virtual Key event
 */
#pragma warning(push)
#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
typedef struct tagKE {
    union {
        BYTE bScanCode;    // Virtual Scan Code (Set 1)
        WCHAR wchInjected; // Unicode char from SendInput()
    };
    USHORT usFlaggedVk;    // Vk | Flags
    DWORD  dwTime;         // time in milliseconds
    HANDLE hDevice;
    KEYBOARD_INPUT_DATA data;
} KE, *PKE;
#pragma warning(pop)

/*
 * Flag (LowLevel and HighLevel) for
 * hex Alt+Numpad mode.
 * If you need to add a new flag for gfInNumpadHexInput,
 * note the variable is BYTE.
 */
#define NUMPAD_HEXMODE_LL       (1)
#define NUMPAD_HEXMODE_HL       (2)

#ifndef SCANCODE_NUMPAD_PLUS
#define SCANCODE_NUMPAD_PLUS    (0x4e)
#endif
#ifndef SCANCODE_NUMPAD_DOT
#define SCANCODE_NUMPAD_DOT     (0x53)
#endif
//
//


//
// from winuserp.h
//

#define VK_UNKNOWN        0xFF

/*
 * vkey table counts, macros, etc. input synchonized key state tables have
 * 2 bits per vkey: fDown, fToggled. Async key state tables have 3 bits:
 * fDown, fToggled, fDownSinceLastRead.
 *
 * Important! The array gafAsyncKeyState matches the bit positions of the
 * afKeyState array in each thread info block. The fDownSinceLastRead bit
 * for the async state is stored in a separate bit array, called
 * gafAsyncKeyStateRecentDown.
 *
 * It is important that the bit positions of gafAsyncKeyState and
 * pti->afKeyState match because we copy from one to the other to maintain
 * key state synchronization between threads.
 *
 * These macros below MUST be used when setting / querying key state.
 */
#define CVKKEYSTATE                 256
#define CBKEYSTATE                  (CVKKEYSTATE >> 2)
#define CBKEYSTATERECENTDOWN        (CVKKEYSTATE >> 3)
#define KEYSTATE_TOGGLE_BYTEMASK    0xAA    // 10101010
#define KEYSTATE_DOWN_BYTEMASK      0x55    // 01010101

/*
 * Two bits per VK (down & toggle) so we can pack 4 VK keystates into 1 byte:
 *
 *              Byte 0                           Byte 1
 * .---.---.---.---.---.---.---.---. .---.---.---.---.---.---.---.---. .-- -
 * | T | D | T | D | T | D | T | D | | T | D | T | D | T | D | T | D | |
 * `---'---'---'---'---'---'---'---' `---'---'---'---'---'---'---'---' `-- -
 * : VK 3  : VK 2  : VK 1  : VK 0  : : VK 7  : VK 6  : VK 5  : VK 4  : :
 *
 * KEY_BYTE(pb, vk)   identifies the byte containing the VK's state
 * KEY_DOWN_BIT(vk)   identifies the VK's down bit within a byte
 * KEY_TOGGLE_BIT(vk) identifies the VK's toggle bit within a byte
 */
#define KEY_BYTE(pb, vk)   pb[((BYTE)(vk)) >> 2]
#define KEY_DOWN_BIT(vk)   (1 << ((((BYTE)(vk)) & 3) << 1))
#define KEY_TOGGLE_BIT(vk) (1 << (((((BYTE)(vk)) & 3) << 1) + 1))

#define TestKeyDownBit(pb, vk)     (KEY_BYTE(pb,vk) &   KEY_DOWN_BIT(vk))
#define SetKeyDownBit(pb, vk)      (KEY_BYTE(pb,vk) |=  KEY_DOWN_BIT(vk))
#define ClearKeyDownBit(pb, vk)    (KEY_BYTE(pb,vk) &= ~KEY_DOWN_BIT(vk))
#define TestKeyToggleBit(pb, vk)   (KEY_BYTE(pb,vk) &   KEY_TOGGLE_BIT(vk))
#define SetKeyToggleBit(pb, vk)    (KEY_BYTE(pb,vk) |=  KEY_TOGGLE_BIT(vk))
#define ClearKeyToggleBit(pb, vk)  (KEY_BYTE(pb,vk) &= ~KEY_TOGGLE_BIT(vk))
#define ToggleKeyToggleBit(pb, vk) (KEY_BYTE(pb,vk) ^=  KEY_TOGGLE_BIT(vk))

#define TM_INMENUMODE     0x0001
#define TM_POSTCHARBREAKS 0x0002

extern BYTE gafRawKeyState[CBKEYSTATE];

#define TestRawKeyDown(vk)     TestKeyDownBit(gafRawKeyState, vk)
#define SetRawKeyDown(vk)      SetKeyDownBit(gafRawKeyState, vk)
#define ClearRawKeyDown(vk)    ClearKeyDownBit(gafRawKeyState, vk)
#define TestRawKeyToggle(vk)   TestKeyToggleBit(gafRawKeyState, vk)
#define SetRawKeyToggle(vk)    SetKeyToggleBit(gafRawKeyState, vk)
#define ClearRawKeyToggle(vk)  ClearKeyToggleBit(gafRawKeyState, vk)
#define ToggleRawKeyToggle(vk) ToggleKeyToggleBit(gafRawKeyState, vk)
#define TestKeyStateToggle(vk) TestKeyToggleBit(gafRawKeyState, vk)
#define TestKeyStateDown(vk)   TestKeyDownBit(gafRawKeyState, vk)  
        
//
//


BOOL
XlateInit(
    void
    );

BYTE
VKFromVSC(
    PKE pke,
    BYTE bPrefix
    );

int
xxxInternalToUnicode(
    __in UINT uVirtKey,
    __in UINT uScanCode,
    __in CONST PBYTE pfvk,
    __out PWCHAR awchChars,
    __in UINT cChar,
    __in UINT uiTMFlags,
    __out PDWORD pdwKeyFlags
    );

WCHAR *
GetKeyName(
    BYTE Vsc
    );

VOID
UpdateRawKeyState(
    BYTE Vk,
    BOOL fBreak
);

ULONG
GetControlKeyState(
    VOID
);
