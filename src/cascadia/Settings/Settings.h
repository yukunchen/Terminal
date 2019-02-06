#pragma once

#include "Settings.g.h"
#include "ITerminalSettings.hpp"

namespace winrt::TerminalSettings::implementation
{
    struct Settings : SettingsT<Settings>, ITerminalSettings
    {
        Settings();

        uint32_t DefaultForeground() override;
        void DefaultForeground(uint32_t value) override;
        uint32_t DefaultBackground() override;
        void DefaultBackground(uint32_t value) override;
        com_array<uint32_t> ColorTable();
        void ColorTable(array_view<uint32_t const> value);

        std::basic_string_view<uint32_t> GetColorTable() override;
        void SetColorTable(std::basic_string_view<uint32_t const> value) override;

        int32_t HistorySize() override;
        void HistorySize(int32_t value) override;
        int32_t InitialRows() override;
        void InitialRows(int32_t value) override;
        int32_t InitialCols() override;
        void InitialCols(int32_t value) override;

    private:
        uint32_t _defaultForeground;
        uint32_t _defaultBackground;
        int32_t _historySize;
        int32_t _initialRows;
        int32_t _initialCols;
    };
}

namespace winrt::TerminalSettings::factory_implementation
{
    struct Settings : SettingsT<Settings, implementation::Settings>
    {
    };
}
