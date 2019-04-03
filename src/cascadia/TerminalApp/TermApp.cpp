#include "pch.h"
#include "TermApp.h"

#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace ::Microsoft::Terminal::TerminalApp;

namespace winrt
{
    namespace MUX = Microsoft::UI::Xaml;
}

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    TermApp::TermApp() :
        _xamlMetadataProviders{  },
        _root{ nullptr },
        _tabView{ nullptr },
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
        _xamlMetadataProviders.emplace_back(MUX::XamlTypeInfo::XamlControlsXamlMetaDataProvider{});

        _tabView = MUX::Controls::TabView{};

        _tabView.SelectionChanged([this](auto&& sender, auto&& /*event*/){
            auto tabView = sender.as<MUX::Controls::TabView>();
            auto selectedIndex = tabView.SelectedIndex();
            if (selectedIndex >= 0)
            {
                try
                {
                    auto tab = _tabs.at(selectedIndex);
                    auto control = tab->GetTerminalControl().GetControl();

                    _tabContent.Children().Clear();
                    _tabContent.Children().Append(control);

                    control.Focus(FocusState::Programmatic);
                }
                CATCH_LOG();
            }
        });

        _root = Controls::Grid{};
        _tabContent = Controls::Grid{};

        auto tabBarGrid = Controls::Grid{};
        auto settingsBtnColDef = Controls::ColumnDefinition();
        settingsBtnColDef.Width(GridLengthHelper::Auto());
        tabBarGrid.ColumnDefinitions().Append(settingsBtnColDef);
        tabBarGrid.ColumnDefinitions().Append(Controls::ColumnDefinition{});

        auto tabBarRowDef = Controls::RowDefinition();
        tabBarRowDef.Height(GridLengthHelper::Auto());

        _root.RowDefinitions().Append(tabBarRowDef);
        _root.RowDefinitions().Append(Controls::RowDefinition{});

        // _root.Children().Append(_tabView);
        _root.Children().Append(tabBarGrid);
        tabBarGrid.Children().Append(_tabView);
        _root.Children().Append(_tabContent);
        // Controls::Grid::SetRow(_tabView, 0);
        Controls::Grid::SetRow(tabBarGrid, 0);
        Controls::Grid::SetRow(_tabContent, 1);

        auto settingsBtn = Controls::Button{};
        auto textView = Controls::TextBlock{};
        textView.Text(L"foo");
        settingsBtn.Content(textView);
        Controls::Grid::SetRow(settingsBtn, 0);
        Controls::Grid::SetColumn(settingsBtn, 1);
        settingsBtn.VerticalAlignment(VerticalAlignment::Stretch);
        settingsBtn.HorizontalAlignment(HorizontalAlignment::Right);
        tabBarGrid.Children().Append(settingsBtn);

        _tabContent.VerticalAlignment(VerticalAlignment::Stretch);
        _tabContent.HorizontalAlignment(HorizontalAlignment::Stretch);

        _OpenNewTab(std::nullopt);
    }

    // Method Description:
    // - Look up the Xaml type associated with a TypeName (name + xaml metadata type)
    //      from a Xaml or Xbf file.
    // Arguments:
    // - type: the {name, xaml metadata type} tuple corresponding to a Xaml element
    // Return Value:
    // - Runtime Xaml type
    Windows::UI::Xaml::Markup::IXamlType TermApp::GetXamlType(Windows::UI::Xaml::Interop::TypeName const& type)
    {
        for (const auto& provider : _xamlMetadataProviders)
        {
            auto xamlType = provider.GetXamlType(type);
            if (xamlType)
            {
                return xamlType;
            }
        }
        return nullptr;
    }

    // Method Description:
    // - Look up the Xaml type associated with a fully-qualified type name
    //      (likely of the format "namespace:element")
    // Arguments:
    // - fullName: the fully-qualified element name (including namespace) from a Xaml element
    // Return Value:
    // - Runtime Xaml type
    Windows::UI::Xaml::Markup::IXamlType TermApp::GetXamlType(hstring const& fullName)
    {
        for (const auto& provider : _xamlMetadataProviders)
        {
            auto xamlType = provider.GetXamlType(fullName);
            if (xamlType)
            {
                return xamlType;
            }
        }
        return nullptr;
    }

    // Method Description:
    // - Return the list of XML namespaces in which Xaml elements can be found
    // Arguments:
    // - <none>
    // Return Value:
    // - The list of XML namespaces in which Xaml elements can be found
    com_array<Windows::UI::Xaml::Markup::XmlnsDefinition> TermApp::GetXmlnsDefinitions()
    {
        std::vector<Windows::UI::Xaml::Markup::XmlnsDefinition> definitions;
        for (const auto& provider : _xamlMetadataProviders)
        {
            auto providerDefinitions = provider.GetXmlnsDefinitions();
            std::copy(
                std::make_move_iterator(providerDefinitions.begin()),
                std::make_move_iterator(providerDefinitions.end()),
                std::back_inserter(definitions));
        }
        return com_array<Windows::UI::Xaml::Markup::XmlnsDefinition>{ definitions };
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
        kb.NewTab([this]() { _OpenNewTab(std::nullopt); });
        kb.CloseTab([this]() { _CloseFocusedTab(); });
        kb.NewTabWithProfile([this](auto index) { _OpenNewTab({ index }); });
        kb.ScrollUp([this]() { _DoScroll(-1); });
        kb.ScrollDown([this]() { _DoScroll(1); });
        kb.NextTab([this]() { _SelectNextTab(true); });
        kb.PrevTab([this]() { _SelectNextTab(false); });
    }

    UIElement TermApp::GetRoot()
    {
        return _root;
    }

    void TermApp::_SetFocusedTabIndex(int tabIndex)
    {
        auto tab = _tabs.at(tabIndex);
        _tabView.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [tab, this](){
            auto tabViewItem = tab->GetTabViewItem();
            _tabView.SelectedItem(tabViewItem);
        });
    }

    // Method Description:
    // - Handle changes in tab layout.
    void TermApp::_UpdateTabView()
    {
        _tabView.Visibility((_tabs.size() > 1) ? Visibility::Visible : Visibility::Collapsed);
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
    void TermApp::_OpenNewTab(std::optional<int> profileIndex)
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

            const auto& selectedProfile = profiles[realIndex];
            GUID profileGuid = selectedProfile.GetGuid();
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

        auto tabViewItem = newTab->GetTabViewItem();
        _tabView.Items().Append(tabViewItem);

        // This is one way to set the tab's selected background color.
        //   tabViewItem.Resources().Insert(winrt::box_value(L"TabViewItemHeaderBackgroundSelected"), a Brush?);

        // This kicks off TabView::SelectionChanged, in response to which we'll attach the terminal's
        // Xaml control to the Xaml root.
        _tabView.SelectedItem(tabViewItem);
        _UpdateTabView();
    }

    // Method Description:
    // - Returns the index in our list of tabs of the currently focused tab. If
    //      no tab is currently selected, returns -1.
    // Arguments:
    // - <none>
    // Return Value:
    // - the index of the currently focused tab if there is one, else -1
    int TermApp::_GetFocusedTabIndex() const
    {
        return _tabView.SelectedIndex();
    }

    // Method Description:
    // - Close the currently focused tab. Focus will move to the left, if possible.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermApp::_CloseFocusedTab()
    {
        if (_tabs.size() > 1)
        {
            int focusedTabIndex = _GetFocusedTabIndex();
            std::shared_ptr<Tab> focusedTab{ _tabs[focusedTabIndex] };

            // We're not calling _FocusTab here because it makes an async dispatch
            // that is practically guaranteed to not happen before we delete the tab.
            _tabView.SelectedIndex((focusedTabIndex > 0) ? focusedTabIndex - 1 : 1);
            _tabView.Items().RemoveAt(focusedTabIndex);
            _tabs.erase(_tabs.begin() + focusedTabIndex);
            _UpdateTabView();
        }
    }

    // Method Description:
    // - Move the viewport of the terminal of the currently focused tab up or
    //      down a number of lines. Negative values of `delta` will move the
    //      view up, and positive values will move the viewport down.
    // Arguments:
    // - delta: a number of lines to move the viewport relative to the current viewport.
    // Return Value:
    // - <none>
    void TermApp::_DoScroll(int delta)
    {
        int focusedTabIndex = _GetFocusedTabIndex();
        _tabs[focusedTabIndex]->Scroll(delta);
    }

    // Method Description:
    // - Sets focus to the tab to the right or left the currently selected tab.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermApp::_SelectNextTab(const bool bMoveRight)
    {
        int focusedTabIndex = _GetFocusedTabIndex();
        auto tabCount = _tabs.size();
        // Wraparound math. By adding tabCount and then calculating modulo tabCount,
        // we clamp the values to the range [0, tabCount) while still supporting moving
        // leftward from 0 to tabCount - 1.
        _SetFocusedTabIndex(
            (tabCount + focusedTabIndex + (bMoveRight ? 1 : -1)) % tabCount
        );
    }

}
