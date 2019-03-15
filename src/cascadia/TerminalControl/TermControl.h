#pragma once

#include "TermControl.g.h"
#include <winrt/MIcrosoft.Terminal.TerminalConnection.h>
#include <winrt/MIcrosoft.Terminal.Settings.h>
#include "../../renderer/base/Renderer.hpp"
#include "../../renderer/dx/DxRenderer.hpp"
#include "../../cascadia/TerminalCore/Terminal.hpp"

namespace winrt::Microsoft::Terminal::TerminalControl::implementation
{
    struct TermControl : TermControlT<TermControl>
    {
        TermControl();
        TermControl(Settings::IControlSettings settings);

        Windows::UI::Xaml::UIElement GetRoot();
        Windows::UI::Xaml::Controls::UserControl GetControl();

        winrt::event_token TitleChanged(TerminalControl::TitleChangedEventArgs const& handler);
        void TitleChanged(winrt::event_token const& token) noexcept;
        winrt::event_token ConnectionClosed(TerminalControl::ConnectionClosedEventArgs const& handler);
        void ConnectionClosed(winrt::event_token const& token) noexcept;

        hstring GetTitle();

        void Close();

        void ScrollViewport(int viewTop);
        int GetScrollOffset();
        winrt::event_token ScrollPositionChanged(TerminalControl::ScrollPositionChangedEventArgs const& handler);
        void ScrollPositionChanged(winrt::event_token const& token) noexcept;

        void SwapChainChanged();
        ~TermControl();

    private:
        winrt::event<TerminalControl::TitleChangedEventArgs> _titleChangeHandlers;
        winrt::event<TerminalControl::ConnectionClosedEventArgs> _connectionClosedHandlers;
        winrt::event<TerminalControl::ScrollPositionChangedEventArgs> _scrollPositionChangedHandlers;

        TerminalConnection::ITerminalConnection _connection;
        bool _initializedTerminal;

        Windows::UI::Xaml::Controls::UserControl _controlRoot;
        Windows::UI::Xaml::Controls::Grid _root;
        Windows::UI::Xaml::Controls::SwapChainPanel _swapChainPanel;
        Windows::UI::Xaml::Controls::Primitives::ScrollBar _scrollBar;
        event_token _connectionOutputEventToken;

        ::Microsoft::Terminal::Core::Terminal* _terminal;

        std::unique_ptr<::Microsoft::Console::Render::Renderer> _renderer;
        std::unique_ptr<::Microsoft::Console::Render::DxEngine> _renderEngine;

        Settings::IControlSettings _settings;
        bool _closing;

        std::optional<int> _lastScrollOffset;

        void _Create();
        void _ApplySettings();
        void _InitializeTerminal();
        void _UpdateScaling();
        void _UpdateFont();
        void _KeyHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void _CharacterHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e);
        void _MouseWheelHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void _ScrollbarChangeHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);

        void _SendInputToConnection(const std::wstring& wstr);
        void _SwapChainSizeChanged(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::SizeChangedEventArgs const& e);
        void _TerminalTitleChanged(const std::wstring_view& wstr);
        void _TerminalScrollPositionChanged(const int viewTop, const int viewHeight, const int bufferSize);

        void _ScrollbarUpdater(Windows::UI::Xaml::Controls::Primitives::ScrollBar scrollbar, const int viewTop, const int viewHeight, const int bufferSize);
    };
}

namespace winrt::Microsoft::Terminal::TerminalControl::factory_implementation
{
    struct TermControl : TermControlT<TermControl, implementation::TermControl>
    {
    };
}
