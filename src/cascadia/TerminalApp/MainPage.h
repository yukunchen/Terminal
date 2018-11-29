//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "../../renderer/base/Renderer.hpp"

#include "../../cascadia/TerminalCore/Terminal.hpp"

// Includes all the types defined in TerminalConnection -
//      including ITerminalConnection, EchoConnection, etc
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

        void SimpleColorClickHandler(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void JapaneseClick(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);
        void SmileyClick(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& args);

        void canvasControl_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl& sender,
                                const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs& args);

      private:
        winrt::TerminalComponent::TerminalControl _GetActiveTerminalControl();
    };
}

namespace winrt::TerminalApp::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
