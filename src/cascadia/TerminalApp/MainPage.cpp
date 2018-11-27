#include "pch.h"
#include "MainPage.h"
#include "CanvasViewRenderThread.hpp"
#include <sstream>
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include "Win2DEngine.h"

using namespace winrt;
using namespace Windows::UI::Xaml;
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
    MainPage::MainPage() :
        _connection{TerminalConnection::ConptyConnection(L"cmd.exe", 32, 80)},
        _canvasView{ nullptr, L"Consolas", 12.0f },
        _initializedTerminal{ false }
    {
        InitializeComponent();
        _canvasView = TerminalCanvasView( canvas00(), L"Consolas", 12.0f );
        // Do this to avoid having to std::bind(canvasControl_Draw, this, placeholders::_1)
        // Which I don't even know if it would work
        canvas00().Draw([&](const auto& s, const auto& args) { terminalView_Draw(s, args); });
        canvas00().CreateResources([&](const auto& /*s*/, const auto& /*args*/)
        {
            _canvasView.Initialize();
            if (!_initializedTerminal)
            {
                // The Canvas view must be initialized first so we can get the size from it.
                _InitializeTerminal();
            }
        });
        
        ApplicationView appView = ApplicationView::GetForCurrentView();
        appView.Title(L"Project Cascadia");
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        _terminal->Write( L"F" );
        _terminal->Write({ L"🌯" });
        _canvasView.Invalidate();
    }

    void MainPage::JapaneseClick(IInspectable const&, RoutedEventArgs const&)
    {
        _terminal->Write({ L"私を押す" });
        _canvasView.Invalidate();
    }

    void MainPage::SmileyClick(IInspectable const&, RoutedEventArgs const&)
    {
        _terminal->Write({ L"😃" });
        _canvasView.Invalidate();
    }

    void MainPage::SimpleColorClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        static BYTE foregroundIndex = 7;
        static BYTE backgroundIndex = 0;

        _terminal->SetForegroundIndex(foregroundIndex);
        _terminal->SetBackgroundIndex(backgroundIndex);

        foregroundIndex = (foregroundIndex + 1) % 16;
        backgroundIndex = (backgroundIndex + 3) % 16;

        _terminal->Write(L"X");

        _canvasView.Invalidate();
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

    void MainPage::_InitializeTerminal()
    {
        if (_initializedTerminal) return;
        // DO NOT USE canvase00().Width(), those are nan?
        float windowWidth = canvas00().Size().Width;
        float windowHeight = canvas00().Size().Height;
        COORD viewportSizeInChars = _canvasView.PixelsToChars(windowWidth, windowHeight);

        _terminal = new Terminal();

        _renderer = std::make_unique<Renderer>(_terminal, nullptr, 0);
        IRenderTarget& renderTarget = *_renderer;

        _terminal->Create(viewportSizeInChars, 9001, renderTarget);

        _renderThread = new CanvasViewRenderThread(_canvasView);
        _renderer->SetThread(_renderThread);

        _renderEngine = std::make_unique<Win2DEngine>(_canvasView,
                                                      Viewport::FromDimensions({0, 0}, viewportSizeInChars));
        _renderer->AddRenderEngine(_renderEngine.get());

        // Display our calculated buffer, viewport size
        // std::wstringstream bufferSizeSS;
        // bufferSizeSS << L"{" << bufferSize.X << L", " << bufferSize.Y << L"}";
        // BufferSizeText().Text(bufferSizeSS.str());

        std::wstringstream viewportSizeSS;
        viewportSizeSS << L"{" << viewportSizeInChars.X << L", " << viewportSizeInChars.Y << L"}";
        ViewportSizeText().Text(viewportSizeSS.str());

        auto fn = [&](const hstring str) {
            _terminal->Write(str.c_str());
        };
        _connectionOutputEventToken = _connection.TerminalOutput(fn);
        _connection.Start();

        _renderThread->EnablePainting();
        _initializedTerminal = true;
    }

    void MainPage::terminalView_Draw(const CanvasControl& sender, const CanvasDrawEventArgs & args)
    {
        // float2 size = sender.Size();
        // float2 center{ size.x / 2.0f, size.y / 2.0f };

        CanvasDrawingSession session = args.DrawingSession();

        // session.FillEllipse(center, center.x - 50.0f, center.y - 50.0f, Colors::DarkSlateGray());

        if (_terminal == nullptr) return;
        // auto textIter = _terminal->GetBuffer().GetTextLineDataAt({ 0, 0 });
        // std::wstring wstr = L"";
        // while (textIter)
        // {
        //     auto view = *textIter;
        //     wstr += view;
        //     textIter += view.size();
        // }

        _canvasView.PrepDrawingSession(session);
        LOG_IF_FAILED(_renderer->PaintFrame());
        // _canvasView.PaintRun({ wstr }, { 0, 0 }, Colors::White(), Colors::Black());
        // _canvasView.FinishDrawingSession();
    }
}
