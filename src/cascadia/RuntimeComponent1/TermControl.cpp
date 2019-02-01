#include "pch.h"
#include "TermControl.h"
using namespace ::Microsoft::Console::Types;

namespace winrt::RuntimeComponent1::implementation
{

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
            _terminal->LockForWriting();
            const auto foundationSize = e.NewSize();
            SIZE classicSize;
            classicSize.cx = (LONG)foundationSize.Width;
            classicSize.cy = (LONG)foundationSize.Height;

            THROW_IF_FAILED(_renderEngine->SetWindowSize(classicSize));
            _renderer->TriggerRedrawAll();
            const auto vp = Viewport::FromInclusive(_renderEngine->GetDirtyRectInChars());

            _terminal->Resize({ vp.Width(), vp.Height() });
            _connection.Resize(vp.Height(), vp.Width());

            _terminal->UnlockForWriting();
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
            _terminal->LockForWriting();
            auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
            nativePanel->SetSwapChain(chain.Get());
            _terminal->UnlockForWriting();
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
            str;
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
        _controlRoot.PreviewKeyDown([&](auto& sender,
            winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e) {
            this->KeyHandler(sender, e);
        });

        _controlRoot.CharacterReceived([&](auto& sender,
            winrt::Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e) {
            this->CharacterHandler(sender, e);
        });

        _initializedTerminal = true;
    }

    void TermControl::CharacterHandler(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                           winrt::Windows::UI::Xaml::Input::CharacterReceivedRoutedEventArgs const& e)
    {
        const auto ch = e.Character();
        if (ch == L'\x08')
        {
            // We want Backspace to be handled by KeyHandler, so the
            //      terminalInput can translate it into a \x7f. So, do nothing
            //      here, so we don't end up sending both a BS and a DEL to the
            //      terminal.
            return;
        }
        auto hstr = to_hstring(ch);
        _connection.WriteInput(hstr);
        e.Handled(true);
    }

    void TermControl::KeyHandler(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                     winrt::Windows::UI::Xaml::Input::KeyRoutedEventArgs const& e)
    {
        // This is super hacky - it seems as though these keys only seem pressed
        // every other time they're pressed
        winrt::Windows::UI::Core::CoreWindow foo = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
        // DONT USE
        //      != CoreVirtualKeyStates::None
        // OR
        //      == CoreVirtualKeyStates::Down
        // Sometimes with the key down, the state is Down | Locked.
        // Sometimes with the key up, the state is Locked.
        // IsFlagSet(Down) is the only correct solution.
        auto ctrlKeyState = foo.GetKeyState(winrt::Windows::System::VirtualKey::Control);
        auto shiftKeyState = foo.GetKeyState(winrt::Windows::System::VirtualKey::Shift);
        auto altKeyState = foo.GetKeyState(winrt::Windows::System::VirtualKey::Menu);

        auto ctrl = (ctrlKeyState & winrt::Windows::UI::Core::CoreVirtualKeyStates::Down) == winrt::Windows::UI::Core::CoreVirtualKeyStates::Down;
        auto shift = (shiftKeyState & winrt::Windows::UI::Core::CoreVirtualKeyStates::Down) == winrt::Windows::UI::Core::CoreVirtualKeyStates::Down;
        auto alt = (altKeyState & winrt::Windows::UI::Core::CoreVirtualKeyStates::Down) == winrt::Windows::UI::Core::CoreVirtualKeyStates::Down;

        auto vkey = e.OriginalKey();

        _terminal->SendKeyEvent((WORD)vkey, ctrl, alt, shift);
    }

}
