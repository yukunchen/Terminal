/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "ColorScheme.h"
#include "../../types/inc/Utils.hpp"

// #include <winrt/Microsoft.Terminal.TerminalApp.h>

using namespace Microsoft::Terminal::TerminalApp;
using namespace ::Microsoft::Console;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace winrt::Microsoft::Terminal::TerminalApp;
using namespace winrt::Windows::Data::Json;

const std::wstring NAME_KEY{ L"name" };
const std::wstring TABLE_KEY{ L"colors" };
const std::wstring FOREGROUND_KEY{ L"foreground" };
const std::wstring BACKGROUND_KEY{ L"background" };

ColorScheme::ColorScheme() :
    _schemeName{ L"" },
    _table{  },
    _defaultForeground{ RGB(242, 242, 242) },
    _defaultBackground{ RGB(12, 12, 12) }
{

}

ColorScheme::~ColorScheme()
{

}

void ColorScheme::ApplyScheme(TerminalSettings terminalSettings) const
{
    terminalSettings.DefaultForeground(_defaultForeground);
    terminalSettings.DefaultBackground(_defaultBackground);

    for (int i = 0; i < _table.size(); i++)
    {
        terminalSettings.SetColorTableEntry(i, _table[i]);
    }
}

JsonObject ColorScheme::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;
    auto fg = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultForeground));
    auto bg = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultBackground));
    auto name = JsonValue::CreateStringValue(_schemeName);
    JsonArray tableArray{};
    for (auto& color : _table)
    {
        auto s = Utils::ColorToHexString(color);
        tableArray.Append(JsonValue::CreateStringValue(s));
    }

    jsonObject.Insert(NAME_KEY, name);
    jsonObject.Insert(FOREGROUND_KEY, fg);
    jsonObject.Insert(BACKGROUND_KEY, bg);
    jsonObject.Insert(TABLE_KEY, tableArray);

    return jsonObject;
}

std::unique_ptr<ColorScheme> ColorScheme::FromJson(winrt::Windows::Data::Json::JsonObject json)
{
    std::unique_ptr<ColorScheme> resultPtr = std::make_unique<ColorScheme>();
    ColorScheme& result = *resultPtr;

    if (json.HasKey(NAME_KEY))
    {
        result._schemeName = json.GetNamedString(NAME_KEY);
    }
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
    if (json.HasKey(TABLE_KEY))
    {
        auto table = json.GetNamedArray(TABLE_KEY);
        int i = 0;
        for (auto v : table)
        {
            if (v.ValueType() == JsonValueType::String)
            {
                auto str = v.GetString();
                auto color = Utils::ColorFromHexString(str.c_str());
                result._table[i] = color;
            }
            i++;
        }
    }

    return std::move(resultPtr);
}
