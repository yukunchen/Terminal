/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Tab.h"

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;

Tab::Tab(winrt::Microsoft::Terminal::TerminalControl::TermControl control) :
    _control{ control },
    _focused{ false },
    _tabButton{ nullptr }
{
    _MakeTabButton();
}

Tab::~Tab()
{
    // When we're destructed, winrt will automatically decrement the refcount
    // of our terminalcontrol.
    // Assuming that refcount hits 0, it'll destruct it on it's own, including
    //      calling Close on the terminal and connection.
}

void Tab::_MakeTabButton()
{

    _tabButton = Controls::Button();
    const auto title = _control.GetTitle();
    // For whatever terrible reason, button.Content() doesn't accept an hstring or wstring.
    // You have to manually make an IInspectable from the hstring
    winrt::Windows::Foundation::IInspectable value = winrt::Windows::Foundation::PropertyValue::CreateString(title);
    _tabButton.Content(value);


    _control.TitleChanged([=](auto newTitle){
        _tabButton.Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [=](){
            winrt::Windows::Foundation::IInspectable value = winrt::Windows::Foundation::PropertyValue::CreateString(newTitle);
            _tabButton.Content(value);
        });
    });

    _tabButton.FontSize(12);
}

winrt::Microsoft::Terminal::TerminalControl::TermControl Tab::GetTerminalControl()
{
    return _control;
}

winrt::Windows::UI::Xaml::Controls::Button Tab::GetTabButton()
{
    return _tabButton;
}


bool Tab::IsFocused()
{
    return _focused;
}

void Tab::SetFocused(bool focused)
{
    _focused = focused;

    if (_focused)
    {
        _Focus();
    }
}

void Tab::_Focus()
{
    _tabButton.Background(Media::SolidColorBrush{winrt::Windows::UI::ColorHelper::FromArgb(255, 0x4f, 0x4f, 0x4f)});
    _tabButton.BorderBrush(Media::SolidColorBrush{winrt::Windows::UI::Colors::Blue()});
    _tabButton.BorderThickness(Thickness{0, 2, 0, 0});
    _focused = true;
    _control.GetControl().Focus(FocusState::Programmatic);
}

// Method Description:
// - Move the viewport of the terminal up or down a number of lines. Negative
//      values of `delta` will move the view up, and positive values will move
//      the viewport down.
// Arguments:
// - delta: a number of lines to move the viewport relative to the current viewport.
// Return Value:
// - <none>
void Tab::Scroll(int delta)
{
    _control.GetControl().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [=](){
        const auto currentOffset = _control.GetScrollOffset();
        _control.ScrollViewport(currentOffset + delta);
    });
}
