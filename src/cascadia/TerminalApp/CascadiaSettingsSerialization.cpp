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
using namespace winrt::Windows::Data::Json;

const std::wstring PROFILES_KEY{ L"profiles" };
const std::wstring KEYBINDINGS_KEY{ L"keybindings" };
const std::wstring SCHEMES_KEY{ L"schemes" };

void CascadiaSettings::LoadAll()
{
    bool foundFile = false;
    if (foundFile)
    {

    }
    else
    {
        _CreateDefaults();
        SaveAll();
    }
}

void CascadiaSettings::SaveAll()
{

    winrt::Windows::Data::Json::JsonObject jsonObject;

    JsonArray schemesArray{};
    for (auto& scheme : _globals._colorSchemes)
    {
        schemesArray.Append(scheme->ToJson());
    }

    JsonArray profilesArray{};
    for (auto& profile : _profiles)
    {
        profilesArray.Append(profile->ToJson());
    }

    jsonObject.Insert(SCHEMES_KEY, schemesArray);
    jsonObject.Insert(PROFILES_KEY, profilesArray);

    auto s = jsonObject.Stringify();
    auto f = s.c_str();
    auto a = 1;
}
