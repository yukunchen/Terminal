#include "pch.h"
#include "Microsoft.Terminal.TerminalApp.AppKeyBindings.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    void AppKeyBindings::SetKeyBinding(TerminalApp::ShortcutAction const& action,
                                       Microsoft::Terminal::TerminalControl::KeyChord const& chord)
    {
        // TODO: if another action is bound to that keybinding,
        //      remove it from the map
        _keyShortcuts[action] = chord;
    }

    bool AppKeyBindings::TryKeyChord(Microsoft::Terminal::TerminalControl::KeyChord const& kc)
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
                _copyTextHandlers();
                return true;
            case ShortcutAction::PasteText:
                _pasteTextHandlers();
                return true;
            case ShortcutAction::NewTab:
                _newTabHandlers();
                return true;
            case ShortcutAction::NewWindow:
                _newWindowHandlers();
                return true;
            case ShortcutAction::CloseWindow:
                _closeWindowHandlers();
                return true;
            case ShortcutAction::CloseTab:
                _closeTabHandlers();
                return true;
        }
        return false;
    }
    // -------------------------------- Events ---------------------------------
    winrt::event_token AppKeyBindings::CopyText(TerminalApp::CopyTextEventArgs const& handler)
    {
        return _copyTextHandlers.add(handler);
    }

    void AppKeyBindings::CopyText(winrt::event_token const& token) noexcept
    {
        _copyTextHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::PasteText(TerminalApp::PasteTextEventArgs const& handler)
    {
        return _pasteTextHandlers.add(handler);
    }

    void AppKeyBindings::PasteText(winrt::event_token const& token) noexcept
    {
        _pasteTextHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::NewTab(TerminalApp::NewTabEventArgs const& handler)
    {
        return _newTabHandlers.add(handler);
    }

    void AppKeyBindings::NewTab(winrt::event_token const& token) noexcept
    {
        _newTabHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::NewWindow(TerminalApp::NewWindowEventArgs const& handler)
    {
        return _newWindowHandlers.add(handler);
    }

    void AppKeyBindings::NewWindow(winrt::event_token const& token) noexcept
    {
        _newWindowHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::CloseWindow(TerminalApp::CloseWindowEventArgs const& handler)
    {
        return _closeWindowHandlers.add(handler);
    }

    void AppKeyBindings::CloseWindow(winrt::event_token const& token) noexcept
    {
        _closeWindowHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::CloseTab(TerminalApp::CloseTabEventArgs const& handler)
    {
        return _closeTabHandlers.add(handler);
    }

    void AppKeyBindings::CloseTab(winrt::event_token const& token) noexcept
    {
        _closeTabHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::SwitchToTab(TerminalApp::SwitchToTabEventArgs const& handler)
    {
        return _switchToTabHandlers.add(handler);
    }

    void AppKeyBindings::SwitchToTab(winrt::event_token const& token) noexcept
    {
        _switchToTabHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::NextTab(TerminalApp::NextTabEventArgs const& handler)
    {
        return _nextTabHandlers.add(handler);
    }

    void AppKeyBindings::NextTab(winrt::event_token const& token) noexcept
    {
        _nextTabHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::PrevTab(TerminalApp::PrevTabEventArgs const& handler)
    {
        return _prevTabHandlers.add(handler);
    }

    void AppKeyBindings::PrevTab(winrt::event_token const& token) noexcept
    {
        _prevTabHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::IncreaseFontSize(TerminalApp::IncreaseFontSizeEventArgs const& handler)
    {
        return _increaseFontSizeHandlers.add(handler);
    }

    void AppKeyBindings::IncreaseFontSize(winrt::event_token const& token) noexcept
    {
        _increaseFontSizeHandlers.remove(token);
    }

    winrt::event_token AppKeyBindings::DecreaseFontSize(TerminalApp::DecreaseFontSizeEventArgs const& handler)
    {
        return _decreaseFontSizeHandlers.add(handler);
    }

    void AppKeyBindings::DecreaseFontSize(winrt::event_token const& token) noexcept
    {
        _decreaseFontSizeHandlers.remove(token);
    }


}
