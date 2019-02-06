#pragma once

#include "Settings.g.h"

namespace winrt::TerminalSettings::implementation
{
    struct Settings : SettingsT<Settings>
    {
        Settings();

        uint32_t DefaultForeground();
        void DefaultForeground(uint32_t value);
        uint32_t DefaultBackground();
        void DefaultBackground(uint32_t value);
        com_array<uint32_t> ColorTable();
        void ColorTable(array_view<uint32_t const> value);
        int32_t HistorySize();
        void HistorySize(int32_t value);
        int32_t InitialRows();
        void InitialRows(int32_t value);
        int32_t InitialCols();
        void InitialCols(int32_t value);

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
