/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "IInputEvent.hpp"

#include <unordered_map>

IInputEvent::~IInputEvent()
{
}

std::unique_ptr<IInputEvent> IInputEvent::Create(_In_ const INPUT_RECORD& record)
{
    switch (record.EventType)
    {
    case KEY_EVENT:
        return std::make_unique<KeyEvent>(record.Event.KeyEvent);
    case MOUSE_EVENT:
        return std::make_unique<MouseEvent>(record.Event.MouseEvent);
    case WINDOW_BUFFER_SIZE_EVENT:
        return std::make_unique<WindowBufferSizeEvent>(record.Event.WindowBufferSizeEvent);
    case MENU_EVENT:
        return std::make_unique<MenuEvent>(record.Event.MenuEvent);
    case FOCUS_EVENT:
        return std::make_unique<FocusEvent>(record.Event.FocusEvent);
    default:
        THROW_HR(E_INVALIDARG);
    }
}

std::deque<std::unique_ptr<IInputEvent>> IInputEvent::Create(_In_reads_(cRecords) const INPUT_RECORD* const pRecords,
                                                             _In_ const size_t cRecords)
{
    std::deque<std::unique_ptr<IInputEvent>> outEvents;
    for (size_t i = 0; i < cRecords; ++i)
    {
        outEvents.push_back(Create(pRecords[i]));
    }
    return outEvents;
}

// Routine Description:
// - checks if flag is present in flags
// Arguments:
// - flags - bit battern to check for flag
// - flag - bit pattern to search for
// Return Value:
// - true if flag is present in flags
// Note:
// - The wil version of IsFlagSet is only to operate on values that
// are compile time constants. This will work with runtime calculated
// values.
static bool RuntimeIsFlagSet(_In_ const DWORD flags, _In_ const DWORD flag)
{
    return !!(flags & flag);
}

// Routine Description:
// - Expands legacy control keys bitsets into a stl set
// Arguments:
// - flags - legacy bitset to expand
// Return Value:
// - set of ControlKeyState values that represent flags
std::unordered_set<ControlKeyState> ExpandControlKeyStateFlags(_In_ const DWORD flags)
{
    std::unordered_set<ControlKeyState> keyStates;

    static const std::unordered_map<DWORD, ControlKeyState> mapping =
    {
        { RIGHT_ALT_PRESSED, ControlKeyState::RightAlt },
        { LEFT_ALT_PRESSED, ControlKeyState::LeftAlt },
        { RIGHT_CTRL_PRESSED, ControlKeyState::RightCtrl },
        { LEFT_CTRL_PRESSED, ControlKeyState::LeftCtrl },
        { SHIFT_PRESSED, ControlKeyState::Shift },
        { NUMLOCK_ON, ControlKeyState::NumLock },
        { SCROLLLOCK_ON, ControlKeyState::ScrollLock },
        { CAPSLOCK_ON, ControlKeyState::CapsLock },
        { ENHANCED_KEY, ControlKeyState::EnhancedKey },
        { NLS_DBCSCHAR, ControlKeyState::NlsDbcsChar },
        { NLS_ALPHANUMERIC, ControlKeyState::NlsAlphanumeric },
        { NLS_KATAKANA, ControlKeyState::NlsKatakana },
        { NLS_HIRAGANA, ControlKeyState::NlsHiragana },
        { NLS_ROMAN, ControlKeyState::NlsRoman },
        { NLS_IME_CONVERSION, ControlKeyState::NlsImeConversion },
        { NLS_IME_DISABLE, ControlKeyState::NlsImeDisable }
    };

    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        if (RuntimeIsFlagSet(flags, it->first))
        {
            keyStates.insert(it->second);
        }
    }

    return keyStates;
}