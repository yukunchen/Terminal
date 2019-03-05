#pragma once

#include "TerminalSettings.g.h"

namespace winrt::Microsoft::Terminal::TerminalApp::implementation
{
    struct TerminalSettings : TerminalSettingsT<TerminalSettings>
    {
        TerminalSettings();

    public:
        // --------------------------- Core Settings ---------------------------
        //  All of these settings are defined in ICoreSettings.
        uint32_t DefaultForeground();
        void DefaultForeground(uint32_t value);
        uint32_t DefaultBackground();
        void DefaultBackground(uint32_t value);
        uint32_t GetColorTableEntry(int32_t index) const;
        void SetColorTableEntry(int32_t index, uint32_t value);
        int32_t HistorySize();
        void HistorySize(int32_t value);
        int32_t InitialRows();
        void InitialRows(int32_t value);
        int32_t InitialCols();
        void InitialCols(int32_t value);
        bool SnapOnInput();
        void SnapOnInput(bool value);
        // ------------------------ End of Core Settings -----------------------

        bool UseAcrylic();
        void UseAcrylic(bool value);
        double TintOpacity();
        void TintOpacity(double value);

        hstring FontFace();
        void FontFace(hstring const& value);
        int32_t FontSize();
        void FontSize(int32_t value);

        winrt::Microsoft::Terminal::TerminalControl::IKeyBindings KeyBindings();
        void KeyBindings(winrt::Microsoft::Terminal::TerminalControl::IKeyBindings const& value);

        hstring Commandline();
        void Commandline(hstring const& value);

        hstring WorkingDirectory();
        void WorkingDirectory(hstring const& value);

        hstring EnvironmentVariables();
        void EnvironmentVariables(hstring const& value);

        winrt::Microsoft::Terminal::Core::ICoreSettings GetSettings();

    private:
        uint32_t _defaultForeground;
        uint32_t _defaultBackground;
        std::array<uint32_t, 16> _colorTable;
        int32_t _historySize;
        int32_t _initialRows;
        int32_t _initialCols;
        bool _snapOnInput;

        bool _useAcrylic;
        double _tintOpacity;
        hstring _fontFace;
        int32_t _fontSize;
        hstring _commandline;
        hstring _workingDir;
        hstring _envVars;
        TerminalControl::IKeyBindings _keyBindings;
    };
}

namespace winrt::Microsoft::Terminal::TerminalApp::factory_implementation
{
    struct TerminalSettings : TerminalSettingsT<TerminalSettings, implementation::TerminalSettings>
    {
    };
}
