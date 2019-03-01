/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include <argb.h>
#include "CascadiaSettings.h"
#include "../TerminalControl/Utils.h"
#include "../../types/inc/utils.hpp"

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

void CascadiaSettings::_CreateDefaultSchemes()
{
    const auto TABLE_SIZE = gsl::narrow<ptrdiff_t>(16);

    auto campbellScheme = std::make_unique<ColorScheme>();
    campbellScheme->_schemeName = L"Campbell";
    campbellScheme->_defaultForeground = RGB(242, 242, 242);
    campbellScheme->_defaultBackground = RGB(12, 12, 12);
    auto campbellSpan = gsl::span<COLORREF>(&campbellScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(campbellSpan);
    Microsoft::Console::Utils::SetColorTableAlpha(campbellSpan, 0xff);

    auto solarizedDarkScheme = std::make_unique<ColorScheme>();
    solarizedDarkScheme->_schemeName = L"Solarized Dark";
    solarizedDarkScheme->_defaultBackground = RGB(  7, 54, 66);
    solarizedDarkScheme->_defaultForeground = RGB(253, 246, 227);
    auto solarizedDarkSpan = gsl::span<COLORREF>(&solarizedDarkScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(solarizedDarkSpan);
    solarizedDarkScheme->_table[0]  = RGB(  7, 54, 66);
    solarizedDarkScheme->_table[1]  = RGB(211, 1, 2);
    solarizedDarkScheme->_table[2]  = RGB(133, 153, 0);
    solarizedDarkScheme->_table[3]  = RGB(181, 137, 0);
    solarizedDarkScheme->_table[4]  = RGB( 38, 139, 210);
    solarizedDarkScheme->_table[5]  = RGB(211, 54, 130);
    solarizedDarkScheme->_table[6]  = RGB( 42, 161, 152);
    solarizedDarkScheme->_table[7]  = RGB(238, 232, 213);
    solarizedDarkScheme->_table[8]  = RGB(  0, 43, 54);
    solarizedDarkScheme->_table[9]  = RGB(203, 75, 22);
    solarizedDarkScheme->_table[10] = RGB( 88, 110, 117);
    solarizedDarkScheme->_table[11] = RGB(101, 123, 131);
    solarizedDarkScheme->_table[12] = RGB(131, 148, 150);
    solarizedDarkScheme->_table[13] = RGB(108, 113, 196);
    solarizedDarkScheme->_table[14] = RGB(147, 161, 161);
    solarizedDarkScheme->_table[15] = RGB(253, 246, 227);
    Microsoft::Console::Utils::SetColorTableAlpha(solarizedDarkSpan, 0xff);

    auto solarizedLightScheme = std::make_unique<ColorScheme>();
    solarizedLightScheme->_schemeName = L"Solarized Light";
    solarizedLightScheme->_defaultBackground = RGB(253, 246, 227);
    solarizedLightScheme->_defaultForeground = RGB(  7, 54, 66);
    auto solarizedLightSpan = gsl::span<COLORREF>(&solarizedLightScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(solarizedLightSpan);
    solarizedLightScheme->_table[0]  = RGB(  7, 54, 66);
    solarizedLightScheme->_table[1]  = RGB(211, 1, 2);
    solarizedLightScheme->_table[2]  = RGB(133, 153, 0);
    solarizedLightScheme->_table[3]  = RGB(181, 137, 0);
    solarizedLightScheme->_table[4]  = RGB( 38, 139, 210);
    solarizedLightScheme->_table[5]  = RGB(211, 54, 130);
    solarizedLightScheme->_table[6]  = RGB( 42, 161, 152);
    solarizedLightScheme->_table[7]  = RGB(238, 232, 213);
    solarizedLightScheme->_table[8]  = RGB(  0, 43, 54);
    solarizedLightScheme->_table[9]  = RGB(203, 75, 22);
    solarizedLightScheme->_table[10] = RGB( 88, 110, 117);
    solarizedLightScheme->_table[11] = RGB(101, 123, 131);
    solarizedLightScheme->_table[12] = RGB(131, 148, 150);
    solarizedLightScheme->_table[13] = RGB(108, 113, 196);
    solarizedLightScheme->_table[14] = RGB(147, 161, 161);
    solarizedLightScheme->_table[15] = RGB(253, 246, 227);
    Microsoft::Console::Utils::SetColorTableAlpha(solarizedLightSpan, 0xff);

    _globals._colorSchemes.push_back(std::move(campbellScheme));
    _globals._colorSchemes.push_back(std::move(solarizedDarkScheme));
    _globals._colorSchemes.push_back(std::move(solarizedLightScheme));

}

void CascadiaSettings::_CreateDefaultProfiles()
{
    auto defaultProfile = std::make_unique<Profile>();
    defaultProfile->_fontFace = L"Consolas";
    defaultProfile->_schemeName = { L"Campbell" };
    defaultProfile->_acrylicTransparency = 0.75;
    defaultProfile->_useAcrylic = true;
    defaultProfile->_name = L"cmd";

    _globals._defaultProfile = defaultProfile->_guid;

    auto powershellProfile = std::make_unique<Profile>();
    powershellProfile->_fontFace = L"Courier New";
    powershellProfile->_commandline = L"powershell.exe";
    powershellProfile->_schemeName = { L"Campbell" };
    powershellProfile->_defaultBackground = RGB(1, 36, 86);
    powershellProfile->_useAcrylic = false;
    powershellProfile->_name = L"Powershell";

    _profiles.push_back(std::move(defaultProfile));
    _profiles.push_back(std::move(powershellProfile));
}

void CascadiaSettings::_CreateDefaultKeybindings()
{
    AppKeyBindings& keyBindings = _globals._keybindings;
    // Set up spme basic default keybindings
    // TODO: read our settings from some source, and configure
    //      keychord,action pairings from that file
    keyBindings.SetKeyBinding(ShortcutAction::NewTab,
                               KeyChord{ KeyModifiers::Ctrl, (int)'T' });

    keyBindings.SetKeyBinding(ShortcutAction::CloseTab,
                               KeyChord{ KeyModifiers::Ctrl, (int)'W' });

    // Yes these are offset by one.
    // Ideally, you'd want C-S-1 to open the _first_ profile, which is index 0
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile0,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'1' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile1,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'2' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile2,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'3' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile3,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'4' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile4,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'5' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile5,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'6' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile6,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'7' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile7,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'8' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile8,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'9' });
    keyBindings.SetKeyBinding(ShortcutAction::NewTabProfile9,
                              KeyChord{ KeyModifiers::Ctrl | KeyModifiers::Shift, (int)'0' });
}

void CascadiaSettings::_CreateDefaults()
{
    _CreateDefaultProfiles();
    _globals._defaultProfile = _profiles[0]->GetGuid();

    _CreateDefaultSchemes();
    _CreateDefaultKeybindings();
}

Profile* CascadiaSettings::_FindProfile(GUID profileGuid)
{
    for (auto& profile : _profiles)
    {
        if (profile->_guid == profileGuid)
        {
            return profile.get();
        }
    }
    return nullptr;
}


TerminalSettings CascadiaSettings::MakeSettings(std::optional<GUID> profileGuidArg)
{
    GUID profileGuid = profileGuidArg ? profileGuidArg.value() : _globals._defaultProfile;
    const Profile* const profile = _FindProfile(profileGuid);
    if (profile == nullptr)
    {
        throw E_INVALIDARG;
    }

    TerminalSettings result = profile->CreateTerminalSettings(_globals._colorSchemes);

    // Place our appropriate global settings into the Terminal Settings
    result.KeyBindings(_globals._keybindings);

    return result;
}

std::vector<GUID> CascadiaSettings::GetProfileGuids()
{
    std::vector<GUID> guids;
    for (auto& profile : _profiles)
    {
        guids.push_back(profile->_guid);
    }
    return guids;
}

std::basic_string_view<std::unique_ptr<Profile>> CascadiaSettings::GetProfiles()
{
    return { &_profiles[0], _profiles.size() };
}

AppKeyBindings CascadiaSettings::GetKeybindings()
{
    return _globals._keybindings;
}
