/*++
Copyright (c) Microsoft Corporation

Module Name:
- CascadiaSettings.hpp

Abstract:
- This class encapsulates all of the settings that are global to the app, and
    not a part of any particular profile.

Author(s):
- Mike Griese - March 2019

--*/
#pragma once
#include "AppKeyBindings.h"
#include "ColorScheme.h"

namespace Microsoft::Terminal::TerminalApp
{
    class GlobalAppSettings;
};

class Microsoft::Terminal::TerminalApp::GlobalAppSettings final
{

public:
    GlobalAppSettings();
    ~GlobalAppSettings();

    const std::vector<ColorScheme>& GetColorSchemes() const noexcept;
    std::vector<ColorScheme>& GetColorSchemes() noexcept;
    void SetDefaultProfile(const GUID defaultProfile) noexcept;
    GUID GetDefaultProfile() const noexcept;

    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings GetKeybindings() const noexcept;

private:
    GUID _defaultProfile;
    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings _keybindings;

    std::vector<ColorScheme> _colorSchemes;

    bool _showStatusline;
};
