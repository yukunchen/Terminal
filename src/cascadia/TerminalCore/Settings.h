#pragma once

#include "ITerminalSettings.hpp"


namespace Microsoft::Terminal::Core
{
    class Settings;
};

class Microsoft::Terminal::Core::Settings final :
    public Microsoft::Terminal::Core::ITerminalSettings
{
public:
    Settings();
    uint32_t DefaultForeground() const noexcept override;
    void DefaultForeground(uint32_t value) override;
    uint32_t DefaultBackground() const noexcept override;
    void DefaultBackground(uint32_t value) override;

    std::basic_string_view<uint32_t> GetColorTable() const noexcept override;
    void SetColorTable(std::basic_string_view<uint32_t const> value) override;

    int32_t HistorySize() const noexcept override;
    void HistorySize(int32_t value) override;
    int32_t InitialRows() const noexcept override;
    void InitialRows(int32_t value) override;
    int32_t InitialCols() const noexcept override;
    void InitialCols(int32_t value) override;
    bool SnapOnInput() const noexcept override;
    void SnapOnInput(bool value) override;

private:
    uint32_t _defaultForeground;
    uint32_t _defaultBackground;
    std::array<uint32_t, 256> _colorTable;
    int32_t _historySize;
    int32_t _initialRows;
    int32_t _initialCols;
    bool _snapOnInput;
};

