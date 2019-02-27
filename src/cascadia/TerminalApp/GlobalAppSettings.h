
#pragma once
#include "AppKeyBindings.h"
#include "ColorScheme.h"

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

    std::vector<std::unique_ptr<ColorScheme>> _colorSchemes;

    bool _showStatusline;
};
