/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Profile.h"

using namespace Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace ::Microsoft::Terminal::Core;


Profile::Profile() :
    _guid{},
    _name{ L"Default" },
    _schemeName{},
    _coreSettings{},
    _commandline{ L"cmd.exe" },
    _fontFace{ L"Consolas" },
    _fontSize{ 12 },
    _acrylicTransparency{ 0.5 },
    _useAcrylic{ false },
    _showScrollbars{ true }
{
    UuidCreate(&_guid);
}

Profile::~Profile()
{

}


void _SetFromCoreSettings(const Settings& sourceSettings,
                          TerminalSettings terminalSettings)
{
    terminalSettings.DefaultForeground(sourceSettings.DefaultForeground());
    terminalSettings.DefaultBackground(sourceSettings.DefaultBackground());

    auto sourceTable = sourceSettings.GetColorTable();
    for (int i = 0; i < sourceTable.size(); i++)
    {
        terminalSettings.SetColorTableEntry(i, sourceTable[i]);
    }

    terminalSettings.HistorySize(sourceSettings.HistorySize());
    terminalSettings.InitialRows(sourceSettings.InitialRows());
    terminalSettings.InitialCols(sourceSettings.InitialCols());
    terminalSettings.SnapOnInput(sourceSettings.SnapOnInput());
}

ColorScheme* _FindScheme(std::vector<std::unique_ptr<ColorScheme>>& schemes,
                         const std::wstring& schemeName)
{
    for (auto& scheme : schemes)
    {
        if (scheme->_schemeName == schemeName)
        {
            return scheme.get();
        }
    }
    return nullptr;
}


TerminalSettings Profile::CreateTerminalSettings(std::vector<std::unique_ptr<ColorScheme>>& schemes) const
{
    TerminalSettings terminalSettings{};

    // Fill in the Terminal Setting's CoreSettings from the profile
    _SetFromCoreSettings(_coreSettings, terminalSettings);

    // Fill in the remaining properties from the profile
    terminalSettings.UseAcrylic(_useAcrylic);
    terminalSettings.TintOpacity(_acrylicTransparency);

    terminalSettings.FontFace(_fontFace);
    terminalSettings.FontSize(_fontSize);

    terminalSettings.Commandline(winrt::to_hstring(_commandline.c_str()));

    if (_schemeName)
    {
        const ColorScheme* const matchingScheme = _FindScheme(schemes, _schemeName.value());
        if (matchingScheme)
        {
            matchingScheme->ApplyScheme(terminalSettings);
        }
    }

    return terminalSettings;
}
