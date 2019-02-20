/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "precomp.h"
#include "Settings.h"

Settings::Settings() :
    _defaultForeground{ 0xffffffff },
    _defaultBackground{ 0x00000000 },
    _historySize{ 9001 },
    _initialRows{ 30 },
    _initialCols{ 80 },
    _snapOnInput{ true }
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
    return _defaultBackground;
}

void Settings::DefaultBackground(uint32_t value)
{
    _defaultBackground = value;
}

std::basic_string_view<uint32_t> Settings::GetColorTable()
{
    throw E_NOTIMPL;
}

void Settings::SetColorTable(std::basic_string_view<uint32_t const> /*value*/)
{
    throw E_NOTIMPL;
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

bool Settings::SnapOnInput()
{
    return _snapOnInput;
}

void Settings::SnapOnInput(bool value)
{
    _snapOnInput = value;
}
