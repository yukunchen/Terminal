/*++
Copyright (c) Microsoft Corporation

Module Name:
- tracing.hpp

Abstract:
- This module is used for recording tracing/debugging information to the telemetry ETW channel
--*/

#pragma once
#include <string>
#include <windows.h>
#include <winmeta.h>
#include <TraceLoggingProvider.h>
#include <telemetry\microsofttelemetry.h>

TRACELOGGING_DECLARE_PROVIDER(g_hConsoleVtRendererTraceProvider);

namespace Microsoft::Console::VirtualTerminal
{
    class RenderTracing final
    {
    public:

        RenderTracing();
        ~RenderTracing();
        void TraceString(const std::string_view& str) const;
    };
}
