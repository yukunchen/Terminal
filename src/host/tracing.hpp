/*++
Copyright (c) Microsoft Corporation

Module Name:
- tracing.hpp

Abstract:
- This module is used for recording tracing/debugging information to the telemetry ETW channel
- The data is not automatically broadcast to telemetry backends as it does not set the TELEMETRY keyword.
- NOTE: Many functions in this file appear to be copy/pastes. This is because the TraceLog documentation warns
        to not be "cute" in trying to reduce its macro usages with variables as it can cause unexpected behavior.

Author(s):
- Michael Niksa (miniksa)     25-Nov-2014
--*/

#pragma once

#if DBG
#define DBGCHARS(_params_)   { Tracing::s_TraceChars _params_ ; }
#define DBGOUTPUT(_params_)  { Tracing::s_TraceOutput _params_ ; }
#else
#define DBGCHARS(_params_)
#define DBGOUTPUT(_params_)
#endif

class Tracing
{
public:
    static void s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_GETLARGESTWINDOWSIZE_MSG* const a);
    static void s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SCREENBUFFERINFO_MSG* const a, _In_ bool const fSet);
    static void s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SETSCREENBUFFERSIZE_MSG* const a);
    static void s_TraceApi(_In_ const NTSTATUS status, _In_ const CONSOLE_SETWINDOWINFO_MSG* const a);

    static void s_TraceApi(_In_ void* buffer, _In_ const CONSOLE_WRITECONSOLE_MSG* const a);

    static void s_TraceApi(_In_ const CONSOLE_SCREENBUFFERINFO_MSG* const a);
    static void s_TraceApi(_In_ const CONSOLE_MODE_MSG* const a, const std::wstring& handleType);
    static void s_TraceApi(_In_ const CONSOLE_SETTEXTATTRIBUTE_MSG* const a);
    static void s_TraceApi(_In_ const CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG* const a);

    static void s_TraceWindowViewport(_In_ const SMALL_RECT viewport);

    static void s_TraceChars(_In_z_ const char* pszMessage, ...);
    static void s_TraceOutput(_In_z_ const char* pszMessage, ...);

    static void s_TraceWindowMessage(_In_ const MSG& msg);
    static void s_TraceInputRecord(_In_ const INPUT_RECORD& inputRecord);

    static void __stdcall TraceFailure(const wil::FailureInfo& failure);

private:
    static ULONG s_ulDebugFlag;
};
