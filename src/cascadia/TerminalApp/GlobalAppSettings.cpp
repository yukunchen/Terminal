/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "GlobalAppSettings.h"

using namespace Microsoft::Terminal::TerminalApp;

GlobalAppSettings::GlobalAppSettings() :
    _keybindings{},
    _defaultProfile{}
{

}

GlobalAppSettings::~GlobalAppSettings()
{

}
