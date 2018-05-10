/*++
Copyright (c) Microsoft Corporation

Module Name:
- conareainfo.h

Abstract:
- This module contains the structures for the console IME

Author:
- KazuM

Revision History:
--*/

#pragma once

#include "../buffer/out/OutputCell.hpp"
#include "../buffer/out/TextAttribute.hpp"
#include "../renderer/inc/FontInfo.hpp"

class SCREEN_INFORMATION;

// Internal structures and definitions used by the conversion area.
class ConversionAreaBufferInfo
{
public:
    COORD coordCaBuffer;
    SMALL_RECT rcViewCaWindow;
    COORD coordConView;

    ConversionAreaBufferInfo(const COORD coordBufferSize);
};

class ConversionAreaInfo final
{
public:
    ConversionAreaInfo(const COORD bufferSize,
                       const COORD windowSize,
                       const CHAR_INFO fill,
                       const CHAR_INFO popupFill,
                       const FontInfo fontInfo);
    ~ConversionAreaInfo();
    ConversionAreaInfo(const ConversionAreaInfo&) = delete;
    ConversionAreaInfo(ConversionAreaInfo&& other);
    ConversionAreaInfo& operator=(const ConversionAreaInfo&) & = delete;
    ConversionAreaInfo& operator=(ConversionAreaInfo&&) & = delete;

    bool IsHidden() const;
    void SetHidden(const bool fIsHidden);
    void ClearArea();
    HRESULT Resize(const COORD newSize);

    void SetViewPos(const COORD pos);
    void SetWindowInfo(const SMALL_RECT view);
    void Paint() const;

    ConversionAreaBufferInfo CaInfo;
    SCREEN_INFORMATION* ScreenBuffer;

private:
    bool _fIsHidden;
};
