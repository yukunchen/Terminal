/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include <argb.h>
#include <conattrs.hpp>
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

// Method Description:
// - Create the set of schemes to use as the default schemes. Currently creates
//      three default color schemes - Campbell (the new cmd color scheme),
//      Solarized Dark and Solarized Light.
// Arguments:
// - <none>
// Return Value:
// - <none>
void CascadiaSettings::_CreateDefaultSchemes()
{
    const auto TABLE_SIZE = gsl::narrow<ptrdiff_t>(COLOR_TABLE_SIZE);

    auto campbellScheme = std::make_unique<ColorScheme>(L"Campbell",
                                                        RGB(242, 242, 242),
                                                        RGB(12, 12, 12));
    auto campbellTable = campbellScheme->GetTable();
    auto campbellSpan = gsl::span<COLORREF>(&campbellTable[0], TABLE_SIZE);
    Microsoft::Console::Utils::InitializeCampbellColorTable(campbellSpan);
    Microsoft::Console::Utils::SetColorTableAlpha(campbellSpan, 0xff);


    auto solarizedDarkScheme = std::make_unique<ColorScheme>(L"Solarized Dark",
                                                             RGB(  7, 54,  66),
                                                             RGB(253, 246, 227));
    auto solarizedDarkTable = solarizedDarkScheme->GetTable();
    auto solarizedDarkSpan = gsl::span<COLORREF>(&solarizedDarkTable[0], TABLE_SIZE);
    solarizedDarkTable[0]  = RGB(  7, 54, 66);
    solarizedDarkTable[1]  = RGB(211, 1, 2);
    solarizedDarkTable[2]  = RGB(133, 153, 0);
    solarizedDarkTable[3]  = RGB(181, 137, 0);
    solarizedDarkTable[4]  = RGB( 38, 139, 210);
    solarizedDarkTable[5]  = RGB(211, 54, 130);
    solarizedDarkTable[6]  = RGB( 42, 161, 152);
    solarizedDarkTable[7]  = RGB(238, 232, 213);
    solarizedDarkTable[8]  = RGB(  0, 43, 54);
    solarizedDarkTable[9]  = RGB(203, 75, 22);
    solarizedDarkTable[10] = RGB( 88, 110, 117);
    solarizedDarkTable[11] = RGB(101, 123, 131);
    solarizedDarkTable[12] = RGB(131, 148, 150);
    solarizedDarkTable[13] = RGB(108, 113, 196);
    solarizedDarkTable[14] = RGB(147, 161, 161);
    solarizedDarkTable[15] = RGB(253, 246, 227);
    Microsoft::Console::Utils::SetColorTableAlpha(solarizedDarkSpan, 0xff);

    auto solarizedLightScheme = std::make_unique<ColorScheme>(L"Solarized Light",
                                                             RGB(253, 246, 227),
                                                             RGB(  7, 54,  66));
    auto solarizedLightTable = solarizedLightScheme->GetTable();
    auto solarizedLightSpan = gsl::span<COLORREF>(&solarizedLightTable[0], TABLE_SIZE);
    solarizedLightTable[0]  = RGB(  7, 54, 66);
    solarizedLightTable[1]  = RGB(211, 1, 2);
    solarizedLightTable[2]  = RGB(133, 153, 0);
    solarizedLightTable[3]  = RGB(181, 137, 0);
    solarizedLightTable[4]  = RGB( 38, 139, 210);
    solarizedLightTable[5]  = RGB(211, 54, 130);
    solarizedLightTable[6]  = RGB( 42, 161, 152);
    solarizedLightTable[7]  = RGB(238, 232, 213);
    solarizedLightTable[8]  = RGB(  0, 43, 54);
    solarizedLightTable[9]  = RGB(203, 75, 22);
    solarizedLightTable[10] = RGB( 88, 110, 117);
    solarizedLightTable[11] = RGB(101, 123, 131);
    solarizedLightTable[12] = RGB(131, 148, 150);
    solarizedLightTable[13] = RGB(108, 113, 196);
    solarizedLightTable[14] = RGB(147, 161, 161);
    solarizedLightTable[15] = RGB(253, 246, 227);
    Microsoft::Console::Utils::SetColorTableAlpha(solarizedLightSpan, 0xff);

    _globals._colorSchemes.push_back(std::move(campbellScheme));
    _globals._colorSchemes.push_back(std::move(solarizedDarkScheme));
    _globals._colorSchemes.push_back(std::move(solarizedLightScheme));

}

// Method Description:
// - Create a set of profiles to use as the "default" profiles when initializing
//      the terminal. Currently, we create two profiles: one for cmd.exe, and
//      one for powershell.
// Arguments:
// - <none>
// Return Value:
// - <none>
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

// Method Description:
// - Set up some default keybindings for the terminal.
// Arguments:
// - <none>
// Return Value:
// - <none>
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

// Method Description:
// - Initialize this object with default color schemes, profiles, and keybindings.
// Arguments:
// - <none>
// Return Value:
// - <none>
void CascadiaSettings::_CreateDefaults()
{
    _CreateDefaultProfiles();
    _globals._defaultProfile = _profiles[0]->GetGuid();

    _CreateDefaultSchemes();
    _CreateDefaultKeybindings();
}

// Method Description:
// - Finds a profile that matches the given GUID. If there is no profile in this
//      settings object that matches, returns nullptr.
// Arguments:
// - profileGuid: the GUID of the profile to return.
// Return Value:
// - a non-ownership pointer to the profile matching the given guid, or nullptr
//      if there is no match.
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

// Method Description:
// - Create a TerminalSettings object from the given profile.
//      If the profileGuidArg is not provided, this method will use the default
//      profile.
//   The TerminalSettings object that is created can be used to initialize both
//      the Control's settings, and the Core settings of the terminal.
// Arguments:
// - profileGuidArg: an optional GUID to use to lookup the profile to create the
//      settings from. If this arg is not provided, or the GUID does not match a
//       profile, then this method will use the default profile.
// Return Value:
// - <none>
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

// Method Description:
// - Returns an iterable collection of all of our Profiles.
// Arguments:
// - <none>
// Return Value:
// - an iterable collection of all of our Profiles.
std::basic_string_view<std::unique_ptr<Profile>> CascadiaSettings::GetProfiles()
{
    return { &_profiles[0], _profiles.size() };
}

// Method Description:
// - Returns the globally configured keybindings
// Arguments:
// - <none>
// Return Value:
// - the globally configured keybindings
AppKeyBindings CascadiaSettings::GetKeybindings()
{
    return _globals._keybindings;
}
