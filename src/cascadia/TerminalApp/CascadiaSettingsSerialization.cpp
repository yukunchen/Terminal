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
#include <appmodel.h>

using namespace ::Microsoft::Terminal::TerminalControl;
using namespace ::Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace winrt::Microsoft::Terminal::TerminalApp;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::Storage;

#define FILENAME L"profiles.json"

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
    winrt::init_apartment();

    winrt::Windows::Data::Json::JsonObject jsonObject;

    JsonArray schemesArray{};
    for (auto& scheme : _globals._colorSchemes)
    {
        auto js = scheme->ToJson();
        auto _schemeCopy = ColorScheme::FromJson(js);
        schemesArray.Append(js);
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

    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, NULL);
    if (rc == APPMODEL_ERROR_NO_PACKAGE)
    {
        _SaveAsUnpackagedApp(s);
    }
    else
    {
        _SaveAsPackagedApp(s);
    }
}

void CascadiaSettings::_SaveAsPackagedApp(const winrt::hstring content)
{
    auto curr = ApplicationData::Current();
    auto folder = curr.LocalFolder();

    auto file_async = folder.CreateFileAsync(FILENAME,
                                             CreationCollisionOption::ReplaceExisting);

    auto file = file_async.get();

    FileIO::WriteTextAsync(file, content).get();
}

void CascadiaSettings::_SaveAsUnpackagedApp(const winrt::hstring content)
{
    auto contentString = content.c_str();
    auto hOut = CreateFileW(FILENAME, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hOut, contentString, content.size() * sizeof(wchar_t), 0, 0);
    CloseHandle(hOut);
}
