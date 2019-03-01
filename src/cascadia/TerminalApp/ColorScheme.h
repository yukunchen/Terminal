
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>


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
    // void ApplyScheme(winrt::Microsoft::Terminal::TerminalControl::TerminalSettings terminalSettings);
    winrt::Windows::Data::Json::JsonObject ToJson() const;

    static ColorScheme FromJson(winrt::Windows::Data::Json::JsonObject json);

// private:
    std::wstring _schemeName;
    std::array<COLORREF, 16> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
