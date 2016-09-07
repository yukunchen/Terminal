/*++
Copyright (c) Microsoft Corporation

Module Name:
- globals.h

Abstract:
- This module contains the global variables used by the console server DLL.

Author:
- Jerry Shea (jerrysh) 21-Sep-1993

Revision History:
- Modified to reduce globals over Console V2 project (MiNiksa/PaulCam, 2014)
--*/

#pragma once

#include "selection.hpp"
#include "server.h"

#include "IRenderData.hpp"
#include "IRenderEngine.hpp"
#include "IRenderer.hpp"
#include "IFontDefaultList.hpp"

#include <TraceLoggingProvider.h>
#include <winmeta.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);

extern UINT g_uiOEMCP;
extern HINSTANCE g_hInstance;
extern UINT g_uiDialogBoxCount;

extern CONSOLE_INFORMATION g_ciConsoleInformation;

extern SHORT g_sVerticalScrollSize;
extern SHORT g_sHorizontalScrollSize;

extern int g_dpi;

extern NTSTATUS g_ntstatusConsoleInputInitStatus;
extern HANDLE g_hConsoleInputInitEvent;
extern DWORD g_dwInputThreadId;

#define WORD_DELIM_MAX  32
extern WCHAR gaWordDelimChars[];

using namespace Microsoft::Console::Render;

extern IRenderer* g_pRender;
extern IRenderData* g_pRenderData;
extern IRenderEngine* g_pRenderEngine;
extern IFontDefaultList* g_pFontDefaultList;
