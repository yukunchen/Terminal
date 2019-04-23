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

static volatile bool s_TraceLoggingInitialized = false;

void InitializeTraceLogging()
{
	if (!s_TraceLoggingInitialized)
	{
		TraceLoggingRegister(g_hTerminalWin32Provider);
		s_TraceLoggingInitialized = true;
	}
}

void UninitializeTraceLogging()
{
	if (s_TraceLoggingInitialized)
	{
		TraceLoggingUnregister(g_hTerminalWin32Provider);
		s_TraceLoggingInitialized = false;
	}
}
