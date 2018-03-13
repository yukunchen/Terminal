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
    if (!keyEvent.IsAltPressed() && !keyEvent.IsCtrlPressed())
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
    if (keyEvent.IsCtrlPressed())
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

    if (keyEvent.IsAltPressed())
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
    if (!keyEvent.IsAltPressed() && !keyEvent.IsCtrlPressed())
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

    return false;
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
    return keyEvent.GetVirtualKeyCode() == VK_PAUSE;
}
