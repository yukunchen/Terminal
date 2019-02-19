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


        _DoNewTab();
    }

    TermApp::~TermApp()
    {
    }

    UIElement TermApp::GetRoot()
    {
        return _root;
    }

    void TermApp::_ResetTabs()
    {
        for (auto& t : _tabs)
        {
            auto button = t->GetTabButton();
            button.Background(nullptr);
            button.Foreground(Media::SolidColorBrush{winrt::Windows::UI::Colors::White()});
            button.BorderBrush(nullptr);
            button.BorderThickness(Thickness{});

            t->SetFocused(false);
        }
    }

    void TermApp::_CreateTabBar()
    {
        _tabBar.Children().Clear();

        // 32 works great for the default button text size
        auto tabBarHeight = (_tabs.size() > 1)? 26 : 0;
        _tabBar.Height(tabBarHeight);

        for (int i = 0; i < _tabs.size(); i++)
        {
            auto& tab = _tabs[i];
            // This was Add() in c#?
            _tabBar.Children().Append(tab->GetTabButton());
        }
    }

    void TermApp::_FocusTab(Tab& tab)
    {
        _tabContent.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [&](){
            _tabContent.Children().Clear();
            auto& mTab = tab;
            auto control = tab.GetTerminalControl();

            // This was Add() in c#?
            _tabContent.Children().Append(tab.GetTerminalControl().GetControl());

            _ResetTabs();

            tab.SetFocused(true);
        });
    }

    void TermApp::_DoNewTab()
    {
        winrt::TerminalControl::TerminalSettings settings{};
        settings.KeyBindings(_keyBindings);

        if (_tabs.size() < 1)
        {
            //// ARGB is 0xAABBGGRR, don't forget
            settings.DefaultBackground(0xff008a);
            settings.UseAcrylic(true);
            settings.TintOpacity(0.5);
            //settings.FontSize = 14;
            //settings.FontFace = "Ubuntu Mono";
            // For the record, this works, but ABSOLUTELY DO NOT use a font that isn't installed.
        }
        else
        {
            unsigned int bg = (unsigned int) (rand() % (0x1000000));
            bool acrylic = (rand() % 2) == 1;

            settings.DefaultBackground(bg);
            settings.UseAcrylic(acrylic);
            settings.TintOpacity(0.5);
            //settings.FontSize = 14;
        }

        TermControl term{ settings };
        auto newTab = std::make_unique<Tab>(term);

        auto newTabPointer = newTab.get();

        newTab->GetTabButton().Click([=](auto s, winrt::Windows::UI::Xaml::RoutedEventArgs e){
            auto mTabPtr = newTabPointer;
            _FocusTab(*newTabPointer);
        });



        // IMPORTANT: List.Add, Grid.Append.
        // If you reverse these, they silently fail
        _tabs.push_back(std::move(newTab));
        // _tabContent.Children().Add(term.GetControl());

        _CreateTabBar();

        _FocusTab(*newTabPointer);

    }
    void TermApp::_DoCloseTab()
    {
    }
}
