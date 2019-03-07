/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "GlobalAppSettings.h"

using namespace Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalApp;

GlobalAppSettings::GlobalAppSettings() :
    _keybindings{},
    _colorSchemes{},
    _defaultProfile{}
{

}

GlobalAppSettings::~GlobalAppSettings()
{

}

const std::vector<std::unique_ptr<ColorScheme>>& GlobalAppSettings::GetColorSchemes() const noexcept
{
    return _colorSchemes;
}


std::vector<std::unique_ptr<ColorScheme>>& GlobalAppSettings::GetColorSchemes() noexcept
{
    return _colorSchemes;
}

void GlobalAppSettings::SetDefaultProfile(const GUID defaultProfile) noexcept
{
    _defaultProfile = defaultProfile;
}

GUID GlobalAppSettings::GetDefaultProfile() const noexcept
{
    return _defaultProfile;
}

AppKeyBindings GlobalAppSettings::GetKeybindings() const noexcept
{
    return _keybindings;
}
