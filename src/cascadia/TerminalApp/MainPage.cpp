#include "pch.h"
#include "MainPage.h"
#include "CanvasViewRenderThread.hpp"
#include <sstream>
#include "../../renderer/inc/DummyRenderTarget.hpp"
#include "Win2DEngine.h"

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
    MainPage::MainPage() :
        // _connection{TerminalConnection::ConptyConnection(L"cmd.exe", 32, 80)},
        _connection{TerminalConnection::EchoConnection()},
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

        // These are important:
        // 1. When we get tapped, focus us
        this->Tapped([&](auto&, auto& e) {
            Focus(FocusState::Pointer);
            e.Handled(true);
        });
        // 2. Focus us. (this might not be important
        this->Focus(FocusState::Programmatic);
        // 3. Make sure we can be focused (why this isn't `Focusable` I'll never know)
        this->IsTabStop(true);
        // 4. Actually not sure about this one. Maybe it isn't necessary either.
        this->AllowFocusOnInteraction(true);

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
        BYTE foregroundIndex = 7;
        BYTE backgroundIndex = 0;
        foregroundIndex = rand() % 16;
        backgroundIndex = rand() % 16;

        _terminal->SetForegroundIndex(foregroundIndex);
        _terminal->SetBackgroundIndex(backgroundIndex);

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
        if (_initializedTerminal)
        {
            return;
        }

        // DO NOT USE canvase00().Width(), those are NaN?
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
        _connection.WriteInput(L"Hello world!");

        _renderThread->EnablePainting();

        // No matter what order these guys are in, The KeyDown's will fire
        //      before the CharacterRecieved, so we can't easily get characters
        //      first, then fallback to getting keys from vkeys.

        // this->PreviewKeyDown([&](auto& /*sender*/, KeyRoutedEventArgs const& e){
        this->KeyDown([&](auto& /*sender*/, KeyRoutedEventArgs const& e) {
            auto vkey = e.OriginalKey();
            auto hstr = to_hstring((int32_t)vkey);
            _connection.WriteInput(hstr);
        });
        this->CharacterReceived([&](auto& /*sender*/, CharacterReceivedRoutedEventArgs const& e) {
            const auto ch = e.Character();
            auto hstr = to_hstring(ch);
            _connection.WriteInput(hstr);
            e.Handled(true);
        });
        
        _initializedTerminal = true;
    }

    void MainPage::terminalView_Draw(const CanvasControl& /*sender*/, const CanvasDrawEventArgs & args)
    {
        CanvasDrawingSession session = args.DrawingSession();

        if (_terminal == nullptr) return;

        _canvasView.PrepDrawingSession(session);
        LOG_IF_FAILED(_renderer->PaintFrame());
    }
}
