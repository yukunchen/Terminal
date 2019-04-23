//
// Declaration of the MainUserControl class.
//

#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"

#include "MainUserControl.g.h"

namespace winrt::TerminalApp::implementation
{
    struct MainUserControl : MainUserControlT<MainUserControl>
    {
        MainUserControl();

        hstring MyProperty();
        void MyProperty(hstring value);
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct MainUserControl : MainUserControlT<MainUserControl, implementation::MainUserControl>
    {
    };
}
