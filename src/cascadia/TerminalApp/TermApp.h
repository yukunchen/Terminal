#pragma once

#include "TermApp.g.h"
#include <winrt/TerminalConnection.h>
#include <winrt/TerminalControl.h>


namespace winrt::TerminalApp::implementation
{
    struct TermApp : TermAppT<TermApp>
    {
        TermApp();

        Windows::UI::Xaml::UIElement GetRoot();

        ~TermApp();

    private:
        Windows::UI::Xaml::Controls::Grid _root;

        void _Create();
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct TermApp : TermAppT<TermApp, implementation::TermApp>
    {
    };
}
