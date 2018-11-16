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
        _renderTarget{},
        _connection{TerminalConnection::ConptyConnection(L"cmd.exe", 32, 80)},
        _engine{ &_dispatch }
    {
        InitializeComponent();
        COORD bufferSize { 80, 9001 };
        UINT cursorSize = 12;
        TextAttribute attr{ 0x7f };
        _buffer = std::make_unique<TextBuffer>(bufferSize, attr, cursorSize, _renderTarget);

        auto fn = [&](const hstring str) {
            std::wstring_view _v(str.c_str());
            _buffer->Write(_v);
            for (int x = 0; x < _v.size(); x++) _buffer->IncrementCursor();
        };
        _connectionOutputEventToken = _connection.TerminalOutput(fn);
        _connection.Start();
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        auto cursorX = _buffer->GetCursor().GetPosition().X;

        const auto burrito = L"🌯";
        OutputCellIterator burriter{ burrito };

        _buffer->Write({ L"F" });
        _buffer->IncrementCursor();

        auto textIter = _buffer->GetTextDataAt({0, 0});

        std::wstring wstr = L"";

        for (int x = 0; x < cursorX; x++)
        {
            auto view = *textIter;
            wstr += view;
            textIter++;
        }
        hstring hstr{ wstr };

        myButton().Content(box_value(hstr));
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

}
