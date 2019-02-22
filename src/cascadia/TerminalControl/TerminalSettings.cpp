#include "pch.h"
#include "TerminalSettings.h"

namespace winrt::Microsoft::Terminal::TerminalControl::implementation
{

    TerminalSettings::TerminalSettings() :
        _settings{},
        _useAcrylic{ false },
        _tintOpacity{ 0.5 },
        _fontFace{ L"Consolas" },
        _fontSize{ 12 },
        _keyBindings{ nullptr }
    {

    }

    uint32_t TerminalSettings::DefaultForeground()
    {
        return _settings.DefaultForeground();
    }

    void TerminalSettings::DefaultForeground(uint32_t value)
    {
        _settings.DefaultForeground(value);
    }

    uint32_t TerminalSettings::DefaultBackground()
    {
        return _settings.DefaultBackground();
    }

    void TerminalSettings::DefaultBackground(uint32_t value)
    {
        _settings.DefaultBackground(value);
    }

    std::basic_string_view<uint32_t> TerminalSettings::GetColorTable()
    {
        return _settings.GetColorTable();
    }

    void TerminalSettings::SetColorTable(std::basic_string_view<uint32_t const> value)
    {
        _settings.SetColorTable(value);
    }

    uint32_t TerminalSettings::GetColorTableEntry(int32_t index)
    {
        return _settings.GetColorTable()[index];
    }

    void TerminalSettings::SetColorTableEntry(int32_t /*index*/, uint32_t /*value*/)
    {
        throw E_NOTIMPL;
    }

    int32_t TerminalSettings::HistorySize()
    {
        return _settings.HistorySize();
    }

    void TerminalSettings::HistorySize(int32_t value)
    {
        _settings.HistorySize(value);
    }

    int32_t TerminalSettings::InitialRows()
    {
        return _settings.InitialRows();
    }

    void TerminalSettings::InitialRows(int32_t value)
    {
        _settings.InitialRows(value);
    }

    int32_t TerminalSettings::InitialCols()
    {
        return _settings.InitialCols();
    }

    void TerminalSettings::InitialCols(int32_t value)
    {
        _settings.InitialCols(value);
    }

    bool TerminalSettings::SnapOnInput()
    {
        return _settings.SnapOnInput();
    }

    void TerminalSettings::SnapOnInput(bool value)
    {
        _settings.SnapOnInput(value);
    }


    bool TerminalSettings::UseAcrylic()
    {
        return _useAcrylic;
    }

    void TerminalSettings::UseAcrylic(bool value)
    {
        _useAcrylic = value;
    }

    double TerminalSettings::TintOpacity()
    {
        return _tintOpacity;
    }

    void TerminalSettings::TintOpacity(double value)
    {
        _tintOpacity = value;
    }

    hstring TerminalSettings::FontFace()
    {
        return _fontFace;
    }

    void TerminalSettings::FontFace(hstring const& value)
    {
        _fontFace = value;
    }

    int32_t TerminalSettings::FontSize()
    {
        return _fontSize;
    }

    void TerminalSettings::FontSize(int32_t value)
    {
        _fontSize = value;
    }

    TerminalControl::IKeyBindings TerminalSettings::KeyBindings()
    {
        return _keyBindings;
    }

    void TerminalSettings::KeyBindings(TerminalControl::IKeyBindings const& value)
    {
        _keyBindings = value;
    }

}
