/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include <argb.h>
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

void _CreateFakeTestProfiles(CascadiaSettings& self)
{
    auto defaultProfile = std::make_unique<Profile>();
    defaultProfile->_fontFace = L"Consolas";
    defaultProfile->_coreSettings.DefaultBackground(0xff008a);
    defaultProfile->_acrylicTransparency = 0.5;
    defaultProfile->_useAcrylic = true;
    defaultProfile->_name = L"Default";

    self._globals._defaultProfile = defaultProfile->_guid;

    auto powershellProfile = std::make_unique<Profile>();
    powershellProfile->_fontFace = L"Courier New";
    powershellProfile->_commandline = L"powershell.exe";
    powershellProfile->_coreSettings.DefaultBackground(RGB(1, 36, 86));
    powershellProfile->_useAcrylic = false;
    defaultProfile->_name = L"Powershell";


    auto cmdProfile = std::make_unique<Profile>();
    cmdProfile->_fontFace = L"Consolas";
    cmdProfile->_commandline = L"cmd.exe";
    cmdProfile->_coreSettings.DefaultBackground(RGB(12, 12, 12));
    cmdProfile->_useAcrylic = true;
    cmdProfile->_acrylicTransparency = 0.75;
    defaultProfile->_name = L"cmd";

    self._profiles.push_back(std::move(defaultProfile));
    self._profiles.push_back(std::move(powershellProfile));
    self._profiles.push_back(std::move(cmdProfile));

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
    terminalSettings.TintOpacity(sourceProfile._acrylicTransparency);

    terminalSettings.FontFace(sourceProfile._fontFace);
    terminalSettings.FontSize(sourceProfile._fontSize);

    terminalSettings.Commandline(winrt::to_hstring(sourceProfile._commandline.c_str()));
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

    TerminalSettings result{};

    // Place our appropriate global settings into the Terminal Settings
    result.KeyBindings(_globals._keybindings);

    _SetFromProfile(*profile, result);

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
