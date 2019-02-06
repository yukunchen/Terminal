#pragma once

class ITerminalSettings
{
    // This will cause compilation errors:
    //virtual ~ITerminalSettings() = 0;
public:
    virtual uint32_t DefaultForeground() = 0;
    virtual void DefaultForeground(uint32_t value) = 0;
    virtual uint32_t DefaultBackground() = 0;
    virtual void DefaultBackground(uint32_t value) = 0;
    virtual std::basic_string_view<uint32_t> GetColorTable() = 0;
    virtual void SetColorTable(std::basic_string_view<uint32_t const> value) = 0;
    virtual int32_t HistorySize() = 0;
    virtual void HistorySize(int32_t value) = 0;
    virtual int32_t InitialRows() = 0;
    virtual void InitialRows(int32_t value) = 0;
    virtual int32_t InitialCols() = 0;
    virtual void InitialCols(int32_t value) = 0;

private:
    uint32_t _defaultForeground;
    uint32_t _defaultBackground;
    int32_t _historySize;
    int32_t _initialRows;
    int32_t _initialCols;
};

//inline ITerminalSettings::~ITerminalSettings() { }
