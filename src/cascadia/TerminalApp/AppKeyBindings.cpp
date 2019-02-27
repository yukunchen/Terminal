#include "pch.h"
#include "AppKeyBindings.h"

#define DEFINE_EVENT(class, name, eventHandler, args) \
    winrt::event_token class::name(args const& handler) { return eventHandler.add(handler); } \
    void class::name(winrt::event_token const& token) noexcept { eventHandler.remove(token); }

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    void AppKeyBindings::SetKeyBinding(TerminalApp::ShortcutAction const& action,
                                       TerminalControl::KeyChord const& chord)
    {
        // TODO: if another action is bound to that keybinding,
        //      remove it from the map
        _keyShortcuts[action] = chord;
    }

    bool AppKeyBindings::TryKeyChord(TerminalControl::KeyChord const& kc)
    {
        for (auto kv : _keyShortcuts)
        {
            auto k = kv.first;
            auto v = kv.second;
            if (v != nullptr &&
                (v.Modifiers() == kc.Modifiers() && v.Vkey() == kc.Vkey()))
            {
                bool handled = _DoAction(k);
                if (handled)
                {
                    return handled;
                }
            }
        }
        return false;
    }
    bool AppKeyBindings::_DoAction(ShortcutAction action)
    {
        switch (action)
        {
            case ShortcutAction::CopyText:
                _CopyTextHandlers();
                return true;
            case ShortcutAction::PasteText:
                _PasteTextHandlers();
                return true;
            case ShortcutAction::NewTab:
                _NewTabHandlers();
                return true;
            case ShortcutAction::NewWindow:
                _NewWindowHandlers();
                return true;
            case ShortcutAction::CloseWindow:
                _CloseWindowHandlers();
                return true;
            case ShortcutAction::CloseTab:
                _CloseTabHandlers();
                return true;
        }
        return false;
    }

    // -------------------------------- Events ---------------------------------
    DEFINE_EVENT(AppKeyBindings, CopyText,         _CopyTextHandlers,         TerminalApp::CopyTextEventArgs);
    DEFINE_EVENT(AppKeyBindings, PasteText,        _PasteTextHandlers,        TerminalApp::PasteTextEventArgs);
    DEFINE_EVENT(AppKeyBindings, NewTab,           _NewTabHandlers,           TerminalApp::NewTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, NewWindow,        _NewWindowHandlers,        TerminalApp::NewWindowEventArgs);
    DEFINE_EVENT(AppKeyBindings, CloseWindow,      _CloseWindowHandlers,      TerminalApp::CloseWindowEventArgs);
    DEFINE_EVENT(AppKeyBindings, CloseTab,         _CloseTabHandlers,         TerminalApp::CloseTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, SwitchToTab,      _SwitchToTabHandlers,      TerminalApp::SwitchToTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, NextTab,          _NextTabHandlers,          TerminalApp::NextTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, PrevTab,          _PrevTabHandlers,          TerminalApp::PrevTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, IncreaseFontSize, _IncreaseFontSizeHandlers, TerminalApp::IncreaseFontSizeEventArgs);
    DEFINE_EVENT(AppKeyBindings, DecreaseFontSize, _DecreaseFontSizeHandlers, TerminalApp::DecreaseFontSizeEventArgs);


}
