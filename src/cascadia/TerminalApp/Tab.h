/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>

class Tab
{

public:
    Tab(winrt::Microsoft::Terminal::TerminalControl::TermControl control);
    ~Tab();

    winrt::Windows::UI::Xaml::Controls::Button GetTabButton();
    winrt::Microsoft::Terminal::TerminalControl::TermControl GetTerminalControl();

    bool IsFocused();
    void SetFocused(bool focused);

private:
    winrt::Microsoft::Terminal::TerminalControl::TermControl _control;
    bool _focused;
    winrt::Windows::UI::Xaml::Controls::Button _tabButton;

    void _MakeTabButton();
    void _Focus();
};
