//
// Declaration of the MyTerminalControl class.
//

#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "TerminalControl.g.h"

// Win2d
#include "winrt/Microsoft.Graphics.Canvas.Text.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"

// Includes all the types defined in TerminalConnection -
//      including ITerminalConnection, EchoConnection, etc
#include <winrt/TerminalConnection.h>

#include "../../renderer/base/Renderer.hpp"
#include "../../cascadia/TerminalCore/Terminal.hpp"

#include "TerminalCanvasView.h"
#include "CanvasViewRenderThread.hpp"


namespace winrt::TerminalComponent::implementation
{
    struct TerminalControl : TerminalControlT<TerminalControl>
    {
        TerminalControl();

        void Prototype_WriteToOutput(winrt::hstring const& text);
        void Prototype_ChangeTextColors(uint8_t fgIndex, uint8_t bgIndex);

        void terminalView_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender,
                               const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);

        ::Microsoft::Terminal::Core::Terminal& GetTerminal();
        TerminalConnection::ITerminalConnection& GetConnection();

        void KeyHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
        void CharacterHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e);
      private:
        // For the record, you can't have a unique_ptr to the interface here.
        // There's no cast from a unique_ptr<A> to a unique_ptr<I> for a class A : I {}
        // This might be by design, I think cppwinrt wants you using refs everywhere.
        TerminalConnection::ITerminalConnection _connection;
        winrt::event_token _connectionOutputEventToken;

        ::Microsoft::Terminal::Core::Terminal* _terminal;

        TerminalCanvasView _canvasView;

        std::unique_ptr<::Microsoft::Console::Render::Renderer> _renderer;
        ::Microsoft::Console::Render::IRenderThread* _renderThread;
        std::unique_ptr<::Microsoft::Console::Render::IRenderEngine> _renderEngine;

        bool _initializedTerminal;

        void _InitializeTerminal();
    };
}

namespace winrt::TerminalComponent::factory_implementation
{
    struct TerminalControl : TerminalControlT<TerminalControl, implementation::TerminalControl>
    {
    };
}
