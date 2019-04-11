/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "GlobalAppSettings.h"
#include "../../types/inc/Utils.hpp"

using namespace Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalApp;
using namespace winrt::Windows::Data::Json;
using namespace ::Microsoft::Console;

static const std::wstring DEFAULTPROFILE_KEY{ L"defaultProfile" };
static const std::wstring ALWAYS_SHOW_TABS_KEY{ L"alwaysShowTabs" };
static const std::wstring SHOW_TITLE_IN_TITLEBAR_KEY{ L"showTerminalTitleInTitlebar" };

GlobalAppSettings::GlobalAppSettings() :
    _keybindings{},
    _colorSchemes{},
    _defaultProfile{},
    _alwaysShowTabs{ false },
    _showTitleInTitlebar{ true }
{

}

GlobalAppSettings::~GlobalAppSettings()
{

}

const std::vector<ColorScheme>& GlobalAppSettings::GetColorSchemes() const noexcept
{
    return _colorSchemes;
}


std::vector<ColorScheme>& GlobalAppSettings::GetColorSchemes() noexcept
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

bool GlobalAppSettings::GetAlwaysShowTabs() const noexcept
{
    return _alwaysShowTabs;
}

void GlobalAppSettings::SetAlwaysShowTabs(const bool showTabs) noexcept
{
    _alwaysShowTabs = showTabs;
}

bool GlobalAppSettings::GetShowTitleInTitlebar() const noexcept
{
    return _showTitleInTitlebar;
}

void GlobalAppSettings::SetShowTitleInTitlebar(const bool showTitleInTitlebar) noexcept
{
    _showTitleInTitlebar = showTitleInTitlebar;
}

// Method Description:
// - Serialize this object to a JsonObject.
// Arguments:
// - <none>
// Return Value:
// - a JsonObject which is an equivalent serialization of this object.
JsonObject GlobalAppSettings::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    const auto guidStr = Utils::GuidToString(_defaultProfile);
    const auto defaultProfile = JsonValue::CreateStringValue(guidStr);

    jsonObject.Insert(DEFAULTPROFILE_KEY, defaultProfile);
    jsonObject.Insert(ALWAYS_SHOW_TABS_KEY,
                      JsonValue::CreateBooleanValue(_alwaysShowTabs));
    jsonObject.Insert(SHOW_TITLE_IN_TITLEBAR_KEY,
                      JsonValue::CreateBooleanValue(_showTitleInTitlebar));

    return jsonObject;
}

// Method Description:
// - Create a new instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a GlobalAppSettings object.
// Return Value:
// - a new GlobalAppSettings instance created from the values in `json`
GlobalAppSettings GlobalAppSettings::FromJson(winrt::Windows::Data::Json::JsonObject json)
{
    GlobalAppSettings result{};

    if (json.HasKey(DEFAULTPROFILE_KEY))
    {
        auto guidString = json.GetNamedString(DEFAULTPROFILE_KEY);
        auto guid = Utils::GuidFromString(guidString.c_str());
        result._defaultProfile = guid;
    }

    if (json.HasKey(ALWAYS_SHOW_TABS_KEY))
    {
        result._alwaysShowTabs = json.GetNamedBoolean(ALWAYS_SHOW_TABS_KEY);
    }

    if (json.HasKey(SHOW_TITLE_IN_TITLEBAR_KEY))
    {
        result._showTitleInTitlebar = json.GetNamedBoolean(SHOW_TITLE_IN_TITLEBAR_KEY);
    }

    return result;
}
