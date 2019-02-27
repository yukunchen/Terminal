
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "../cascadia/TerminalCore/Settings.h"
#include "ColorScheme.h"

namespace Microsoft::Terminal::TerminalApp
{
    class Profile;
};

class Microsoft::Terminal::TerminalApp::Profile
{

public:
    Profile();
    ~Profile();

    winrt::Microsoft::Terminal::TerminalControl::TerminalSettings CreateTerminalSettings(std::vector<std::unique_ptr<::Microsoft::Terminal::TerminalApp::ColorScheme>>& schemes) const;
// private:

    GUID _guid;
    std::wstring _name;

    std::optional<std::wstring> _schemeName;

    ::Microsoft::Terminal::Core::Settings _coreSettings;

    std::wstring _commandline;
    std::wstring _fontFace;
    int32_t _fontSize;
    double _acrylicTransparency;
    bool _useAcrylic;
    bool _showScrollbars;

};
