/*++
Copyright (c) Microsoft Corporation

Module Name:
- tracing.hpp

Abstract:
- This module is used for recording tracing/debugging information to the telemetry ETW channel
- The data is not automatically broadcast to telemetry backends as it does not set the TELEMETRY keyword.
- NOTE: Many functions in this file appear to be copy/pastes. This is because the TraceLog documentation warns
        to not be "cute" in trying to reduce its macro usages with variables as it can cause unexpected behavior.

--*/

#pragma once

#include "telemetry.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    class ParserTracing sealed
    {
    public:

        ParserTracing();
        ~ParserTracing();

        void TraceStateChange(_In_ PCWSTR const pwszName) const;
        void TraceOnAction(_In_ PCWSTR const pwszName) const;
        void TraceOnExecute(const wchar_t wch) const;
        void TraceOnEvent(_In_ PCWSTR const pwszName) const;
        void TraceCharInput(const wchar_t wch);

        void AddSequenceTrace(const wchar_t wch);
        void DispatchSequenceTrace(const bool fSuccess);
        void ClearSequenceTrace();
        void DispatchPrintRunTrace(_In_reads_(cchString) wchar_t* pwsString, const size_t cchString) const;

    private:
        static const size_t s_cMaxSequenceTrace = 32;

        wchar_t _rgwchSequenceTrace[s_cMaxSequenceTrace];
        size_t _cchSequenceTrace;


    };
}
