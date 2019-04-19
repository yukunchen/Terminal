#pragma once

#include "TermApp.g.h"
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "Tab.h"
#include "CascadiaSettings.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    struct TermApp : TermAppT<TermApp>
    {
        TermApp();

        Windows::UI::Xaml::UIElement GetRoot();
        void Create();
        void LoadSettings();

        Windows::Foundation::Point GetLaunchDimensions(uint32_t dpi);

        ~TermApp();

        Windows::UI::Xaml::Markup::IXamlType GetXamlType(Windows::UI::Xaml::Interop::TypeName const& type);
        Windows::UI::Xaml::Markup::IXamlType GetXamlType(hstring const& fullName);
        com_array<Windows::UI::Xaml::Markup::XmlnsDefinition> GetXmlnsDefinitions();

        winrt::event_token TitleChanged(Microsoft::Terminal::TerminalControl::TitleChangedEventArgs const& handler);
        void TitleChanged(winrt::event_token const& token) noexcept;

        hstring GetTitle();

    private:
        // Xaml interop: this list of providers will be queried to resolve types
        // encountered when loading .xaml and .xbf files.
        std::vector<Windows::UI::Xaml::Markup::IXamlMetadataProvider> _xamlMetadataProviders;

        winrt::event<Microsoft::Terminal::TerminalControl::TitleChangedEventArgs> _titleChangeHandlers;

        // If you add controls here, but forget to null them either here or in
        // the ctor, you're going to have a bad time. It'll mysteriously fail to
        // activate the app
        Windows::UI::Xaml::Controls::Grid _root{ nullptr };
        Microsoft::UI::Xaml::Controls::TabView _tabView{ nullptr };
        Windows::UI::Xaml::Controls::Grid _tabRow{ nullptr };
        Windows::UI::Xaml::Controls::Grid _tabContent{ nullptr };
        Windows::UI::Xaml::Controls::SplitButton _newTabButton{ nullptr };
        Windows::UI::Xaml::Controls::Button _hamburgerButton{ nullptr };

        std::vector<std::shared_ptr<Tab>> _tabs;

        std::unique_ptr<::Microsoft::Terminal::TerminalApp::CascadiaSettings> _settings;

        bool _loadedInitialSettings;

        void _Create();
        void _CreateNewTabFlyout();

        void _SettingsButtonOnClick(const IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& eventArgs);
        void _FeedbackButtonOnClick(const IInspectable& sender, const Windows::UI::Xaml::RoutedEventArgs& eventArgs);

        void _UpdateTabView();

        void _CreateNewTabFromSettings(winrt::Microsoft::Terminal::Settings::TerminalSettings settings);

        void _OpenNewTab(std::optional<int> profileIndex);
        void _CloseFocusedTab();
        void _SelectNextTab(const bool bMoveRight);

        void _SetFocusedTabIndex(int tabIndex);
        int _GetFocusedTabIndex() const;

        void _DoScroll(int delta);
        // Todo: add more event implementations here
        // MSFT:20641986: Add keybindings for New Window

        void _OnTabSelectionChanged(const IInspectable& sender, const Windows::UI::Xaml::Controls::SelectionChangedEventArgs& eventArgs);
        void _OnTabClosing(const IInspectable& sender, const Microsoft::UI::Xaml::Controls::TabViewTabClosingEventArgs& eventArgs);
        void _OnTabItemsChanged(const IInspectable& sender, const Windows::Foundation::Collections::IVectorChangedEventArgs& eventArgs);
    };
}

namespace winrt::Microsoft::Terminal::TerminalApp::factory_implementation
{
    struct TermApp : TermAppT<TermApp, implementation::TermApp>
    {
    };
}
