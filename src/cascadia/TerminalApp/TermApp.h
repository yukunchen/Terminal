#pragma once

#include "TermApp.g.h"
#include <winrt/Microsoft.Terminal.TerminalConnection.h>
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "Tab.h"
#include "CascadiaSettings.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    struct TermApp : TermAppT<TermApp>
    {
        TermApp();

        Windows::UI::Xaml::UIElement GetRoot();

        ~TermApp();

    private:
        Windows::UI::Xaml::Controls::Grid _root;
        Windows::UI::Xaml::Controls::StackPanel _tabBar;
        Windows::UI::Xaml::Controls::Grid _tabContent;
        std::vector<std::unique_ptr<Tab>> _tabs;

        ::Microsoft::Terminal::TerminalApp::CascadiaSettings _settings;

        void _Create();

        void _LoadSettings();

        void _ResetTabs();
        void _CreateTabBar();
        void _FocusTab(Tab& tab);
        void _CreateNewTabFromSettings(winrt::Microsoft::Terminal::TerminalControl::TerminalSettings settings);


        void _DoNewTab(std::optional<int> profileIndex);
        void _DoCloseTab();
        // Todo: add more event implementations here
    };
}

namespace winrt::Microsoft::Terminal::TerminalApp::factory_implementation
{
    struct TermApp : TermAppT<TermApp, implementation::TermApp>
    {
    };
}
