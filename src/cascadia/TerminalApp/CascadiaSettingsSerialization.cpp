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

    //auto hOut = CreateFile(L"profiles.json", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    //auto hOut = CreateFile(L"profiles.json", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


    auto hOut = CreateFileW(L"%USERPROFILE%\\profiles.json", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        auto gle = GetLastError();
        if (gle == ERROR_ACCESS_DENIED)
        {
            auto curr = winrt::Windows::Storage::ApplicationData::Current();
            auto folder = curr.LocalFolder();
            auto file_async = folder.CreateFileAsync(L"profiles.json");
            auto file = file_async.GetResults();
            //winrt::Windows::Storage::Streams::IBuffer outBuffer = winrt::Windows::Storage::Streams::FileOutputStream()
            auto oStream = file.OpenAsync(winrt::Windows::Storage::FileAccessMode::ReadWrite).GetResults().GetOutputStreamAt(0);
            winrt::Windows::Storage::Streams::DataWriter _writer{ oStream };
            _writer.WriteString(s);
            _writer.Close();
        }
        else
        {
            throw gle;
        }
    }
    else
    {
        WriteFile(hOut, f, s.size() * sizeof(wchar_t), 0, 0);
        CloseHandle(hOut);
    }
}
