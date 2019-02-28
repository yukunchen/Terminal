#include "pch.h"
#include "TerminalSettings.h"

using namespace ::Microsoft::Terminal::Core;

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
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

    uint32_t TerminalSettings::DefaultForeground() const noexcept
    {
        return _defaultForeground;
    }

    void TerminalSettings::DefaultForeground(uint32_t value)
    {
        _defaultForeground = value;
    }

    uint32_t TerminalSettings::DefaultBackground() const noexcept
    {
        return _defaultBackground;
    }

    void TerminalSettings::DefaultBackground(uint32_t value)
    {
        _defaultBackground = value;
    }

    std::basic_string_view<uint32_t> TerminalSettings::GetColorTable() const noexcept
    {
        return std::basic_string_view<uint32_t>(&_colorTable[0], _colorTable.size());
    }

    void TerminalSettings::SetColorTable(std::basic_string_view<uint32_t const> value)
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

    uint32_t TerminalSettings::GetColorTableEntry(int32_t index) const
    {
        return _colorTable[index];
    }

    void TerminalSettings::SetColorTableEntry(int32_t index, uint32_t value)
    {
        if (index > _colorTable.size())
        {
            throw E_INVALIDARG;
        }
        _colorTable[index] = value;
    }

    int32_t TerminalSettings::HistorySize() const noexcept
    {
        return _historySize;
    }

    void TerminalSettings::HistorySize(int32_t value)
    {
        _historySize = value;
    }

    int32_t TerminalSettings::InitialRows() const noexcept
    {
        return _initialRows;
    }

    void TerminalSettings::InitialRows(int32_t value)
    {
        _initialRows = value;
    }

    int32_t TerminalSettings::InitialCols() const noexcept
    {
        return _initialCols;
    }

    void TerminalSettings::InitialCols(int32_t value)
    {
        _initialCols = value;
    }

    bool TerminalSettings::SnapOnInput() const noexcept
    {
        return _snapOnInput;
    }

    void TerminalSettings::SnapOnInput(bool value)
    {
        _snapOnInput = value;
    }

    bool TerminalSettings::UseAcrylic() const noexcept
    {
        return _useAcrylic;
    }

    void TerminalSettings::UseAcrylic(bool value)
    {
        _useAcrylic = value;
    }

    double TerminalSettings::TintOpacity() const noexcept
    {
        return _tintOpacity;
    }

    void TerminalSettings::TintOpacity(double value)
    {
        _tintOpacity = value;
    }

    hstring TerminalSettings::FontFace() const noexcept
    {
        return _fontFace;
    }

    void TerminalSettings::FontFace(hstring const& value)
    {
        _fontFace = value;
    }

    int32_t TerminalSettings::FontSize() const noexcept
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

    hstring TerminalSettings::Commandline() const noexcept
    {
        return _commandline;
    }

    void TerminalSettings::Commandline(hstring const& value)
    {
        _commandline = value;
    }

    hstring TerminalSettings::WorkingDirectory() const noexcept
    {
        return _workingDir;
    }

    void TerminalSettings::WorkingDirectory(hstring const& value)
    {
        _workingDir = value;
    }

    hstring TerminalSettings::EnvironmentVariables() const noexcept
    {
        return _envVars;
    }

    void TerminalSettings::EnvironmentVariables(hstring const& value)
    {
        _envVars = value;
    }

}
