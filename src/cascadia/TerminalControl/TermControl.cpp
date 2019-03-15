/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/
#include "pch.h"
#include "TermControl.h"
#include <argb.h>

using namespace ::Microsoft::Console::Types;
using namespace ::Microsoft::Terminal::Core;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::Microsoft::Terminal::Settings;

namespace winrt::Microsoft::Terminal::TerminalControl::implementation
{

    TermControl::TermControl() :
        _connection{ TerminalConnection::ConhostConnection(winrt::to_hstring("cmd.exe"), 30, 80) },
        _initializedTerminal{ false },
        _root{ nullptr },
        _controlRoot{ nullptr },
        _swapChainPanel{ nullptr },
        _settings{},
        _closing{ false },
        _lastScrollOffset{ std::nullopt }
    {
        _Create();
    }

    TermControl::TermControl(Settings::IControlSettings settings) :
        _connection{ TerminalConnection::ConhostConnection(winrt::to_hstring("cmd.exe"), 30, 80) },
        _initializedTerminal{ false },
        _root{ nullptr },
        _controlRoot{ nullptr },
        _swapChainPanel{ nullptr },
        _settings{ settings },
        _closing{ false },
        _lastScrollOffset{ std::nullopt }
    {
        _Create();
    }

    void TermControl::_Create()
    {
        // Create a dummy UserControl to use as the "root" of our control we'll
        //      build manually.
        Controls::UserControl myControl;
        _controlRoot = myControl;

        Controls::Grid container;

        Controls::ColumnDefinition contentColumn{};
        Controls::ColumnDefinition scrollbarColumn{};
        contentColumn.Width(GridLength{ 1.0, GridUnitType::Star });
        scrollbarColumn.Width(GridLength{ 1.0, GridUnitType::Auto });

        container.ColumnDefinitions().Append(contentColumn);
        container.ColumnDefinitions().Append(scrollbarColumn);

        _scrollBar = Controls::Primitives::ScrollBar{};
        _scrollBar.Orientation(Controls::Orientation::Vertical);
        _scrollBar.IndicatorMode(Controls::Primitives::ScrollingIndicatorMode::MouseIndicator);
        _scrollBar.HorizontalAlignment(HorizontalAlignment::Right);
        _scrollBar.VerticalAlignment(VerticalAlignment::Stretch);

        // Initialize the scrollbar with some placeholder values.
        // The scrollbar will be updated with real values on _Initialize
        _scrollBar.Maximum(1);
        _scrollBar.ViewportSize(10);
        _scrollBar.IsTabStop(false);
        _scrollBar.SmallChange(1);
        _scrollBar.LargeChange(4);

        // Create the SwapChainPanel that will display our content
        Controls::SwapChainPanel swapChainPanel;
        swapChainPanel.SetValue(FrameworkElement::HorizontalAlignmentProperty(),
            box_value(HorizontalAlignment::Stretch));
        swapChainPanel.SetValue(FrameworkElement::HorizontalAlignmentProperty(),
            box_value(HorizontalAlignment::Stretch));

        swapChainPanel.SizeChanged(std::bind(&TermControl::_SwapChainSizeChanged,
                                             this,
                                             std::placeholders::_1,
                                             std::placeholders::_2));

        swapChainPanel.CompositionScaleChanged([this](auto&&, auto&&){
            _UpdateScaling();
        });

        // Initialize the terminal only once the swapchainpanel is loaded - that
        //      way, we'll be able to query the real pixel size it got on layout
        swapChainPanel.Loaded([&] (auto /*s*/, auto /*e*/){
            _InitializeTerminal();
        });

        container.Children().Append(swapChainPanel);
        container.Children().Append(_scrollBar);
        Controls::Grid::SetColumn(swapChainPanel, 0);
        Controls::Grid::SetColumn(_scrollBar, 1);

        _root = container;
        _swapChainPanel = swapChainPanel;
        _controlRoot.Content(_root);

        _ApplySettings();

        // These are important:
        // 1. When we get tapped, focus us
        _controlRoot.Tapped([&](auto&, auto& e) {
            _controlRoot.Focus(FocusState::Pointer);
            e.Handled(true);
        });
        // 2. Make sure we can be focused (why this isn't `Focusable` I'll never know)
        _controlRoot.IsTabStop(true);
        // 3. Actually not sure about this one. Maybe it isn't necessary either.
        _controlRoot.AllowFocusOnInteraction(true);

        // DON'T CALL _InitializeTerminal here - wait until the swap chain is loaded to do that.
    }

    // Method Description:
    // - Style our UI elements based on the values in our _settings, and set up
    //   other control-specific settings.
    //   * Sets up the background of the control with the provided BG color,
    //      acrylic or not, and if acrylic, then uses the opacity from _settings.
    //   * Gets the commandline out of the _settings and creates a ConhostConnection
    //      with the given commandline.
    // - Core settings will be passed to the terminal in _InitializeTerminal
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermControl::_ApplySettings()
    {
        winrt::Windows::UI::Color bgColor{};
        uint32_t bg = _settings.DefaultBackground();
        const auto R = GetRValue(bg);
        const auto G = GetGValue(bg);
        const auto B = GetBValue(bg);
        bgColor.R = R;
        bgColor.G = G;
        bgColor.B = B;
        bgColor.A = 255;

        if (_settings.UseAcrylic())
        {
            Media::AcrylicBrush acrylic{};
            acrylic.BackgroundSource(Media::AcrylicBackgroundSource::HostBackdrop);
            acrylic.FallbackColor(bgColor);
            acrylic.TintColor(bgColor);
            acrylic.TintOpacity(_settings.TintOpacity());
            _root.Background(acrylic);

            // If we're acrylic, we want to make sure that the default BG color
            // is transparent, so we can see the acrylic effect on text with the
            // default BG color.
            _settings.DefaultBackground(ARGB(0, R, G, B));
        }
        else
        {
            Media::SolidColorBrush solidColor{};
            solidColor.Color(bgColor);
            _root.Background(solidColor);
            _settings.DefaultBackground(RGB(R, G, B));
        }

        _connection = TerminalConnection::ConhostConnection(_settings.Commandline(), 30, 80);
    }

    TermControl::~TermControl()
    {
        _closing = true;
        // Don't let anyone else do something to the buffer.
        _terminal->LockForWriting();

        if (_connection != nullptr)
        {
            _connection.Close();
        }

        _renderer->TriggerTeardown();

        _swapChainPanel = nullptr;
        _root = nullptr;
        _connection = nullptr;
    }

    UIElement TermControl::GetRoot()
    {
        return _root;
    }

    Controls::UserControl TermControl::GetControl()
    {
        return _controlRoot;
    }

    void TermControl::SwapChainChanged()
    {
        if (!_initializedTerminal)
        {
            return;
        }

        auto chain = _renderEngine->GetSwapChain();
        _swapChainPanel.Dispatcher().RunAsync(CoreDispatcherPriority::High, [=]()
        {
            _terminal->LockForWriting();
            auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
            nativePanel->SetSwapChain(chain.Get());
            _terminal->UnlockForWriting();
        });
    }
    void TermControl::_UpdateScaling()
    {
        static auto s_lastCompScaleX = 0;
        static auto s_lastCompScaleY = 0;

        auto compScaleX = _swapChainPanel.CompositionScaleX();
        auto compScaleY = _swapChainPanel.CompositionScaleY();

        if (compScaleX == s_lastCompScaleX)
        {
            return;
        }

        s_lastCompScaleX = compScaleX;
        s_lastCompScaleY = compScaleY;

        _UpdateFont();

    }

    void TermControl::_UpdateFont()
    {
        auto compScaleX = _swapChainPanel.CompositionScaleX();
        auto compScaleY = _swapChainPanel.CompositionScaleY();
        // auto newDpi = 96.0 / compScaleX;
        auto newDpi = 96.0 * compScaleX;
        //auto newDpi = 96.0;

        // Prepare the font we want to use from the settings
        const auto* fontFace = _settings.FontFace().c_str();
        const auto* fallbackFontFace = L"Consolas";
        const short fontHeight = gsl::narrow<short>(_settings.FontSize());
        // The font width doesn't terribly matter, we'll only be using the height to look it up
        FontInfoDesired fi(fontFace, 0, 10, { 0, fontHeight }, 65001);
        FontInfo actual(fontFace, 0, 10, { 0, fontHeight }, 65001, false);
        try
        {
            // TODO: If the font doesn't exist, this doesn't actually fail.
            //      We need a way to gracefully fallback.
            _renderer->TriggerFontChange(newDpi, fi, actual);
        }
        catch (...)
        {
            // The font width doesn't terribly matter, we'll only be using the height to look it up
            FontInfoDesired fiFallback(fallbackFontFace, 0, 10, { 0, fontHeight }, 65001);
            FontInfo actualFallback(fallbackFontFace, 0, 10, { 0, fontHeight }, 65001, false);
            _renderer->TriggerFontChange(newDpi, fiFallback, actualFallback);
        }

        _renderer->TriggerRedrawAll();
    }


    void TermControl::_InitializeTerminal()
    {
        if (_initializedTerminal)
        {
            return;
        }

        const auto windowWidth = _swapChainPanel.ActualWidth();  // Width() and Height() are NaN?
        const auto windowHeight = _swapChainPanel.ActualHeight();

        _terminal = new ::Microsoft::Terminal::Core::Terminal();

        // First create the render thread.
        auto renderThread = std::make_unique<::Microsoft::Console::Render::RenderThread>();
        // Stash a local pointer to the render thread, so we can enable it after
        //       we hand off ownership to the renderer.
        auto* const localPointerToThread = renderThread.get();
        _renderer = std::make_unique<::Microsoft::Console::Render::Renderer>(_terminal, nullptr, 0, std::move(renderThread));
        ::Microsoft::Console::Render::IRenderTarget& renderTarget = *_renderer;

        // Set up the DX Engine
        auto dxEngine = std::make_unique<::Microsoft::Console::Render::DxEngine>();
        _renderer->AddRenderEngine(dxEngine.get());

        _UpdateFont();

        // Determine the size of the window, in characters.
        // Fist set up the dx engine with the window size in pixels.
        // Then, using the font, get the number of characters that can fit.
        // Resize our terminal connection to match that size, and initialize the terminal with that size.
        const auto viewInPixels = Viewport::FromDimensions({ 0, 0 },
                                                           { static_cast<short>(windowWidth), static_cast<short>(windowHeight) });
        THROW_IF_FAILED(dxEngine->SetWindowSize({ viewInPixels.Width(), viewInPixels.Height() }));
        const auto vp = dxEngine->GetViewportInCharacters(viewInPixels);
        const auto width = vp.Width();
        const auto height = vp.Height();
        _connection.Resize(height, width);

        // Override the default width and height to match the size of the swapChainPanel
        _settings.InitialCols(width);
        _settings.InitialRows(height);

        _terminal->CreateFromSettings(_settings, renderTarget);

        // Tell the DX Engine to notify us when the swap chain changes.
        dxEngine->SetCallback(std::bind(&TermControl::SwapChainChanged, this));

        THROW_IF_FAILED(dxEngine->Enable());
        _renderEngine = std::move(dxEngine);

        auto onRecieveOutputFn = [&](const hstring str) {
            _terminal->Write(str.c_str());
        };
        _connectionOutputEventToken = _connection.TerminalOutput(onRecieveOutputFn);

        auto inputFn = std::bind(&TermControl::_SendInputToConnection, this, std::placeholders::_1);
        _terminal->SetWriteInputCallback(inputFn);

        THROW_IF_FAILED(localPointerToThread->Initialize(_renderer.get()));

        auto chain = _renderEngine->GetSwapChain();
        _swapChainPanel.Dispatcher().RunAsync(CoreDispatcherPriority::High, [=]()
        {
            _terminal->LockConsole();
            auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
            nativePanel->SetSwapChain(chain.Get());
            _terminal->UnlockConsole();
        });

        // Set up the height of the ScrollViewer and the grid we're using to fake our scrolling height
        auto bottom = _terminal->GetViewport().BottomExclusive();
        auto bufferHeight = bottom;

        const auto originalMaximum = _scrollBar.Maximum();
        const auto originalMinimum = _scrollBar.Minimum();
        const auto originalValue = _scrollBar.Value();
        const auto originalViewportSize = _scrollBar.ViewportSize();

        _scrollBar.Maximum(bufferHeight - bufferHeight);
        _scrollBar.Minimum(0);
        _scrollBar.Value(0);
        _scrollBar.ViewportSize(bufferHeight);

        _scrollBar.ValueChanged([=](auto& sender, const Controls::Primitives::RangeBaseValueChangedEventArgs& args) {
            _ScrollbarChangeHandler(sender, args);
        });

        _root.PointerWheelChanged([=](auto& sender, const Input::PointerRoutedEventArgs& args) {
            _MouseWheelHandler(sender, args);
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
                                        Input::KeyRoutedEventArgs const& e) {
            this->_KeyHandler(sender, e);
        });

        _controlRoot.CharacterReceived([&](auto& sender,
                                           Input::CharacterReceivedRoutedEventArgs const& e) {
            this->_CharacterHandler(sender, e);
        });

        auto pfnTitleChanged = std::bind(&TermControl::_TerminalTitleChanged, this, std::placeholders::_1);
        _terminal->SetTitleChangedCallback(pfnTitleChanged);

        auto pfnScrollPositionChanged = std::bind(&TermControl::_TerminalScrollPositionChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        _terminal->SetScrollPositionChangedCallback(pfnScrollPositionChanged);

        // Focus the control here. If we do it up above (in _Create_), then the
        //      focus won't actually get passed to us. I believe this is because
        //      we're not technically a part of the UI tree yet, so focusing us
        //      becomes a no-op.
        _controlRoot.Focus(FocusState::Programmatic);

        _connection.Start();
        _initializedTerminal = true;
    }

    void TermControl::_CharacterHandler(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                       Input::CharacterReceivedRoutedEventArgs const& e)
    {
        if (_closing) return;

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

    void TermControl::_KeyHandler(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                 Input::KeyRoutedEventArgs const& e)
    {
        if (_closing) return;
        // This is super hacky - it seems as though these keys only seem pressed
        // every other time they're pressed
        CoreWindow foo = CoreWindow::GetForCurrentThread();
        // DONT USE
        //      != CoreVirtualKeyStates::None
        // OR
        //      == CoreVirtualKeyStates::Down
        // Sometimes with the key down, the state is Down | Locked.
        // Sometimes with the key up, the state is Locked.
        // IsFlagSet(Down) is the only correct solution.
        auto ctrlKeyState = foo.GetKeyState(VirtualKey::Control);
        auto shiftKeyState = foo.GetKeyState(VirtualKey::Shift);
        auto altKeyState = foo.GetKeyState(VirtualKey::Menu);

        auto ctrl = (ctrlKeyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
        auto shift = (shiftKeyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
        auto alt = (altKeyState & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;

        auto vkey = static_cast<WORD>(e.OriginalKey());

        bool handled = false;
        auto bindings = _settings.KeyBindings();
        if (bindings)
        {
            KeyChord chord(ctrl, alt, shift, vkey);
            handled = bindings.TryKeyChord(chord);
        }

        if (!handled)
        {
            _terminal->SendKeyEvent(vkey, ctrl, alt, shift);
        }
        else
        {
            e.Handled(true);
        }
    }

    void TermControl::_MouseWheelHandler(Windows::Foundation::IInspectable const& /*sender*/,
                                         Input::PointerRoutedEventArgs const& args)
    {
        auto delta = args.GetCurrentPoint(_root).Properties().MouseWheelDelta();

        const auto currentOffset = this->GetScrollOffset();

        // negative = down, positive = up
        // However, for us, the signs are flipped.
        const auto rowDelta = delta < 0 ? 1.0 : -1.0;

        // Experiment with scrolling MUCH faster, by scrolling a number of pixels
        const auto windowHeight = _swapChainPanel.ActualHeight();
        const auto viewRows = (double)_terminal->GetBufferHeight();
        const auto fontSize = windowHeight / viewRows;
        const auto biggerDelta = -1 * delta / fontSize;
        // TODO: Should we be getting some setting from the system
        //      for number of lines scrolled?
        // With one of the precision mouses, one click is always a multiple of 120,
        // but the "smooth scrolling" mode results in non-int values

        // Conhost seems to use four lines at a time, so we'll emulate that for now.
        double newValue = (4 * rowDelta) + (currentOffset);

        // Clear our expected scroll offset. The viewport will now move in
        //      response to our user input.
        _lastScrollOffset = std::nullopt;
        // The scroll bar's ValueChanged handler will actually move the viewport
        //      for us.
        _scrollBar.Value(static_cast<int>(newValue));
    }

    void TermControl::_ScrollbarChangeHandler(Windows::Foundation::IInspectable const& sender,
                                              Controls::Primitives::RangeBaseValueChangedEventArgs const& args)
    {
        const auto newValue = args.NewValue();

        // If we've stored a lastScrollOffset, that means the terminal has
        //      initiated some scrolling operation. We're responding to that event here.
        if (_lastScrollOffset.has_value())
        {
            // If this event's offset is the same as the last offset message
            //      we've sent, then clear out the expected offset. We do this
            //      because in that case, the message we're replying to was the
            //      last scroll event we raised.
            // Regardless, we're going to ignore this message, because the
            //      terminal is already in the scroll position it wants.
            const auto ourLastOffset = _lastScrollOffset.value();
            if (newValue == ourLastOffset)
            {
                _lastScrollOffset = std::nullopt;
            }
        }
        else
        {
            // This is a scroll event that wasn't initiated by the termnial
            //      itself - it was initiated by the mouse wheel, or the scrollbar.
            this->ScrollViewport(static_cast<int>(newValue));
        }
    }

    void TermControl::_SendInputToConnection(const std::wstring& wstr)
    {
        _connection.WriteInput(wstr);
    }

    void TermControl::_SwapChainSizeChanged(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                            SizeChangedEventArgs const& e)
    {
        if (!_initializedTerminal)
        {
            return;
        }

        _terminal->LockForWriting();
        const auto foundationSize = e.NewSize();
        SIZE classicSize;
        classicSize.cx = (LONG)foundationSize.Width;
        classicSize.cy = (LONG)foundationSize.Height;
        auto compScaleX = _swapChainPanel.CompositionScaleX();
        auto compScaleY = _swapChainPanel.CompositionScaleY();

        _UpdateScaling();
        // auto compScaleX = _swapChainPanel.CompositionScaleX();
        // auto compScaleY = _swapChainPanel.CompositionScaleY();
        // auto newDpi = 96.0 * compScaleX;
        // // THROW_IF_FAILED(_renderEngine->UpdateDpi((int)newDpi));

        // const auto* fontFace = _settings.FontFace().c_str();
        // const short fontHeight = gsl::narrow<short>(_settings.FontSize());
        // FontInfoDesired fi(fontFace, 0, 10, { 0, fontHeight }, 65001);
        // FontInfo actual(fontFace, 0, 10, { 0, fontHeight }, 65001, false);
        // _renderer->TriggerFontChange(newDpi, fi, actual);

        THROW_IF_FAILED(_renderEngine->SetWindowSize(classicSize));
        _renderer->TriggerRedrawAll();
        const auto vp = Viewport::FromInclusive(_renderEngine->GetDirtyRectInChars());

        // If this function succeeds with S_FALSE, then the terminal didn't
        //      actually change size. No need to notify the connection of this
        //      no-op.
        // TODO: Resizing is super fucked right now.
        // Try filling the buffer with output (esp with text with a colored BG)
        // then opening a new tab, and switch between them a couple times.
        // It seems like the entire buffer gets cleared.
        const HRESULT hr = _terminal->UserResize({ vp.Width(), vp.Height() });
        if (SUCCEEDED(hr) && hr != S_FALSE)
        {
            _connection.Resize(vp.Height(), vp.Width());
        }

        _terminal->UnlockForWriting();

    }

    void TermControl::_TerminalTitleChanged(const std::wstring_view& wstr)
    {
        _titleChangeHandlers(winrt::hstring{ wstr });
    }

    // Method Description:
    // - Update the postion and size of the scrollbar to match the given
    //      viewport top, viewport height, and buffer size.
    //   The change will be actually handled in _ScrollbarChangeHandler.
    //   This should be done on the UI thread. Make sure the caller is calling
    //      us in a RunAsync block.
    // Arguments:
    // - <none>
    // - viewTop: the top of the visible viewport, in rows. 0 indicates the top
    //      of the buffer.
    // - viewHeight: the height of the viewport in rows.
    // - bufferSize: the length of the buffer, in rows
    // Return Value:
    // - <none>
    void TermControl::_ScrollbarUpdater(Controls::Primitives::ScrollBar scrollBar,
                                        const int viewTop,
                                        const int viewHeight,
                                        const int bufferSize)
    {
        const auto hiddenContent = bufferSize - viewHeight;
        scrollBar.Maximum(hiddenContent);
        scrollBar.Minimum(0);
        scrollBar.ViewportSize(viewHeight);

        scrollBar.Value(viewTop);
    }

    // Method Description:
    // - Update the postion and size of the scrollbar to match the given
    //      viewport top, viewport height, and buffer size.
    //   Additionally fires a ScrollPositionChanged event for anyone who's
    //      registered an event handler for us.
    // Arguments:
    // - <none>
    // - viewTop: the top of the visible viewport, in rows. 0 indicates the top
    //      of the buffer.
    // - viewHeight: the height of the viewport in rows.
    // - bufferSize: the length of the buffer, in rows
    // Return Value:
    // - <none>
    void TermControl::_TerminalScrollPositionChanged(const int viewTop,
                                                     const int viewHeight,
                                                     const int bufferSize)
    {
        // Update our scrollbar
        _scrollBar.Dispatcher().RunAsync(CoreDispatcherPriority::Low, [=]() {
            _ScrollbarUpdater(_scrollBar, viewTop, viewHeight, bufferSize);
        });

        // Set this value as our next expected scroll position.
        _lastScrollOffset = { viewTop };
        _scrollPositionChangedHandlers(viewTop, viewHeight, bufferSize);
    }

    winrt::event_token TermControl::TitleChanged(TerminalControl::TitleChangedEventArgs const& handler)
    {
        return _titleChangeHandlers.add(handler);
    }

    void TermControl::TitleChanged(winrt::event_token const& token) noexcept
    {
        _titleChangeHandlers.remove(token);
    }

    winrt::event_token TermControl::ConnectionClosed(TerminalControl::ConnectionClosedEventArgs const& handler)
    {
        return _connectionClosedHandlers.add(handler);
    }

    void TermControl::ConnectionClosed(winrt::event_token const& token) noexcept
    {
        _connectionClosedHandlers.remove(token);
    }

    hstring TermControl::GetTitle()
    {
        if (!_initializedTerminal) return L"";

        hstring hstr(_terminal->GetConsoleTitle());
        return hstr;
    }
    void TermControl::Close()
    {
        if (!_closing)
        {
            this->~TermControl();
        }
    }

    winrt::event_token TermControl::ScrollPositionChanged(TerminalControl::ScrollPositionChangedEventArgs const& handler)
    {
        return _scrollPositionChangedHandlers.add(handler);
    }

    void TermControl::ScrollPositionChanged(winrt::event_token const& token) noexcept
    {
        _scrollPositionChangedHandlers.remove(token);
    }
    void TermControl::ScrollViewport(int viewTop)
    {
        _terminal->UserScrollViewport(viewTop);
    }
    int TermControl::GetScrollOffset()
    {
        return _terminal->GetScrollOffset();
    }

}
