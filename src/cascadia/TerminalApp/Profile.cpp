/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Profile.h"
#include "../../types/inc/Utils.hpp"

using namespace Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalApp;
using namespace winrt::Windows::Data::Json;
using namespace ::Microsoft::Console;


const std::wstring NAME_KEY{ L"name" };
const std::wstring GUID_KEY{ L"guid" };
const std::wstring COLORSCHEME_KEY{ L"colorscheme" };

const std::wstring FOREGROUND_KEY{ L"foreground" };
const std::wstring BACKGROUND_KEY{ L"background" };
const std::wstring COLORTABLE_KEY{ L"colorTable" };
const std::wstring HISTORYSIZE_KEY{ L"historySize" };
const std::wstring INITIALROWS_KEY{ L"initialRows" };
const std::wstring INITIALCOLS_KEY{ L"initialCols" };
const std::wstring SNAPONINPUT_KEY{ L"snapOnInput" };

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

    _defaultForeground{ 0xffffffff },
    _defaultBackground{ 0x00000000 },
    _colorTable{},
    _historySize{ 9001 },
    _initialRows{ 30 },
    _initialCols{ 80 },
    _snapOnInput{ true },

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

GUID Profile::GetGuid() const noexcept
{
    return _guid;
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
    terminalSettings.DefaultForeground(_defaultForeground);
    terminalSettings.DefaultBackground(_defaultBackground);
    for (int i = 0; i < _colorTable.size(); i++)
    {
        terminalSettings.SetColorTableEntry(i, _colorTable[i]);
    }
    terminalSettings.HistorySize(_historySize);
    terminalSettings.InitialRows(_initialRows);
    terminalSettings.InitialCols(_initialCols);
    terminalSettings.SnapOnInput(_snapOnInput);

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

JsonObject Profile::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    // Profile-specific settings
    const auto guidStr = Utils::GuidToString(_guid);
    const auto guid = JsonValue::CreateStringValue(guidStr);
    const auto name = JsonValue::CreateStringValue(_name);

    // Core Settings
    const auto historySize = JsonValue::CreateNumberValue(_historySize);
    const auto initialRows = JsonValue::CreateNumberValue(_initialRows);
    const auto initialCols = JsonValue::CreateNumberValue(_initialCols);
    const auto snapOnInput = JsonValue::CreateBooleanValue(_snapOnInput);

    // Control Settings
    const auto cmdline = JsonValue::CreateStringValue(_commandline);
    const auto fontFace = JsonValue::CreateStringValue(_fontFace);
    const auto fontSize = JsonValue::CreateNumberValue(_fontSize);
    const auto acrylicTransparency = JsonValue::CreateNumberValue(_acrylicTransparency);
    const auto useAcrylic = JsonValue::CreateBooleanValue(_useAcrylic);
    const auto showScrollbars = JsonValue::CreateBooleanValue(_showScrollbars);

    jsonObject.Insert(GUID_KEY,                guid);
    jsonObject.Insert(NAME_KEY,                name);

    if (_schemeName)
    {
        const auto scheme = JsonValue::CreateStringValue(_schemeName.value());
        jsonObject.Insert(COLORSCHEME_KEY,     scheme);
    }
    else
    {
        const auto defaultForeground = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultForeground));
        const auto defaultBackground = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultBackground));

        JsonArray tableArray{};
        for (auto& color : _colorTable)
        {
            auto s = Utils::ColorToHexString(color);
            tableArray.Append(JsonValue::CreateStringValue(s));
        }

        jsonObject.Insert(FOREGROUND_KEY,      defaultForeground);
        jsonObject.Insert(BACKGROUND_KEY,      defaultBackground);
        jsonObject.Insert(COLORTABLE_KEY,      tableArray);

    }

    jsonObject.Insert(HISTORYSIZE_KEY,         historySize);
    jsonObject.Insert(INITIALROWS_KEY,         initialRows);
    jsonObject.Insert(INITIALCOLS_KEY,         initialCols);
    jsonObject.Insert(SNAPONINPUT_KEY,         snapOnInput);

    jsonObject.Insert(COMMANDLINE_KEY,         cmdline);
    jsonObject.Insert(FONTFACE_KEY,            fontFace);
    jsonObject.Insert(FONTSIZE_KEY,            fontSize);
    jsonObject.Insert(ACRYLICTRANSPARENCY_KEY, acrylicTransparency);
    jsonObject.Insert(USEACRYLIC_KEY,          useAcrylic);
    jsonObject.Insert(SHOWSCROLLBARS_KEY,      showScrollbars);

    return jsonObject;
}

std::unique_ptr<Profile> Profile::FromJson(winrt::Windows::Data::Json::JsonObject json)
{
    std::unique_ptr<Profile> resultPtr = std::make_unique<Profile>();
    Profile& result = *resultPtr;

    if (json.HasKey(NAME_KEY))
    {
        result._name = json.GetNamedString(NAME_KEY);
    }
    if (json.HasKey(GUID_KEY))
    {
        auto guidString = json.GetNamedString(GUID_KEY);
        auto guid = Utils::GuidFromString(guidString.c_str());
        result._guid = guid;
    }

    if (json.HasKey(GUID_KEY))
    {
        auto guidString = json.GetNamedString(GUID_KEY);
        auto guid = Utils::GuidFromString(guidString.c_str());
        result._guid = guid;
    }


    if (json.HasKey(COLORSCHEME_KEY))
    {
        result._schemeName = json.GetNamedString(COLORSCHEME_KEY);
    }
    else
    {
        if (json.HasKey(FOREGROUND_KEY))
        {
            auto fgString = json.GetNamedString(FOREGROUND_KEY);
            auto color = Utils::ColorFromHexString(fgString.c_str());
            result._defaultForeground = color;
        }
        if (json.HasKey(BACKGROUND_KEY))
        {
            auto bgString = json.GetNamedString(BACKGROUND_KEY);
            auto color = Utils::ColorFromHexString(bgString.c_str());
            result._defaultBackground = color;
        }
        if (json.HasKey(COLORTABLE_KEY))
        {
            auto table = json.GetNamedArray(COLORTABLE_KEY);
            int i = 0;
            for (auto v : table)
            {
                if (v.ValueType() == JsonValueType::String)
                {
                    auto str = v.GetString();
                    auto color = Utils::ColorFromHexString(str.c_str());
                    result._colorTable[i] = color;
                }
                i++;
            }
        }
    }
    if (json.HasKey(HISTORYSIZE_KEY))
    {
        result._historySize = json.GetNamedNumber(HISTORYSIZE_KEY);
    }
    if (json.HasKey(INITIALROWS_KEY))
    {
        result._initialRows = json.GetNamedNumber(INITIALROWS_KEY);
    }
    if (json.HasKey(INITIALCOLS_KEY))
    {
        result._initialCols = json.GetNamedNumber(INITIALCOLS_KEY);
    }
    if (json.HasKey(SNAPONINPUT_KEY))
    {
        result._snapOnInput = json.GetNamedBoolean(SNAPONINPUT_KEY);
    }


    if (json.HasKey(COMMANDLINE_KEY))
    {
        result._commandline = json.GetNamedString(COMMANDLINE_KEY);
    }
    if (json.HasKey(FONTFACE_KEY))
    {
        result._fontFace = json.GetNamedString(FONTFACE_KEY);
    }
    if (json.HasKey(FONTSIZE_KEY))
    {
        result._fontSize = json.GetNamedNumber(FONTSIZE_KEY);
    }
    if (json.HasKey(ACRYLICTRANSPARENCY_KEY))
    {
        result._acrylicTransparency = json.GetNamedNumber(ACRYLICTRANSPARENCY_KEY);
    }
    if (json.HasKey(USEACRYLIC_KEY))
    {
        result._useAcrylic = json.GetNamedBoolean(USEACRYLIC_KEY);
    }
    if (json.HasKey(SHOWSCROLLBARS_KEY))
    {
        result._showScrollbars = json.GetNamedBoolean(SHOWSCROLLBARS_KEY);
    }

    return std::move(resultPtr);
}
