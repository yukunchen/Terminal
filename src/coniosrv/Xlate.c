/*++

Copyright (c) 2004  Microsoft Corporation

Module Name:

    xlate.c

Abstract:

    This module implements hardware keyboard to Win32 Virtual Key mapping.

Author:

    Rich Pletcher (richp) 10-Mar-2004   Taken from user sources

Revision History:

--*/

//
// Core NT headers
//

#include "precomp.h"

#include "ConIoSrv.h"

#include "Api.h"
#include "xlate.h"

typedef PKBDTABLES (__stdcall *PKBDLAYERDESCRIPTOR)(VOID);

PKBDTABLES pKbdTbl = NULL;

///**************************************************************************
// This table list keys that may affect Virtual Key values when held down.
//
// See kbd.h for a full description.
//
// 101/102key keyboard (type 4):
//    Virtual Key values vary only if CTRL is held down.
// 84-86 key keyboards (type 3):
//    Virtual Key values vary if one of SHIFT, CTRL or ALT is held down.
///**************************************************************************

VK_TO_BIT aVkToBits_VK[] = {
    { VK_SHIFT,   KBDSHIFT }, // 0x01
    { VK_CONTROL, KBDCTRL  }, // 0x02
    { VK_MENU,    KBDALT   }, // 0x04
    { 0,          0        }
};

//
// How some Virtual Key values change when a CONTROL key is held down.
//
ULONG aulControlCvt_VK[] = {
    MAKELONG(VK_NUMLOCK,  VK_PAUSE | KBDEXT),
    MAKELONG(VK_SCROLL,   VK_CANCEL),
    MAKELONG(0,0)
};

///**************************************************************************
// Tables defining how some Virtual Key values are modified when other keys
// are held down.
// Translates key combinations into indices for gapulCvt_VK_101[] or for
// gapulCvt_VK_84[] or for
//
// See kbd.h for a full description.
//
///**************************************************************************

MODIFIERS Modifiers_VK_STANDARD = {
    &aVkToBits_VK[0],
    4,                 // Maximum modifier bitmask/index
    {
        SHFT_INVALID,  // no keys held down    (no VKs are modified)
        0,             // SHIFT held down      84-86 key kbd
        1,             // CTRL held down       101/102 key kbd
        SHFT_INVALID,  // CTRL-SHIFT held down (no VKs are modified)
        2              // ALT held down        84-86 key kbd
    }
};

///**************************************************************************
// A tables of pointers indexed by the number obtained from Modify_VK.
// If a pointer is non-NULL then the table it points to is searched for
// Virtual Key that should have their values changed.
// There are two versions: one for 84-86 key kbds, one for 101/102 key kbds.
// gapulCvt_VK is initialized with the default (101/102 key kbd).
///**************************************************************************
ULONG *gapulCvt_VK_101[] = {
    NULL,                 // No VKs are changed by SHIFT being held down
    aulControlCvt_VK,     // Some VKs are changed by CTRL being held down
    NULL                  // No VKs are changed by ALT being held down
};

PULONG *gapulCvt_VK = gapulCvt_VK_101;

PMODIFIERS gpModifiers_VK = &Modifiers_VK_STANDARD;


//
// Raw Key state: this is the low-level async keyboard state.
// (assuming Scancodes are correctly translated to Virtual Keys). It is used
// for modifying and processing key events as they are received in ntinput.c
// The Virtual Keys recorded here are obtained directly from the Virtual
// Scancode via the awVSCtoVK[] table: no shift-state, numlock or other
// conversions are applied.
// This IS affected by injected keystrokes (SendInput, keybd_event) so that
// on-screen-keyboards and other accessibility components work just like the
// real keyboard: with the exception of the SAS (Ctrl-Alt-Del), which checks
// real physically pressed modifier keys (gfsSASModifiersDown).
// Left & right SHIFT, CTRL and ALT keys are distinct. (VK_RSHIFT etc.)
// See also: SetRawKeyDown() etc.
///
BYTE gafRawKeyState[CBKEYSTATE];


BOOL
XlateInit(
    void
    )
{
    PKBDLAYERDESCRIPTOR pKbdLayerDescriptor;
    HMODULE hKeyMap = LoadLibraryExW(L"kbdus.dll", NULL, 0);

    if (hKeyMap == NULL)
        return FALSE;

    pKbdLayerDescriptor = (PKBDLAYERDESCRIPTOR)GetProcAddress(hKeyMap, "KbdLayerDescriptor");

    if (pKbdLayerDescriptor)

        pKbdTbl = pKbdLayerDescriptor();

    if (pKbdTbl)
        return TRUE;

    FreeLibrary(hKeyMap);

    return FALSE;
}

//
// Determine the state of all the Modifier Keys (a Modifier Key is any key
// that may modify values produced by other keys: these are commonly SHIFT,
// CTRL and/or ALT). Build a bit-mask (wModBits) to encode which modifier
// keys are depressed.
///
WORD
GetModifierBits(
    PMODIFIERS pModifiers,
    LPBYTE afKeyState
    )
{
    PVK_TO_BIT pVkToBit = pModifiers->pVkToBit;
    WORD wModBits = 0;

    while (pVkToBit->Vk) {
        if (TestKeyDownBit(afKeyState, pVkToBit->Vk)) {
            wModBits |= pVkToBit->ModBits;
        }
        pVkToBit++;
    }

    return wModBits;
}

//
// Given modifier bits, return the modification number.
///
WORD
GetModificationNumber(
    PMODIFIERS pModifiers,
    WORD wModBits
    )
{
    if (wModBits > pModifiers->wMaxModBits) {
         return SHFT_INVALID;
    }

    return pModifiers->ModNumber[wModBits];
}

//****************************************************************************
// VKFromVSC
//
// This function is called from KeyEvent() after each call to VSCFromSC.  The
// keyboard input data passed in is translated to a virtual key code.
// This translation is dependent upon the currently depressed modifier keys.
//
// For instance, scan codes representing the number pad keys may be
// translated into VK_NUMPAD codes or cursor movement codes depending
// upon the state of NumLock and the modifier keys.
//
// History:
//
//*****************************************************************************
BYTE
VKFromVSC(
    PKE pke,
    BYTE bPrefix
    )
{
    USHORT usVKey;
    PVSC_VK pVscVk;
    static BOOL fVkPause;

    //
    // Initialize as an unknown VK (unrecognised scancode)
    ///
    pke->usFlaggedVk = usVKey = VK_UNKNOWN;

    pke->bScanCode &= 0x7F;

    if (pKbdTbl == NULL)
        return VK_UNKNOWN;

    if (bPrefix == 0) {
        if (pke->bScanCode < pKbdTbl->bMaxVSCtoVK) {
            //
            // direct index into non-prefix table
            ///
            usVKey = pKbdTbl->pusVSCtoVK[pke->bScanCode];
            if (usVKey == 0) {
                return 0xFF;
            }
        } else {
            return 0xFF;
        }
    } else {
        //
        // Scan the E0 or E1 prefix table for a match
        ///
        if (bPrefix == 0xE0) {
            //
            // Set the KBDEXT (extended key) bit in case the scancode is not
            // found in the table (eg: FUJITSU POS keyboard #65436)
            ///
            usVKey |= KBDEXT;
            //
            // Ignore the SHIFT keystrokes generated by the hardware
            ///
            if ((pke->bScanCode == SCANCODE_LSHIFT) ||
                    (pke->bScanCode == SCANCODE_RSHIFT)) {
                return 0;
            }
            pVscVk = pKbdTbl->pVSCtoVK_E0;
        } else if (bPrefix == 0xE1) {
            pVscVk = pKbdTbl->pVSCtoVK_E1;
        } else {
            //
            // Unrecognized prefix (from ScancodeMap?) produces an
            // unextended and unrecognized VK.
            ///
            return 0xFF;
        }
        while (pVscVk->Vk) {
            if (pVscVk->Vsc == pke->bScanCode) {
                usVKey = pVscVk->Vk;
                break;
            }
            pVscVk++;
        }
    }

    //
    // Scancode set 1 returns PAUSE button as E1 1D 45 (E1 Ctrl NumLock)
    // so convert E1 Ctrl to VK_PAUSE, and remember to discard the NumLock
    ///
    if (fVkPause) {
        //
        // This is the "45" part of the Pause scancode sequence. Discard
        // this key event: It is a false NumLock
        ///
        fVkPause = FALSE;
        return 0;
    }
    if (usVKey == VK_PAUSE) {
        //
        // This is the "E1 1D" part of the Pause scancode sequence. Alter
        // the scancode to the value Windows expects for Pause, and remember
        // to discard the "45" scancode that will follow.
        ///
        pke->bScanCode = 0x45;
        fVkPause = TRUE;
    }

    //
    // Convert to a different VK if some modifier keys are depressed.
    //
    if (usVKey & KBDMULTIVK) {
        WORD nMod;
        PULONG pul;

        nMod = GetModificationNumber(gpModifiers_VK,
                                     GetModifierBits(gpModifiers_VK, gafRawKeyState));

        //
        // Scan gapulCvt_VK[nMod] for matching VK.
        //
        if ((nMod != SHFT_INVALID) && ((pul = gapulCvt_VK[nMod]) != NULL)) {
            while (*pul != 0) {
                if (LOBYTE(*pul) == LOBYTE(usVKey)) {
                    pke->usFlaggedVk = (USHORT)HIWORD(*pul);
                    return (BYTE)pke->usFlaggedVk;
                }
                pul++;
            }
        }
    }

    pke->usFlaggedVk = usVKey;
    return (BYTE)usVKey;
}

#define RIPMSG1(a1, a2, a3)
#define TAGMSG0(a1, a2);
#define TAGMSG1(a1, a2, a3)
#define TAGMSG2(a1, a2, a3, a4)
#define xxxMessageBeep(a1)

#define UserAssert(a1)

//
// macros used locally to make life easier
//
#define ISCAPSLOCKON(pf) (TestKeyToggleBit(pf, VK_CAPITAL) != 0)
#define ISNUMLOCKON(pf)  (TestKeyToggleBit(pf, VK_NUMLOCK) != 0)
#define ISSHIFTDOWN(w)   (w & 0x01)
#define ISKANALOCKON(pf) (TestKeyToggleBit(pf, VK_KANA)    != 0)

enum {
    NUMPADCONV_OEMCP = 0,
    NUMPADCONV_HKLCP,
    NUMPADCONV_HEX_HKLCP,
    NUMPADCONV_HEX_UNICODE,
};

BYTE gfInNumpadHexInput;
BOOL gfEnableHexNumpad = TRUE;

//
// TranslateInjectedVKey
//
// Returns the number of characters (cch) translated.
//
// Note on VK_PACKET:
// Currently, the only purpose of VK_PACKET is to inject a Unicode character
// into the input stream, but it is intended to be extensible to include other
// manipulations of the input stream (including the message loop so that IMEs
// can be involved).  For example, we might send commands to the IME or other
// parts of the system.
// For Unicode character injection, we tried widening virtual keys to 32 bits
// of the form nnnn00e7, where nnnn is 0x0000 - 0xFFFF (representing Unicode
// characters 0x0000 - 0xFFFF) See KEYEVENTF_UNICODE.
// But many apps truncate wParam to 16-bits (poorly ported from 16-bits?) and
// several AV with these VKs (indexing into a table by WM_KEYDOWN wParam?) so
// we have to cache the character in pti->wchInjected for TranslateMessage to
// pick up (cf. GetMessagePos, GetMessageExtraInfo and GetMessageTime)
//
int
TranslateInjectedVKey(
    __in UINT uScanCode,
    __out PWCHAR awchChars,
    __in UINT uiTMFlags
    )
{
    UNREFERENCED_PARAMETER(uScanCode);
    UNREFERENCED_PARAMETER(awchChars);
    UNREFERENCED_PARAMETER(uiTMFlags);

    return 0;
}


//**************************************************************************
// aVkToVsc[] - table associating Virtual Key codes with Virtual Scancodes
//
// Ordered, 0-terminated.
//
// This is used for those Virtual Keys that do not appear in ausVK_???[]
// These are not the base Virtual Keys.  They require some modifier key
// depression (CTRL, ALT, SHIFT) or NumLock On to be generated.
//
// All the scancodes listed below should be marked KBDMULTIVK or KBDNUMPAD in
// ausVK_???[].
//
// This table is used by MapVirtualKey(wVk, 0).
//***************************************************************************
BYTE aVkNumpad[] = {
    VK_NUMPAD7,  VK_NUMPAD8,  VK_NUMPAD9, 0xFF, // 0x47 0x48 0x49 (0x4A)
    VK_NUMPAD4,  VK_NUMPAD5,  VK_NUMPAD6, 0xFF, // 0x4B 0x4C 0x4D (0x4E)
    VK_NUMPAD1,  VK_NUMPAD2,  VK_NUMPAD3,       // 0x4F 0x50 0x51
    VK_NUMPAD0,  VK_DECIMAL,  0                 // 0x50 0x51
};

#define NUMPADSPC_INVALID   (-1)

int
NumPadScanCodeToHex(
    UINT uScanCode,
    UINT uVirKey
    )
{
    if (uScanCode >= SCANCODE_NUMPAD_FIRST && uScanCode <= SCANCODE_NUMPAD_LAST) {
        int digit = aVkNumpad[uScanCode - SCANCODE_NUMPAD_FIRST];

        if (digit != 0xff) {
            return digit - VK_NUMPAD0;
        }

        return NUMPADSPC_INVALID;
    }

    if (gfInNumpadHexInput & NUMPAD_HEXMODE_HL) {
        //
        // Full keyboard
        //
        if (uVirKey >= L'A' && uVirKey <= L'F') {
            return uVirKey - L'A' + 0xa;
        }

        if (uVirKey >= L'0' && uVirKey <= L'9') {
            return uVirKey - L'0';
        }
    }

    return NUMPADSPC_INVALID;
}

SHORT
InternalVkKeyScanEx(
    WCHAR wchChar,
    PKBDTABLES pKbdTbl
    )
{
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    BYTE nShift;
    WORD wModBits, wModNumCtrl, wModNumShiftCtrl;
    SHORT shRetvalCtrl = 0;
    SHORT shRetvalShiftCtrl = 0;

    //
    // Ctrl and Shift-Control combinations are less favored, so determine
    // the values for nShift which we prefer not to use if at all possible.
    // This is for compatibility with Windows 95/98, which only returns a
    // Ctrl or Shift+Ctrl combo as a last resort. See bugs #78891 & #229141
    //
    wModNumCtrl = GetModificationNumber(pKbdTbl->pCharModifiers, KBDCTRL);
    wModNumShiftCtrl = GetModificationNumber(pKbdTbl->pCharModifiers, KBDSHIFT | KBDCTRL);

    for (pVKT = pKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
        for (pVK = pVKT->pVkToWchars;
                pVK->VirtualKey != 0;
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize)) {
            for (nShift = 0; nShift < pVKT->nModifications; nShift++) {
                if (pVK->wch[nShift] == wchChar) {
                    //
                    // A matching character has been found!
                    //
                    if (pVK->VirtualKey == 0xff) {
                        //
                        // dead char: back up to previous line to get the VK.
                        //
                        pVK = (PVK_TO_WCHARS1)((PBYTE)pVK - pVKT->cbSize);
                    }

                    //
                    // If this is the first Ctrl or the first Shift+Ctrl match,
                    // remember in case we don't find any better match.
                    // In the meantime, keep on looking.
                    //
                    if (nShift == wModNumCtrl) {
                        if (shRetvalCtrl == 0) {
                            shRetvalCtrl = (SHORT)MAKEWORD(pVK->VirtualKey, KBDCTRL);
                        }
                    } else if (nShift == wModNumShiftCtrl) {
                        if (shRetvalShiftCtrl == 0) {
                            shRetvalShiftCtrl = (SHORT)MAKEWORD(pVK->VirtualKey, KBDCTRL | KBDSHIFT);
                        }
                    } else {
                        //
                        // this seems like a very good match!
                        //
                        goto GoodMatchFound;
                    }
                }
            }
        }
    }

    //
    // Didn't find a good match: use whatever Ctrl/Shift+Ctrl match was found
    //
    if (shRetvalCtrl) {
        return shRetvalCtrl;
    }

    if (shRetvalShiftCtrl) {
        return shRetvalShiftCtrl;
    }

    //
    // May be a control character not explicitly in the layout tables
    //
    if (wchChar < 0x0020) {
        //
        // Ctrl+char -> char - 0x40
        //
        return (SHORT)MAKEWORD((wchChar + 0x40), KBDCTRL);
    }
    return -1;

GoodMatchFound:
    //
    // Scan aModification[] to find nShift: the index will be a bitmask
    // representing the Shifter Keys that need to be pressed to produce
    // this Shift State.
    //
    for (wModBits = 0;
         wModBits <= pKbdTbl->pCharModifiers->wMaxModBits;
         wModBits++)
    {
        if (pKbdTbl->pCharModifiers->ModNumber[wModBits] == nShift) {
            if (pVK->VirtualKey == 0xff) {
                //
                // The previous entry contains the actual virtual key in this case.
                //
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK - pVKT->cbSize);
            }
            return (SHORT)MAKEWORD(pVK->VirtualKey, wModBits);
        }
    }

    //
    // We should never reach this here.
    //
    RIPMSG1(RIP_ERROR, "InternalVkKeyScanEx error: wchChar = 0x%x", wchChar);
    return -1;
}

#undef MODIFIER_FOR_ALT_NUMPAD

#define MODIFIER_FOR_ALT_NUMPAD(wModBit) \
    ((((wModBits) & ~KBDKANA) == KBDALT) || (((wModBits) & ~KBDKANA) == (KBDALT | KBDSHIFT)))


int
xxxInternalToUnicode(
    __in UINT uVirtKey,
    __in UINT uScanCode,
    __in CONST PBYTE pfvk,
    __out_ecount(cChar) PWCHAR awchChars,
    __in UINT cChar,
    __in UINT uiTMFlags,
    __out PDWORD pdwKeyFlags
    )
{
    WORD wModBits, nShift;
    WCHAR *pUniChar;
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    static WORD NumpadChar;
    static WORD VKLastDown;
    static BYTE ConvMode;   // 0 == NUMPADCONV_OEMCP

    PLIGATURE1 pLigature;

    *pdwKeyFlags = (uScanCode & KBDBREAK);

    if ((BYTE)uVirtKey == VK_UNKNOWN) {
        //
        // WindowsBug 311712: this could be the case of
        // unrecognized scancode.
        //
        RIPMSG1(RIP_WARNING, "VK_UNKNOWN, vsc=%02x", uScanCode);
        return 0;
    }

    UserAssert(pKbdTbl != NULL);

    pUniChar = awchChars;

    uScanCode &= (0xFF | KBDEXT);

    if (*pdwKeyFlags & KBDBREAK) {        // break code processing
        //
        // Finalize number pad processing
        //
        //
        if (uVirtKey == VK_MENU) {
            if (NumpadChar) {
                if (ConvMode == NUMPADCONV_HEX_UNICODE) {
                    *pUniChar = NumpadChar;
                }

                //
                // Clear Alt-Numpad state, the ALT key-release generates 1 character.
                //
                VKLastDown = 0;
                ConvMode = NUMPADCONV_OEMCP;
                NumpadChar = 0;
                gfInNumpadHexInput &= ~NUMPAD_HEXMODE_HL;

                return 1;
            } else if (ConvMode != NUMPADCONV_OEMCP) {
                ConvMode = NUMPADCONV_OEMCP;
            }
        } else if (uVirtKey == VKLastDown) {
            //
            // The most recently depressed key has now come up: we are now
            // ready to accept a new NumPad key for Alt-Numpad processing.
            //
            VKLastDown = 0;
        }
    }

    if (!(*pdwKeyFlags & KBDBREAK) || (uiTMFlags & TM_POSTCHARBREAKS)) {
        //
        // Get the character modification bits.
        // The bit-mask (wModBits) encodes depressed modifier keys:
        // these bits are commonly KBDSHIFT, KBDALT and/or KBDCTRL
        // (representing Shift, Alt and Ctrl keys respectively)
        //
        wModBits = GetModifierBits(pKbdTbl->pCharModifiers, pfvk);

        //
        // If the current shift state is either Alt or Alt-Shift:
        //
        //   1. If a menu is currently displayed then clear the
        //      alt bit from wModBits and proceed with normal
        //      translation.
        //
        //   2. If this is a number pad key then do alt-<numpad>
        //      calculations.
        //
        //   3. Otherwise, clear alt bit and proceed with normal
        //      translation.
        //

        //
        // Equivalent code is in xxxKeyEvent() to check the
        // low level mode. If you change this code, you may
        // need to change xxxKeyEvent() as well.
        //
        if (!(*pdwKeyFlags & KBDBREAK) && MODIFIER_FOR_ALT_NUMPAD(wModBits)) {
            //
            // If this is a numeric numpad key
            //
            if ((uiTMFlags & TM_INMENUMODE) == 0) {
                if (gfEnableHexNumpad && uScanCode == SCANCODE_NUMPAD_DOT) {
                    if ((gfInNumpadHexInput & NUMPAD_HEXMODE_HL) == 0) {
                        //
                        // If the first key is '.', then we're
                        // entering hex input lang input mode.
                        //
                        ConvMode = NUMPADCONV_HEX_HKLCP;
                        //
                        // Inidicate to the rest of the system
                        // we're in Hex Alt+Numpad mode.
                        //
                        gfInNumpadHexInput |= NUMPAD_HEXMODE_HL;
                        TAGMSG0(DBGTAG_ToUnicode, "NUMPADCONV_HEX_HKLCP");
                    } else {
                        goto ExitNumpadMode;
                    }
                } else if (gfEnableHexNumpad && uScanCode == SCANCODE_NUMPAD_PLUS) {
                    if ((gfInNumpadHexInput & NUMPAD_HEXMODE_HL) == 0) {
                        //
                        // If the first key is '+', then we're
                        // entering hex UNICODE input mode.
                        //
                        ConvMode = NUMPADCONV_HEX_UNICODE;
                        //
                        // Inidicate to the rest of the system
                        // we're in Hex Alt+Numpad mode.
                        //
                        gfInNumpadHexInput |= NUMPAD_HEXMODE_HL;
                        TAGMSG0(DBGTAG_ToUnicode, "NUMPADCONV_HEX_UNICODE");
                    } else {
                        goto ExitNumpadMode;
                    }
                } else {
                    int digit = NumPadScanCodeToHex(uScanCode, uVirtKey);

                    if (digit < 0) {
                        goto ExitNumpadMode;
                    }

                    //
                    // Ignore repeats
                    //
                    if (VKLastDown == uVirtKey) {
                        return 0;
                    }

                    switch (ConvMode) {
                    case NUMPADCONV_HEX_HKLCP:
                    case NUMPADCONV_HEX_UNICODE:
                        //
                        // Input is treated as hex number.
                        //
                        TAGMSG1(DBGTAG_ToUnicode,
                                "->NUMPADCONV_HEX_*: old NumpadChar=%02x",
                                NumpadChar);
                        NumpadChar = (WORD)(NumpadChar * 0x10 + digit);
                        TAGMSG1(DBGTAG_ToUnicode,
                                "<-NUMPADCONV_HEX_*: new NumpadChar=%02x",
                                NumpadChar);
                        break;
                    default:
                       //
                       // Input is treated as decimal number.
                       //
                       NumpadChar = (WORD)(NumpadChar * 10 + digit);

                       //
                       // Do Alt-Numpad0 processing
                       //
                       if (NumpadChar == 0 && digit == 0) {
                           ConvMode = NUMPADCONV_HKLCP;
                       }
                       break;
                    }
                }
                VKLastDown = (WORD)uVirtKey;
            } else {
ExitNumpadMode:
                //
                // Clear Alt-Numpad state and the ALT shift state.
                //
                VKLastDown = 0;
                ConvMode = NUMPADCONV_OEMCP;
                NumpadChar = 0;
                wModBits &= ~KBDALT;
                gfInNumpadHexInput &= ~NUMPAD_HEXMODE_HL;
            }
        }

        //
        // LShift/RSHift+Backspace -> Left-to-Right and Right-to-Left marker
        //
        if ((uVirtKey == VK_BACK) && (pKbdTbl->fLocaleFlags & KLLF_LRM_RLM)) {
            #pragma prefast(push) 
            #pragma prefast(disable: __WARNING_INCORRECT_ANNOTATION, "Author of code knows limits of buffer, but cannot annotate since length is not passed as parameter or buffer is of constant length")

            if (TestKeyDownBit(pfvk, VK_LSHIFT)) {
                *pUniChar = 0x200E; // LRM
                return 1;
            } else if (TestKeyDownBit(pfvk, VK_RSHIFT)) {
                *pUniChar = 0x200F; // RLM
                return 1;
            }

            #pragma prefast(pop)
        } else if (((WORD)uVirtKey == VK_PACKET) && (LOBYTE(uScanCode) == 0)) {
            return TranslateInjectedVKey(uScanCode, awchChars, uiTMFlags);
        }

        //
        // Scan through all the shift-state tables until a matching Virtual
        // Key is found.
        //
        for (pVKT = pKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
            pVK = pVKT->pVkToWchars;
            while (pVK->VirtualKey != 0) {
                if (pVK->VirtualKey == (BYTE)uVirtKey) {
                    goto VK_Found;
                }
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
            }
        }

        //
        // Not found: virtual key is not a character.
        //
        goto ReturnBadCharacter;

VK_Found:
        //
        // The virtual key has been found in table pVKT, at entry pVK
        //

        //
        // If KanaLock affects this key and it is on: toggle KANA state
        // only if no other state is on. "KANALOK" attributes only exist
        // in Japanese keyboard layout, and only Japanese keyboard hardware
        // can be "KANA" lock on state.
        //
        if ((pVK->Attributes & KANALOK) && (ISKANALOCKON(pfvk))) {
            wModBits |= KBDKANA;
        } else {
            //
            // If CapsLock affects this key and it is on: toggle SHIFT state
            // only if no other state is on.
            // (CapsLock doesn't affect SHIFT state if Ctrl or Alt are down).
            // OR
            // If CapsLockAltGr affects this key and it is on: toggle SHIFT
            // state only if both Alt & Control are down.
            // (CapsLockAltGr only affects SHIFT if AltGr is being used).
            //
            if ((pVK->Attributes & CAPLOK) && ((wModBits & ~KBDSHIFT) == 0) &&
                    ISCAPSLOCKON(pfvk)) {
                wModBits ^= KBDSHIFT;
            } else if ((pVK->Attributes & CAPLOKALTGR) &&
                    ((wModBits & (KBDALT | KBDCTRL)) == (KBDALT | KBDCTRL)) &&
                    ISCAPSLOCKON(pfvk)) {
                wModBits ^= KBDSHIFT;
            }
        }

        //
        // If SGCAPS affects this key and CapsLock is on: use the next entry
        // in the table, but not is Ctrl or Alt are down.
        // (SGCAPS is used in Swiss-German, Czech and Czech 101 layouts)
        //
        if ((pVK->Attributes & SGCAPS) && ((wModBits & ~KBDSHIFT) == 0) &&
                ISCAPSLOCKON(pfvk)) {
            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
        }

        //
        // Convert the shift-state bitmask into one of the enumerated
        // logical shift states.
        //
        nShift = GetModificationNumber(pKbdTbl->pCharModifiers, wModBits);

        if (nShift == SHFT_INVALID) {
            //
            // An invalid combination of Shifter Keys
            //
            goto ReturnBadCharacter;

        } else if ((nShift < pVKT->nModifications) &&
                (pVK->wch[nShift] != WCH_NONE)) {
            //
            // There is an entry in the table for this combination of
            // Shift State (nShift) and Virtual Key (uVirtKey).
            //
            if (pVK->wch[nShift] == WCH_DEAD) {
                //
                // It is a dead character: the next entry contains
                // its value.
                //
                pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);

                goto ReturnGoodCharacter;

            } else if (pVK->wch[nShift] == WCH_LGTR) {
                //
                // It is a ligature.  Look in ligature table for a match.
                //
                if ((GET_KBD_VERSION(pKbdTbl) == 0) || ((pLigature = pKbdTbl->pLigature) == NULL)) {
                    //
                    // Hey, where's the table?
                    //
                    xxxMessageBeep(0);
                    goto ReturnBadCharacter;
                }

                while (pLigature->VirtualKey != 0) {
                    int iLig = 0;
                    UINT cwchT = 0;

                    if ((pLigature->VirtualKey == pVK->VirtualKey) &&
                            (pLigature->ModificationNumber == nShift)) {
                        //
                        // Found the ligature!
                        //
                        while ((iLig < pKbdTbl->nLgMax) && (cwchT < cChar)) {
                            if (pLigature->wch[iLig] == WCH_NONE) {
                                //
                                // End of ligature.
                                //
                                return cwchT;
                            }

#pragma prefast (suppress: __WARNING_INCORRECT_ANNOTATION_STRING, "cwchT < cChar")
                            pUniChar[cwchT++] = pLigature->wch[iLig];

                            iLig++;
                        }

                        return cwchT;
                    }
                    //
                    // Not a match, try the next entry.
                    //
                    pLigature = (PLIGATURE1)((PBYTE)pLigature + pKbdTbl->cbLgEntry);
                }
                //
                // No match found!
                //
                xxxMessageBeep(0);
                goto ReturnBadCharacter;
            }

            //
            // Match found: return the unshifted character
            //
            TAGMSG2(DBGTAG_ToUnicode,
                    "xxxInternalToUnicode: Match found '%C'(%x), goto ReturnGoodChar",
                    pVK->wch[nShift],
                    pVK->wch[nShift]);
            goto ReturnGoodCharacter;

        } else if (wModBits == KBDCTRL ||
                   wModBits == (KBDCTRL | KBDSHIFT) ||
                   wModBits == (KBDKANA | KBDCTRL) ||
                   wModBits == (KBDKANA | KBDCTRL| KBDSHIFT)) {
            //
            // There was no entry for this combination of Modification (nShift)
            // and Virtual Key (uVirtKey).  It may still be an ASCII control
            // character though:
            //
            if ((uVirtKey >= 'A') && (uVirtKey <= 'Z')) {
                //
                // If the virtual key is in the range A-Z we can convert
                // it directly to a control character.  Otherwise, we
                // need to search the control key conversion table for
                // a match to the virtual key.
                //
                *pUniChar = (WORD)(uVirtKey & 0x1f);
                return 1;
            } else if ((uVirtKey >= 0xFF61) && (uVirtKey <= 0xFF91)) {
                //
                // If the virtual key is in range FF61-FF91 (halfwidth
                // katakana), we convert it to Virtual scan code with
                // KANA modifier.
                //
                *pUniChar = (WORD)(InternalVkKeyScanEx((WCHAR)uVirtKey,pKbdTbl) & 0x1f);
                return 1;
            }
        }
    }

ReturnBadCharacter:
    // pkl->wchDiacritic = 0;
    return 0;

ReturnGoodCharacter:

    *pUniChar = (WORD)pVK->wch[nShift];
    return 1;
}


WCHAR *
GetKeyName(
    BYTE Vsc
    )
{
    PVSC_LPWSTR pKN;

    pKN = pKbdTbl->pKeyNames;

    while (pKN->vsc != 0) {
        if (Vsc == pKN->vsc)
            return pKN->pwsz;
        pKN++;
    }

    return NULL;
}

/***************************************************************************\
* UINT InternalMapVirtualKeyEx(UINT wCode, UINT wType, PKBDTABLES pKbdTbl);
*
* History:
* IanJa 5/13/91  from Win3.1 \\pucus\win31ro!drivers\keyboard\getname.asm
* GregoryW 2/21/95  renamed from _MapVirtualKey and added third parameter.
\***************************************************************************/

UINT InternalMapVirtualKeyEx(
    UINT wCode,
    UINT wType,
    PKBDTABLES pKbdTbl)
{
    PVK_TO_WCHARS1 pVK;
    PVK_TO_WCHAR_TABLE pVKT;
    UINT VkRet = 0;
    USHORT usScanCode;
    PVSC_VK pVscVk;
    PBYTE pVkNumpad;

    switch (wType) {
    case MAPVK_VK_TO_VSC:
    case MAPVK_VK_TO_VSC_EX:

        /*
        * Convert Virtual Key (wCode) to Scan Code
        */
        if ((wCode >= VK_SHIFT) && (wCode <= VK_MENU)) {

            /*
            * Convert ambiguous Shift/Control/Alt keys to left-hand keys
            */
            wCode = (UINT)((wCode - VK_SHIFT) * 2 + VK_LSHIFT);
        }

        /*
        * Scan through the table that maps Virtual Scancodes to Virtual Keys
        * for non-extended keys.
        */
        for (usScanCode = 0; usScanCode < pKbdTbl->bMaxVSCtoVK; usScanCode++) {
            if ((UINT)LOBYTE(pKbdTbl->pusVSCtoVK[usScanCode]) == wCode) {
                return usScanCode & 0xFF;
            }
        }

        /*
        * Scan through the table that maps Virtual Scancodes to Virtual Keys
        * for extended keys.
        */
        if (pKbdTbl->pVSCtoVK_E0) {
            for (pVscVk = pKbdTbl->pVSCtoVK_E0; pVscVk->Vk; pVscVk++) {
                if ((UINT)LOBYTE(pVscVk->Vk) == wCode) {
                    if (wType == MAPVK_VK_TO_VSC_EX) {
                        return pVscVk->Vsc | 0xe000;
                    }
                    return (UINT)pVscVk->Vsc;
                }
            }
        }

        if (wType == MAPVK_VK_TO_VSC_EX) {
            for (pVscVk = pKbdTbl->pVSCtoVK_E1; pVscVk && pVscVk->Vk; ++pVscVk) {
                if (LOBYTE(pVscVk->Vk) == wCode) {
                    return pVscVk->Vsc | 0xe100;
                }
            }
        }

        /*
        * There was no match: maybe the Virtual Key can only be generated
        * with Numlock on. Scan through aVkNumpad[] to determine scancode.
        */
        for (pVkNumpad = aVkNumpad; *pVkNumpad != 0; pVkNumpad++) {
            if ((UINT)(*pVkNumpad) == wCode) {
                return (UINT)(pVkNumpad - aVkNumpad) + SCANCODE_NUMPAD_FIRST;
            }
        }

        return 0;   // No match found!

    case MAPVK_VSC_TO_VK:
    case MAPVK_VSC_TO_VK_EX:

        /*
        * Convert Scan Code (wCode) to Virtual Key, disregarding modifier keys
        * and NumLock key etc.  Returns 0 for no corresponding Virtual Key
        */
        if (wCode < (UINT)(pKbdTbl->bMaxVSCtoVK)) {
            VkRet = (UINT)LOBYTE(pKbdTbl->pusVSCtoVK[wCode]);
        }
        else {
            switch (wCode & ~0xff) {
            case 0xe000:
                pVscVk = pKbdTbl->pVSCtoVK_E0;
                break;
            case 0xe100:
                pVscVk = pKbdTbl->pVSCtoVK_E1;
                break;
            default:
                pVscVk = NULL;
                break;
            }

            if (pVscVk) {
                /*
                * Scan the E0/E1 prefix tables for a match.
                */
                for (; pVscVk->Vk; pVscVk++) {
                    if (pVscVk->Vsc == (BYTE)wCode) {
                        VkRet = (UINT)LOBYTE(pVscVk->Vk);
                        break;
                    }
                }
            }
        }

        if ((wType == MAPVK_VSC_TO_VK) && (VkRet >= VK_LSHIFT) && (VkRet <= VK_RMENU)) {

            /*
            * Convert left/right Shift/Control/Alt keys to ambiguous keys
            * (neither left nor right)
            */
            VkRet = (UINT)((VkRet - VK_LSHIFT) / 2 + VK_SHIFT);
        }

        if (VkRet == 0xFF) {
            VkRet = 0;
        }
        return VkRet;

    case MAPVK_VK_TO_CHAR:

        /*
        * Bogus Win3.1 functionality: despite SDK documenation, return uppercase for
        * VK_A through VK_Z
        */
        if ((wCode >= (WORD)'A') && (wCode <= (WORD)'Z')) {
            return wCode;
        }

        // HIWORD is no loner the wchar, due to app compat problems #287134
        // We should not return the wchar from pti->wchInjected that cached
        // at GetMessage time.
        // (delete this commented-out section by end of March 1999 - IanJa)
        //
        // if (LOWORD(wCode) == VK_PACKET) {
        //    return HIWORD(wCode);
        // }

        /*
        * Convert Virtual Key (wCode) to ANSI.
        * Search each Shift-state table in turn, looking for the Virtual Key.
        */
        if (pKbdTbl->pVkToWcharTable) {
            for (pVKT = pKbdTbl->pVkToWcharTable; pVKT->pVkToWchars != NULL; pVKT++) {
                pVK = pVKT->pVkToWchars;
                while (pVK->VirtualKey != 0) {
                    if ((UINT)pVK->VirtualKey == wCode) {

                        /*
                        * Match found: return the unshifted character
                        */
                        if (pVK->wch[0] == WCH_DEAD) {

                            /*
                            * It is a dead character: the next entry contains its
                            * value.  Set the high bit to indicate dead key
                            * (undocumented behaviour)
                            */
                            pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
                            return pVK->wch[0] | (UINT)0x80000000;
                        }
                        else if (pVK->wch[0] == WCH_NONE) {
                            return 0; // 9013
                        }
                        if (pVK->wch[0] == WCH_NONE) {
                            return 0;
                        }
                        return pVK->wch[0];
                    }
                    pVK = (PVK_TO_WCHARS1)((PBYTE)pVK + pVKT->cbSize);
                }
            }
        }

    default:
        //UserSetLastError(ERROR_INVALID_PARAMETER);
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

    /*
    * Can't find translation, or wType was invalid
    */
    return 0;
}

//***************************************************************************
// UpdateRawKeyState
//
// A helper routine for ProcessKeyboardInput.
// Based on a VK and a make/break flag, this function will update the physical
// keystate table.
//
// History:
// 10-13-91 IanJa        Created.
//***************************************************************************
VOID
UpdateRawKeyState(
    BYTE Vk,
    BOOL fBreak
    )
{
    if (fBreak) {
        ClearRawKeyDown(Vk);
    }
    else {

        //
        // This is a key make.  If the key was not already down, update the
        // physical toggle bit.
        //
        if (!TestRawKeyDown(Vk)) {
            ToggleRawKeyToggle(Vk);
        }

        //
        // This is a make, so turn on the physical key down bit.
        //
        SetRawKeyDown(Vk);
    }
}

ULONG
GetControlKeyState(
    VOID
    )
{
    ULONG ControlKeyState = 0;

    if (TestRawKeyDown(VK_LMENU)) {
        ControlKeyState |= LEFT_ALT_PRESSED;
    }

    if (TestRawKeyDown(VK_RMENU)) {
        ControlKeyState |= RIGHT_ALT_PRESSED;
    }

    if (TestRawKeyDown(VK_LCONTROL)) {
        ControlKeyState |= LEFT_CTRL_PRESSED;
    }

    if (TestRawKeyDown(VK_RCONTROL)) {
        ControlKeyState |= RIGHT_CTRL_PRESSED;
    }

    if (TestRawKeyDown(VK_SHIFT)) {
        ControlKeyState |= SHIFT_PRESSED;
    }

    //
    // TODO: Some bits are missing.
    //

    return ControlKeyState;
}

#pragma region API Request Implementations

UINT ApiMapVirtualKey(
    _In_ UINT wCode,
    _In_ UINT wMapType)
{  

    UINT retval;

    retval = (DWORD)InternalMapVirtualKeyEx(wCode,
                                            wMapType,
                                            pKbdTbl);

    return retval;
} 

SHORT ApiVkKeyScan(
    _In_ WCHAR cChar)
{
    WCHAR wChar;
    SHORT retval;

    wChar = cChar;

    retval = (DWORD)InternalVkKeyScanEx(wChar,
                                        pKbdTbl);

    return retval;
}

/***************************************************************************\
* _GetKeyState (API)
*
* This API returns the up/down and toggle state of the specified VK based
* on the input synchronized keystate in the current queue.  The toggle state
* is mainly for 'state' keys like Caps-Lock that are toggled each time you
* press them.
*
* History:
* 11-11-90 DavidPe      Created.
\***************************************************************************/
SHORT ApiGetKeyState(
    _In_ int vk)
{
    UINT wKeyState;

    if ((UINT)vk >= CVKKEYSTATE) {
        return 0;
    }

    wKeyState = 0;

    /*
     * Set the toggle bit.
     */
    if (TestKeyStateToggle(vk))
        wKeyState = 0x0001;

    /*
     * Set the keyup/down bit.
     */
    if (TestKeyStateDown(vk)) {
        /*
         * Used to be wKeyState|= 0x8000.Fix for bug 28820; Ctrl-Enter
         * accelerator doesn't work on Nestscape Navigator Mail 2.0
         */
        wKeyState |= 0xff80;  // This is what 3.1 returned!!!!
    }

    return (SHORT)wKeyState;
}

#pragma endregion
