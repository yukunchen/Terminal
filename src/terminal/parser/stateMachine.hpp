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
                friend class OutputStateMachineEngine;

            public:
                StateMachine(_In_ TermDispatch* const pDispatch);
                ~StateMachine();

                IStateMachineEngine* _pEngine = nullptr;
                
                TermDispatch* GetDispatch()
                {
                    return _pDispatch;
                }

                void ProcessCharacter(_In_ wchar_t const wch);
                void ProcessString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);

                void ResetState();
                
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

                // static const TermDispatch::GraphicsOptions s_defaultGraphicsOption = TermDispatch::GraphicsOptions::Off;
                // _Success_(return)
                // bool _GetGraphicsOptions(_Out_writes_(*pcOptions) TermDispatch::GraphicsOptions* rgGraphicsOptions, _Inout_ size_t* pcOptions) const;

                // static const TermDispatch::EraseType s_defaultEraseType = TermDispatch::EraseType::ToEnd;
                // _Success_(return)
                // bool _GetEraseOperation(_Out_ TermDispatch::EraseType* const pEraseType) const;

                // static const unsigned int s_uiDefaultCursorDistance = 1;
                // _Success_(return)
                // bool _GetCursorDistance(_Out_ unsigned int* const puiDistance) const;

                // static const unsigned int s_uiDefaultScrollDistance = 1;
                // _Success_(return)
                // bool _GetScrollDistance(_Out_ unsigned int* const puiDistance) const;

                // static const unsigned int s_uiDefaultConsoleWidth = 80;
                // _Success_(return)
                // bool _GetConsoleWidth(_Out_ unsigned int* const puiConsoleWidth) const;

                // static const unsigned int s_uiDefaultLine = 1;
                // static const unsigned int s_uiDefaultColumn = 1;
                // _Success_(return)
                // bool _GetXYPosition(_Out_ unsigned int* const puiLine, _Out_ unsigned int* const puiColumn) const;

                // _Success_(return)
                // bool _GetDeviceStatusOperation(_Out_ TermDispatch::AnsiStatusType* const pStatusType) const;

                // _Success_(return)
                // bool _VerifyHasNoParameters() const;

                // _Success_(return)
                // bool _VerifyDeviceAttributesParams() const;

                // _Success_(return)
                // bool _GetPrivateModeParams(_Out_writes_(*pcParams) TermDispatch::PrivateModeParams* rgPrivateModeParams, _Inout_ size_t* pcParams) const;

                // static const SHORT s_sDefaultTopMargin = 0;
                // static const SHORT s_sDefaultBottomMargin = 0;
                // _Success_(return)
                // bool _GetTopBottomMargins(_Out_ SHORT* const psTopMargin, _Out_ SHORT* const psBottomMargin) const;

                // _Success_(return)
                // bool _GetOscTitle(_Outptr_result_buffer_(*pcchTitle) wchar_t** const ppwchTitle, _Out_ unsigned short * pcchTitle);

                // static const SHORT s_sDefaultTabDistance = 1;
                // _Success_(return)
                // bool _GetTabDistance(_Out_ SHORT* const psDistance) const;

                // static const SHORT s_sDefaultTabClearType = 0;
                // _Success_(return)
                // bool _GetTabClearType(_Out_ SHORT* const psClearType) const;

                // static const DesignateCharsetTypes s_DefaultDesignateCharsetType = DesignateCharsetTypes::G0;
                // _Success_(return)
                // bool _GetDesignateType(_Out_ DesignateCharsetTypes* const pDesignateType) const;

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

                TermDispatch* _pDispatch;
                VTStates _state;

                static const short s_cIntermediateMax = 1;
                wchar_t _wchIntermediate;
                unsigned short _cIntermediate;

                static const short s_cParamsMax = 16;
                unsigned short _rgusParams[s_cParamsMax];
                unsigned short _cParams;
                unsigned short* _pusActiveParam;
                unsigned short _iParamAccumulatePos;

                static const short s_cOscStringMaxLength = 256;
                unsigned short _sOscParam;
                unsigned short _sOscNextChar;
                wchar_t _pwchOscStringBuffer[s_cOscStringMaxLength];

            };
        };
    };
};