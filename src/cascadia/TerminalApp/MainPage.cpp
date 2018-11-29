#include "pch.h"
#include "MainPage.h"
#include <sstream>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::ViewManagement;

using namespace winrt::Microsoft::Graphics::Canvas;
using namespace winrt::Microsoft::Graphics::Canvas::Text;
using namespace winrt::Microsoft::Graphics::Canvas::UI::Xaml;

using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;

using namespace Windows::UI;

using namespace ::Microsoft::Terminal::Core;
using namespace ::Microsoft::Console::Render;
using namespace ::Microsoft::Console::Types;

namespace winrt::TerminalApp::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();

        ApplicationView appView = ApplicationView::GetForCurrentView();
        appView.Title(L"Project Cascadia");
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        auto t = _GetActiveTerminalControl();

        t.Prototype_WriteToOutput({ L"F" });
        t.Prototype_WriteToOutput({ L"🌯" });
    }

    void MainPage::JapaneseClick(IInspectable const&, RoutedEventArgs const&)
    {
        auto t = _GetActiveTerminalControl();
        t.Prototype_WriteToOutput({ L"私を押す" });
    }

    void MainPage::SmileyClick(IInspectable const&, RoutedEventArgs const&)
    {
        auto t = _GetActiveTerminalControl();
        t.Prototype_WriteToOutput({ L"😃" });
    }

    void MainPage::SimpleColorClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        BYTE foregroundIndex = rand() % 16;
        BYTE backgroundIndex = rand() % 16;

        auto t = _GetActiveTerminalControl();
        t.Prototype_ChangeTextColors(foregroundIndex, backgroundIndex);
        t.Prototype_WriteToOutput({ L"X" });
    }

    void MainPage::canvasControl_Draw(const CanvasControl& sender, const CanvasDrawEventArgs & args)
    {
        CanvasTextFormat format;
        format.HorizontalAlignment(CanvasHorizontalAlignment::Center);
        format.VerticalAlignment(CanvasVerticalAlignment::Center);
        format.FontSize(72.0f);
        format.FontFamily(L"Segoe UI Semibold");

        float2 size = sender.Size();
        float2 center{ size.x / 2.0f, size.y / 2.0f };
        Rect bounds{ 0.0f, 0.0f, size.x, size.y };

        CanvasDrawingSession session = args.DrawingSession();

        session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());
        session.DrawText(L"Win2D with\nC++/WinRT!", bounds, Colors::Orange(), format);
    }

    TerminalComponent::TerminalControl MainPage::_GetActiveTerminalControl()
    {
        // This actually doesn't work the way I want at all-
        // If you click any of the buttons, the terminal will lose focus,
        //      meaning we always select t00 as the active terminal
        auto t00 = terminal00();
        auto t01 = terminal01();
        if (t00.IsFocusEngaged())
        {
            return t00;
        }
        else if (t01.IsFocusEngaged())
        {
            return t01;
        }
        else
        {
            return t00;
        }
    }

}
