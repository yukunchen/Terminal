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
#include <memory>

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

class ConversionAreaInfo final
{
public:
    ConversionAreaInfo(const COORD bufferSize,
                       const COORD windowSize,
                       const CHAR_INFO fill,
                       const CHAR_INFO popupFill,
                       const FontInfo* const pFontInfo);
    ~ConversionAreaInfo();

    bool IsHidden() const;
    void SetHidden(_In_ bool const fIsHidden);

    ConversionAreaBufferInfo CaInfo;
    SCREEN_INFORMATION* ScreenBuffer;

private:
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
    std::vector<std::unique_ptr<ConversionAreaInfo>> ConvAreaCompStr;

    ConsoleImeInfo();
    ~ConsoleImeInfo();

    void RefreshAreaAttributes();

    [[nodiscard]]
    NTSTATUS AddConversionArea();
};
