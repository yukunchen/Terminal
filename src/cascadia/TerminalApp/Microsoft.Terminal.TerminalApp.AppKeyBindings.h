#pragma once

#include "AppKeyBindings.g.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    struct AppKeyBindings : AppKeyBindingsT<AppKeyBindings>
    {
        AppKeyBindings() = default;

        bool TryKeyChord(Microsoft::Terminal::TerminalControl::KeyChord const& kc);
        void SetKeyBinding(TerminalApp::ShortcutAction const& action, Microsoft::Terminal::TerminalControl::KeyChord const& chord);

        winrt::event_token CopyText(TerminalApp::CopyTextEventArgs const& handler);
        void CopyText(winrt::event_token const& token) noexcept;
        winrt::event_token PasteText(TerminalApp::PasteTextEventArgs const& handler);
        void PasteText(winrt::event_token const& token) noexcept;
        winrt::event_token NewTab(TerminalApp::NewTabEventArgs const& handler);
        void NewTab(winrt::event_token const& token) noexcept;
        winrt::event_token NewWindow(TerminalApp::NewWindowEventArgs const& handler);
        void NewWindow(winrt::event_token const& token) noexcept;
        winrt::event_token CloseWindow(TerminalApp::CloseWindowEventArgs const& handler);
        void CloseWindow(winrt::event_token const& token) noexcept;
        winrt::event_token CloseTab(TerminalApp::CloseTabEventArgs const& handler);
        void CloseTab(winrt::event_token const& token) noexcept;
        winrt::event_token SwitchToTab(TerminalApp::SwitchToTabEventArgs const& handler);
        void SwitchToTab(winrt::event_token const& token) noexcept;
        winrt::event_token NextTab(TerminalApp::NextTabEventArgs const& handler);
        void NextTab(winrt::event_token const& token) noexcept;
        winrt::event_token PrevTab(TerminalApp::PrevTabEventArgs const& handler);
        void PrevTab(winrt::event_token const& token) noexcept;
        winrt::event_token IncreaseFontSize(TerminalApp::IncreaseFontSizeEventArgs const& handler);
        void IncreaseFontSize(winrt::event_token const& token) noexcept;
        winrt::event_token DecreaseFontSize(TerminalApp::DecreaseFontSizeEventArgs const& handler);
        void DecreaseFontSize(winrt::event_token const& token) noexcept;

    private:
        // This intuitively seems backwards, ~~on purpose~~.
        // We need to have at most one key chord bound to a single action,
        // so we're mapping action->keychord.
        // We could theoretically do it the other way around too.
        //      In both cases, we'll have to remove duplicate values from the map,
        //      so maybe doing it the other way around wouldn't be bad, and then
        //      we'd have an O(1) lookup, O(N) insert/delete
        //      instead of O(N) for lookup, insert and delete
        // TODO ^
        std::map<TerminalApp::ShortcutAction, Microsoft::Terminal::TerminalControl::KeyChord> _keyShortcuts;
        bool _DoAction(ShortcutAction action);

        winrt::event<TerminalApp::CopyTextEventArgs> _copyTextHandlers;
        winrt::event<TerminalApp::PasteTextEventArgs> _pasteTextHandlers;
        winrt::event<TerminalApp::NewTabEventArgs> _newTabHandlers;
        winrt::event<TerminalApp::NewWindowEventArgs> _newWindowHandlers;
        winrt::event<TerminalApp::CloseWindowEventArgs> _closeWindowHandlers;
        winrt::event<TerminalApp::CloseTabEventArgs> _closeTabHandlers;
        winrt::event<TerminalApp::SwitchToTabEventArgs> _switchToTabHandlers;
        winrt::event<TerminalApp::NextTabEventArgs> _nextTabHandlers;
        winrt::event<TerminalApp::PrevTabEventArgs> _prevTabHandlers;
        winrt::event<TerminalApp::IncreaseFontSizeEventArgs> _increaseFontSizeHandlers;
        winrt::event<TerminalApp::DecreaseFontSizeEventArgs> _decreaseFontSizeHandlers;

    };
}

namespace winrt::Microsoft::Terminal::TerminalApp::factory_implementation
{
    struct AppKeyBindings : AppKeyBindingsT<AppKeyBindings, implementation::AppKeyBindings>
    {
    };
}
