/*++
Copyright (c) Microsoft Corporation

Module Name:
- conimeinfo.h

Abstract:
- This module contains the structures for the console IME

Author:
- KazuM

Revision History:
--*/

#pragma once

#include "conime.h"

class SCREEN_INFORMATION;

// Internal structures and definitions used by the conversion area.
typedef struct _CONVERSION_AREA_BUFFER_INFO
{
    COORD coordCaBuffer;
    SMALL_RECT rcViewCaWindow;
    COORD coordConView;
} CONVERSION_AREA_BUFFER_INFO, *PCONVERSION_AREA_BUFFER_INFO;

typedef struct _CONVERSIONAREA_INFORMATION
{
    DWORD ConversionAreaMode;
#define CA_HIDDEN      0x01 // Set:Hidden    Reset:Active

    CONVERSION_AREA_BUFFER_INFO CaInfo;
    SCREEN_INFORMATION *ScreenBuffer;

    struct _CONVERSIONAREA_INFORMATION *ConvAreaNext;
} CONVERSIONAREA_INFORMATION, *PCONVERSIONAREA_INFORMATION;

typedef struct _CONSOLE_IME_INFORMATION
{
    // Composition String information
    LPCONIME_UICOMPMESSAGE CompStrData;
    BOOLEAN SavedCursorVisible; // whether cursor is visible (set by user)

    // IME compositon string information
    ULONG NumberOfConvAreaCompStr;
    PCONVERSIONAREA_INFORMATION *ConvAreaCompStr;

    // Root of conversion area information
    PCONVERSIONAREA_INFORMATION ConvAreaRoot;
} CONSOLE_IME_INFORMATION, *PCONSOLE_IME_INFORMATION;
