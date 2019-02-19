#include "pch.h"
#include "TermApp.h"
#include <argb.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::TerminalControl;

namespace winrt::TerminalApp::implementation
{
    TermApp::TermApp() :
        _root{ nullptr },
        _tabBar{ nullptr },
        _tabContent{ nullptr },
        _keyBindings{  },
        _tabs{  }
    {
        _Create();

        // Set up spme basic defailt keybindings
        // TODO: read our settings from some source, and configure
        //      keychord,action pairings from that file
        _keyBindings.SetKeyBinding(ShortcutAction::NewTab,
                                   KeyChord{ KeyModifiers::Ctrl, (int)'T' });

        _keyBindings.SetKeyBinding(ShortcutAction::CloseTab,
                                   KeyChord{ KeyModifiers::Ctrl, (int)'W' });

        // Hook up the KeyBinding object's events to our handlers.
        // TODO: as we implement more events, hook them up here.
        // They should all be hooked up here, regardless of whether or not
        //      there's an actual keychord for them.
        _keyBindings.NewTab([&]() { this->_DoNewTab(); });
        _keyBindings.CloseTab([&]() { this->_DoCloseTab(); });

    }

    void TermApp::_Create()
    {
        Controls::Grid container;

        _root = Controls::Grid();
        _tabBar = Controls::StackPanel();
        _tabContent = Controls::Grid();


        auto tabBarRowDef = Controls::RowDefinition();
        tabBarRowDef.Height(GridLengthHelper::Auto());
        

        _root.RowDefinitions().Append(tabBarRowDef);
        _root.RowDefinitions().Append(Controls::RowDefinition{});

        _root.Children().Append(_tabBar);
        _root.Children().Append(_tabContent);
        Controls::Grid::SetRow(_tabBar, 0);
        Controls::Grid::SetRow(_tabContent, 1);

        _tabBar.Height(0);
        _tabBar.Orientation(Controls::Orientation::Horizontal);
        _tabBar.HorizontalAlignment(HorizontalAlignment::Stretch);

        _tabContent.VerticalAlignment(VerticalAlignment::Stretch);
        _tabContent.HorizontalAlignment(HorizontalAlignment::Stretch);
    }

    TermApp::~TermApp()
    {
    }

    UIElement TermApp::GetRoot()
    {
        return _root;
    }


    void TermApp::_DoNewTab()
    {
    }
    void TermApp::_DoCloseTab()
    {
    }
}
