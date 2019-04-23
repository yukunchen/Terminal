#include "pch.h"
#include "MainUserControl.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::TerminalApp::implementation
{
    MainUserControl::MainUserControl()
    {
        InitializeComponent();
    }

    winrt::hstring MainUserControl::MyProperty()
    {
        return { L"foo" };
        // return userControl().MyProperty();
    }

    void MainUserControl::MyProperty(winrt::hstring /*value*/)
    {

        // userControl().MyProperty(value);
    }
}
