/*++
Copyright (c) Microsoft Corporation

Module Name:
- DefaultSettings.h

Abstract:
- A header with a bunch of default values used for settings, especially for
        terminal settings used by Cascadia
Author(s):
- Mike Griese - March 2019

--*/
#pragma once

constexpr COLORREF DEFAULT_FOREGROUND = 0x00ffffff;
constexpr COLORREF DEFAULT_FOREGROUND_WITH_ALPHA = 0xff000000 | DEFAULT_FOREGROUND;
constexpr COLORREF DEFAULT_BACKGROUND = 0x00000000;
constexpr COLORREF DEFAULT_BACKGROUND_WITH_ALPHA = 0xff000000 | DEFAULT_BACKGROUND;
constexpr short DEFAULT_HISTORY_SIZE = 9001;
const std::wstring DEFAULT_FONT_FACE { L"Consolas" };
constexpr int DEFAULT_FONT_SIZE = 12;

constexpr int DEFAULT_ROWS = 30;
constexpr int DEFAULT_COLS = 120;
