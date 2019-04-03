/*++
Copyright (c) Microsoft Corporation

Module Name:
- CascadiaSettings.hpp

Abstract:
- This class acts as the container for all app settings. It's composed of two
        parts: Globals, which are app-wide settings, and Profiles, which contain
        a set of settings that apply to a single instance of the terminal.
  Also contains the logic for serializing and deserializing this object.

Author(s):
- Mike Griese - March 2019

--*/
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "GlobalAppSettings.h"
#include "Profile.h"


namespace Microsoft::Terminal::TerminalApp
{
    class CascadiaSettings;
};

class Microsoft::Terminal::TerminalApp::CascadiaSettings final
{

public:
    CascadiaSettings();
    ~CascadiaSettings();

    static std::unique_ptr<CascadiaSettings> LoadAll();
    void SaveAll() const;

    winrt::Microsoft::Terminal::Settings::TerminalSettings MakeSettings(std::optional<GUID> profileGuid) const;

    GlobalAppSettings& GlobalSettings();

    std::basic_string_view<Profile> GetProfiles() const noexcept;

    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings GetKeybindings() const noexcept;

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<CascadiaSettings> FromJson(winrt::Windows::Data::Json::JsonObject json);

    static winrt::hstring GetSettingsPath();

private:
    GlobalAppSettings _globals;
    std::vector<Profile> _profiles;

    const Profile* _FindProfile(GUID profileGuid) const noexcept;

    void _CreateDefaultKeybindings();
    void _CreateDefaultSchemes();
    void _CreateDefaultProfiles();
    void _CreateDefaults();

    static bool _IsPackaged();
    static void _SaveAsPackagedApp(const winrt::hstring content);
    static void _SaveAsUnpackagedApp(const winrt::hstring content);
    static std::wstring _GetFullPathToUnpackagedSettingsFile();
    static winrt::hstring _GetPackagedSettingsPath();
    static std::optional<winrt::hstring> _LoadAsPackagedApp();
    static std::optional<winrt::hstring> _LoadAsUnpackagedApp();

};
