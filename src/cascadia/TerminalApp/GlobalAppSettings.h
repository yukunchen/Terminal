
#pragma once
#include "AppKeyBindings.h"

namespace Microsoft::Terminal::TerminalApp
{
    class GlobalAppSettings;
};

class Microsoft::Terminal::TerminalApp::GlobalAppSettings
{

public:
    GlobalAppSettings();
    ~GlobalAppSettings();


// private:
    GUID _defaultProfile;
    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings _keybindings;

    // std::vector<ColorScheme> _colorSchemes;

    bool _showStatusline;
};
