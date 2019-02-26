/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Profile.h"

using namespace Microsoft::Terminal::TerminalApp;

Profile::Profile() :
    _guid{},
    _name{ L"Default" },
    _coreSettings{},
    _commandline{ L"cmd.exe" },
    _fontFace{ L"Consolas" },
    _fontSize{ 12 },
    _acrylicTransparency{ 0.5 },
    _useAcrylic{ false },
    _showScrollbars{ true }
{
    UuidCreate(&_guid);
}

Profile::~Profile()
{

}
