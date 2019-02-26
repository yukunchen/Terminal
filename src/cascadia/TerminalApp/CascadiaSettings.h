
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
    winrt::Microsoft::Terminal::TerminalControl::TerminalSettings MakeSettings(std::optional<GUID> profileGuid);

    std::vector<GUID> GetProfileGuids();
    std::basic_string_view<std::unique_ptr<Profile>> GetProfiles();

// private:
    GlobalAppSettings _globals;
    std::vector<std::unique_ptr<Profile>> _profiles;

    Profile* _FindProfile(GUID profileGuid);


};
