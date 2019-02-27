
#pragma once

namespace Microsoft::Terminal::TerminalApp
{
    class ColorScheme;
};

class Microsoft::Terminal::TerminalApp::ColorScheme
{

public:
    ColorScheme();
    ~ColorScheme();

// private:
    std::wstring _schemeName;
    std::array<COLORREF, 16> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
