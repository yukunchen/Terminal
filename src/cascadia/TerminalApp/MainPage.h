//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "../../buffer/out/textBuffer.hpp"
#include "../../renderer/inc/DummyRenderTarget.hpp"

// Win2d
//#include <winrt/Microsoft.Graphics.Canvas.Text.h>
//#include <winrt/Microsoft.Graphics.Canvas.UI.Xaml.h>

#include "winrt/Microsoft.Graphics.Canvas.Text.h"
#include "winrt/Microsoft.Graphics.Canvas.UI.Xaml.h"

namespace winrt::TerminalApp::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void ClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void canvasControl_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender,
                                const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);

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
