
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>

namespace Microsoft::Terminal::TerminalApp
{
    class ColorScheme;
};

class Microsoft::Terminal::TerminalApp::ColorScheme
{

public:
    ColorScheme();
    ~ColorScheme();

    void ApplyScheme(winrt::Microsoft::Terminal::TerminalControl::TerminalSettings terminalSettings) const;
    // void ApplyScheme(winrt::Microsoft::Terminal::TerminalControl::TerminalSettings terminalSettings);
    winrt::Windows::Data::Json::JsonObject ToJson() const;

// private:
    std::wstring _schemeName;
    std::array<COLORREF, 16> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
