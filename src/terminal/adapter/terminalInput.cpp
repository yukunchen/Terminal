/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <windows.h>
#include "terminalInput.hpp"

#include "strsafe.h"

#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>

#ifdef BUILD_ONECORE_INTERACTIVITY
#include "..\..\interactivity\inc\VtApiRedirection.hpp"
#endif

using namespace Microsoft::Console::VirtualTerminal;

DWORD const dwAltGrFlags = LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED;

TerminalInput::TerminalInput(_In_ std::function<void(INPUT_RECORD*,DWORD)> pfn)
{
    _pfnWriteEvents = pfn;
}

TerminalInput::~TerminalInput()
{

}

// See http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys
//    For the source for these tables.
// Also refer to the values in terminfo for kcub1, kcud1, kcuf1, kcuu1, kend, khome.
//   the 'xterm' setting lists the application mode versions of these sequences.
const TerminalInput::_TermKeyMap TerminalInput::s_rgCursorKeysNormalMapping[] 
{
    { VK_UP, L"\x1b[A" },
    { VK_DOWN, L"\x1b[B" },
    { VK_RIGHT, L"\x1b[C" },
    { VK_LEFT, L"\x1b[D" },
    { VK_HOME, L"\x1b[H" },
    { VK_END, L"\x1b[F" },
};

const TerminalInput::_TermKeyMap TerminalInput::s_rgCursorKeysApplicationMapping[] 
{
    { VK_UP, L"\x1bOA" },
    { VK_DOWN, L"\x1bOB" },
    { VK_RIGHT, L"\x1bOC" },
    { VK_LEFT, L"\x1bOD" },
    { VK_HOME, L"\x1bOH" },
    { VK_END, L"\x1bOF" },
};

const TerminalInput::_TermKeyMap TerminalInput::s_rgKeypadNumericMapping[] 
{
    // HEY YOU. UPDATE THE MAX LENGTH DEF WHEN YOU MAKE CHANGES HERE.
    { VK_BACK, L"\x7f"},
    { VK_PAUSE, L"\x1a" },
    { VK_ESCAPE, L"\x1b" },
    { VK_INSERT, L"\x1b[2~" },
    { VK_DELETE, L"\x1b[3~" },
    { VK_PRIOR, L"\x1b[5~" },
    { VK_NEXT, L"\x1b[6~" },
    { VK_F1, L"\x1bOP" }, // also \x1b[11~, PuTTY uses \x1b\x1b[A
    { VK_F2, L"\x1bOQ" }, // also \x1b[12~, PuTTY uses \x1b\x1b[B
    { VK_F3, L"\x1bOR" }, // also \x1b[13~, PuTTY uses \x1b\x1b[C
    { VK_F4, L"\x1bOS" }, // also \x1b[14~, PuTTY uses \x1b\x1b[D
    { VK_F5, L"\x1b[15~" },
    { VK_F6, L"\x1b[17~" },
    { VK_F7, L"\x1b[18~" },
    { VK_F8, L"\x1b[19~" },
    { VK_F9, L"\x1b[20~" },
    { VK_F10, L"\x1b[21~" },
    { VK_F11, L"\x1b[23~" },
    { VK_F12, L"\x1b[24~" },
};

//Application mode - Some terminals support both a "Numeric" input mode, and an "Application" mode
//  The standards vary on what each key translates to in the various modes, so I tried to make it as close 
//  to the VT220 standard as possible.
//  The notable difference is in the arrow keys, which in application mode translate to "^[0A" (etc) as opposed to "^[[A" in numeric
//Some very unclear documentation at http://invisible-island.net/xterm/ctlseqs/ctlseqs.html also suggests alternate encodings for F1-4
//  which I have left in the comments on those entries as something to possibly add in the future, if need be.
//It seems to me as though this was used for early numpad implementations, where presently numlock would enable 
//  "numeric" mode, outputting the numbers on the keys, while "application" mode does things like pgup/down, arrow keys, etc.
//These keys aren't translated at all in numeric mode, so I figured I'd leave them out of the numeric table.
const TerminalInput::_TermKeyMap TerminalInput::s_rgKeypadApplicationMapping[] 
{
    // HEY YOU. UPDATE THE MAX LENGTH DEF WHEN YOU MAKE CHANGES HERE.
    { VK_BACK, L"\x7f" },
    { VK_PAUSE, L"\x1a" },
    { VK_ESCAPE, L"\x1b" },
    { VK_INSERT, L"\x1b[2~" },
    { VK_DELETE, L"\x1b[3~" },
    { VK_PRIOR, L"\x1b[5~" },
    { VK_NEXT, L"\x1b[6~" },
    { VK_F1, L"\x1bOP" }, // also \x1b[11~, PuTTY uses \x1b\x1b[A
    { VK_F2, L"\x1bOQ" }, // also \x1b[12~, PuTTY uses \x1b\x1b[B
    { VK_F3, L"\x1bOR" }, // also \x1b[13~, PuTTY uses \x1b\x1b[C
    { VK_F4, L"\x1bOS" }, // also \x1b[14~, PuTTY uses \x1b\x1b[D
    { VK_F5, L"\x1b[15~" },
    { VK_F6, L"\x1b[17~" },
    { VK_F7, L"\x1b[18~" },
    { VK_F8, L"\x1b[19~" },
    { VK_F9, L"\x1b[20~" },
    { VK_F10, L"\x1b[21~" },
    { VK_F11, L"\x1b[23~" },
    { VK_F12, L"\x1b[24~" },
    // The numpad has a variety of mappings, none of which seem standard or really configurable by the OS.
    // See http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-PC-Style-Function-Keys 
    //   to see just how convoluted this all is.
    // PuTTY uses a set of mappings that don't work in ViM without reamapping them back to the numpad 
    // (see http://vim.wikia.com/wiki/PuTTY_numeric_keypad_mappings#Comments)
    // I think the best solution is to just not do any for the time being.
    // Putty also provides configuration for choosing which of the 5 mappings it has through the settings, which is more work than we can manage now. 
    // { VK_MULTIPLY, L"\x1bOj" },     // PuTTY: \x1bOR (I believe putty is treating the top row of the numpad as PF1-PF4)
    // { VK_ADD, L"\x1bOk" },          // PuTTY: \x1bOl, \x1bOm (with shift)
    // { VK_SEPARATOR, L"\x1bOl" },    // ? I'm not sure which key this is...
    // { VK_SUBTRACT, L"\x1bOm" },     // \x1bOS
    // { VK_DECIMAL, L"\x1bOn" },      // \x1bOn
    // { VK_DIVIDE, L"\x1bOo" },       // \x1bOQ
    // { VK_NUMPAD0, L"\x1bOp" },
    // { VK_NUMPAD1, L"\x1bOq" },
    // { VK_NUMPAD2, L"\x1bOr" },
    // { VK_NUMPAD3, L"\x1bOs" },
    // { VK_NUMPAD4, L"\x1bOt" },
    // { VK_NUMPAD5, L"\x1bOu" }, // \x1b0E
    // { VK_NUMPAD5, L"\x1bOE" }, // PuTTY \x1b[G
    // { VK_NUMPAD6, L"\x1bOv" },
    // { VK_NUMPAD7, L"\x1bOw" },
    // { VK_NUMPAD8, L"\x1bOx" },
    // { VK_NUMPAD9, L"\x1bOy" },
    // { '=', L"\x1bOX" },      // I've also seen these codes mentioned in some documentation,
    // { VK_SPACE, L"\x1bO " }, //  but I wasn't really sure if they should be included or not...
    // { VK_TAB, L"\x1bOI" },   // So I left them here as a reference just in case.
};

// Sequences to send when a modifier is pressed with any of these keys
// Basically, the 'm' will be replaced with a character indicating which modifier keys are pressed.
const TerminalInput::_TermKeyMap TerminalInput::s_rgModifierKeyMapping[] 
{
    // HEY YOU. UPDATE THE MAX LENGTH DEF WHEN YOU MAKE CHANGES HERE.
    { VK_UP, L"\x1b[1;mA" },
    { VK_DOWN, L"\x1b[1;mB" },
    { VK_RIGHT, L"\x1b[1;mC" },  
    { VK_LEFT, L"\x1b[1;mD" },   
    { VK_HOME, L"\x1b[1;mH" },   
    { VK_END, L"\x1b[1;mF" },   
    { VK_F1, L"\x1b[1;mP" },   
    { VK_F2, L"\x1b[1;mQ" },   
    { VK_F3, L"\x1b[1;mR" },   
    { VK_F4, L"\x1b[1;mS" },   
    { VK_F5, L"\x1b[15;m~" },   
    { VK_F6, L"\x1b[17;m~" },   
    { VK_F7, L"\x1b[18;m~" },   
    { VK_F8, L"\x1b[19;m~" },   
    { VK_F9, L"\x1b[20;m~" },   
    { VK_F10, L"\x1b[21;m~" },   
    { VK_F11, L"\x1b[23;m~" },   
    { VK_F12, L"\x1b[24;m~" },   
    // Ubuntu's inputrc also defines \x1b[5C, \x1b\x1bC (and D) as 'forward/backward-word' mappings
    // I believe '\x1b\x1bC' is listed because the C1 ESC (x9B) gets encoded as 
    //  \xC2\x9B, but then translated to \x1b\x1b if the C1 codepoint isn't supported by the current encoding 
};

// Do NOT include the null terminator in the count.
const size_t TerminalInput::_TermKeyMap::s_cchMaxSequenceLength = 7; // UPDATE THIS DEF WHEN THE LONGEST MAPPED STRING CHANGES

const size_t TerminalInput::s_cCursorKeysNormalMapping      = ARRAYSIZE(s_rgCursorKeysNormalMapping);
const size_t TerminalInput::s_cCursorKeysApplicationMapping = ARRAYSIZE(s_rgCursorKeysApplicationMapping);
const size_t TerminalInput::s_cKeypadNumericMapping         = ARRAYSIZE(s_rgKeypadNumericMapping);
const size_t TerminalInput::s_cKeypadApplicationMapping     = ARRAYSIZE(s_rgKeypadApplicationMapping);
const size_t TerminalInput::s_cModifierKeyMapping           = ARRAYSIZE(s_rgModifierKeyMapping);

void TerminalInput::ChangeKeypadMode(_In_ bool const fApplicationMode)
{
    _fKeypadApplicationMode = fApplicationMode;
}

void TerminalInput::ChangeCursorKeysMode(_In_ bool const fApplicationMode)
{
    _fCursorApplicationMode = fApplicationMode;
}

bool TerminalInput::s_IsShiftPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent)
{
    return IsFlagSet(pKeyEvent->dwControlKeyState, SHIFT_PRESSED);
}

bool TerminalInput::s_IsAltPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent)
{
    return IsAnyFlagSet(pKeyEvent->dwControlKeyState, LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED);
}

bool TerminalInput::s_IsCtrlPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent)
{
    return IsAnyFlagSet(pKeyEvent->dwControlKeyState, LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
}

bool TerminalInput::s_IsModifierPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent)
{
    return s_IsShiftPressed(pKeyEvent) || s_IsAltPressed(pKeyEvent) || s_IsCtrlPressed(pKeyEvent);
}

bool TerminalInput::s_IsCursorKey(_In_ const KEY_EVENT_RECORD* const pKeyEvent)
{
    // true iff vk in [End, Home, Left, Up, Right, Down]
    return (pKeyEvent->wVirtualKeyCode >= VK_END) && (pKeyEvent->wVirtualKeyCode <= VK_DOWN);
}

const size_t TerminalInput::GetKeyMappingLength(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const
{
    size_t length = 0;
    if (s_IsCursorKey(pKeyEvent))
    {
        length = (_fCursorApplicationMode) ? s_cCursorKeysApplicationMapping : s_cCursorKeysNormalMapping;
    }
    else
    {
        length = (_fKeypadApplicationMode) ? s_cKeypadApplicationMapping : s_cKeypadNumericMapping;
    }
    return length;
}

const TerminalInput::_TermKeyMap* TerminalInput::GetKeyMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const
{
    const TerminalInput::_TermKeyMap* mapping = nullptr;

    if (s_IsCursorKey(pKeyEvent))
    {
        mapping = (_fCursorApplicationMode) ? s_rgCursorKeysApplicationMapping : s_rgCursorKeysNormalMapping;
    }
    else
    {
        mapping = (_fKeypadApplicationMode) ? s_rgKeypadApplicationMapping : s_rgKeypadNumericMapping;
    }
    return mapping;
}

// Routine Description:
// - Searches the s_ModifierKeyMapping for a entry corresponding to this key event. 
//      Changes the second to last byte to correspond to the currently pressed modifier keys
//      before sending to the input.
// Arguments:
// - pKeyEvent - Key event to translate
// Return Value:
// - True if there was a match to a key translation, and we successfully modified and sent it to the input
bool TerminalInput::_SearchWithModifier(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const
{

    const TerminalInput::_TermKeyMap* pMatchingMapping;
    bool fSuccess = _SearchKeyMapping(pKeyEvent,
                                      s_rgModifierKeyMapping,
                                      s_cModifierKeyMapping,
                                      &pMatchingMapping);
    if (fSuccess)
    {
        size_t cch = 0;
        if (SUCCEEDED(StringCchLengthW(pMatchingMapping->pwszSequence, _TermKeyMap::s_cchMaxSequenceLength + 1, &cch)) && cch > 0)
        {
            wchar_t* rwchModifiedSequence = new wchar_t[cch+1];
            if (rwchModifiedSequence != nullptr)
            {
                memcpy(rwchModifiedSequence, pMatchingMapping->pwszSequence, cch * sizeof(wchar_t));
                const bool fShift = s_IsShiftPressed(pKeyEvent);
                const bool fAlt = s_IsAltPressed(pKeyEvent);
                const bool fCtrl = s_IsCtrlPressed(pKeyEvent);
                rwchModifiedSequence[cch-2] = L'1' + (fShift? 1 : 0) + (fAlt? 2 : 0) + (fCtrl? 4 : 0);
                rwchModifiedSequence[cch] = 0;
                _SendInputSequence(rwchModifiedSequence);
                fSuccess = true;
                delete [] rwchModifiedSequence;
            }
        } 
    }
    return fSuccess;
}

// Routine Description:
// - Searches the keyMapping for a entry corresponding to this key event, and returns it.
// Arguments:
// - pKeyEvent - Key event to translate
// - keyMapping - Array of key mappings to search
// - cKeyMapping - number of entries in keyMapping
// - pMatchingMapping - Where to put the pointer to the found match
// Return Value:
// - True if there was a match to a key translation
bool TerminalInput::_SearchKeyMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent,
                                      _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                      _In_ size_t const cKeyMapping,
                                      _Out_ const TerminalInput::_TermKeyMap** pMatchingMapping) const
{
    bool fKeyTranslated = false;
    for (size_t i = 0; i < cKeyMapping; i++)
    {
        const _TermKeyMap* const pMap = &(keyMapping[i]);

        if (pMap->wVirtualKey == pKeyEvent->wVirtualKeyCode)
        {
            fKeyTranslated = true;
            *pMatchingMapping = pMap;
            break;
        }
    }
    return fKeyTranslated;
}

// Routine Description:
// - Searches the input array of mappings, and sends it to the input if a match was found.
// Arguments:
// - pKeyEvent - Key event to translate
// - keyMapping - Array of key mappings to search
// - cKeyMapping - number of entries in keyMapping
// Return Value:
// - True if there was a match to a key translation, and we successfully sent it to the input
bool TerminalInput::_TranslateDefaultMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent,
                                             _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                             _In_ size_t const cKeyMapping) const
{
    const TerminalInput::_TermKeyMap* pMatchingMapping;
    bool fSuccess = _SearchKeyMapping(pKeyEvent, keyMapping, cKeyMapping, &pMatchingMapping);
    if (fSuccess)
    {
        _SendInputSequence(pMatchingMapping->pwszSequence);
        fSuccess = true;
    }
    return fSuccess;
}

bool TerminalInput::HandleKey(_In_ const INPUT_RECORD* const pInput) const
{
    // By default, we fail to handle the key
    bool fKeyHandled = false;

    // On key presses, prepare to translate to VT compatible sequences
    if (pInput->EventType == KEY_EVENT)
    {
        KEY_EVENT_RECORD key = pInput->Event.KeyEvent;

        // Only need to handle key down. See raw key handler (see RawReadWaitRoutine in stream.cpp)
        if (key.bKeyDown == TRUE)
        {
            // For AltGr enabled keyboards, the Windows system will emit Left Ctrl + Right Alt as the modifier keys and 
            // will have pretranslated the UnicodeChar to the proper alternative value.
            // Through testing with Ubuntu, PuTTY, and Emacs for Windows, it was discovered that any instance of Left Ctrl + Right Alt
            // will strip out those two modifiers and send the unicode value straight through to the system.
            // Holding additional modifiers in addition to Left Ctrl + Right Alt will then light those modifiers up again for the unicode value.
            // Therefore to handle AltGr properly, our first step needs to be to check if both Left Ctrl + Right Alt are pressed...
            // ... and if they are both pressed, strip them out of the control key state.
            bool fAltGRPressed = (key.dwControlKeyState & dwAltGrFlags) == dwAltGrFlags;
            if (fAltGRPressed)
            {
                key.dwControlKeyState &= ~(dwAltGrFlags);
            }

            if (s_IsAltPressed(&key) && s_IsCtrlPressed(&key)
                && (key.uChar.UnicodeChar == 0 || key.uChar.UnicodeChar == 0x20)
                && ((key.wVirtualKeyCode > 0x40 && key.wVirtualKeyCode <= 0x5A) || key.wVirtualKeyCode == VK_SPACE) )
            {
                // For Alt+Ctrl+Key messages, the UnicodeChar is NOT the Ctrl+key char, it's null.
                //      So we need to get the char from the vKey.
                //      EXCEPT for Alt+Ctrl+Space. Then the UnicodeChar is space, not NUL.
                wchar_t wchPressedChar = (wchar_t) MapVirtualKeyW(key.wVirtualKeyCode, MAPVK_VK_TO_CHAR);
                // This is a trick - C-Spc is supposed to send NUL. So quick change space -> @ (0x40)
                wchPressedChar = (wchPressedChar == 0x20)? 0x40 : wchPressedChar;
                if (wchPressedChar >= 0x40 && wchPressedChar < 0x7F) 
                {
                    //shift the char to the ctrl range
                    wchPressedChar -= 0x40;
                    _SendEscapedInputSequence(wchPressedChar);
                    fKeyHandled = true;
                }
            }
            // ALT is a sequence of ESC + KEY.
            else if (key.uChar.UnicodeChar != 0 && s_IsAltPressed(&key))
            {
                _SendEscapedInputSequence(key.uChar.UnicodeChar);
                fKeyHandled = true;
            } 
            else if (s_IsCtrlPressed(&key))
            {
                if ((key.uChar.UnicodeChar == L' ' ) || // Ctrl+Space
                     // when Ctrl+@ comes through, the unicodechar will be '\x0', and the vkey will be VkKeyScanW(0), the vkey for null
                     (key.uChar.UnicodeChar == L'\x0' && key.wVirtualKeyCode == LOBYTE(VkKeyScanW(0)))) 
                {
                    _SendNullInputSequence(key.dwControlKeyState);
                    fKeyHandled = true;
                }   
            }

            // If a modifier key was pressed, then we need to try and send the modified sequence.
            if (!fKeyHandled && s_IsModifierPressed(&key))
            {
                // Translate the key using the modifier table
                fKeyHandled = _SearchWithModifier(&key);
            }
             
            if (!fKeyHandled)
            {
                // For perf optimization, filter out any typically printable Virtual Keys (e.g. A-Z)
                // This is in lieu of an O(1) sparse table or other such less-maintanable methods.
                // VK_CANCEL is an exception and we want to send the associated uChar as is.
                if ((key.wVirtualKeyCode < '0' || key.wVirtualKeyCode > 'Z') && key.wVirtualKeyCode != VK_CANCEL)
                {
                    fKeyHandled = _TranslateDefaultMapping(&key, GetKeyMapping(&key), GetKeyMappingLength(&key));
                }
                else
                {
                    WCHAR rgwchSequence[2];
                    rgwchSequence[0] = key.uChar.UnicodeChar;
                    rgwchSequence[1] = L'\0';
                    _SendInputSequence(rgwchSequence);
                    fKeyHandled = true;
                }
            }
        }
    }

    return fKeyHandled;
}

// Routine Description:
// - Sends the given char as a sequence representing Alt+wch, also the same as
//      Meta+wch.
// Arguments:
// - wch - character to send to input paired with Esc
// Return Value:
// - None
void TerminalInput::_SendEscapedInputSequence(_In_ const wchar_t wch) const
{
    INPUT_RECORD rgInput[2];
    rgInput[0].EventType = KEY_EVENT;
    rgInput[0].Event.KeyEvent.bKeyDown = TRUE;
    rgInput[0].Event.KeyEvent.dwControlKeyState = 0;
    rgInput[0].Event.KeyEvent.wRepeatCount = 1;
    rgInput[0].Event.KeyEvent.wVirtualKeyCode = 0;
    rgInput[0].Event.KeyEvent.wVirtualScanCode = 0;
    rgInput[0].Event.KeyEvent.uChar.UnicodeChar = L'\x1b';

    rgInput[1].EventType = KEY_EVENT;
    rgInput[1].Event.KeyEvent.bKeyDown = TRUE;
    rgInput[1].Event.KeyEvent.dwControlKeyState = 0;
    rgInput[1].Event.KeyEvent.wRepeatCount = 1;
    rgInput[1].Event.KeyEvent.wVirtualKeyCode = 0;
    rgInput[1].Event.KeyEvent.wVirtualScanCode = 0;
    rgInput[1].Event.KeyEvent.uChar.UnicodeChar = wch;
    
    _pfnWriteEvents(rgInput, 2);
}

void TerminalInput::_SendNullInputSequence(_In_ DWORD const dwControlKeyState) const
{
    INPUT_RECORD irInput;
    irInput.EventType = KEY_EVENT;
    irInput.Event.KeyEvent.bKeyDown = TRUE;
    irInput.Event.KeyEvent.dwControlKeyState = dwControlKeyState;
    irInput.Event.KeyEvent.wRepeatCount = 1;
    irInput.Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKeyScanW(0));
    irInput.Event.KeyEvent.wVirtualScanCode = 0;
    irInput.Event.KeyEvent.uChar.UnicodeChar = L'\x0';
    
    _pfnWriteEvents(&irInput, 1);
}

void TerminalInput::_SendInputSequence(_In_ PCWSTR const pwszSequence) const
{
    size_t cch = 0;
    // + 1 to max sequence length for null terminator count which is required by StringCchLengthW
    if (SUCCEEDED(StringCchLengthW(pwszSequence, _TermKeyMap::s_cchMaxSequenceLength + 1, &cch)) && cch > 0)
    {        
        INPUT_RECORD rgInput[_TermKeyMap::s_cchMaxSequenceLength];

        for (size_t i = 0; i < cch; i++)
        {
            rgInput[i].EventType = KEY_EVENT;
            rgInput[i].Event.KeyEvent.bKeyDown = TRUE;
            rgInput[i].Event.KeyEvent.dwControlKeyState = 0;
            rgInput[i].Event.KeyEvent.wRepeatCount = 1;
            rgInput[i].Event.KeyEvent.wVirtualKeyCode = 0;
            rgInput[i].Event.KeyEvent.wVirtualScanCode = 0;

            rgInput[i].Event.KeyEvent.uChar.UnicodeChar = pwszSequence[i];
        }
        //This cast is safe because we know that _TermKeyMap::s_cchMaxSequenceLength + 1 < MAX_DWORD
        _pfnWriteEvents(rgInput, (DWORD)cch);
    }
}   
