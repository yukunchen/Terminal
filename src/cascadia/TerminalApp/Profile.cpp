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
using namespace winrt::Windows::Data::Json;


const std::wstring NAME_KEY{ L"name" };
const std::wstring GUID_KEY{ L"guid" };
const std::wstring COMMANDLINE_KEY{ L"commandline" };
const std::wstring FONTFACE_KEY{ L"fontFace" };
const std::wstring FONTSIZE_KEY{ L"fontSize" };
const std::wstring ACRYLICTRANSPARENCY_KEY{ L"acrylicOpacity" };
const std::wstring USEACRYLIC_KEY{ L"useAcrylic" };
const std::wstring SHOWSCROLLBARS_KEY{ L"showScrollbars" };

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


std::wstring GuidToString(GUID guid)
{
    wchar_t guid_cstr[39];
    swprintf(guid_cstr, sizeof(guid_cstr),
             L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    return std::wstring(guid_cstr);
}

JsonObject Profile::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    const auto guidStr = GuidToString(_guid);

    const auto guid = JsonValue::CreateStringValue(guidStr);
    const auto name = JsonValue::CreateStringValue(_name);
    const auto cmdline = JsonValue::CreateStringValue(_commandline);
    const auto fontFace = JsonValue::CreateStringValue(_fontFace);
    const auto fontSize = JsonValue::CreateNumberValue(_fontSize);
    const auto acrylicTransparency = JsonValue::CreateNumberValue(_acrylicTransparency);
    const auto useAcrylic = JsonValue::CreateBooleanValue(_useAcrylic);
    const auto showScrollbars = JsonValue::CreateBooleanValue(_showScrollbars);

    jsonObject.Insert(NAME_KEY,                name);
    jsonObject.Insert(GUID_KEY,                guid);
    jsonObject.Insert(COMMANDLINE_KEY,         cmdline);
    jsonObject.Insert(FONTFACE_KEY,            fontFace);
    jsonObject.Insert(FONTSIZE_KEY,            fontSize);
    jsonObject.Insert(ACRYLICTRANSPARENCY_KEY, acrylicTransparency);
    jsonObject.Insert(USEACRYLIC_KEY,          useAcrylic);
    jsonObject.Insert(SHOWSCROLLBARS_KEY,      showScrollbars);

    return jsonObject;
}
