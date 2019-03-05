
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

    const std::vector<std::unique_ptr<ColorScheme>>& GetColorSchemes() const noexcept;
    std::vector<std::unique_ptr<ColorScheme>>& GetColorSchemes() noexcept;
    void SetDefaultProfile(const GUID defaultProfile) noexcept;
    GUID GetDefaultProfile() const noexcept;

    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings GetKeybindings() noexcept;

private:
    GUID _defaultProfile;
    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings _keybindings;

    std::vector<std::unique_ptr<ColorScheme>> _colorSchemes;

    bool _showStatusline;
};
