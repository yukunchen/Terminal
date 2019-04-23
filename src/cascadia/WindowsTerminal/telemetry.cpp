/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "pch.h"

#include "telemetry.hpp"

#include "../../dep/telemetry/microsofttelemetry.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_hTerminalWin32Provider,
    "Microsoft.Windows.Terminal.Win32",
    // {ef95331e-0ed7-55ba-39cf-c9d0d95499e0}
    (0xef95331e, 0x0ed7, 0x55ba, 0x39, 0xcf, 0xc9, 0xd0, 0xd9, 0x54, 0x99, 0xe0),
    TraceLoggingOptionMicrosoftTelemetry());

Telemetry::Telemetry()
{
	TraceLoggingRegister(g_hTerminalWin32Provider);
	TraceLoggingWriteStart(_activity, "ActivityStart");
}
#pragma warning(pop)

Telemetry::~Telemetry()
{
	TraceLoggingWriteStop(_activity, "ActivityStop");
	TraceLoggingUnregister(g_hTerminalWin32Provider);
}