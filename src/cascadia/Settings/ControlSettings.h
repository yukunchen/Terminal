#pragma once

#include "ControlSettings.g.h"

namespace winrt::TerminalSettings::implementation
{
    struct ControlSettings : ControlSettingsT<ControlSettings>
    {
        ControlSettings() = default;

        // TerminalSettings::Settings TerminalSettings();
        // void TerminalSettings(TerminalSettings::Settings const& value);
    };
}

namespace winrt::TerminalSettings::factory_implementation
{
    struct ControlSettings : ControlSettingsT<ControlSettings, implementation::ControlSettings>
    {
    };
}
