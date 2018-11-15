#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Windows::UI::Xaml;

using namespace winrt::Microsoft::Graphics::Canvas;
using namespace winrt::Microsoft::Graphics::Canvas::Text;
using namespace winrt::Microsoft::Graphics::Canvas::UI::Xaml;
// using namespace Microsoft::Graphics::Canvas::UI::Xaml;

using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;

using namespace Windows::UI;

namespace winrt::TerminalApp::implementation
{
    MainPage::MainPage() :
        _renderTarget{}
    {
        InitializeComponent();
        COORD bufferSize { 80, 9001 };
        UINT cursorSize = 12;
        TextAttribute attr{ 0x7f };
        _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, _renderTarget);

        // Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl control;
        // control.Draw([=](CanvasControl const& sender, CanvasDrawEventArgs const& args)
        // {
        //     float2 size = sender.Size();
        //     float2 center{ size.x / 2.0f, size.y / 2.0f };
        //     Rect bounds{ 0.0f, 0.0f, size.x, size.y };

        //     CanvasDrawingSession session = args.DrawingSession();

        //     session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());
        //     session.DrawText(L"Win2D with\nC++/WinRT!", bounds, Colors::Orange(), format);
        // });
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        auto cursorX = _buffer->GetCursor().GetPosition().X;

        const auto burrito = L"🌯";
        OutputCellIterator burriter{ burrito };

        _buffer->Write({ L"F" });
        _buffer->IncrementCursor();

        // DebugBreak();
        // _buffer->Write(burriter);
        // _buffer->IncrementCursor();

        auto textIter = _buffer->GetTextDataAt({0, 0});

        //if (_buffer->GetCursor().GetPosition().X > 1) DebugBreak();

        std::wstring wstr = L"";
        //std::wstring wstr(*textIter);

         for (int x = 0; x < cursorX; x++)
         {
             auto view = *textIter;
             wstr += view;
             textIter++;
         }
        hstring hstr{ wstr };
        // myButton().Content(box_value(L"Clicked"));
        myButton().Content(box_value(hstr));
    }

    void MainPage::canvasControl_Draw(const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasControl & sender, const winrt::Microsoft::Graphics::Canvas::UI::Xaml::CanvasDrawEventArgs & args)
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

    // // void MainPage::canvasControl_Draw(CanvasControl& sender, CanvasDrawEventArgs& args)
    // void MainPage::canvasControl_Draw(CanvasControl& sender, CanvasDrawEventArgs& args);
    // {
    //     // args.DrawingSession.DrawEllipse(155, 115, 80, 30, Colors::Black, 3);
    //     // args.DrawingSession.DrawText("Hello, world!", 100, 100, Colors::Yellow);

    //     float2 size = sender.Size();
    //     float2 center{size.x / 2.0f, size.y / 2.0f};
    //     Rect bounds{0.0f, 0.0f, size.x, size.y};

    //     CanvasDrawingSession session = args.DrawingSession();

    //     session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());
    //     session.DrawText(L"Win2D with\nC++/WinRT!", bounds, Colors::Orange(), format);
    // }

    // //void MainPage::canvasControl_Draw(CanvasControl& const sender, CanvasDrawEventArgs& const args)
    // //{
    // //
    // //}
}
