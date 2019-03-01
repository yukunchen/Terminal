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

std::wstring ColorToHexString(COLORREF color)
{
    std::wstringstream ss;
    ss << L"#" << std::uppercase << std::setfill(L'0') << std::hex;
    ss << std::setw(2) << GetRValue(color);
    ss << std::setw(2) << GetGValue(color);
    ss << std::setw(2) << GetBValue(color);

    return ss.str();
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
