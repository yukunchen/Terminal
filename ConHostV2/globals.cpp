/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#pragma once

#include "precomp.h"

#include "globals.h"

#include "renderFontDefaults.hpp"

UINT g_uiOEMCP;
HINSTANCE g_hInstance;
UINT g_uiDialogBoxCount;

CONSOLE_INFORMATION g_ciConsoleInformation;

SHORT g_sVerticalScrollSize;
SHORT g_sHorizontalScrollSize;

int g_dpi = USER_DEFAULT_SCREEN_DPI;

NTSTATUS g_ntstatusConsoleInputInitStatus;
HANDLE g_hConsoleInputInitEvent;
DWORD g_dwInputThreadId;

WCHAR gaWordDelimChars[WORD_DELIM_MAX];

IRenderer* g_pRender;
IRenderData* g_pRenderData;
IRenderEngine* g_pRenderEngine;

IFontDefaultList* g_pFontDefaultList;
