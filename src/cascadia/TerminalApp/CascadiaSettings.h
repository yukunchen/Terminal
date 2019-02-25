
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

// private:
    GlobalAppSettings _globals;
    std::vector<std::unique_ptr<Microsoft::Terminal::TerminalApp::Profile>> _profiles;

};
