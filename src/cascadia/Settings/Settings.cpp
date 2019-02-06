#include "pch.h"
#include "Settings.h"

namespace winrt::TerminalSettings::implementation
{
    Settings::Settings() :
        _defaultForeground{ 0xffffffff },
        _defaultBackground{ 0x00000000 },
        _historySize{ 9001 },
        _initialRows{ 30 },
        _initialCols{ 80 }
    {

    }

    uint32_t Settings::DefaultForeground()
    {
        return _defaultForeground;
    }

    void Settings::DefaultForeground(uint32_t value)
    {
        _defaultForeground = value;
    }

    uint32_t Settings::DefaultBackground()
    {
        return _defaultForeground;
    }

    void Settings::DefaultBackground(uint32_t value)
    {
        _defaultForeground = value;
    }

    com_array<uint32_t> Settings::ColorTable()
    {
        throw hresult_not_implemented();
    }

    void Settings::ColorTable(array_view<uint32_t const> value)
    {
        throw hresult_not_implemented();
    }

    int32_t Settings::HistorySize()
    {
        return _historySize;
    }

    void Settings::HistorySize(int32_t value)
    {
        _historySize = value;
    }

    int32_t Settings::InitialRows()
    {
        return _initialRows;
    }

    void Settings::InitialRows(int32_t value)
    {
        _initialRows = value;
    }

    int32_t Settings::InitialCols()
    {
        return _initialCols;
    }

    void Settings::InitialCols(int32_t value)
    {
        _initialCols = value;
    }
}
