//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include <microsoft.graphics.canvas.h>

namespace winrt::TerminalApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void canvasControl_Draw(ABI::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender, ABI::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);
    private:
        DummyRenderTarget _renderTarget;
        std::unique_ptr<TextBuffer> _buffer;
        // Microsoft::Graphics::Canvas::UI::Xaml
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
