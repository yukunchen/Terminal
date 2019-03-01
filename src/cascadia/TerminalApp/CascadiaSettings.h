
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

    void LoadAll();
    void SaveAll();
    winrt::Microsoft::Terminal::TerminalApp::TerminalSettings MakeSettings(std::optional<GUID> profileGuid);

    std::vector<GUID> GetProfileGuids();
    std::basic_string_view<std::unique_ptr<Profile>> GetProfiles();

    winrt::Microsoft::Terminal::TerminalApp::AppKeyBindings GetKeybindings();

private:
    GlobalAppSettings _globals;
    std::vector<std::unique_ptr<Profile>> _profiles;

    Profile* _FindProfile(GUID profileGuid);

    void _CreateFakeSchemes();
    void _CreateFakeTestProfiles();

    void _CreateDefaults();

    static bool _IsPackaged();
    static void _SaveAsPackagedApp(const winrt::hstring content);
    static void _SaveAsUnpackagedApp(const winrt::hstring content);

};
