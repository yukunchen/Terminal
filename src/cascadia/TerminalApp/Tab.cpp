/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Tab.h"

using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Core;

Tab::Tab(winrt::TerminalControl::TermControl control) :
    _control{ control },
    _focused{ false },
    _tabButton{ nullptr }
{
    _MakeTabButton();
}

Tab::~Tab()
{
    if (_control != nullptr)
    {
        _control.Close();
        _control = nullptr;
    }
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
