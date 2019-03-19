#include "pch.h"
#include "AppKeyBindings.h"

// This is a helper macro for defining the body of events.
// Winrt events need a method for adding a callback to the event and removing
//      the callback. This macro will define them both for you, because they
//      don't really vary from event to event.
#define DEFINE_EVENT(class, name, eventHandler, args) \
    winrt::event_token class::name(const args& handler) { return eventHandler.add(handler); } \
    void class::name(const winrt::event_token& token) noexcept { eventHandler.remove(token); }

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    void AppKeyBindings::SetKeyBinding(TerminalApp::ShortcutAction const& action,
                                       Settings::KeyChord const& chord)
    {
        // TODO: MSFT:20814698 if another action is bound to that keybinding,
        //      remove it from the map
        _keyShortcuts[action] = chord;
    }

    bool AppKeyBindings::TryKeyChord(Settings::KeyChord const& kc)
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

            case ShortcutAction::NewTabProfile0:
                _NewTabWithProfileHandlers(0);
                return true;
            case ShortcutAction::NewTabProfile1:
                _NewTabWithProfileHandlers(1);
                return true;
            case ShortcutAction::NewTabProfile2:
                _NewTabWithProfileHandlers(2);
                return true;
            case ShortcutAction::NewTabProfile3:
                _NewTabWithProfileHandlers(3);
                return true;
            case ShortcutAction::NewTabProfile4:
                _NewTabWithProfileHandlers(4);
                return true;
            case ShortcutAction::NewTabProfile5:
                _NewTabWithProfileHandlers(5);
                return true;
            case ShortcutAction::NewTabProfile6:
                _NewTabWithProfileHandlers(6);
                return true;
            case ShortcutAction::NewTabProfile7:
                _NewTabWithProfileHandlers(7);
                return true;
            case ShortcutAction::NewTabProfile8:
                _NewTabWithProfileHandlers(8);
                return true;
            case ShortcutAction::NewTabProfile9:
                _NewTabWithProfileHandlers(9);
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

            case ShortcutAction::ScrollUp:
                _ScrollUpHandlers();
                return true;
            case ShortcutAction::ScrollDown:
                _ScrollDownHandlers();
                return true;

        }
        return false;
    }

    // -------------------------------- Events ---------------------------------
    DEFINE_EVENT(AppKeyBindings, CopyText,          _CopyTextHandlers,          TerminalApp::CopyTextEventArgs);
    DEFINE_EVENT(AppKeyBindings, PasteText,         _PasteTextHandlers,         TerminalApp::PasteTextEventArgs);
    DEFINE_EVENT(AppKeyBindings, NewTab,            _NewTabHandlers,            TerminalApp::NewTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, NewTabWithProfile, _NewTabWithProfileHandlers, TerminalApp::NewTabWithProfileEventArgs);
    DEFINE_EVENT(AppKeyBindings, NewWindow,         _NewWindowHandlers,         TerminalApp::NewWindowEventArgs);
    DEFINE_EVENT(AppKeyBindings, CloseWindow,       _CloseWindowHandlers,       TerminalApp::CloseWindowEventArgs);
    DEFINE_EVENT(AppKeyBindings, CloseTab,          _CloseTabHandlers,          TerminalApp::CloseTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, SwitchToTab,       _SwitchToTabHandlers,       TerminalApp::SwitchToTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, NextTab,           _NextTabHandlers,           TerminalApp::NextTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, PrevTab,           _PrevTabHandlers,           TerminalApp::PrevTabEventArgs);
    DEFINE_EVENT(AppKeyBindings, IncreaseFontSize,  _IncreaseFontSizeHandlers,  TerminalApp::IncreaseFontSizeEventArgs);
    DEFINE_EVENT(AppKeyBindings, DecreaseFontSize,  _DecreaseFontSizeHandlers,  TerminalApp::DecreaseFontSizeEventArgs);
    DEFINE_EVENT(AppKeyBindings, ScrollUp,          _ScrollUpHandlers,          TerminalApp::ScrollUpEventArgs);
    DEFINE_EVENT(AppKeyBindings, ScrollDown,        _ScrollDownHandlers,        TerminalApp::ScrollDownEventArgs);


}
