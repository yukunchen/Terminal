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
using namespace ::Microsoft::Console;

#define FILENAME L"profiles.json"

const std::wstring DEFAULTPROFILE_KEY{ L"defaultProfile" };
const std::wstring PROFILES_KEY{ L"profiles" };
const std::wstring KEYBINDINGS_KEY{ L"keybindings" };
const std::wstring SCHEMES_KEY{ L"schemes" };

std::unique_ptr<CascadiaSettings> CascadiaSettings::LoadAll()
{
    std::unique_ptr<CascadiaSettings> resultPtr;
    std::optional<winrt::hstring> fileData;
    if (_IsPackaged())
    {
        fileData = _LoadAsPackagedApp();
    }
    else
    {
        fileData = _LoadAsUnpackagedApp();
    }

    bool foundFile = fileData.has_value();
    if (foundFile)
    {
        const auto actualData = fileData.value();
        JsonValue root{ nullptr };
        bool parsed = JsonValue::TryParse(actualData, root);
        if (parsed)
        {
            JsonObject obj = root.GetObjectW();
            resultPtr = FromJson(obj);
        }
    }
    else
    {
        resultPtr = std::make_unique<CascadiaSettings>();
        resultPtr->_CreateDefaults();
        resultPtr->SaveAll();
    }


    return std::move(resultPtr);
}

void CascadiaSettings::SaveAll() const
{
    const JsonObject json = ToJson();
    auto s = json.Stringify();

    if (_IsPackaged())
    {
        _SaveAsPackagedApp(s);
    }
    else
    {
        _SaveAsUnpackagedApp(s);
    }
}


JsonObject CascadiaSettings::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    const auto guidStr = Utils::GuidToString(_globals._defaultProfile);
    const auto defaultProfile = JsonValue::CreateStringValue(guidStr);

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

    jsonObject.Insert(DEFAULTPROFILE_KEY, defaultProfile);
    jsonObject.Insert(SCHEMES_KEY, schemesArray);
    jsonObject.Insert(PROFILES_KEY, profilesArray);

    return jsonObject;
}

std::unique_ptr<CascadiaSettings> CascadiaSettings::FromJson(JsonObject json)
{
    std::unique_ptr<CascadiaSettings> resultPtr = std::make_unique<CascadiaSettings>();

    if (json.HasKey(DEFAULTPROFILE_KEY))
    {
        auto guidString = json.GetNamedString(DEFAULTPROFILE_KEY);
        auto guid = Utils::GuidFromString(guidString.c_str());
        resultPtr->_globals._defaultProfile = guid;
    }

    if (json.HasKey(SCHEMES_KEY))
    {
        auto schemes = json.GetNamedArray(SCHEMES_KEY);
        for (auto schemeJson : schemes)
        {
            if (schemeJson.ValueType() == JsonValueType::Object)
            {
                auto schemeObj = schemeJson.GetObjectW();
                auto scheme = ColorScheme::FromJson(schemeObj);
                resultPtr->_globals._colorSchemes.push_back(std::move(scheme));
            }
        }
    }

    if (json.HasKey(PROFILES_KEY))
    {
        auto profiles = json.GetNamedArray(PROFILES_KEY);
        for (auto profileJson : profiles)
        {
            if (profileJson.ValueType() == JsonValueType::Object)
            {
                auto profileObj = profileJson.GetObjectW();
                auto profile = Profile::FromJson(profileObj);
                resultPtr->_profiles.push_back(std::move(profile));
            }
        }
    }

    // TODO:MSFT:
    // Load the keybindings from the file as well
    resultPtr->_CreateDefaultKeybindings();

    return std::move(resultPtr);
}

bool CascadiaSettings::_IsPackaged()
{
    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, NULL);
    return rc != APPMODEL_ERROR_NO_PACKAGE;
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

std::optional<winrt::hstring> CascadiaSettings::_LoadAsPackagedApp()
{
    auto curr = ApplicationData::Current();
    auto folder = curr.LocalFolder();
    auto file_async = folder.TryGetItemAsync(FILENAME);
    auto file = file_async.get();
    if (file == nullptr)
    {
        return {};
    }
    const auto storageFile = file.as<StorageFile>();

    return { FileIO::ReadTextAsync(storageFile).get() };
}

std::optional<winrt::hstring> CascadiaSettings::_LoadAsUnpackagedApp()
{
    const auto hFile = CreateFileW(FILENAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    const auto fileSize = GetFileSize(hFile, nullptr);
    wchar_t* buffer = new wchar_t[fileSize / sizeof(wchar_t)];
    DWORD read = 0;
    ReadFile(hFile, buffer, fileSize, &read, nullptr);
    CloseHandle(hFile);

    return { buffer };
}
