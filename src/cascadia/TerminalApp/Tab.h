/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once
#include <winrt/TerminalControl.h>

class Tab
{

public:
    Tab(winrt::TerminalControl::TermControl control);
    ~Tab();

    winrt::Windows::UI::Xaml::Controls::Button GetTabButton();
    winrt::TerminalControl::TermControl GetTerminalControl();

    bool IsFocused();
    void SetFocused(bool focused);

private:
    winrt::TerminalControl::TermControl _control;
    bool _focused;
    winrt::Windows::UI::Xaml::Controls::Button _tabButton;

    void _MakeTabButton();
    void _Focus();
};
