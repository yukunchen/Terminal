/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once
#include <winrt/TerminalControl.h>

class Tab
{
    Tab();

private:
    winrt::TerminalControl::TermControl _control;
    bool _focused;
    winrt::Windows::UI::Xaml::Controls::Button _tabButton;
};
