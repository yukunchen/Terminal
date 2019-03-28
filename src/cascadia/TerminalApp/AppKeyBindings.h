#pragma once

#include "AppKeyBindings.g.h"

// This is a helper macro to make declaring events easier.
// This will declare the event handler and the methods for adding and removing a
// handler callback from the event
#define DECLARE_EVENT(name, eventHandler, args) \
    public: \
    winrt::event_token name(const args& handler); \
    void name(const winrt::event_token& token) noexcept; \
    private: \
    winrt::event<args> eventHandler;


namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    struct KeyChordHash
    {
        std::size_t operator()(const Settings::KeyChord& key) const
        {
            std::hash<int32_t> keyHash;
            std::hash<Settings::KeyModifiers> modifiersHash;
            std::size_t hashedKey = keyHash(key.Vkey());
            std::size_t hashedMods = modifiersHash(key.Modifiers());
            return hashedKey ^ hashedMods;
        }
    };

    struct KeyChordEquality
    {
        bool operator()(const Settings::KeyChord& lhs, const Settings::KeyChord& rhs) const
        {
            return lhs.Modifiers() == rhs.Modifiers() && lhs.Vkey() == rhs.Vkey();
        }
    };

    struct AppKeyBindings : AppKeyBindingsT<AppKeyBindings>
    {
        AppKeyBindings() = default;

        bool TryKeyChord(Settings::KeyChord const& kc);
        void SetKeyBinding(TerminalApp::ShortcutAction const& action, Settings::KeyChord const& chord);

        DECLARE_EVENT(CopyText,          _CopyTextHandlers,          TerminalApp::CopyTextEventArgs);
        DECLARE_EVENT(PasteText,         _PasteTextHandlers,         TerminalApp::PasteTextEventArgs);
        DECLARE_EVENT(NewTab,            _NewTabHandlers,            TerminalApp::NewTabEventArgs);
        DECLARE_EVENT(NewTabWithProfile, _NewTabWithProfileHandlers, TerminalApp::NewTabWithProfileEventArgs);
        DECLARE_EVENT(NewWindow,         _NewWindowHandlers,         TerminalApp::NewWindowEventArgs);
        DECLARE_EVENT(CloseWindow,       _CloseWindowHandlers,       TerminalApp::CloseWindowEventArgs);
        DECLARE_EVENT(CloseTab,          _CloseTabHandlers,          TerminalApp::CloseTabEventArgs);
        DECLARE_EVENT(SwitchToTab,       _SwitchToTabHandlers,       TerminalApp::SwitchToTabEventArgs);
        DECLARE_EVENT(NextTab,           _NextTabHandlers,           TerminalApp::NextTabEventArgs);
        DECLARE_EVENT(PrevTab,           _PrevTabHandlers,           TerminalApp::PrevTabEventArgs);
        DECLARE_EVENT(IncreaseFontSize,  _IncreaseFontSizeHandlers,  TerminalApp::IncreaseFontSizeEventArgs);
        DECLARE_EVENT(DecreaseFontSize,  _DecreaseFontSizeHandlers,  TerminalApp::DecreaseFontSizeEventArgs);
        DECLARE_EVENT(ScrollUp,          _ScrollUpHandlers,          TerminalApp::ScrollUpEventArgs);
        DECLARE_EVENT(ScrollDown,        _ScrollDownHandlers,        TerminalApp::ScrollDownEventArgs);

    private:
        std::unordered_map<Settings::KeyChord, TerminalApp::ShortcutAction, KeyChordHash, KeyChordEquality> _keyShortcuts;
        bool _DoAction(ShortcutAction action);

    };
}

namespace winrt::Microsoft::Terminal::TerminalApp::factory_implementation
{
    struct AppKeyBindings : AppKeyBindingsT<AppKeyBindings, implementation::AppKeyBindings>
    {
    };
}
