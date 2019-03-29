/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>

class Tab
{

public:
    Tab(winrt::Microsoft::Terminal::TerminalControl::TermControl control);
    ~Tab();

    winrt::Microsoft::UI::Xaml::Controls::TabViewItem GetTabViewItem();
    winrt::Microsoft::Terminal::TerminalControl::TermControl GetTerminalControl();

    bool IsFocused();
    void SetFocused(bool focused);

    void Scroll(int delta);

private:
    winrt::Microsoft::Terminal::TerminalControl::TermControl _control;
    bool _focused;
    winrt::Microsoft::UI::Xaml::Controls::TabViewItem _tabViewItem;

    void _MakeTabViewItem();
    void _Focus();
};
