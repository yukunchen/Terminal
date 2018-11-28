//
// Declaration of the TerminalControl class.
//

#pragma once

#include "TerminalControl.g.h"
#include "../../renderer/base/Renderer.hpp"

#include "../../cascadia/TerminalCore/Terminal.hpp"
#include "TerminalCanvasView.h"
#include "CanvasViewRenderThread.hpp"

// Includes all the types defined in TerminalConnection -
//      including ITerminalConnection, EchoConnection, etc
#include <winrt/TerminalConnection.h>

// Win2d
#include "winrt/Microsoft.Graphics.Canvas.Text.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"

namespace winrt::TerminalApp::implementation
{
    struct TerminalControl : TerminalControlT<TerminalControl>
    {
        TerminalControl();

        void terminalView_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender,
                               const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);

        ::Microsoft::Terminal::Core::Terminal& GetTerminal();
        TerminalConnection::ITerminalConnection& GetConnection();
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

namespace winrt::TerminalApp::factory_implementation
{
    struct TerminalControl : TerminalControlT<TerminalControl, implementation::TerminalControl>
    {
    };
}
