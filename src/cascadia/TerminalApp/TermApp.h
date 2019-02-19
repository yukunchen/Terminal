#pragma once

#include "TermApp.g.h"
#include <winrt/TerminalConnection.h>
#include <winrt/TerminalControl.h>
#include "Tab.h"

namespace winrt::TerminalApp::implementation
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
        TerminalApp::AppKeyBindings _keyBindings;
        std::vector<std::unique_ptr<Tab>> _tabs;

        void _Create();

        void _ResetTabs();
        void _CreateTabBar();
        void _FocusTab(Tab& tab);


        void _DoNewTab();
        void _DoCloseTab();
        // Todo: add more event implementations here
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct TermApp : TermAppT<TermApp, implementation::TermApp>
    {
    };
}
