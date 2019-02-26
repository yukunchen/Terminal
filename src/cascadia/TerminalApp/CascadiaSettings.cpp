/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "CascadiaSettings.h"
#include "../TerminalControl/Utils.h"

using namespace ::Microsoft::Terminal::Core;
using namespace ::Microsoft::Terminal::TerminalControl;
using namespace ::Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace winrt::Microsoft::Terminal::TerminalApp;

CascadiaSettings::CascadiaSettings() :
    _globals{},
    _profiles{}
{

}

CascadiaSettings::~CascadiaSettings()
{

}

void CascadiaSettings::LoadAll()
{
    auto defaultProfile = std::make_unique<Profile>();
    // As a test:
    defaultProfile->_fontFace = L"Ubuntu Mono";

    _profiles.push_back(std::move(defaultProfile));

    AppKeyBindings& keyBindings = _globals._keybindings;
    // Set up spme basic default keybindings
    // TODO: read our settings from some source, and configure
    //      keychord,action pairings from that file
    keyBindings.SetKeyBinding(ShortcutAction::NewTab,
                               KeyChord{ KeyModifiers::Ctrl, (int)'T' });

    keyBindings.SetKeyBinding(ShortcutAction::CloseTab,
                               KeyChord{ KeyModifiers::Ctrl, (int)'W' });


}

void CascadiaSettings::SaveAll()
{

}


void _SetFromCoreSettings(const Settings& sourceSettings,
                          TerminalSettings terminalSettings)
{
    // TODO Color Table
    terminalSettings.DefaultForeground(sourceSettings.DefaultForeground());
    terminalSettings.DefaultBackground(sourceSettings.DefaultBackground());
    terminalSettings.HistorySize(sourceSettings.HistorySize());
    terminalSettings.InitialRows(sourceSettings.InitialRows());
    terminalSettings.InitialCols(sourceSettings.InitialCols());
    terminalSettings.SnapOnInput(sourceSettings.SnapOnInput());
}

void _SetFromProfile(const Profile& sourceProfile,
                     TerminalSettings terminalSettings)
{
    // Fill in the Terminal Setting's CoreSettings from the profile
    _SetFromCoreSettings(sourceProfile._coreSettings, terminalSettings);

    // Fill in the remaining properties from the profile
    terminalSettings.UseAcrylic(sourceProfile._useAcrylic);
    terminalSettings.FontFace(sourceProfile._fontFace);
    terminalSettings.FontSize(sourceProfile._fontSize);
}


TerminalSettings CascadiaSettings::MakeSettings(std::optional<GUID> profileGuid)
{
    TerminalSettings result{};

    // Place our appropriate global settings into the Terminal Settings
    result.KeyBindings(_globals._keybindings);

    auto& profile = _profiles[0];

    _SetFromProfile(*profile.get(), result);

    return result;
}

std::vector<GUID> CascadiaSettings::GetProfileGuids()
{
    std::vector<GUID> guids;
    for (auto& profile : _profiles)
    {
        guids.push_back(profile._guid);
    }
    return guids;
}
