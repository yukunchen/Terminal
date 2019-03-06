
#pragma once
#include <winrt/Microsoft.Terminal.Core.h>
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>
#include "../../inc/conattrs.hpp"
#include <conattrs.hpp>

namespace Microsoft::Terminal::TerminalApp
{
    class ColorScheme;
};

class Microsoft::Terminal::TerminalApp::ColorScheme
{

public:
    ColorScheme();
    ColorScheme(std::wstring name, COLORREF defaultFg, COLORREF defaultBg);
    ~ColorScheme();

    void ApplyScheme(winrt::Microsoft::Terminal::TerminalApp::TerminalSettings terminalSettings) const;

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<ColorScheme> FromJson(winrt::Windows::Data::Json::JsonObject json);

    std::wstring_view GetName() const noexcept;
    std::array<COLORREF, COLOR_TABLE_SIZE>& GetTable() noexcept;
    COLORREF GetForeground() const noexcept;
    COLORREF GetBackground() const noexcept;

private:
    std::wstring _schemeName;
    std::array<COLORREF, COLOR_TABLE_SIZE> _table;
    COLORREF _defaultForeground;
    COLORREF _defaultBackground;
};
