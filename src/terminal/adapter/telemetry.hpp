/*++
Copyright (c) Microsoft Corporation

Module Name:
- telemetry.hpp

Abstract:
- This module is used for recording all telemetry feedback from the console virtual terminal parser

--*/
#pragma once

// Including TraceLogging essentials for the binary
#include <windows.h>
#include <winmeta.h>
#include <TraceLoggingProvider.h>
#include <telemetry\microsofttelemetry.h>

TRACELOGGING_DECLARE_PROVIDER(g_hConsoleVirtTermParserEventTraceProvider);
