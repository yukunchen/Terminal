
#pragma once
#include "ColorScheme.h"

// #include <winrt/Microsoft.Terminal.TerminalApp.h>

namespace Microsoft::Terminal::TerminalApp
{
    class Profile;
};

class Microsoft::Terminal::TerminalApp::Profile
{

public:
    Profile();
    ~Profile();

    winrt::Microsoft::Terminal::TerminalApp::TerminalSettings CreateTerminalSettings(std::vector<std::unique_ptr<::Microsoft::Terminal::TerminalApp::ColorScheme>>& schemes) const;

    winrt::Windows::Data::Json::JsonObject ToJson() const;

// private:

    GUID _guid;
    std::wstring _name;

    std::optional<std::wstring> _schemeName;

    uint32_t _defaultForeground;
    uint32_t _defaultBackground;
    std::array<uint32_t, 16> _colorTable;
    int32_t _historySize;
    int32_t _initialRows;
    int32_t _initialCols;
    bool _snapOnInput;

    std::wstring _commandline;
    std::wstring _fontFace;
    int32_t _fontSize;
    double _acrylicTransparency;
    bool _useAcrylic;
    bool _showScrollbars;

};
