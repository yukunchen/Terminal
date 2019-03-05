
#pragma once
#include "ColorScheme.h"

// #include <winrt/Microsoft.Terminal.TerminalApp.h>

namespace Microsoft::Terminal::TerminalApp
{
    class Profile;
};

class Microsoft::Terminal::TerminalApp::Profile
{

public:
    Profile();
    ~Profile();

    winrt::Microsoft::Terminal::TerminalApp::TerminalSettings CreateTerminalSettings(const std::vector<std::unique_ptr<::Microsoft::Terminal::TerminalApp::ColorScheme>>& schemes) const;

    winrt::Windows::Data::Json::JsonObject ToJson() const;
    static std::unique_ptr<Profile> FromJson(winrt::Windows::Data::Json::JsonObject json);

    GUID GetGuid() const noexcept;

    void SetFontFace(std::wstring fontFace) noexcept;
    void SetColorScheme(std::optional<std::wstring> schemeName) noexcept;
    void SetAcrylicOpacity(double opacity) noexcept;
    void SetCommandline(std::wstring cmdline) noexcept;
    void SetName(std::wstring name) noexcept;
    void SetUseAcrylic(bool useAcrylic) noexcept;
    void SetDefaultForeground(COLORREF defaultForeground) noexcept;
    void SetDefaultBackground(COLORREF defaultBackground) noexcept;

private:

    GUID _guid;
    std::wstring _name;

    std::optional<std::wstring> _schemeName;

    uint32_t _defaultForeground;
    uint32_t _defaultBackground;
    std::array<uint32_t, 16> _colorTable;
    int32_t _historySize;
    int32_t _initialRows;
    int32_t _initialCols;
    bool _snapOnInput;

    std::wstring _commandline;
    std::wstring _fontFace;
    int32_t _fontSize;
    double _acrylicTransparency;
    bool _useAcrylic;

    //MSFT:20738109: Scrollbar state should be an enum: Always Visible, Reveal on Hover, Always Hide
    bool _showScrollbars;

};
