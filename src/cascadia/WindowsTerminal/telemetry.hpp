/*++
Copyright (c) Microsoft Corporation

Module Name:
- telemetry.hpp

Abstract:
- This module is used for recording all telemetry feedback from the terminal app

--*/
#pragma once

#include "winmeta.h" // Needed for WINEVENT_KEYWORD_TELEMETRY
#include <TraceLoggingActivity.h>
#include <telemetry/microsofttelemetry.h>

class Telemetry
{
public:
	// Implement this as a singleton class.
	static Telemetry& Instance()
	{
		static Telemetry s_Instance;
		return s_Instance;
	}
private:
	// Used to prevent multiple instances
	Telemetry();
	~Telemetry();
	Telemetry(Telemetry const&);
	void operator=(Telemetry const&);
	TraceLoggingActivity<g_hTerminalWin32Provider> _activity;
};