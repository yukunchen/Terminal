#include "pch.h"
#include "Microsoft.Terminal.TerminalControl.KeyChord.h"

namespace winrt::Microsoft::Terminal::TerminalControl::implementation
{
    KeyChord::KeyChord(bool ctrl, bool alt, bool shift, int32_t vkey) :
        _modifiers{ (ctrl ? TerminalControl::KeyModifiers::Ctrl : TerminalControl::KeyModifiers::None) |
                    (alt ? TerminalControl::KeyModifiers::Alt : TerminalControl::KeyModifiers::None) |
                    (shift ? TerminalControl::KeyModifiers::Shift : TerminalControl::KeyModifiers::None) },
        _vkey{ vkey }
    {
    }

    KeyChord::KeyChord(TerminalControl::KeyModifiers const& modifiers, int32_t vkey) :
        _modifiers{ modifiers },
        _vkey{ vkey }
    {
    }

    TerminalControl::KeyModifiers KeyChord::Modifiers()
    {
        return _modifiers;
    }

    void KeyChord::Modifiers(TerminalControl::KeyModifiers const& value)
    {
        _modifiers = value;
    }

    int32_t KeyChord::Vkey()
    {
        return _vkey;
    }

    void KeyChord::Vkey(int32_t value)
    {
        _vkey = value;
    }
}
