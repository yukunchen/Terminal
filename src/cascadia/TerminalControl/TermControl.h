#pragma once

#include "TermControl.g.h"
#include <winrt/TerminalConnection.h>
#include "../../renderer/base/Renderer.hpp"
#include "../../renderer/dx/DxRenderer.hpp"
#include "../../cascadia/TerminalCore/Terminal.hpp"

namespace winrt::TerminalControl::implementation
{
    struct TermControl : TermControlT<TermControl>
    {
        TermControl();
        Windows::UI::Xaml::UIElement GetRoot();
        Windows::UI::Xaml::Controls::UserControl GetControl();
        void SwapChainChanged();
        ~TermControl();

    private:
        TerminalConnection::ITerminalConnection _connection;
        bool _initializedTerminal;

        Windows::UI::Xaml::Controls::UserControl _controlRoot;
        Windows::UI::Xaml::UIElement _root;
        Windows::UI::Xaml::Controls::SwapChainPanel _swapChainPanel;
        event_token _connectionOutputEventToken;

        ::Microsoft::Terminal::Core::Terminal* _terminal;

        std::unique_ptr<::Microsoft::Console::Render::Renderer> _renderer;
        std::unique_ptr<::Microsoft::Console::Render::DxEngine> _renderEngine;

        void _InitializeTerminal();
        void KeyHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void CharacterHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e);

        void _SendInputToConnection(const std::wstring& wstr);
        void _SwapChainSizeChanged(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::SizeChangedEventArgs const& e);
    };
}

namespace winrt::TerminalControl::factory_implementation
{
    struct TermControl : TermControlT<TermControl, implementation::TermControl>
    {
    };
}
