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
#include "ConsoleArguments.hpp"

#include "..\renderer\inc\IRenderData.hpp"
#include "..\renderer\inc\IRenderEngine.hpp"
#include "..\renderer\inc\IRenderer.hpp"
#include "..\renderer\inc\IFontDefaultList.hpp"

#include "..\server\DeviceComm.h"

#include <TraceLoggingProvider.h>
#include <winmeta.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);

#define WORD_DELIM_MAX  32

using namespace Microsoft::Console::Render;

class Globals
{
public:
     UINT uiOEMCP;
     UINT uiWindowsCP;
     HINSTANCE hInstance;
     UINT uiDialogBoxCount;

     ConsoleArguments launchArgs;

     CONSOLE_INFORMATION* getConsoleInformation();

     DeviceComm* pDeviceComm;

     wil::unique_event_nothrow hInputEvent;

     SHORT sVerticalScrollSize;
     SHORT sHorizontalScrollSize;

     int dpi = USER_DEFAULT_SCREEN_DPI;

     NTSTATUS ntstatusConsoleInputInitStatus;
     wil::unique_event_nothrow hConsoleInputInitEvent;
     DWORD dwInputThreadId;

     WCHAR aWordDelimChars[WORD_DELIM_MAX];

     IRenderer* pRender;
     // TODO: 14522377 Come back and remove this
     // ConIoSrv needs another way to get ahold of this object, and now that
     //   the renderer holds on to the engines all in a collection, coniosrv
     //   can't be sure which engine is the actual WddmConEngine
     IRenderEngine* pWddmconEngine;
     
     IFontDefaultList* pFontDefaultList;

private:
     CONSOLE_INFORMATION* ciConsoleInformation;
};
