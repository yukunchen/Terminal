#include "precomp.h"
#include "IslandWindow.h"
#include <winrt/TerminalConnection.h>

#include "../../renderer/base/Renderer.hpp"
#include "../../renderer/dx/DxRenderer.hpp"
#include "../../cascadia/TerminalCore/Terminal.hpp"

using namespace winrt;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;
using namespace ::Microsoft::Console::Types;

class TermControl
{
public:
    TermControl();
    winrt::Windows::UI::Xaml::UIElement GetRoot();
    winrt::Windows::UI::Xaml::Controls::UserControl GetControl();
    void SwapChainChanged();
    ~TermControl();
private:
    winrt::TerminalConnection::ITerminalConnection _connection;
    bool _initializedTerminal;

    winrt::Windows::UI::Xaml::Controls::UserControl _controlRoot;
    winrt::Windows::UI::Xaml::UIElement _root;
    winrt::Windows::UI::Xaml::Controls::SwapChainPanel _swapChainPanel;
    winrt::event_token _connectionOutputEventToken;

    ::Microsoft::Terminal::Core::Terminal* _terminal;

    std::unique_ptr<::Microsoft::Console::Render::Renderer> _renderer;
    std::unique_ptr<::Microsoft::Console::Render::DxEngine> _renderEngine;

    void _InitializeTerminal();
    //void KeyHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e);
    //void CharacterHandler(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e);

};

TermControl::TermControl() :
    _connection{ TerminalConnection::ConhostConnection(winrt::to_hstring("cmd.exe"), 30, 80) },
    _initializedTerminal{ false },
    _root{ nullptr },
    _controlRoot{ nullptr },
    _swapChainPanel{ nullptr }
{
    winrt::Windows::UI::Xaml::Controls::UserControl myControl;
    _controlRoot = myControl;
    winrt::Windows::UI::Xaml::Controls::Grid container;
    winrt::Windows::UI::Xaml::Controls::SwapChainPanel swapChainPanel;
    swapChainPanel.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::HorizontalAlignment::Stretch));
    swapChainPanel.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::HorizontalAlignment::Stretch));

    // swapChainPanel.SizeChanged([&](IInspectable const& /*sender*/, SizeChangedEventArgs const& e)=>{
    swapChainPanel.SizeChanged([&](winrt::Windows::Foundation::IInspectable const& /*sender*/, const auto& e){

        const auto foundationSize = e.NewSize();
        SIZE classicSize;
        classicSize.cx = (LONG)foundationSize.Width;
        classicSize.cy = (LONG)foundationSize.Height;

        THROW_IF_FAILED(_renderEngine->SetWindowSize(classicSize));
        _renderer->TriggerRedrawAll();
        const auto vp = Viewport::FromInclusive(_renderEngine->GetDirtyRectInChars());
        _terminal->Resize({ vp.Width(), vp.Height() });
        _connection.Resize(vp.Height(), vp.Width());
    });
    container.Children().Append(swapChainPanel);
    _root = container;
    _swapChainPanel = swapChainPanel;
    _controlRoot.Content(_root);

    //// These are important:
    // 1. When we get tapped, focus us
    _controlRoot.Tapped([&](auto&, auto& e) {
        _controlRoot.Focus(winrt::Windows::UI::Xaml::FocusState::Pointer);
        e.Handled(true);
    });
    // 2. Focus us. (this might not be important
    _controlRoot.Focus(winrt::Windows::UI::Xaml::FocusState::Programmatic);
    // 3. Make sure we can be focused (why this isn't `Focusable` I'll never know)
    _controlRoot.IsTabStop(true);
    // 4. Actually not sure about this one. Maybe it isn't necessary either.
    _controlRoot.AllowFocusOnInteraction(true);

    _InitializeTerminal();
}
TermControl::~TermControl()
{
    _connection.Close();
}

winrt::Windows::UI::Xaml::UIElement TermControl::GetRoot()
{
    return _root;
}


winrt::Windows::UI::Xaml::Controls::UserControl TermControl::GetControl()
{
    return _controlRoot;
}

void TermControl::SwapChainChanged()
{
    auto chain = _renderEngine->GetSwapChain();
    _swapChainPanel.Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::High, [=]()
    {
        _terminal->LockConsole();
        auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
        nativePanel->SetSwapChain(chain.Get());
        _terminal->UnlockConsole();
    });
}

void TermControl::_InitializeTerminal()
{
    if (_initializedTerminal)
    {
        return;
    }

    float windowWidth = 300;
    float windowHeight = 300;

    _terminal = new ::Microsoft::Terminal::Core::Terminal();

    // First create the render thread.
    auto renderThread = std::make_unique<::Microsoft::Console::Render::RenderThread>();
    // Stash a local pointer to the render thread, so we can enable it after
    //       we hand off ownership to the renderer.
    auto* const localPointerToThread = renderThread.get();
    _renderer = std::make_unique<::Microsoft::Console::Render::Renderer>(_terminal, nullptr, 0, std::move(renderThread));
    ::Microsoft::Console::Render::IRenderTarget& renderTarget = *_renderer;

    _terminal->Create({ 10, 10 }, 9001, renderTarget);

    auto dxEngine = std::make_unique<::Microsoft::Console::Render::DxEngine>();

    _renderer->AddRenderEngine(dxEngine.get());

    FontInfoDesired fi(L"Consolas", 0, 10, { 8, 12 }, 437);
    FontInfo actual(L"Consolas", 0, 10, { 8, 12 }, 437, false);
    _renderer->TriggerFontChange(96, fi, actual);

    THROW_IF_FAILED(dxEngine->SetWindowSize({ (LONG)windowWidth, (LONG)windowHeight }));
    dxEngine->SetCallback(std::bind(&TermControl::SwapChainChanged, this));
    THROW_IF_FAILED(dxEngine->Enable());
    _renderEngine = std::move(dxEngine);

    auto onRecieveOutputFn = [&](const hstring str) {
        _terminal->Write(str.c_str());
    };
    _connectionOutputEventToken = _connection.TerminalOutput(onRecieveOutputFn);
    _connection.Resize(10, 10);
    _connection.Start();

    auto inputFn = [&](const std::wstring& wstr)
    {
        _connection.WriteInput(wstr);
    };
    _terminal->_pfnWriteInput = inputFn;

    THROW_IF_FAILED(localPointerToThread->Initialize(_renderer.get()));

    auto chain = _renderEngine->GetSwapChain();
    _swapChainPanel.Dispatcher().RunAsync(winrt::Windows::UI::Core::CoreDispatcherPriority::High, [=]()
    {
        _terminal->LockConsole();
        auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
        nativePanel->SetSwapChain(chain.Get());
        _terminal->UnlockConsole();
    });

    localPointerToThread->EnablePainting();

    // No matter what order these guys are in, The KeyDown's will fire
    //      before the CharacterRecieved, so we can't easily get characters
    //      first, then fallback to getting keys from vkeys.
    // TODO: This apparently handles keys and characters correctly, though
    //      I'd keep an eye on it, and test more.
    // I presume that the characters that aren't translated by terminalInput
    //      just end up getting ignored, and the rest of the input comes
    //      through CharacterRecieved.
    // I don't believe there's a difference between KeyDown and
    //      PreviewKeyDown for our purposes
    _root.PreviewKeyDown([&](auto& /*sender*/,
        winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& /*e*/) {
        //this->KeyHandler(sender, e);
    });

    _root.CharacterReceived([&](auto& /*sender*/,
        winrt::Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& /*e*/) {
        //this->CharacterHandler(sender, e);
    });

    _initializedTerminal = true;
}



winrt::Windows::UI::Xaml::UIElement CreateDefaultContent() {
    winrt::Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
    acrylicBrush.BackgroundSource(winrt::Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
    acrylicBrush.TintOpacity(0.5);
    acrylicBrush.TintColor(winrt::Windows::UI::Colors::Red());

    winrt::Windows::UI::Xaml::Controls::Grid container;
    container.Margin(winrt::Windows::UI::Xaml::ThicknessHelper::FromLengths(100, 100, 100, 100));
    /*container.Background(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::LightSlateGray() });*/
    container.Background(acrylicBrush);

    winrt::Windows::UI::Xaml::Controls::Button b;
    b.Width(600);
    b.Height(60);
    b.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::VerticalAlignment::Center));

    b.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::HorizontalAlignment::Center));
    b.Foreground(winrt::Windows::UI::Xaml::Media::SolidColorBrush{ winrt::Windows::UI::Colors::White() });

    winrt::Windows::UI::Xaml::Controls::TextBlock tb;
    tb.Text(L"Hello Win32 love XAML and C++/WinRT xx");
    b.Content(tb);
    tb.FontSize(30.0f);
    container.Children().Append(b);

    winrt::Windows::UI::Xaml::Controls::TextBlock dpitb;
    dpitb.Text(L"(p.s. high DPI just got much easier for win32 devs)");
    dpitb.Foreground(winrt::Windows::UI::Xaml::Media::SolidColorBrush{ winrt::Windows::UI::Colors::White() });
    dpitb.Margin(winrt::Windows::UI::Xaml::ThicknessHelper::FromLengths(10, 10, 10, 10));
    dpitb.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::VerticalAlignment::Bottom));

    dpitb.SetValue(winrt::Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(winrt::Windows::UI::Xaml::HorizontalAlignment::Right));
    container.Children().Append(dpitb);

    return container;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment(apartment_type::single_threaded);

    IslandWindow window;

    auto defaultContent = CreateDefaultContent();

    TermControl term{};
    defaultContent.as<winrt::Windows::UI::Xaml::Controls::Grid>().Children().Append(term.GetControl());

    window.SetRootContent(defaultContent);

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        DispatchMessage(&message);
    }
}
