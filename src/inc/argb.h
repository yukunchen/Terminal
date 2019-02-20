/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#pragma once

constexpr COLORREF ARGB(const BYTE a, const BYTE r, const BYTE g, const BYTE b) noexcept
{
    return (a<<24) | (b<<16) | (g<<8) | (r);
}

#ifdef RGB
#undef RGB
#define RGB(r, g, b) (ARGB(255, (r), (g), (b)))
#endif
