
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "../cascadia/TerminalCore/Settings.h"

namespace Microsoft::Terminal::TerminalApp
{
    class Profile;
};

class Microsoft::Terminal::TerminalApp::Profile
{

public:
    Profile();
    ~Profile();


// private:

    GUID _guid;
    std::wstring _name;

    ::Microsoft::Terminal::Core::Settings _coreSettings;

    std::wstring _commandline;
    std::wstring _fontFace;
    int32_t _fontSize;
    double _acrylicTransparency;
    bool _useAcrylic;
    bool _showScrollbars;

};
