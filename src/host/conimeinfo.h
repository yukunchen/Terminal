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

#include "..\inc\conime.h"
#include <vector>

class SCREEN_INFORMATION;

// Internal structures and definitions used by the conversion area.
class ConversionAreaBufferInfo
{
public:
    COORD coordCaBuffer;
    SMALL_RECT rcViewCaWindow;
    COORD coordConView;

    ConversionAreaBufferInfo(_In_ COORD const coordBufferSize);
};

class ConversionAreaInfo
{
public:

    ConversionAreaBufferInfo CaInfo;
    SCREEN_INFORMATION* const ScreenBuffer;

    static NTSTATUS s_CreateInstance(_Outptr_ ConversionAreaInfo** const ppInfo);

    bool IsHidden() const;
    void SetHidden(_In_ bool const fIsHidden);

    ~ConversionAreaInfo();

private:
    ConversionAreaInfo(_In_ COORD const coordBufferSize,
                       _In_ SCREEN_INFORMATION* const pScreenInfo);

    bool _fIsHidden;
};

class ConsoleImeInfo
{
public:
    // Composition String information
    LPCONIME_UICOMPMESSAGE CompStrData;
    BOOLEAN SavedCursorVisible; // whether cursor is visible (set by user)

    // IME compositon string information
    // There is one "composition string" per line that must be rendered on the screen
    std::vector<ConversionAreaInfo*> ConvAreaCompStr;

    ConsoleImeInfo();
    ~ConsoleImeInfo();

    void RefreshAreaAttributes();

    NTSTATUS AddConversionArea();
};
