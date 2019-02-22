#pragma once

#include "KeyChord.g.h"

namespace winrt::TerminalControl::implementation
{
    struct KeyChord : KeyChordT<KeyChord>
    {
        KeyChord() = default;
        KeyChord(TerminalControl::KeyModifiers const& modifiers, int32_t vkey);
        KeyChord(bool ctrl, bool alt, bool shift, int32_t vkey);

        TerminalControl::KeyModifiers Modifiers();
        void Modifiers(TerminalControl::KeyModifiers const& value);
        int32_t Vkey();
        void Vkey(int32_t value);

    private:
        TerminalControl::KeyModifiers _modifiers;
        int32_t _vkey;
    };
}

namespace winrt::TerminalControl::factory_implementation
{
    struct KeyChord : KeyChordT<KeyChord, implementation::KeyChord>
    {
    };
}
