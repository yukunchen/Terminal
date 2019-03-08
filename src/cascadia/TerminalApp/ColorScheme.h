/*++
Copyright (c) Microsoft Corporation

Module Name:
- ColorScheme.hpp

Abstract:
- A color scheme is a single set of colors to use as the terminal colors. These
    schemes are named, and can be used to quickly change all the colors of the
    terminal to another scheme.

Author(s):
- Mike Griese - March 2019

--*/
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "../../inc/conattrs.hpp"
#include <conattrs.hpp>

namespace Microsoft::Terminal::TerminalApp
{
    class ColorScheme;
};

class Microsoft::Terminal::TerminalApp::ColorScheme
{

public:
    ColorScheme();
    ColorScheme(std::wstring name, COLORREF defaultFg, COLORREF defaultBg);
    ~ColorScheme();

    void ApplyScheme(winrt::Microsoft::Terminal::Settings::TerminalSettings terminalSettings) const;

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<ColorScheme> FromJson(winrt::Windows::Data::Json::JsonObject json);

    std::wstring_view GetName() const noexcept;
    std::array<COLORREF, COLOR_TABLE_SIZE>& GetTable() noexcept;
    COLORREF GetForeground() const noexcept;
    COLORREF GetBackground() const noexcept;

private:
    std::wstring _schemeName;
    std::array<COLORREF, COLOR_TABLE_SIZE> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
