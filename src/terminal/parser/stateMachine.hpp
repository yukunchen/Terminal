/*++
Copyright (c) Microsoft Corporation

Module Name:
- stateMachine.hpp

Abstract:
- This declares the entire state machine for handling Virtual Terminal Sequences
- It is currently designed to handle up to VT100 level escape sequences
- The design is based from the specifications at http://vt100.net

Author(s):
- Michael Niksa (MiNiksa) 30-July-2015
- Mike Griese (migrie) 18 Aug 2017 - Abstracted the engine away for input parsing.
--*/

#pragma once

#include "IStateMachineEngine.hpp"
#include "telemetry.hpp"
#include "tracing.hpp"
#include <memory>

namespace Microsoft::Console::VirtualTerminal
{
    class StateMachine sealed
    {
#ifdef UNIT_TESTING
        friend class OutputEngineTest;
        friend class InputEngineTest;
#endif

    public:
        StateMachine(_In_ std::shared_ptr<IStateMachineEngine> pEngine);

        void ProcessCharacter(_In_ wchar_t const wch);
        void ProcessString(_Inout_updates_(cch) wchar_t* const rgwch, _In_ size_t const cch);
        // Note: There is intentionally not a ProcessString that operates
        //      on a wstring. This is because the in buffer needs to be mutable
        //      and c_str() only gives you const data.

        void ResetState();

        bool FlushToTerminal();

        static const short s_cIntermediateMax = 1;
        static const short s_cParamsMax = 16;
        static const short s_cOscStringMaxLength = 256;

    private:
        static bool s_IsActionableFromGround(_In_ wchar_t const wch);
        static bool s_IsC0Code(_In_ wchar_t const wch);
        static bool s_IsC1Csi(_In_ wchar_t const wch);
        static bool s_IsIntermediate(_In_ wchar_t const wch);
        static bool s_IsDelete(_In_ wchar_t const wch);
        static bool s_IsEscape(_In_ wchar_t const wch);
        static bool s_IsCsiIndicator(_In_ wchar_t const wch);
        static bool s_IsCsiDelimiter(_In_ wchar_t const wch);
        static bool s_IsCsiParamValue(_In_ wchar_t const wch);
        static bool s_IsCsiPrivateMarker(_In_ wchar_t const wch);
        static bool s_IsCsiInvalid(_In_ wchar_t const wch);
        static bool s_IsOscIndicator(_In_ wchar_t const wch);
        static bool s_IsOscDelimiter(_In_ wchar_t const wch);
        static bool s_IsOscParamValue(_In_ wchar_t const wch);
        static bool s_IsOscInvalid(_In_ wchar_t const wch);
        static bool s_IsOscTerminator(_In_ wchar_t const wch);
        static bool s_IsOscTerminationInitiator(_In_ wchar_t const wch);
        static bool s_IsDesignateCharsetIndicator(_In_ wchar_t const wch);
        static bool s_IsCharsetCode(_In_ wchar_t const wch);
        static bool s_IsNumber(_In_ wchar_t const wch);
        static bool s_IsSs3Indicator(_In_ wchar_t const wch);

        void _ActionExecute(_In_ wchar_t const wch);
        void _ActionPrint(_In_ wchar_t const wch);
        void _ActionEscDispatch(_In_ wchar_t const wch);
        void _ActionCollect(_In_ wchar_t const wch);
        void _ActionParam(_In_ wchar_t const wch);
        void _ActionCsiDispatch(_In_ wchar_t const wch);
        void _ActionOscParam(_In_ wchar_t const wch);
        void _ActionOscPut(_In_ wchar_t const wch);
        void _ActionOscDispatch(_In_ wchar_t const wch);
        void _ActionSs3Dispatch(_In_ wchar_t const wch);

        void _ActionClear();
        void _ActionIgnore();

        void _EnterGround();
        void _EnterEscape();
        void _EnterEscapeIntermediate();
        void _EnterCsiEntry();
        void _EnterCsiParam();
        void _EnterCsiIgnore();
        void _EnterCsiIntermediate();
        void _EnterOscParam();
        void _EnterOscString();
        void _EnterOscTermination();
        void _EnterSs3Entry();
        void _EnterSs3Param();

        void _EventGround(_In_ wchar_t const wch);
        void _EventEscape(_In_ wchar_t const wch);
        void _EventEscapeIntermediate(_In_ wchar_t const wch);
        void _EventCsiEntry(_In_ wchar_t const wch);
        void _EventCsiIntermediate(_In_ wchar_t const wch);
        void _EventCsiIgnore(_In_ wchar_t const wch);
        void _EventCsiParam(_In_ wchar_t const wch);
        void _EventOscParam(_In_ wchar_t const wch);
        void _EventOscString(_In_ wchar_t const wch);
        void _EventOscTermination(_In_ wchar_t const wch);
        void _EventSs3Entry(_In_ wchar_t const wch);
        void _EventSs3Param(_In_ wchar_t const wch);

        enum class VTStates
        {
            Ground,
            Escape,
            EscapeIntermediate,
            CsiEntry,
            CsiIntermediate,
            CsiIgnore,
            CsiParam,
            OscParam,
            OscString,
            OscTermination,
            Ss3Entry,
            Ss3Param
        };

        Microsoft::Console::VirtualTerminal::ParserTracing _trace;
        std::shared_ptr<IStateMachineEngine> const _pEngine;

        VTStates _state;

        wchar_t _wchIntermediate;
        unsigned short _cIntermediate;

        unsigned short _rgusParams[s_cParamsMax];
        unsigned short _cParams;
        unsigned short* _pusActiveParam;
        unsigned short _iParamAccumulatePos;

        unsigned short _sOscParam;
        unsigned short _sOscNextChar;
        wchar_t _pwchOscStringBuffer[s_cOscStringMaxLength];

        wchar_t* _pwchCurr;
        wchar_t* _pwchSequenceStart;
        size_t _currRunLength;

    };
}
