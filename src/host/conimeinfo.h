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

class ConversionAreaInfo
{
    // std::make_unique needs the constructor for ConversionAreaInfo
    // to be public so that it can access it but ConversionAreaInfo
    // objects should only be created by
    // s_CreateInstance. ConstructorGuard is here to enforce this
    // contract by only being allowed to be created from within the
    // ConversionAreaInfo class itself and required by this
    // ConversionAreaInfo constructor.
    class ConstructorGuard final
    {
      public:
        explicit ConstructorGuard() {}
    };

public:

    ConversionAreaBufferInfo CaInfo;
    SCREEN_INFORMATION* const ScreenBuffer;

    static NTSTATUS s_CreateInstance(_Inout_ std::unique_ptr<ConversionAreaInfo>& convAreaInfo);

    bool IsHidden() const;
    void SetHidden(_In_ bool const fIsHidden);

    ~ConversionAreaInfo();

    ConversionAreaInfo(_In_ COORD const coordBufferSize,
                       _In_ SCREEN_INFORMATION* const pScreenInfo,
                       _In_ ConstructorGuard guard);

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

    NTSTATUS AddConversionArea();
};
