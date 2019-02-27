/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Settings.h"
using namespace Microsoft::Terminal::Core;

Settings::Settings() :
    _defaultForeground{ 0xffffffff },
    _defaultBackground{ 0x00000000 },
    _colorTable{},
    _historySize{ 9001 },
    _initialRows{ 30 },
    _initialCols{ 80 },
    _snapOnInput{ true }
{

}

uint32_t Settings::DefaultForeground() const noexcept
{
    return _defaultForeground;
}

void Settings::DefaultForeground(uint32_t value)
{
    _defaultForeground = value;
}

uint32_t Settings::DefaultBackground() const noexcept
{
    return _defaultBackground;
}

void Settings::DefaultBackground(uint32_t value)
{
    _defaultBackground = value;
}

std::basic_string_view<uint32_t> Settings::GetColorTable() const noexcept
{
    return std::basic_string_view<uint32_t>(&_colorTable[0], _colorTable.size());
}

void Settings::SetColorTable(std::basic_string_view<uint32_t const> value)
{
    if (value.size() != _colorTable.size())
    {
        throw E_INVALIDARG;
    }
    for (int i = 0; i < value.size(); i++)
    {
        _colorTable[i] = value[i];
    }
}

uint32_t Settings::GetColorTableEntry(int32_t index) const
{
    return _colorTable[index];
}

void Settings::SetColorTableEntry(int32_t index, uint32_t value)
{
    if (index > _colorTable.size())
    {
        throw E_INVALIDARG;
    }
    _colorTable[index] = value;
}

int32_t Settings::HistorySize() const noexcept
{
    return _historySize;
}

void Settings::HistorySize(int32_t value)
{
    _historySize = value;
}

int32_t Settings::InitialRows() const noexcept
{
    return _initialRows;
}

void Settings::InitialRows(int32_t value)
{
    _initialRows = value;
}

int32_t Settings::InitialCols() const noexcept
{
    return _initialCols;
}

void Settings::InitialCols(int32_t value)
{
    _initialCols = value;
}

bool Settings::SnapOnInput() const noexcept
{
    return _snapOnInput;
}

void Settings::SnapOnInput(bool value)
{
    _snapOnInput = value;
}
