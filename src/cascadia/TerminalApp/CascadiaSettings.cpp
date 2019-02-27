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

void _CreateFakeSchemes(CascadiaSettings& self)
{
    const auto TABLE_SIZE = gsl::narrow<ptrdiff_t>(16);

    auto defaultScheme = std::make_unique<ColorScheme>();
    defaultScheme->_schemeName = L"Default";
    defaultScheme->_defaultForeground = RGB(242, 242, 242);
    defaultScheme->_defaultBackground = 0xff008a;
    auto defaultSpan = gsl::span<COLORREF>(&defaultScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(defaultSpan);
    Microsoft::Console::Utils::SetColorTableAlpha(defaultSpan, 0xff);

    auto campbellScheme = std::make_unique<ColorScheme>();
    campbellScheme->_schemeName = L"Campbell";
    campbellScheme->_defaultForeground = RGB(242, 242, 242);
    campbellScheme->_defaultBackground = RGB(12, 12, 12);
    auto campbellSpan = gsl::span<COLORREF>(&campbellScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(campbellSpan);
    Microsoft::Console::Utils::SetColorTableAlpha(campbellSpan, 0xff);

    auto powershellScheme = std::make_unique<ColorScheme>();
    powershellScheme->_schemeName = L"Powershell";
    powershellScheme->_defaultBackground = RGB(1, 36, 86);
    auto powershellSpan = gsl::span<COLORREF>(&powershellScheme->_table[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(powershellSpan);
    Microsoft::Console::Utils::SetColorTableAlpha(powershellSpan, 0xff);
    powershellScheme->_table[3] = RGB(1, 36, 86);
    powershellScheme->_table[5] = RGB(238, 237, 240);

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

    self._globals._colorSchemes.push_back(std::move(defaultScheme));
    self._globals._colorSchemes.push_back(std::move(campbellScheme));
    self._globals._colorSchemes.push_back(std::move(powershellScheme));
    self._globals._colorSchemes.push_back(std::move(solarizedDarkScheme));
    self._globals._colorSchemes.push_back(std::move(solarizedLightScheme));

}

void _CreateFakeTestProfiles(CascadiaSettings& self)
{
    auto defaultProfile = std::make_unique<Profile>();
    defaultProfile->_fontFace = L"Consolas";
    defaultProfile->_schemeName = { L"Default" };
    defaultProfile->_acrylicTransparency = 0.5;
    defaultProfile->_useAcrylic = true;
    defaultProfile->_name = L"Default";

    self._globals._defaultProfile = defaultProfile->_guid;

    auto powershellProfile = std::make_unique<Profile>();
    powershellProfile->_fontFace = L"Courier New";
    powershellProfile->_commandline = L"powershell.exe";
    powershellProfile->_schemeName = { L"Powershell" };
    powershellProfile->_useAcrylic = false;
    powershellProfile->_name = L"Powershell";

    auto cmdProfile = std::make_unique<Profile>();
    cmdProfile->_fontFace = L"Consolas";
    cmdProfile->_commandline = L"cmd.exe";
    cmdProfile->_schemeName = { L"Campbell" };
    cmdProfile->_useAcrylic = true;
    cmdProfile->_acrylicTransparency = 0.75;
    cmdProfile->_name = L"cmd";

    auto solarizedDarkProfile = std::make_unique<Profile>();
    solarizedDarkProfile->_fontFace = L"Consolas";
    solarizedDarkProfile->_commandline = L"wsl.exe";
    solarizedDarkProfile->_schemeName = { L"Solarized Dark" };
    solarizedDarkProfile->_useAcrylic = true;
    solarizedDarkProfile->_acrylicTransparency = 0.75;
    solarizedDarkProfile->_name = L"Solarized Dark";


    auto solarizedDarkProfile2 = std::make_unique<Profile>();
    solarizedDarkProfile2->_fontFace = L"Consolas";
    solarizedDarkProfile2->_commandline = L"cmd.exe";
    solarizedDarkProfile2->_schemeName = { L"Solarized Dark" };
    solarizedDarkProfile2->_useAcrylic = true;
    solarizedDarkProfile2->_name = L"Solarized Dark 2";


    auto solarizedLightProfile = std::make_unique<Profile>();
    solarizedLightProfile->_fontFace = L"Consolas";
    solarizedLightProfile->_commandline = L"cmd.exe";
    solarizedLightProfile->_schemeName = { L"Solarized Light" };
    solarizedLightProfile->_useAcrylic = true;
    solarizedLightProfile->_name = L"Solarized Light";

    self._profiles.push_back(std::move(defaultProfile));
    self._profiles.push_back(std::move(powershellProfile));
    self._profiles.push_back(std::move(cmdProfile));
    self._profiles.push_back(std::move(solarizedDarkProfile));
    self._profiles.push_back(std::move(solarizedDarkProfile2));
    self._profiles.push_back(std::move(solarizedLightProfile));

    for (int i = 0; i < 5; i++)
    {
        auto randProfile = std::make_unique<Profile>();
        unsigned int bg = (unsigned int) (rand() % (0x1000000));
        bool acrylic = (rand() % 2) == 1;
        int shell = (rand() % 3);

        randProfile->_coreSettings.DefaultBackground(bg);
        randProfile->_useAcrylic = acrylic;
        randProfile->_acrylicTransparency = 0.5;

        if (shell == 0)
        {
            randProfile->_commandline = L"cmd.exe";
            randProfile->_fontFace = L"Consolas";
        }
        else if (shell == 1)
        {
            randProfile->_commandline = L"powershell.exe";
            randProfile->_fontFace = L"Courier New";
        }
        else if (shell == 2)
        {
            randProfile->_commandline = L"wsl.exe";
            randProfile->_fontFace = L"Ubuntu Mono";
        }

        self._profiles.push_back(std::move(randProfile));
    }

    // powershellProfile->_fontFace = L"Lucidia Console";
}

void CascadiaSettings::LoadAll()
{
    // As a test:
    _CreateFakeTestProfiles(*this);
    _CreateFakeSchemes(*this);

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

void CascadiaSettings::SaveAll()
{

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
