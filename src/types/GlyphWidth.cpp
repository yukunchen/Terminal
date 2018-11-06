/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "inc/CodepointWidthDetector.hpp"
#include "inc/GlyphWidth.hpp"

static CodepointWidthDetector widthDetector;

bool IsGlyphFullWidth(const std::wstring_view glyph)
{
    return widthDetector.IsWide(glyph);
}

bool IsGlyphFullWidth(const wchar_t wch)
{
    return widthDetector.IsWide(wch);
}

void SetGlyphWidthFallback(std::function<bool(const std::wstring_view)> pfnFallback)
{
    widthDetector.SetFallbackMethod(pfnFallback);
}
