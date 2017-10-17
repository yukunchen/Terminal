/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "KeyEventHelpers.hpp"
#include "cmdline.h"
#include "../interactivity/inc/ServiceLocator.hpp"

// Routine Description:
// - checks if this key event is a special key for line editing
// Arguments:
// - keyEvent - the keyEvent to check
// Return Value:
// - true if this key has special relevance to line editing, false otherwise
bool IsCommandLineEditingKey(_In_ const KeyEvent& keyEvent)
{
    if (!IsAnyFlagSet(keyEvent._activeModifierKeys, ALT_PRESSED | CTRL_PRESSED))
    {
        switch (keyEvent.GetVirtualKeyCode())
        {
        case VK_ESCAPE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_INSERT:
        case VK_DELETE:
        case VK_F1:
        case VK_F2:
        case VK_F3:
        case VK_F4:
        case VK_F5:
        case VK_F6:
        case VK_F7:
        case VK_F8:
        case VK_F9:
            return true;
        default:
            break;
        }
    }
    if (IsAnyFlagSet(keyEvent._activeModifierKeys, CTRL_PRESSED))
    {
        switch (keyEvent.GetVirtualKeyCode())
        {
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_RIGHT:
            return true;
        default:
            break;
        }
    }

    // Extended edit key handling
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() && ::GetKeySubst(keyEvent))
    {
        // If wUnicodeChar is specified in KeySubst,
        // the key should be handled as a normal key.
        // Basically this is for VK_BACK keys.
        return (keyEvent._charData == 0);
    }

    if (IsAnyFlagSet(keyEvent._activeModifierKeys, ALT_PRESSED))
    {
        switch (keyEvent.GetVirtualKeyCode())
        {
        case VK_F7:
        case VK_F10:
            return true;
        default:
            break;
        }
    }
    return false;
}

// Routine Description:
// - checks if this key event is a special key for popups
// Arguments:
// - keyEvent - the keyEvent to check
// Return Value:
// - true if this key has special relevance to popups, false otherwise
bool IsCommandLinePopupKey(_In_ const KeyEvent& keyEvent)
{
    if (!IsAnyFlagSet(keyEvent._activeModifierKeys, ALT_PRESSED | CTRL_PRESSED))
    {
        switch (keyEvent.GetVirtualKeyCode())
        {
        case VK_ESCAPE:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_F2:
        case VK_F4:
        case VK_F7:
        case VK_F9:
            return true;
        default:
            break;
        }
    }

    // Extended key handling
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() && ::GetKeySubst(keyEvent))
    {
        return (keyEvent._charData == 0);
    }

    return false;
}

// Routine Description:
// - updates key event information based on the extended key
// substitution table.
// Arguments:
// - keyEvent - the keyEvent to update
// Return Value:
// - The extended key substitution object used to update the KeyEvent
// Note:
// - Modifies key event data.
const ExtKeySubst* const ParseEditKeyInfo(_Inout_ KeyEvent& keyEvent)
{
    const ExtKeySubst* const pKeySubst = ::GetKeySubst(keyEvent);

    if (pKeySubst)
    {
        // Substitute the input with ext key.
        keyEvent._activeModifierKeys = pKeySubst->wMod;
        keyEvent.SetVirtualKeyCode(pKeySubst->wVirKey);
        keyEvent._charData = pKeySubst->wUnicodeChar;
    }

    return pKeySubst;
}

// Routine Description:
// - Gets the extended key substitution object associated with the
// data in this KeyEvent, if applicable.
// Arguments:
// - keyEvent - the keyEvent to check
// Return Value:
// - The associated extended key substitution object or nullptr if one
// doesn't exist.
const ExtKeySubst* const GetKeySubst(_In_ const KeyEvent& keyEvent)
{
    // If not extended mode, or Control key or Alt key is not pressed, or virtual keycode is out of range, just bail.
    if (!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() ||
        (keyEvent._activeModifierKeys & (CTRL_PRESSED | ALT_PRESSED)) == 0 ||
        keyEvent.GetVirtualKeyCode() < 'A' || keyEvent.GetVirtualKeyCode() > 'Z')
    {
        return nullptr;
    }

    const ExtKeyDef* const pKeyDef = GetKeyDef(keyEvent.GetVirtualKeyCode());
    const ExtKeySubst* pKeySubst;

    // Get the KeySubst based on the modifier status.
    if (IsAnyFlagSet(keyEvent._activeModifierKeys, ALT_PRESSED))
    {
        if (IsAnyFlagSet(keyEvent._activeModifierKeys, CTRL_PRESSED))
        {
            pKeySubst = &pKeyDef->keys[2];
        }
        else
        {
            pKeySubst = &pKeyDef->keys[1];
        }
    }
    else
    {
        assert(IsAnyFlagSet(keyEvent._activeModifierKeys, CTRL_PRESSED));
        pKeySubst = &pKeyDef->keys[0];
    }

    // If the conbination is not defined, just bail.
    if (pKeySubst->wVirKey == 0)
    {
        return nullptr;
    }

    return pKeySubst;
}

// Routine Description:
// - checks if the key event's data is a pause key press.
// Arguments:
// - keyEvent - the keyEvent to check
// Return Value:
// - true if the key press data is a pause.
// Note:
// - The default key is Ctrl-S if extended edit keys are not specified.
bool IsPauseKey(_In_ const KeyEvent& keyEvent)
{
    bool isPauseKey = false;
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey())
    {
        const ExtKeySubst* const pKeySubst = ::GetKeySubst(keyEvent);
        isPauseKey = (pKeySubst != nullptr && pKeySubst->wVirKey == VK_PAUSE);
    }
    else
    {
        isPauseKey = (keyEvent.GetVirtualKeyCode() == L'S' && CTRL_BUT_NOT_ALT(keyEvent._activeModifierKeys));
    }
    return isPauseKey;
}
