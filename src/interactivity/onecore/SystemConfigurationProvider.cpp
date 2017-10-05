/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "SystemConfigurationProvider.hpp"

#define DEFAULT_TT_FONT_FACENAME L"__DefaultTTFont__"

#define DEFAULT_CARET_BLINK_TIME                  0x212
#define DEFAULT_IS_CARET_BLINKING_ENABLED         true
#define DEFAULT_NUMBER_OF_MOUSE_BUTTONS           3
#define DEFAULT_NUMBER_OF_WHEEL_SCROLL_LINES      3
#define DEFAULT_NUMBER_OF_WHEEL_SCROLL_CHARACTERS 3

using namespace Microsoft::Console::Interactivity::OneCore;

UINT SystemConfigurationProvider::GetCaretBlinkTime()
{
    return DEFAULT_CARET_BLINK_TIME;
}

bool SystemConfigurationProvider::IsCaretBlinkingEnabled()
{
    return DEFAULT_IS_CARET_BLINKING_ENABLED;
}

int SystemConfigurationProvider::GetNumberOfMouseButtons()
{
    if (IsGetSystemMetricsPresent())
    {
        return GetSystemMetrics(SM_CMOUSEBUTTONS);
    }
    else
    {
        return DEFAULT_NUMBER_OF_MOUSE_BUTTONS;
    }
}

ULONG SystemConfigurationProvider::GetNumberOfWheelScrollLines()
{
    return DEFAULT_NUMBER_OF_WHEEL_SCROLL_LINES;
}

ULONG SystemConfigurationProvider::GetNumberOfWheelScrollCharacters()
{
    return DEFAULT_NUMBER_OF_WHEEL_SCROLL_CHARACTERS;
}

void SystemConfigurationProvider::GetSettingsFromLink(
    _Inout_ Settings* pLinkSettings,
    _Inout_updates_bytes_(*pdwTitleLength) LPWSTR pwszTitle,
    _Inout_ PDWORD pdwTitleLength,
    _In_ PCWSTR pwszCurrDir,
    _In_ PCWSTR pwszAppName)
{
    UNREFERENCED_PARAMETER(pwszTitle);
    UNREFERENCED_PARAMETER(pdwTitleLength);
    UNREFERENCED_PARAMETER(pwszCurrDir);
    UNREFERENCED_PARAMETER(pwszAppName);

    // While both OneCore console renderers use TrueType fonts, there is no
    // advanced font support on that platform. Namely, there is no way to pick
    // neither the font nor the font size. Since this choice of TrueType font
    // is made implicitly by the renderers, the rest of the console is not aware
    // of it and the renderer procedure goes on to translate output text so that
    // it be renderable with raster fonts, which messes up the final output.
    // Hence, we make it seem like the console is in fact configred to use a
    // TrueType font by the user.

    pLinkSettings->SetFaceName(DEFAULT_TT_FONT_FACENAME, ARRAYSIZE(DEFAULT_TT_FONT_FACENAME));
    pLinkSettings->SetFontFamily(TMPF_TRUETYPE);

    return;
}
