#include "pch.h"
#include "TerminalSettings.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    TerminalSettings::TerminalSettings() :
        _defaultForeground{ 0xffffffff },
        _defaultBackground{ 0x00000000 },
        _colorTable{},
        _historySize{ 9001 },
        _initialRows{ 30 },
        _initialCols{ 80 },
        _snapOnInput{ true },
        _useAcrylic{ false },
        _tintOpacity{ 0.5 },
        _fontFace{ L"Consolas" },
        _fontSize{ 12 },
        _keyBindings{ nullptr }
    {

    }

    winrt::Microsoft::Terminal::Core::ICoreSettings TerminalSettings::GetSettings()
    {
        return *this;
    }

    uint32_t TerminalSettings::DefaultForeground()
    {
        return _defaultForeground;
    }

    void TerminalSettings::DefaultForeground(uint32_t value)
    {
        _defaultForeground = value;
    }

    uint32_t TerminalSettings::DefaultBackground()
    {
        return _defaultBackground;
    }

    void TerminalSettings::DefaultBackground(uint32_t value)
    {
        _defaultBackground = value;
    }

    std::basic_string_view<uint32_t> TerminalSettings::GetColorTable()
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

    int32_t TerminalSettings::HistorySize()
    {
        return _historySize;
    }

    void TerminalSettings::HistorySize(int32_t value)
    {
        _historySize = value;
    }

    int32_t TerminalSettings::InitialRows()
    {
        return _initialRows;
    }

    void TerminalSettings::InitialRows(int32_t value)
    {
        _initialRows = value;
    }

    int32_t TerminalSettings::InitialCols()
    {
        return _initialCols;
    }

    void TerminalSettings::InitialCols(int32_t value)
    {
        _initialCols = value;
    }

    bool TerminalSettings::SnapOnInput()
    {
        return _snapOnInput;
    }

    void TerminalSettings::SnapOnInput(bool value)
    {
        _snapOnInput = value;
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

    hstring TerminalSettings::Commandline()
    {
        return _commandline;
    }

    void TerminalSettings::Commandline(hstring const& value)
    {
        _commandline = value;
    }

    hstring TerminalSettings::WorkingDirectory()
    {
        return _workingDir;
    }

    void TerminalSettings::WorkingDirectory(hstring const& value)
    {
        _workingDir = value;
    }

    hstring TerminalSettings::EnvironmentVariables()
    {
        return _envVars;
    }

    void TerminalSettings::EnvironmentVariables(hstring const& value)
    {
        _envVars = value;
    }

}
