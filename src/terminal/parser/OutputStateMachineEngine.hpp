/*++
Copyright (c) Microsoft Corporation

Module Name:
- OutputStateMachineEngine.hpp

Abstract:
- This is the implementation of the client VT output state machine engine.

Author(s): 
- Mike Griese (migrie) 18 Aug 2017
--*/
#pragma once

#include "termDispatch.hpp"
#include "telemetry.hpp"
#include "IStateMachineEngine.hpp"

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class StateMachine;

            class OutputStateMachineEngine : public IStateMachineEngine
            {
            public:
                OutputStateMachineEngine(_In_ TermDispatch* const pDispatch);
                ~OutputStateMachineEngine();

                bool ActionExecute(_In_ wchar_t const wch) override;
                bool ActionPrint(_In_ wchar_t const wch) override;
                bool ActionPrintString(_Inout_updates_(cch) wchar_t* const rgwch, _In_ size_t const cch) override;
                bool ActionEscDispatch(_In_ wchar_t const wch,
                                       _In_ const unsigned short cIntermediate,
                                       _In_ const wchar_t wchIntermediate) override;
                bool ActionCsiDispatch(_In_ wchar_t const wch, 
                                       _In_ const unsigned short cIntermediate,
                                       _In_ const wchar_t wchIntermediate,
                                       _In_ const unsigned short* const rgusParams,
                                       _In_ const unsigned short cParams);
                bool ActionClear() override;
                bool ActionIgnore() override;
                bool ActionOscDispatch(_In_ wchar_t const wch,
                                       _In_ const unsigned short sOscParam,
                                       _Inout_ wchar_t* const pwchOscStringBuffer,
                                       _In_ const unsigned short cchOscString) override;
                bool ActionSs3Dispatch(_In_ wchar_t const wch, 
                                       _In_ const unsigned short* const rgusParams,
                                       _In_ const unsigned short cParams) override;

                bool FlushAtEndOfString() const override;

            private:
                TermDispatch* _pDispatch;

                bool _IntermediateQuestionMarkDispatch(_In_ wchar_t const wchAction, _In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams);
                bool _IntermediateExclamationDispatch(_In_ wchar_t const wch);


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
                static const TermDispatch::GraphicsOptions s_defaultGraphicsOption = TermDispatch::GraphicsOptions::Off;
                _Success_(return)
                bool _GetGraphicsOptions(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_writes_(*pcOptions) TermDispatch::GraphicsOptions* rgGraphicsOptions, _Inout_ size_t* pcOptions) const;

                static const TermDispatch::EraseType s_defaultEraseType = TermDispatch::EraseType::ToEnd;
                _Success_(return)
                bool _GetEraseOperation(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ TermDispatch::EraseType* const pEraseType) const;

                static const unsigned int s_uiDefaultCursorDistance = 1;
                _Success_(return)
                bool _GetCursorDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiDistance) const;

                static const unsigned int s_uiDefaultScrollDistance = 1;
                _Success_(return)
                bool _GetScrollDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiDistance) const;

                static const unsigned int s_uiDefaultConsoleWidth = 80;
                _Success_(return)
                bool _GetConsoleWidth(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiConsoleWidth) const;

                static const unsigned int s_uiDefaultLine = 1;
                static const unsigned int s_uiDefaultColumn = 1;
                _Success_(return)
                bool _GetXYPosition(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiLine, _Out_ unsigned int* const puiColumn) const;

                _Success_(return)
                bool _GetDeviceStatusOperation(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ TermDispatch::AnsiStatusType* const pStatusType) const;

                _Success_(return)
                bool _VerifyHasNoParameters(_In_ const unsigned short cParams) const;

                _Success_(return)
                bool _VerifyDeviceAttributesParams(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams) const;

                _Success_(return)
                bool _GetPrivateModeParams(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_writes_(*pcParams) TermDispatch::PrivateModeParams* rgPrivateModeParams, _Inout_ size_t* pcParams) const;

                static const SHORT s_sDefaultTopMargin = 0;
                static const SHORT s_sDefaultBottomMargin = 0;
                _Success_(return)
                bool _GetTopBottomMargins(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psTopMargin, _Out_ SHORT* const psBottomMargin) const;

                _Success_(return)
                bool _GetOscTitle(_Inout_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString, _Outptr_result_buffer_(*pcchTitle) wchar_t** const ppwchTitle, _Out_ unsigned short * pcchTitle);

                static const SHORT s_sDefaultTabDistance = 1;
                _Success_(return)
                bool _GetTabDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psDistance) const;

                static const SHORT s_sDefaultTabClearType = 0;
                _Success_(return)
                bool _GetTabClearType(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psClearType) const;

                static const DesignateCharsetTypes s_DefaultDesignateCharsetType = DesignateCharsetTypes::G0;
                _Success_(return)
                bool _GetDesignateType(_In_ const wchar_t wchIntermediate, _Out_ DesignateCharsetTypes* const pDesignateType) const;


            };
        }
    }
}
