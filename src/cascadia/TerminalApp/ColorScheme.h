
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
    ~ColorScheme();

    void ApplyScheme(winrt::Microsoft::Terminal::TerminalApp::TerminalSettings terminalSettings) const;

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<ColorScheme> FromJson(winrt::Windows::Data::Json::JsonObject json);

private:
    std::wstring _schemeName;
    std::array<COLORREF, COLOR_TABLE_SIZE> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
