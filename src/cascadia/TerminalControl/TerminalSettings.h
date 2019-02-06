#pragma once

#include "TerminalSettings.g.h"
#include "../../cascadia/TerminalCore/Settings.h"

namespace winrt::TerminalControl::implementation
{
    // Implements ::ITerminalSettings to make sure that the
    // winrt::TerminalControl::ITerminalSettings and ITerminalSettings stay in sync
    struct TerminalSettings : TerminalSettingsT<TerminalSettings>, public ::ITerminalSettings
    {
        TerminalSettings() = default;

    public:
        uint32_t DefaultForeground() override;
        void DefaultForeground(uint32_t value) override;
        uint32_t DefaultBackground() override;
        void DefaultBackground(uint32_t value) override;

        std::basic_string_view<uint32_t> GetColorTable() override;
        void SetColorTable(std::basic_string_view<uint32_t const> value) override;

        uint32_t GetColorTableEntry(int32_t index);
        void SetColorTableEntry(int32_t index, uint32_t value);

        int32_t HistorySize() override;
        void HistorySize(int32_t value) override;
        int32_t InitialRows() override;
        void InitialRows(int32_t value) override;
        int32_t InitialCols() override;
        void InitialCols(int32_t value) override;

        private:
            Settings _settings;
    };
}

namespace winrt::TerminalControl::factory_implementation
{
    struct TerminalSettings : TerminalSettingsT<TerminalSettings, implementation::TerminalSettings>
    {
    };
}
