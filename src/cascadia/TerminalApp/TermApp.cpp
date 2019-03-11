#include "pch.h"
#include "TermApp.h"
#include <argb.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace ::Microsoft::Terminal::TerminalApp;

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    TermApp::TermApp() :
        _root{ nullptr },
        _tabBar{ nullptr },
        _tabContent{ nullptr },
        _settings{  },
        _tabs{  }
    {
        // For your own sanity, it's better to do setup outside the ctor.
        // If you do any setup in the ctor that ends up throwing an exception,
        // then it might look like TermApp just failed to activate, which will
        // cause you to chase down the rabbit hole of "why is TermApp not
        // registered?" when it definitely is.
    }

    // Method Description:
    // - Load our settings, and build the UI for the terminal app. Before this
    //      method is called, it should not be assumed that the TerminalApp is
    //      usable.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermApp::Create()
    {
        _LoadSettings();
        _Create();
    }

    TermApp::~TermApp()
    {
    }

    // Method Description:
    // - Create all of the initial UI elements of the Terminal app.
    //    * Creates the tab bar, initially hidden.
    //    * Creates the tab content area, which is where we'll display the tabs/panes.
    //    * Initializes the first terminal control, using the default profile,
    //      and adds it to our list of tabs.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
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

        _DoNewTab(std::nullopt);
    }

    // Method Description:
    // - Initialized our settings. See CascadiaSettings for more details.
    //      Additionally hooks up our callbacks for keybinding events to the
    //      keybindings object.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermApp::_LoadSettings()
    {
        _settings = CascadiaSettings::LoadAll();

        // Hook up the KeyBinding object's events to our handlers.
        // TODO: as we implement more events, hook them up here.
        // They should all be hooked up here, regardless of whether or not
        //      there's an actual keychord for them.
        auto kb = _settings->GetKeybindings();
        kb.NewTab([this]() { _DoNewTab(std::nullopt); });
        kb.CloseTab([this]() { _DoCloseTab(); });
        kb.NewTabWithProfile([this](auto index) { _DoNewTab({ index }); });
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

        for (auto& tab : _tabs)
        {
            _tabBar.Children().Append(tab->GetTabButton());
        }
    }

    void TermApp::_FocusTab(std::weak_ptr<Tab> tab)
    {
        _tabContent.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [tab, this](){
            auto sharedTab = tab.lock();
            if (sharedTab)
            {
                _tabContent.Children().Clear();

                _tabContent.Children().Append(sharedTab->GetTerminalControl().GetControl());

                _ResetTabs();

                sharedTab->SetFocused(true);

            }
        });
    }

    // Method Description:
    // - Open a new tab. This will create the TerminalControl hosting the
    //      terminal, and add a new Tab to our list of tabs. The method can
    //      optionally be provided a profile index, which will be used to create
    //      a tab using the profile in that index.
    //      If no index is provided, the default profile will be used.
    // Arguments:
    // - profileIndex: an optional index into the list of profiles to use to
    //      initialize this tab up with.
    // Return Value:
    // - <none>
    void TermApp::_DoNewTab(std::optional<int> profileIndex)
    {
        TerminalSettings settings;

        if (profileIndex)
        {
            const auto realIndex = profileIndex.value();
            const auto profiles = _settings->GetProfiles();

            // If we don't have that many profiles, then do nothing.
            if (realIndex >= profiles.size())
            {
                return;
            }

            auto& selectedProfile = profiles[realIndex];
            GUID profileGuid = selectedProfile->GetGuid();
            settings = _settings->MakeSettings(profileGuid);
        }
        else
        {
            // Create a tab using the default profile
            settings = _settings->MakeSettings(std::nullopt);
        }

        _CreateNewTabFromSettings(settings);
    }

    // Method Description:
    // - Creates a new tab with the given settings. If the tab bar is not being
    //      currently displayed, it will be shown.
    // Arguments:
    // - settings: the TerminalSettings object to use to create the TerminalControl with.
    // Return Value:
    // - <none>
    void TermApp::_CreateNewTabFromSettings(TerminalSettings settings)
    {
        // Initialize the new tab
        TermControl term{ settings };

        // Add the new tab to the list of our tabs.
        auto newTab = _tabs.emplace_back(std::make_shared<Tab>(term));

        // Create an onclick handler for the new tab's button, to allow us to
        //      focus the tab when it's pressed.
        newTab->GetTabButton().Click([weakTabPtr = std::weak_ptr<Tab>(newTab), this](auto&&, auto&&){
            auto tab = weakTabPtr.lock();
            if (tab)
            {
                _FocusTab(tab);
            }
        });

        // Update the tab bar. If this is the second tab we've created, then
        //      we'll make the tab bar visible for the first time.
        _CreateTabBar();

        _FocusTab(newTab);
    }

    void TermApp::_DoCloseTab()
    {
        if (_tabs.size() > 1)
        {
            size_t focusedTabIndex = -1;
            for(size_t i = 0; i < _tabs.size(); i++)
            {
                if (_tabs[i]->IsFocused())
                {
                    focusedTabIndex = i;
                    break;
                }
            }

            if (focusedTabIndex == -1)
            {
                // TODO: at least one tab should be focused...
                return;
            }

            std::shared_ptr<Tab> focusedTab{ _tabs[focusedTabIndex] };
            _tabs.erase(_tabs.begin()+focusedTabIndex);

            _CreateTabBar();
            _FocusTab(_tabs[0]);
        }
    }
}
