//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include "../../terminal/parser/OutputStateMachineEngine.hpp"
// #include "../../terminal/adapter/termDispatch.hpp"

#include "MyDispatch.hpp"

#include <winrt/TerminalConnection.h>

// Win2d
#include "winrt/Microsoft.Graphics.Canvas.Text.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"

namespace winrt::TerminalApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void canvasControl_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender,
                                const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);

      private:
        DummyRenderTarget _renderTarget;
        std::unique_ptr<TextBuffer> _buffer;
        // For the record, you can't have a unique_ptr to the interface here.
        // There's no cast from a unique_ptr<A> to a unique_ptr<I> for a class A : I {}
        // This might be by design, I think cppwinrt wants you using refs everywhere.
        TerminalConnection::ITerminalConnection _connection;
        winrt::event_token _connectionOutputEventToken;
        // Microsoft::Graphics::Canvas::UI::Xaml

        ::Microsoft::Console::VirtualTerminal::OutputStateMachineEngine _engine;
        MyDispatch _dispatch;
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
