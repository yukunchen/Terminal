/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "tracing.hpp"

enum TraceKeywords
{
    //Font = 0x001, // _DBGFONTS
    //Font2 = 0x002, // _DBGFONTS2
    Chars = 0x004, // _DBGCHARS
    Output = 0x008, // _DBGOUTPUT
    General = 0x100,
    API = 0x400,
    All = 0xFFF
};
DEFINE_ENUM_FLAG_OPERATORS(TraceKeywords);

ULONG Tracing::s_ulDebugFlag = 0x0;

void Tracing::s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_GETLARGESTWINDOWSIZE_MSG* const a)
{
    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "API_GetLargestWindowSize",
        TraceLoggingHexInt32(status, "ResultCode"),
        TraceLoggingInt32(a->Size.X, "MaxWindowWidthInChars"),
        TraceLoggingInt32(a->Size.Y, "MaxWindowHeightInChars"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::API)
        );
}

void Tracing::s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SCREENBUFFERINFO_MSG* const a, _In_ bool const fSet)
{
    // Duplicate copies required by TraceLogging documentation ("don't get cute" examples)
    // Using logic inside these macros can make problems. Do all logic outside macros.

    if (fSet)
    {
        TraceLoggingWrite(g_hConhostV2EventTraceProvider, "API_SetConsoleScreenBufferInfo",
            TraceLoggingHexInt32(status, "ResultCode"),
            TraceLoggingInt32(a->Size.X, "BufferWidthInChars"),
            TraceLoggingInt32(a->Size.Y, "BufferHeightInChars"),
            TraceLoggingInt32(a->CurrentWindowSize.X, "WindowWidthInChars"),
            TraceLoggingInt32(a->CurrentWindowSize.Y, "WindowHeightInChars"),
            TraceLoggingInt32(a->MaximumWindowSize.X, "MaxWindowWidthInChars"),
            TraceLoggingInt32(a->MaximumWindowSize.Y, "MaxWindowHeightInChars"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(TraceKeywords::API)
            );
    }
    else
    {
        TraceLoggingWrite(g_hConhostV2EventTraceProvider, "API_GetConsoleScreenBufferInfo",
            TraceLoggingHexInt32(status, "ResultCode"),
            TraceLoggingInt32(a->Size.X, "BufferWidthInChars"),
            TraceLoggingInt32(a->Size.Y, "BufferHeightInChars"),
            TraceLoggingInt32(a->CurrentWindowSize.X, "WindowWidthInChars"),
            TraceLoggingInt32(a->CurrentWindowSize.Y, "WindowHeightInChars"),
            TraceLoggingInt32(a->MaximumWindowSize.X, "MaxWindowWidthInChars"),
            TraceLoggingInt32(a->MaximumWindowSize.Y, "MaxWindowHeightInChars"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(TraceKeywords::API)
            );
    }
}

void Tracing::s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SETSCREENBUFFERSIZE_MSG* const a)
{
    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "API_SetConsoleScreenBufferSize",
        TraceLoggingHexInt32(status, "ResultCode"),
        TraceLoggingInt32(a->Size.X, "BufferWidthInChars"),
        TraceLoggingInt32(a->Size.Y, "BufferHeightInChars"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::API)
        );
}

void Tracing::s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SETWINDOWINFO_MSG* const a)
{
    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "API_SetConsoleWindowInfo",
        TraceLoggingHexInt32(status, "ResultCode"),
        TraceLoggingBool(a->Absolute, "IsWindowRectAbsolute"),
        TraceLoggingInt32(a->Window.Left, "WindowRectLeft"),
        TraceLoggingInt32(a->Window.Right, "WindowRectRight"),
        TraceLoggingInt32(a->Window.Top, "WindowRectTop"),
        TraceLoggingInt32(a->Window.Bottom, "WindowRectBottom"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::API)
        );
}

void Tracing::s_TraceWindowViewport(_In_ const SMALL_RECT* const psrView)
{
    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "WindowViewport",
        TraceLoggingInt32(psrView->Bottom - psrView->Top, "ViewHeight"),
        TraceLoggingInt32(psrView->Right - psrView->Left, "ViewWidth"),
        TraceLoggingInt32(psrView->Top, "OriginTop"),
        TraceLoggingInt32(psrView->Left, "OriginLeft"),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::General)
        );
}

void Tracing::s_TraceChars(_In_z_ const char* pszMessage, ...)
{
    va_list args;
    va_start(args, pszMessage);
    char szBuffer[256] = "";
    vsprintf_s(szBuffer, ARRAYSIZE(szBuffer), pszMessage, args);
    va_end(args);

    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "CharsTrace",
        TraceLoggingString(szBuffer),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::Chars)
        );

    if (s_ulDebugFlag & TraceKeywords::Chars)
    {
        OutputDebugStringA(szBuffer);
    }
}

void Tracing::s_TraceOutput(_In_z_ const char* pszMessage, ...)
{
    va_list args;
    va_start(args, pszMessage);
    char szBuffer[256] = "";
    vsprintf_s(szBuffer, ARRAYSIZE(szBuffer), pszMessage, args);
    va_end(args);

    TraceLoggingWrite(g_hConhostV2EventTraceProvider, "OutputTrace",
        TraceLoggingString(szBuffer),
        TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
        TraceLoggingKeyword(TraceKeywords::Output)
        );

    if (s_ulDebugFlag & TraceKeywords::Output)
    {
        OutputDebugStringA(szBuffer);
    }
}
