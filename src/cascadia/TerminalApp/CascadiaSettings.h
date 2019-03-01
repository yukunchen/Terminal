
#pragma once
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include "GlobalAppSettings.h"
#include "Profile.h"


namespace Microsoft::Terminal::TerminalApp
{
    class CascadiaSettings;
};

class Microsoft::Terminal::TerminalApp::CascadiaSettings
{

public:
    CascadiaSettings();
    ~CascadiaSettings();

    static std::unique_ptr<CascadiaSettings> LoadAll();
    void SaveAll() const;

    winrt::Microsoft::Terminal::TerminalApp::TerminalSettings MakeSettings(std::optional<GUID> profileGuid);

    std::vector<GUID> GetProfileGuids();
    std::basic_string_view<std::unique_ptr<Profile>> GetProfiles();

    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings GetKeybindings();

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<CascadiaSettings> FromJson(winrt::Windows::Data::Json::JsonObject json);

private:
    GlobalAppSettings _globals;
    std::vector<std::unique_ptr<Profile>> _profiles;

    Profile* _FindProfile(GUID profileGuid);

    void _CreateDefaultKeybindings();
    void _CreateDefaultSchemes();
    void _CreateDefaultProfiles();
    void _CreateDefaults();

    static bool _IsPackaged();
    static void _SaveAsPackagedApp(const winrt::hstring content);
    static void _SaveAsUnpackagedApp(const winrt::hstring content);
    static std::optional<winrt::hstring> _LoadAsPackagedApp();
    static std::optional<winrt::hstring> _LoadAsUnpackagedApp();
};
