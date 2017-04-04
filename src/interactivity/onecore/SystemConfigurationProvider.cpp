/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "SystemConfigurationProvider.hpp"

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
    return DEFAULT_NUMBER_OF_MOUSE_BUTTONS;
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
    UNREFERENCED_PARAMETER(pLinkSettings);
    UNREFERENCED_PARAMETER(pwszTitle);
    UNREFERENCED_PARAMETER(pdwTitleLength);
    UNREFERENCED_PARAMETER(pwszCurrDir);
    UNREFERENCED_PARAMETER(pwszAppName);

    return;
}
