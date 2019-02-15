#include "pch.h"
#include "TermApp.h"
#include <argb.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;

namespace winrt::TerminalApp::implementation
{
    TermApp::TermApp() :
        _root{ nullptr }
    {
        _Create();
    }

    void TermApp::_Create()
    {
        Controls::Grid container;

        _root = container;
    }

    TermApp::~TermApp()
    {
    }

    UIElement TermApp::GetRoot()
    {
        return _root;
    }
}
