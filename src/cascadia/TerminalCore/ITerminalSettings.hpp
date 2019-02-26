#pragma once


namespace Microsoft::Terminal::Core
{
    class ITerminalSettings;
};

class Microsoft::Terminal::Core::ITerminalSettings
{
public:
    virtual ~ITerminalSettings() = 0;

    virtual uint32_t DefaultForeground() const noexcept = 0;
    virtual void DefaultForeground(uint32_t value) = 0;
    virtual uint32_t DefaultBackground() const noexcept = 0;
    virtual void DefaultBackground(uint32_t value) = 0;
    virtual std::basic_string_view<uint32_t> GetColorTable() const noexcept = 0;
    virtual void SetColorTable(std::basic_string_view<uint32_t const> value) = 0;
    virtual int32_t HistorySize() const noexcept = 0;
    virtual void HistorySize(int32_t value) = 0;
    virtual int32_t InitialRows() const noexcept = 0;
    virtual void InitialRows(int32_t value) = 0;
    virtual int32_t InitialCols() const noexcept = 0;
    virtual void InitialCols(int32_t value) = 0;
    virtual bool SnapOnInput() const noexcept = 0;
    virtual void SnapOnInput(bool value) = 0;

};

// See docs/virtual-dtors.md for an explanation of why this is weird.
inline Microsoft::Terminal::Core::ITerminalSettings::~ITerminalSettings() { }
