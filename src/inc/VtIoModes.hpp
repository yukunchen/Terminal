/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

enum VtIoMode
{
    XTERM_256,
    WIN_TELNET
};

const wchar_t* const XTERM_256_STRING = L"xterm-256"; 
const wchar_t* const WIN_TELNET_STRING = L"win-telnet"; 
const wchar_t* const DEFAULT_STRING = L""; 
