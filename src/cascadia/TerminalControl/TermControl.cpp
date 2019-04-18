/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/
#include "pch.h"
#include "TermControl.h"
#include <argb.h>
#include <DefaultSettings.h>
#include <unicode.hpp>
#include <Utf16Parser.hpp>

using namespace ::Microsoft::Console::Types;
using namespace ::Microsoft::Terminal::Core;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;
using namespace winrt::Microsoft::Terminal::Settings;

namespace winrt::Microsoft::Terminal::TerminalControl::implementation
{

    TermControl::TermControl() :
        TermControl(Settings::TerminalSettings{})
    {
    }

    TermControl::TermControl(Settings::IControlSettings settings) :
        _connection{ TerminalConnection::ConhostConnection(winrt::to_hstring("cmd.exe"), winrt::hstring(), 30, 80) },
        _initializedTerminal{ false },
        _root{ nullptr },
        _controlRoot{ nullptr },
        _swapChainPanel{ nullptr },
        _settings{ settings },
        _closing{ false },
        _lastScrollOffset{ std::nullopt },
        _desiredFont{ DEFAULT_FONT_FACE.c_str(), 0, 10, { 0, DEFAULT_FONT_SIZE }, CP_UTF8 },
        _actualFont{ DEFAULT_FONT_FACE.c_str(), 0, 10, { 0, DEFAULT_FONT_SIZE }, CP_UTF8, false },
        _touchAnchor{ std::nullopt },
        _leadingSurrogate{}
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


        swapChainPanel.SizeChanged({ this, &TermControl::_SwapChainSizeChanged });
        swapChainPanel.CompositionScaleChanged({ this, &TermControl::_SwapChainScaleChanged });

        // Initialize the terminal only once the swapchainpanel is loaded - that
        //      way, we'll be able to query the real pixel size it got on layout
        swapChainPanel.Loaded([this] (auto /*s*/, auto /*e*/){
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
        _controlRoot.Tapped([this](auto&, auto& e) {
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

        // Initialize our font information.
        const auto* fontFace = _settings.FontFace().c_str();
        const short fontHeight = gsl::narrow<short>(_settings.FontSize());
        // The font width doesn't terribly matter, we'll only be using the
        //      height to look it up
        // The other params here also largely don't matter.
        //      The family is only used to determine if the font is truetype or
        //      not, but DX doesn't use that info at all.
        //      The Codepage is additionally not actually used by the DX engine at all.
        _actualFont = { fontFace, 0, 10, { 0, fontHeight }, CP_UTF8, false };
        _desiredFont = { _actualFont };

        _connection = TerminalConnection::ConhostConnection(_settings.Commandline(), _settings.StartingDirectory(), 30, 80);
    }

    TermControl::~TermControl()
    {
        _closing = true;
        // Don't let anyone else do something to the buffer.
        auto lock = _terminal->LockForWriting();

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
            auto lock = _terminal->LockForWriting();
            auto nativePanel = _swapChainPanel.as<ISwapChainPanelNative>();
            nativePanel->SetSwapChain(chain.Get());
        });
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

        // Initialize our font with the renderer
        // We don't have to care about DPI. We'll get a change message immediately if it's not 96
        // and react accordingly.
        _UpdateFont();

        const COORD windowSize{ static_cast<short>(windowWidth), static_cast<short>(windowHeight) };

        // Fist set up the dx engine with the window size in pixels.
        // Then, using the font, get the number of characters that can fit.
        // Resize our terminal connection to match that size, and initialize the terminal with that size.
        const auto viewInPixels = Viewport::FromDimensions({ 0, 0 }, windowSize);
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

        auto onRecieveOutputFn = [this](const hstring str) {
            _terminal->Write(str.c_str());
        };
        _connectionOutputEventToken = _connection.TerminalOutput(onRecieveOutputFn);

        auto inputFn = std::bind(&TermControl::_SendInputToConnection, this, std::placeholders::_1);
        _terminal->SetWriteInputCallback(inputFn);

        THROW_IF_FAILED(localPointerToThread->Initialize(_renderer.get()));

        auto chain = _renderEngine->GetSwapChain();
        _swapChainPanel.Dispatcher().RunAsync(CoreDispatcherPriority::High, [this, chain]()
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
        _scrollBar.ValueChanged({ this, &TermControl::_ScrollbarChangeHandler });

        _root.PointerWheelChanged({ this, &TermControl::_MouseWheelHandler });
        _root.PointerPressed({ this, &TermControl::_MouseClickHandler });
        _root.PointerMoved({ this, &TermControl::_MouseMovedHandler });
        _root.PointerReleased({ this, &TermControl::_PointerReleasedHandler });

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
        // These two handlers _must_ be on _controlRoot, not _root.
        _controlRoot.PreviewKeyDown({this, &TermControl::_KeyHandler });
        _controlRoot.CharacterReceived({this, &TermControl::_CharacterHandler });

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
        if (_closing)
        {
            return;
        }

        const auto ch = e.Character();
        if (ch == L'\x08')
        {
            // We want Backspace to be handled by KeyHandler, so the
            //      terminalInput can translate it into a \x7f. So, do nothing
            //      here, so we don't end up sending both a BS and a DEL to the
            //      terminal.
            return;
        }
        else if (Utf16Parser::IsLeadingSurrogate(ch))
        {
            if (_leadingSurrogate.has_value())
            {
                // we already were storing a leading surrogate but we got another one. Go ahead and send the
                // saved surrogate piece and save the new one
                auto hstr = to_hstring(_leadingSurrogate.value());
                _connection.WriteInput(hstr);
            }
            // save the leading portion of a surrogate pair so that they can be sent at the same time
            _leadingSurrogate.emplace(ch);
        }
        else if (_leadingSurrogate.has_value())
        {
            std::wstring wstr;
            wstr.reserve(2);
            wstr.push_back(_leadingSurrogate.value());
            wstr.push_back(ch);
            _leadingSurrogate.reset();

            auto hstr = to_hstring(wstr.c_str());
            _connection.WriteInput(hstr);
        }
        else
        {
            auto hstr = to_hstring(ch);
            _connection.WriteInput(hstr);
        }
        e.Handled(true);
    }

    void TermControl::_KeyHandler(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                  Input::KeyRoutedEventArgs const& e)
    {
        // mark event as handled and do nothing if...
        //   - closing
        //   - key modifier is pressed
        // NOTE: for key combos like CTRL + C, two events are fired (one for CTRL, one for 'C'). We care about the 'C' event and then check for key modifiers below.
        if (_closing || e.OriginalKey() == VirtualKey::Control || e.OriginalKey() == VirtualKey::Shift || e.OriginalKey() == VirtualKey::Menu)
        {
            e.Handled(true);
            return;
        }
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

        auto ctrl = WI_IsFlagSet(ctrlKeyState, CoreVirtualKeyStates::Down);
        auto shift = WI_IsFlagSet(shiftKeyState, CoreVirtualKeyStates::Down);
        auto alt = WI_IsFlagSet(altKeyState, CoreVirtualKeyStates::Down);

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
            _terminal->ClearSelection();
            _terminal->SendKeyEvent(vkey, ctrl, alt, shift);
        }
        else
        {
            e.Handled(true);
        }
    }

    // Method Description:
    // - handle a mouse click event. Begin selection process.
    // Arguments:
    // - sender: not used
    // - args: event data
    // Return Value:
    // - N/A
    void TermControl::_MouseClickHandler(Windows::Foundation::IInspectable const& /*sender*/,
                                         Input::PointerRoutedEventArgs const& args)
    {
        const auto ptr = args.Pointer();
        const auto point = args.GetCurrentPoint(_root);

        if (ptr.PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Mouse)
        {

            if (point.Properties().IsLeftButtonPressed())
            {
                const auto cursorPosition = point.Position();

                const auto fontSize = _renderer->GetFontSize();

                const COORD terminalPosition = {
                    static_cast<SHORT>(cursorPosition.X / fontSize.X),
                    static_cast<SHORT>(cursorPosition.Y / fontSize.Y)
                };

                // save location before rendering
                _terminal->SetSelectionAnchor(terminalPosition);

                // handle ALT key
                const auto modifiers = args.KeyModifiers();
                const auto altEnabled = WI_IsFlagSet(modifiers, VirtualKeyModifiers::Menu);
                _terminal->SetBoxSelection(altEnabled);

                _renderer->TriggerSelection();
            }
        }
        else if (ptr.PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Touch)
        {
            const auto contactRect = point.Properties().ContactRect();
            // Set our touch rect, to start a pan.
            _touchAnchor = winrt::Windows::Foundation::Point{ contactRect.X, contactRect.Y };
        }

        args.Handled(true);
    }

    // Method Description:
    // - handle a mouse moved event. Specifically handling mouse drag to update selection process.
    // Arguments:
    // - sender: not used
    // - args: event data
    // Return Value:
    // - N/A
    void TermControl::_MouseMovedHandler(Windows::Foundation::IInspectable const& /*sender*/,
                                         Input::PointerRoutedEventArgs const& args)
    {
        const auto ptr = args.Pointer();
        const auto point = args.GetCurrentPoint(_root);

        if (ptr.PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Mouse)
        {
            if (point.Properties().IsLeftButtonPressed())
            {
                const auto cursorPosition = point.Position();

                const auto fontSize = _renderer->GetFontSize();

                const COORD terminalPosition = {
                    static_cast<SHORT>(cursorPosition.X / fontSize.X),
                    static_cast<SHORT>(cursorPosition.Y / fontSize.Y)
                };

                // save location (for rendering) + render
                _terminal->SetEndSelectionPosition(terminalPosition);
                _renderer->TriggerSelection();
            }
        }
        else if (ptr.PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Touch && _touchAnchor)
        {
            const auto contactRect = point.Properties().ContactRect();
            winrt::Windows::Foundation::Point newTouchPoint{ contactRect.X, contactRect.Y };
            const auto anchor = _touchAnchor.value();

            // Get the difference between the point we've dragged to and the start of the touch.
            const float fontHeight = float(_renderer->GetFontSize().Y);
            const float dy = newTouchPoint.Y - anchor.Y;

            // If we've moved more than one row of text, we'll want to scroll the viewport
            if (std::abs(dy) > fontHeight)
            {
                // Multiply by -1, because moving the touch point down will
                // create a positive delta, but we want the viewport to move up,
                // so we'll need a negative scroll amount (and the inverse for
                // panning down)
                const float numRows = -1.0f * (dy / fontHeight);

                const auto currentOffset = this->GetScrollOffset();
                const double newValue = (numRows) + (currentOffset);

                // Clear our expected scroll offset. The viewport will now move
                //      in response to our user input.
                _lastScrollOffset = std::nullopt;
                _scrollBar.Value(static_cast<int>(newValue));

                // Use this point as our new scroll anchor.
                _touchAnchor = newTouchPoint;
            }
        }
        args.Handled(true);
    }

    // Method Description:
    // - Event handler for the PointerReleased event. We use this to de-anchor
    //   touch events, to stop scrolling via touch.
    // Arguments:
    // - sender: not used
    // - args: event data
    // Return Value:
    // - <none>
    void TermControl::_PointerReleasedHandler(Windows::Foundation::IInspectable const& /*sender*/,
                                              Input::PointerRoutedEventArgs const& args)
    {
        const auto ptr = args.Pointer();

        if (ptr.PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Touch)
        {
            _touchAnchor = std::nullopt;
        }

        args.Handled(true);
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

    // Method Description:
    // - Update the font with the renderer. This will be called either when the
    //      font changes or the DPI changes, as DPI changes will necessitate a
    //      font change. This method will *not* change the buffer/viewport size
    //      to account for the new glyph dimensions. Callers should make sure to
    //      appropriately call _DoResize after this method is called.
    // Arguments:
    // - <none>
    // Return Value:
    // - <none>
    void TermControl::_UpdateFont()
    {
        auto lock = _terminal->LockForWriting();

        const int newDpi = static_cast<int>(static_cast<double>(USER_DEFAULT_SCREEN_DPI) * _swapChainPanel.CompositionScaleX());

        // TODO: MSFT:20895307 If the font doesn't exist, this doesn't
        //      actually fail. We need a way to gracefully fallback.
        _renderer->TriggerFontChange(newDpi, _desiredFont, _actualFont);

    }

    // Method Description:
    // - Triggered when the swapchain changes size. We use this to resize the
    //      terminal buffers to match the new visible size.
    // Arguments:
    // - e: a SizeChangedEventArgs with the new dimensions of the SwapChainPanel
    // Return Value:
    // - <none>
    void TermControl::_SwapChainSizeChanged(winrt::Windows::Foundation::IInspectable const& /*sender*/,
                                            SizeChangedEventArgs const& e)
    {
        if (!_initializedTerminal)
        {
            return;
        }

        auto lock = _terminal->LockForWriting();

        const auto foundationSize = e.NewSize();

        _DoResize(foundationSize.Width, foundationSize.Height);
    }

    void TermControl::_SwapChainScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel const& sender,
                                             Windows::Foundation::IInspectable const& /*args*/)
    {
        const auto scale = sender.CompositionScaleX();
        const auto dpi = (int)(scale * USER_DEFAULT_SCREEN_DPI);

        // TODO: MSFT: 21169071 - Shouldn't this all happen through _renderer and trigger the invalidate automatically on DPI change?
        THROW_IF_FAILED(_renderEngine->UpdateDpi(dpi));
        _renderer->TriggerRedrawAll();
    }

    // Method Description:
    // - Process a resize event that was initiated by the user. This can either be due to the user resizing the window (causing the swapchain to resize) or due to the DPI changing (causing us to need to resize the buffer to match)
    // Arguments:
    // - newWidth: the new width of the swapchain, in pixels.
    // - newHeight: the new height of the swapchain, in pixels.
    // Return Value:
    // - <none>
    void TermControl::_DoResize(const double newWidth, const double newHeight)
    {
        SIZE size;
        size.cx = static_cast<long>(newWidth);
        size.cy = static_cast<long>(newHeight);

        // Tell the dx engine that our window is now the new size.
        THROW_IF_FAILED(_renderEngine->SetWindowSize(size));

        // Invalidate everything
        _renderer->TriggerRedrawAll();

        // Convert our new dimensions to characters
        const auto viewInPixels = Viewport::FromDimensions({ 0, 0 },
                                                           { static_cast<short>(size.cx), static_cast<short>(size.cy) });
        const auto vp = _renderEngine->GetViewportInCharacters(viewInPixels);

        // If this function succeeds with S_FALSE, then the terminal didn't
        //      actually change size. No need to notify the connection of this
        //      no-op.
        // TODO: MSFT:20642295 Resizing the buffer will corrupt it
        // I believe we'll need support for CSI 2J, and additionally I think
        //      we're resetting the viewport to the top
        const HRESULT hr = _terminal->UserResize({ vp.Width(), vp.Height() });
        if (SUCCEEDED(hr) && hr != S_FALSE)
        {
            _connection.Resize(vp.Height(), vp.Width());
        }
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
