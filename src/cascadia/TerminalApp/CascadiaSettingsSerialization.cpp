/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include <argb.h>
#include "CascadiaSettings.h"
#include "../../types/inc/utils.hpp"
#include <appmodel.h>

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


// Method Description:
// - Creates a CascadiaSettings from whatever's saved on disk, or instantiates
//      a new one with the defalt values. If we're running as a packaged app,
//      it will load the settings from our packaged localappdata. If we're
//      running as an unpackaged application, it will read it from the path
//      we've set under localappdata.
// Arguments:
// - <none>
// Return Value:
// - a unique_ptr containing a new CascadiaSettings object.
std::unique_ptr<CascadiaSettings> CascadiaSettings::LoadAll()
{
    std::unique_ptr<CascadiaSettings> resultPtr;
    std::optional<winrt::hstring> fileData = _IsPackaged() ?
                                             _LoadAsPackagedApp() : _LoadAsUnpackagedApp();

    const bool foundFile = fileData.has_value();
    if (foundFile)
    {
        const auto actualData = fileData.value();

        // TODO: MSFT:20720124 - convert actualData from utf-8 to utf-16, so we
        //      can build a hstrng from it (so Windows.Json can parse it)
        // Do we even need to canvert from utf-8? Or can Windows.Json handle utf-8?

        JsonValue root{ nullptr };
        bool parsedSuccessfully = JsonValue::TryParse(actualData, root);
        if (parsedSuccessfully)
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

// Method Description:
// - Serialize this settings structure, and save it to a file. The location of
//      the file changes depending whether we're running as a packaged
//      application or not.
// Arguments:
// - <none>
// Return Value:
// - <none>
void CascadiaSettings::SaveAll() const
{
    const JsonObject json = ToJson();
    auto serializedSettings = json.Stringify();

    // TODO: MSFT:20720124 - serializedSettings is a utf-16 string currently.
    // Investigate converting it to utf-8 and saving it as utf-8

    if (_IsPackaged())
    {
        _SaveAsPackagedApp(serializedSettings);
    }
    else
    {
        _SaveAsUnpackagedApp(serializedSettings);
    }
}

// Method Description:
// - Serialize this object to a JsonObject.
// Arguments:
// - <none>
// Return Value:
// - a JsonObject which is an equivalent serialization of this object.
JsonObject CascadiaSettings::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    const auto guidStr = Utils::GuidToString(_globals.GetDefaultProfile());
    const auto defaultProfile = JsonValue::CreateStringValue(guidStr);

    JsonArray schemesArray{};
    const auto& colorSchemes = _globals.GetColorSchemes();
    for (auto& scheme : colorSchemes)
    {
        schemesArray.Append(scheme->ToJson());
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

// Method Description:
// - Create a newe instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a CascadiaSettings object.
// Return Value:
// - a new CascadiaSettings instance created from the values in `json`
std::unique_ptr<CascadiaSettings> CascadiaSettings::FromJson(JsonObject json)
{
    std::unique_ptr<CascadiaSettings> resultPtr = std::make_unique<CascadiaSettings>();

    if (json.HasKey(DEFAULTPROFILE_KEY))
    {
        auto guidString = json.GetNamedString(DEFAULTPROFILE_KEY);
        auto guid = Utils::GuidFromString(guidString.c_str());
        resultPtr->_globals.SetDefaultProfile(guid);
    }

    auto& resultSchemes = resultPtr->_globals.GetColorSchemes();
    if (json.HasKey(SCHEMES_KEY))
    {
        auto schemes = json.GetNamedArray(SCHEMES_KEY);
        for (auto schemeJson : schemes)
        {
            if (schemeJson.ValueType() == JsonValueType::Object)
            {
                auto schemeObj = schemeJson.GetObjectW();
                auto scheme = ColorScheme::FromJson(schemeObj);
                resultSchemes.push_back(std::move(scheme));
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

    // TODO:MSFT:20700157
    // Load the keybindings from the file as well
    resultPtr->_CreateDefaultKeybindings();

    return std::move(resultPtr);
}

// Function Description:
// - Returns true if we're running in a packaged context. If we are, then we
//      have to use the Windows.Storage API's to save/load our files. If we're
//      not, then we won't be able to use those API's.
// Arguments:
// - <none>
// Return Value:
// - true iff we're running in a packaged context.
bool CascadiaSettings::_IsPackaged()
{
    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, NULL);
    return rc != APPMODEL_ERROR_NO_PACKAGE;
}

// Method Description:
// - Writes the given content to our settings file using the Windows.Storage
//      APIS's. This will only work within the context of an application with
//      package identity, so make sure to call _IsPackaged before calling this method.
//   Will overwrite any existing content in the file.
// Arguments:
// - content: the given string of content to write to the file.
// Return Value:
// - <none>
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
    // TODO: MSFT:20719950 This path should still be under localappdata
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
    // TODO: MSFT:20719950 - Don't just try the current working directory, look
    //      in some well-known place.
    const auto hFile = CreateFileW(FILENAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return {};
    }

    const auto fileSize = GetFileSize(hFile, nullptr);
    wchar_t* buffer = new wchar_t[fileSize / sizeof(wchar_t)];
    DWORD bytesRead = 0;
    ReadFile(hFile, buffer, fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);

    const winrt::hstring fileData = { buffer, bytesRead / sizeof(wchar_t) };
    delete[] buffer;

    return { fileData };
}
