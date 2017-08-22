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
--*/

#pragma once

#include "termDispatch.hpp"
#include "IStateMachineEngine.hpp"
#include "OutputStateMachineEngine.hpp"
#include "telemetry.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {

            class StateMachine sealed
            {
#ifdef UNIT_TESTING
                friend class StateMachineTest;
#endif

            public:
                StateMachine(_In_ TermDispatch* const pDispatch);
                ~StateMachine();

                
                TermDispatch* GetDispatch()
                {
                    return _pDispatch;
                }

                void ProcessCharacter(_In_ wchar_t const wch);
                void ProcessString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);

                void ResetState();

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
                static bool s_IsDesignateCharsetIndicator(_In_ wchar_t const wch);
                static bool s_IsCharsetCode(_In_ wchar_t const wch);

                enum VTActionCodes : wchar_t
                {
                    CUU_CursorUp = L'A',
                    CUD_CursorDown = L'B',
                    CUF_CursorForward = L'C',
                    CUB_CursorBackward = L'D',
                    CNL_CursorNextLine = L'E',
                    CPL_CursorPrevLine = L'F',
                    CHA_CursorHorizontalAbsolute = L'G',
                    CUP_CursorPosition = L'H',
                    ED_EraseDisplay = L'J',
                    EL_EraseLine = L'K',
                    SU_ScrollUp = L'S',
                    SD_ScrollDown = L'T',
                    ICH_InsertCharacter = L'@',
                    DCH_DeleteCharacter = L'P',
                    SGR_SetGraphicsRendition = L'm',
                    DECSC_CursorSave = L'7',
                    DECRC_CursorRestore = L'8',
                    DECSET_PrivateModeSet = L'h',
                    DECRST_PrivateModeReset = L'l',
                    ANSISYSSC_CursorSave = L's', // NOTE: Overlaps with DECLRMM/DECSLRM. Fix when/if implemented.
                    ANSISYSRC_CursorRestore = L'u', // NOTE: Overlaps with DECSMBV. Fix when/if implemented.
                    DECKPAM_KeypadApplicationMode = L'=',
                    DECKPNM_KeypadNumericMode = L'>',
                    DSR_DeviceStatusReport = L'n',
                    DA_DeviceAttributes = L'c',
                    DECSCPP_SetColumnsPerPage = L'|',
                    IL_InsertLine = L'L',
                    DL_DeleteLine = L'M', // Yes, this is the same as RI, however, RI is not preceeded by a CSI, and DL is.
                    VPA_VerticalLinePositionAbsolute = L'd',
                    DECSTBM_SetScrollingRegion = L'r',
                    RI_ReverseLineFeed = L'M',
                    HTS_HorizontalTabSet = L'H', // Not a CSI, so doesn't overlap with CUP
                    CHT_CursorForwardTab = L'I',
                    CBT_CursorBackTab = L'Z',
                    TBC_TabClear = L'g',
                    ECH_EraseCharacters = L'X',
                    HVP_HorizontalVerticalPosition = L'f',
                    DECSTR_SoftReset = L'p',
                    RIS_ResetToInitialState = L'c' // DA is prefaced by CSI, RIS by ESC
                };

                enum OscActionCodes : unsigned int
                {
                    SetIconAndWindowTitle = 0,
                    SetWindowIcon = 1,
                    SetWindowTitle = 2
                };

                enum class DesignateCharsetTypes
                {
                    G0,
                    G1,
                    G2,
                    G3
                };

                void _ActionExecute(_In_ wchar_t const wch);
                void _ActionPrint(_In_ wchar_t const wch);
                void _ActionEscDispatch(_In_ wchar_t const wch);
                void _ActionCollect(_In_ wchar_t const wch);
                void _ActionParam(_In_ wchar_t const wch);
                void _ActionCsiDispatch(_In_ wchar_t const wch);
                void _ActionOscParam(_In_ wchar_t const wch);
                void _ActionOscPut(_In_ wchar_t const wch);
                void _ActionOscDispatch(_In_ wchar_t const wch);
                bool _IntermediateQuestionMarkDispatch(_In_ wchar_t const wchAction);
                bool _IntermediateExclamationDispatch(_In_ wchar_t const wchAction);

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

                void _EventGround(_In_ wchar_t const wch);
                void _EventEscape(_In_ wchar_t const wch);
                void _EventEscapeIntermediate(_In_ wchar_t const wch);
                void _EventCsiEntry(_In_ wchar_t const wch);
                void _EventCsiIntermediate(_In_ wchar_t const wch);
                void _EventCsiIgnore(_In_ wchar_t const wch);
                void _EventCsiParam(_In_ wchar_t const wch);
                void _EventOscParam(_In_ wchar_t const wch);
                void _EventOscString(_In_ wchar_t const wch);

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
                    OscString
                };
                
                IStateMachineEngine* _pEngine = nullptr;

                TermDispatch* _pDispatch;
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

            };
        };
    };
};