/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include <precomp.h>

#include "stateMachine.hpp"

#include "ascii.hpp"

using namespace Microsoft::Console::VirtualTerminal;

Tracing _trace;

StateMachine::StateMachine(_In_ TermDispatch* const pDispatch) :
    _pDispatch(pDispatch),
    _state(VTStates::Ground)
{
    _ActionClear();
}

StateMachine::~StateMachine()
{

}

// Routine Description:
// - Determines if a character indicates an action that should be taken in the ground state - 
//     These are C0 characters and CAN, SUB, and ESC 
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsActionableFromGround(_In_ wchar_t const wch)
{
    return wch <= AsciiChars::US;
}

// Routine Description:
// - Determines if a character belongs to the C0 escape range.
//   This is character sequences less than a space character (null, backspace, new line, etc.)
//   See also https://en.wikipedia.org/wiki/C0_and_C1_control_codes
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsC0Code(_In_ wchar_t const wch)
{
    return (wch >= AsciiChars::NUL && wch <= AsciiChars::ETB) ||
           wch == AsciiChars::EM ||
           (wch >= AsciiChars::FS && wch <= AsciiChars::US);
}

// Routine Description:
// - Determines if a character is a valid intermediate in an VT escape sequence.
//   Intermediates are punctuation type characters that are generally vendor specific and
//   modify the operational mode of a command.
//   See also http://vt100.net/emu/dec_ansi_parser
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsIntermediate(_In_ wchar_t const wch)
{
    return wch >= L' ' && wch <= L'/'; // 0x20 - 0x2F
}

// Routine Description:
// - Determines if a character is the delete character.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsDelete(_In_ wchar_t const wch)
{
    return wch == AsciiChars::DEL;
}

// Routine Description:
// - Determines if a character is the escape character.
//   Used to start escape sequences.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsEscape(_In_ wchar_t const wch)
{
    return wch == AsciiChars::ESC;
}

// Routine Description:
// - Determines if a character is "control sequence" beginning indicator.
//   This immediately follows an escape and signifies a varying length control sequence.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsCsiIndicator(_In_ wchar_t const wch)
{
    return wch == L'['; // 0x5B
}

// Routine Description:
// - Determines if a character is a delimiter between two parameters in a "control sequence"
//   This occurs in the middle of a control sequence after escape and CsiIndicator have been recognized
//   between a series of parameters.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsCsiDelimiter(_In_ wchar_t const wch)
{
    return wch == L';'; // 0x3B
}

// Routine Description:
// - Determines if a character is a valid parameter value
//   Parameters must be numerical digits. 
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsCsiParamValue(_In_ wchar_t const wch)
{
    return wch >= L'0' && wch <= L'9'; // 0x30 - 0x39
}

// Routine Description:
// - Determines if a character is a private range marker for a control sequence.
//   Private range markers indicate vendor-specific behavior.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsCsiPrivateMarker(_In_ wchar_t const wch)
{
    return wch == L'<' || wch == L'=' || wch == L'>' || wch == L'?'; // 0x3C - 0x3F
}

// Routine Description:
// - Determines if a character is invalid in a control sequence
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsCsiInvalid(_In_ wchar_t const wch)
{
    return wch == L':'; // 0x3A
}

// Routine Description:
// - Determines if a character is "operating system control string" beginning indicator.
//   This immediately follows an escape and signifies a varying length control string.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsOscIndicator(_In_ wchar_t const wch)
{
    return wch == L']'; // 0x5D
}

// Routine Description:
// - Determines if a character is a delimiter between two parameters in a "operating system control sequence"
//   This occurs in the middle of a control sequence after escape and OscIndicator have been recognized,
//   after the paramater indicating which OSC action to take. 
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsOscDelimiter(_In_ wchar_t const wch)
{
    return wch == L';'; // 0x3B
}

// Routine Description:
// - Determines if a character is a valid parameter value for an OSC String,
//     that is, the indicator of which OSC action to take.
//   Parameters must be numerical digits. 
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsOscParamValue(_In_ wchar_t const wch)
{
    return wch >= L'0' && wch <= L'9'; // 0x30 - 0x39
}

// Routine Description:
// - Determines if a character should be ignored in a operating system control sequence
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsOscInvalid(_In_ wchar_t const wch)
{
    return wch <= L'\x17' ||
           wch == L'\x19' ||
           (wch >= L'\x1c' && wch <= L'\x1f') ; 
}

// Routine Description:
// - Determines if a character is "operating system control string" termination indicator.
//   This signals the end of an OSC string collection.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsOscTerminator(_In_ wchar_t const wch)
{
    return wch == L'\x7'; // Bell character
}

// Routine Description:
// - Triggers the Execute action to indicate that the listener should immediately respond to a C0 control character.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionExecute(_In_ wchar_t const wch)
{
    _trace.TraceOnExecute(wch);
    _pDispatch->Execute(wch);
}

// Routine Description:
// - Triggers the Print action to indicate that the listener should render the character given.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionPrint(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"Print");
    _pDispatch->Print(wch); // call print
}



// Routine Description:
// - Triggers the EscDispatch action to indicate that the listener should handle a simple escape sequence.
//   These sequences traditionally start with ESC and a simple letter. No complicated parameters.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionEscDispatch(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"EscDispatch");

    bool fSuccess = false;

    // no intermediates.
    if (_cIntermediate == 0)
    {
        switch (wch)
        {
        case VTActionCodes::CUU_CursorUp:
            fSuccess = _pDispatch->CursorUp(1);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::CUU);
            break;
        case VTActionCodes::CUD_CursorDown:
            fSuccess = _pDispatch->CursorDown(1);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::CUD);
            break;
        case VTActionCodes::CUF_CursorForward:
            fSuccess = _pDispatch->CursorForward(1);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::CUF);
            break;
        case VTActionCodes::CUB_CursorBackward:
            fSuccess = _pDispatch->CursorBackward(1);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::CUB);
            break;
        case VTActionCodes::DECSC_CursorSave:
            fSuccess = _pDispatch->CursorSavePosition();
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECSC);
            break;
        case VTActionCodes::DECRC_CursorRestore:
            fSuccess = _pDispatch->CursorRestorePosition();
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECRC);
            break;
        case VTActionCodes::DECKPAM_KeypadApplicationMode:
            fSuccess = _pDispatch->SetKeypadMode(true);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECKPAM);
            break;
        case VTActionCodes::DECKPNM_KeypadNumericMode:
            fSuccess = _pDispatch->SetKeypadMode(false);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECKPNM);
            break;
        case VTActionCodes::RI_ReverseLineFeed:
            fSuccess = _pDispatch->ReverseLineFeed();
            TermTelemetry::Instance().Log(TermTelemetry::Codes::RI);
            break;
        case VTActionCodes::HTS_HorizontalTabSet:
            fSuccess = _pDispatch->HorizontalTabSet();
            TermTelemetry::Instance().Log(TermTelemetry::Codes::HTS);
            break;
        }
    }
    else if (_cIntermediate == 1)
    {
        DesignateCharsetTypes designateType = s_DefaultDesignateCharsetType;
        fSuccess = _GetDesignateType(&designateType);
        switch (designateType)
        {
        case DesignateCharsetTypes::G0:
            fSuccess = _pDispatch->DesignateCharset(wch);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DesignateG0);
            break;
        case DesignateCharsetTypes::G1:
            fSuccess = false;
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DesignateG1);
            break;
        case DesignateCharsetTypes::G2:
            fSuccess = false;
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DesignateG2);
            break;
        case DesignateCharsetTypes::G3:
            fSuccess = false;
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DesignateG3);
            break;
        }
    }

    // Trace the result.
    _trace.DispatchSequenceTrace(fSuccess);

    if (!fSuccess)
    {
        // Suppress it and log telemetry on failed cases
        TermTelemetry::Instance().LogFailed(wch);
    }
}

// Routine Description:
// - Triggers the CsiDispatch action to indicate that the listener should handle a control sequence.
//   These sequences perform various API-type commands that can include many parameters.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionCsiDispatch(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"CsiDispatch");

    bool fSuccess = false;
    unsigned int uiDistance = 0;
    unsigned int uiLine = 0;
    unsigned int uiColumn = 0;
    SHORT sTopMargin = 0;
    SHORT sBottomMargin = 0;
    SHORT sNumTabs = 0;
    SHORT sClearType = 0;
    TermDispatch::EraseType eraseType = TermDispatch::EraseType::ToEnd;
    TermDispatch::GraphicsOptions rgGraphicsOptions[s_cParamsMax];
    size_t cOptions = ARRAYSIZE(rgGraphicsOptions);
    TermDispatch::AnsiStatusType deviceStatusType = (TermDispatch::AnsiStatusType)-1; // there is no default status type.
    

    if (_cIntermediate == 0)
    {
        // fill params
        switch (wch)
        {
        case VTActionCodes::CUU_CursorUp:
        case VTActionCodes::CUD_CursorDown:
        case VTActionCodes::CUF_CursorForward:
        case VTActionCodes::CUB_CursorBackward:
        case VTActionCodes::CNL_CursorNextLine:
        case VTActionCodes::CPL_CursorPrevLine:
        case VTActionCodes::CHA_CursorHorizontalAbsolute:
        case VTActionCodes::VPA_VerticalLinePositionAbsolute:
        case VTActionCodes::ICH_InsertCharacter:
        case VTActionCodes::DCH_DeleteCharacter:
        case VTActionCodes::ECH_EraseCharacters:
            fSuccess = _GetCursorDistance(&uiDistance);
            break;
        case VTActionCodes::HVP_HorizontalVerticalPosition:
        case VTActionCodes::CUP_CursorPosition:
            fSuccess = _GetXYPosition(&uiLine, &uiColumn);
            break;
        case VTActionCodes::DECSTBM_SetScrollingRegion:
            fSuccess = _GetTopBottomMargins(&sTopMargin, &sBottomMargin);
            break;
        case VTActionCodes::ED_EraseDisplay:
        case VTActionCodes::EL_EraseLine:
            fSuccess = _GetEraseOperation(&eraseType);
            break;
        case VTActionCodes::SGR_SetGraphicsRendition:
            fSuccess = _GetGraphicsOptions(rgGraphicsOptions, &cOptions);
            break;
        case VTActionCodes::DSR_DeviceStatusReport:
            fSuccess = _GetDeviceStatusOperation(&deviceStatusType);
            break;
        case VTActionCodes::DA_DeviceAttributes:
            fSuccess = _VerifyDeviceAttributesParams();
            break;
        case VTActionCodes::SU_ScrollUp:
        case VTActionCodes::SD_ScrollDown:
            fSuccess = _GetScrollDistance(&uiDistance);
            break;
        case VTActionCodes::ANSISYSSC_CursorSave:
        case VTActionCodes::ANSISYSRC_CursorRestore:
            fSuccess = _VerifyHasNoParameters();
            break;
        case VTActionCodes::IL_InsertLine:
        case VTActionCodes::DL_DeleteLine:
            fSuccess = _GetScrollDistance(&uiDistance);
            break;
        case VTActionCodes::CHT_CursorForwardTab:
        case VTActionCodes::CBT_CursorBackTab:
            fSuccess = _GetTabDistance(&sNumTabs);
            break;
        case VTActionCodes::TBC_TabClear:
            fSuccess = _GetTabClearType(&sClearType);
            break;
        default:
            // If no params to fill, param filling was successful.
            fSuccess = true;
            break;
        }

        // if param filling successful, try to dispatch
        if (fSuccess)
        {
            switch (wch)
            {
            case VTActionCodes::CUU_CursorUp:
                fSuccess = _pDispatch->CursorUp(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CUU);
                break;
            case VTActionCodes::CUD_CursorDown:
                fSuccess = _pDispatch->CursorDown(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CUD);
                break;
            case VTActionCodes::CUF_CursorForward:
                fSuccess = _pDispatch->CursorForward(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CUF);
                break;
            case VTActionCodes::CUB_CursorBackward:
                fSuccess = _pDispatch->CursorBackward(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CUB);
                break;
            case VTActionCodes::CNL_CursorNextLine:
                fSuccess = _pDispatch->CursorNextLine(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CNL);
                break;
            case VTActionCodes::CPL_CursorPrevLine:
                fSuccess = _pDispatch->CursorPrevLine(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CPL);
                break;
            case VTActionCodes::CHA_CursorHorizontalAbsolute:
                fSuccess = _pDispatch->CursorHorizontalPositionAbsolute(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CHA);
                break;
            case VTActionCodes::VPA_VerticalLinePositionAbsolute:
                fSuccess = _pDispatch->VerticalLinePositionAbsolute(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::VPA);
                break;
            case VTActionCodes::CUP_CursorPosition:
            case VTActionCodes::HVP_HorizontalVerticalPosition:
                fSuccess = _pDispatch->CursorPosition(uiLine, uiColumn);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CUP);
                break;
            case VTActionCodes::DECSTBM_SetScrollingRegion:
                fSuccess = _pDispatch->SetTopBottomScrollingMargins(sTopMargin, sBottomMargin);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DECSTBM);
                break;
            case VTActionCodes::ICH_InsertCharacter:
                fSuccess = _pDispatch->InsertCharacter(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::ICH);
                break;
            case VTActionCodes::DCH_DeleteCharacter:
                fSuccess = _pDispatch->DeleteCharacter(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DCH);
                break;
            case VTActionCodes::ED_EraseDisplay:
                fSuccess = _pDispatch->EraseInDisplay(eraseType);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::ED);
                break;
            case VTActionCodes::EL_EraseLine:
                fSuccess = _pDispatch->EraseInLine(eraseType);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::EL);
                break;
            case VTActionCodes::SGR_SetGraphicsRendition:
                fSuccess = _pDispatch->SetGraphicsRendition(rgGraphicsOptions, cOptions);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::SGR);
                break;
            case VTActionCodes::DSR_DeviceStatusReport:
                fSuccess = _pDispatch->DeviceStatusReport(deviceStatusType); 
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DSR);
                break;
            case VTActionCodes::DA_DeviceAttributes:
                fSuccess = _pDispatch->DeviceAttributes();
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DA);
                break;
            case VTActionCodes::SU_ScrollUp:
                fSuccess = _pDispatch->ScrollUp(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::SU);
                break;
            case VTActionCodes::SD_ScrollDown:
                fSuccess = _pDispatch->ScrollDown(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::SD);
                break;
            case VTActionCodes::ANSISYSSC_CursorSave:
                fSuccess = _pDispatch->CursorSavePosition();
                TermTelemetry::Instance().Log(TermTelemetry::Codes::ANSISYSSC);
                break;
            case VTActionCodes::ANSISYSRC_CursorRestore:
                fSuccess = _pDispatch->CursorRestorePosition();
                TermTelemetry::Instance().Log(TermTelemetry::Codes::ANSISYSRC);
                break;
            case VTActionCodes::IL_InsertLine:
                fSuccess = _pDispatch->InsertLine(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::IL);
                break;
            case VTActionCodes::DL_DeleteLine:
                fSuccess = _pDispatch->DeleteLine(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DL);
                break;
            case VTActionCodes::CHT_CursorForwardTab:
                fSuccess = _pDispatch->ForwardTab(sNumTabs);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CHT);
                break;
            case VTActionCodes::CBT_CursorBackTab:
                fSuccess = _pDispatch->BackwardsTab(sNumTabs);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::CBT);
                break;
            case VTActionCodes::TBC_TabClear:
                fSuccess = _pDispatch->TabClear(sClearType);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::TBC);
                break;
            case VTActionCodes::ECH_EraseCharacters:
                fSuccess = _pDispatch->EraseCharacters(uiDistance);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::ECH);
                break;
            default:
                // If no functions to call, overall dispatch was a failure.
                fSuccess = false;
                break;
            }
        }
    }
    else if (_cIntermediate == 1)
    {
        switch (_wchIntermediate)
        {
        case L'?':
            fSuccess = _IntermediateQuestionMarkDispatch(wch);
            break;
        case L'!':
            fSuccess = _IntermediateExclamationDispatch(wch);
            break;
        }
    }

    // Trace the result.
    _trace.DispatchSequenceTrace(fSuccess);

    if (!fSuccess)
    {
        // Suppress it and log telemetry on failed cases
        TermTelemetry::Instance().LogFailed(wch);
    }
}


// Routine Description:
// - Handles actions that have postfix params on an intermediate '?', such as DECTCEM, DECCOLM, ATT610
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - True if handled successfully. False otherwise.
bool StateMachine::_IntermediateQuestionMarkDispatch(_In_ wchar_t const wchAction)
{
    _trace.TraceOnAction(L"_IntermediateQuestionMarkDispatch");
    bool fSuccess = false;

    TermDispatch::PrivateModeParams rgPrivateModeParams[s_cParamsMax];
    size_t cOptions = ARRAYSIZE(rgPrivateModeParams);
    // Ensure that there was the right number of params
    switch (wchAction)
    {
        case VTActionCodes::DECSET_PrivateModeSet:
        case VTActionCodes::DECRST_PrivateModeReset:
            fSuccess = _GetPrivateModeParams(rgPrivateModeParams, &cOptions);
            break;
            
        default:
            // If no params to fill, param filling was successful.
            fSuccess = true;
            break;
    }
    if (fSuccess)
    {
        switch(wchAction)
        {
        case VTActionCodes::DECSET_PrivateModeSet:
            fSuccess = _pDispatch->SetPrivateModes(rgPrivateModeParams, cOptions);
            //TODO: MSFT:6367459 Add specific telemetry for each of the DECSET/DECRST codes
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECSET);
            break;
        case VTActionCodes::DECRST_PrivateModeReset:
            fSuccess = _pDispatch->ResetPrivateModes(rgPrivateModeParams, cOptions);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::DECRST);
            break;
        default:
            // If no functions to call, overall dispatch was a failure.
            fSuccess = false;
            break;
        }
    }
    return fSuccess;
}


// Routine Description:
// - Handles actions that have an intermediate '!', such as DECSTR
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - True if handled successfully. False otherwise.
bool StateMachine::_IntermediateExclamationDispatch(_In_ wchar_t const wchAction)
{
    _trace.TraceOnAction(L"_IntermediateExclamationDispatch");
    bool fSuccess = false;

    switch(wchAction)
    {
    case VTActionCodes::DECSTR_SoftReset:
        fSuccess = _pDispatch->SoftReset();
        TermTelemetry::Instance().Log(TermTelemetry::Codes::DECSTR);
        break;
    default:
        // If no functions to call, overall dispatch was a failure.
        fSuccess = false;
        break;
    }
    return fSuccess;
}
// Routine Description:
// - Triggers the Collect action to indicate that the state machine should store this character as part of an escape/control sequence.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionCollect(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"Collect");

    // store collect data
    if (_cIntermediate < s_cIntermediateMax)
    {
        _wchIntermediate = wch;
    }
    
    _cIntermediate++;
}

// Routine Description:
// - Triggers the Param action to indicate that the state machine should store this character as a part of a parameter
//   to a control sequence.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionParam(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"Param");

    // if we're past the end, this param is just ignored.
    // verify both the count and that the pointer didn't run off the end of the array
    if (_cParams < s_cParamsMax && _pusActiveParam < &_rgusParams[s_cParamsMax])
    {
        if (_iParamAccumulatePos == 0)
        {
            // if we've just started filling a new parameter, increment the total number of params we have.
            _cParams++;
        }

        // store parameter data
        if (wch == L';')
        {
            // delimiter, move to next param
            _pusActiveParam++;

            // clear out the accumulator count to prepare for the next one
            _iParamAccumulatePos = 0;
        }
        else
        {
            // don't bother accumulating if we're storing more than 4 digits (since we're putting it into a short)
            if (_iParamAccumulatePos < 5)
            {
                // convert character into digit.
                unsigned short const usDigit = wch - L'0'; // convert character into value

                // multiply existing values by 10 to make space in the 1s digit
                *_pusActiveParam *= 10;

                // mark that we've now stored another digit.
                _iParamAccumulatePos++;

                // store the digit in the 1s place.
                *_pusActiveParam += usDigit;

                if (*_pusActiveParam > SHORT_MAX)
                {
                    *_pusActiveParam = SHORT_MAX;
                }
            }
            else
            {
                *_pusActiveParam = SHORT_MAX; 
            }
        }
    }
}

// Routine Description:
// - Triggers the Clear action to indicate that the state machine should erase all internal state.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionClear()
{
    _trace.TraceOnAction(L"Clear");

    // clear all internal stored state.
    _wchIntermediate = 0;
    _cIntermediate = 0;

    for (unsigned short i = 0; i < s_cParamsMax; i++)
    {
        _rgusParams[i] = 0;
    }

    _cParams = 0;
    _iParamAccumulatePos = 0;
    _pusActiveParam = _rgusParams; // set pointer back to beginning of array

    _sOscParam = 0;
    _sOscNextChar = 0;

}

// Routine Description:
// - Triggers the Ignore action to indicate that the state machine should eat this character and say nothing.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionIgnore()
{
    // do nothing.
    _trace.TraceOnAction(L"Ignore");
}

// Routine Description:
// - Stores this character as part of the param indicating which OSC action to take.
// Arguments:
// - wch - Character to collect.
// Return Value:
// - <none>
void StateMachine::_ActionOscParam(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"OscParamCollect");

    // don't bother accumulating if we're storing more than 4 digits (since we're putting it into a short)
    if (_iParamAccumulatePos < 5)
    {
        // convert character into digit.
        unsigned short const usDigit = wch - L'0'; // convert character into value

        // multiply existing values by 10 to make space in the 1s digit
        _sOscParam *= 10;

        // mark that we've now stored another digit.
        _iParamAccumulatePos++;

        // store the digit in the 1s place.
        _sOscParam += usDigit;

        if (_sOscParam > SHORT_MAX)
        {
            _sOscParam = SHORT_MAX;
        }
    }
    else
    {
        _sOscParam = SHORT_MAX; 
    }  
}

// Routine Description:
// - Stores this character as part of the OSC string
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionOscPut(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"OscPut");

    // if we're past the end, this param is just ignored.
    // need to leave one char for \0 at end
    if (_sOscNextChar < s_cOscStringMaxLength - 1)
    {
        _pwchOscStringBuffer[_sOscNextChar] = wch;
        _sOscNextChar++;
        //we'll place the null at the end of the string when we send the actual action.
    }
}

// Routine Description:
// - Triggers the CsiDispatch action to indicate that the listener should handle a control sequence.
//   These sequences perform various API-type commands that can include many parameters.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - <none>
void StateMachine::_ActionOscDispatch(_In_ wchar_t const wch)
{
    _trace.TraceOnAction(L"OscDispatch");

    bool fSuccess = false;
    wchar_t* pwchTitle = nullptr;  
    unsigned short sCchTitleLength = 0;

    switch (_sOscParam)
    {
    case OscActionCodes::SetIconAndWindowTitle:
    case OscActionCodes::SetWindowIcon:
    case OscActionCodes::SetWindowTitle:
        fSuccess = _GetOscTitle(&pwchTitle, &sCchTitleLength);
        break;
    }  
    if (fSuccess)
    {
        switch (_sOscParam)
        {
        case OscActionCodes::SetIconAndWindowTitle:
        case OscActionCodes::SetWindowIcon:
        case OscActionCodes::SetWindowTitle:
            fSuccess = _pDispatch->SetWindowTitle(pwchTitle, sCchTitleLength);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::OSCWT);
        default:
            // If no functions to call, overall dispatch was a failure.
            fSuccess = false;
            break;
        }        
    }  
    // Trace the result.
    _trace.DispatchSequenceTrace(fSuccess);

    if (!fSuccess)
    {
        // Suppress it and log telemetry on failed cases
        TermTelemetry::Instance().LogFailed(wch);
    }
}


// Routine Description:
// - Moves the state machine into the Ground state.
//   This state is entered:
//   1. By default at the beginning of operation
//   2. After any execute/dispatch action.
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterGround()
{
    _state = VTStates::Ground;
    _trace.TraceStateChange(L"Ground");
}

// Routine Description:
// - Moves the state machine into the Escape state.
//   This state is entered:
//   1. When the Escape character is seen at any time.
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterEscape()
{
    _state = VTStates::Escape;
    _trace.TraceStateChange(L"Escape");
    _ActionClear();
    _trace.ClearSequenceTrace();
}

// Routine Description:
// - Moves the state machine into the EscapeIntermediate state.
//   This state is entered:
//   1. When EscIntermediate characters are seen after an Escape entry (only from the Escape state)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterEscapeIntermediate()
{
    _state = VTStates::EscapeIntermediate;
    _trace.TraceStateChange(L"EscapeIntermediate");
}

// Routine Description:
// - Moves the state machine into the CsiEntry state.
//   This state is entered:
//   1. When the CsiEntry character is seen after an Escape entry (only from the Escape state)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterCsiEntry()
{
    _state = VTStates::CsiEntry;
    _trace.TraceStateChange(L"CsiEntry");
    _ActionClear();
}

// Routine Description:
// - Moves the state machine into the CsiParam state.
//   This state is entered:
//   1. When valid parameter characters are detected on entering a CSI (from CsiEntry state)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterCsiParam()
{
    _state = VTStates::CsiParam;
    _trace.TraceStateChange(L"CsiParam");
}

// Routine Description:
// - Moves the state machine into the CsiIgnore state.
//   This state is entered:
//   1. When an invalid character is detected during a CSI sequence indicating we should ignore the whole sequence.
//      (From CsiEntry, CsiParam, or CsiIntermediate states.)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterCsiIgnore()
{
    _state = VTStates::CsiIgnore;
    _trace.TraceStateChange(L"CsiIgnore");
}

// Routine Description:
// - Moves the state machine into the CsiIntermediate state.
//   This state is entered:
//   1. When an intermediate character is seen immediately after entering a control sequence (from CsiEntry)
//   2. When an intermediate character is seen while collecting parameter data (from CsiParam)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterCsiIntermediate()
{
    _state = VTStates::CsiIntermediate;
    _trace.TraceStateChange(L"CsiIntermediate");
}

// Routine Description:
// - Moves the state machine into the OscParam state.
//   This state is entered:
//   1. When an OscEntry character (']') is seen after an Escape entry (only from the Escape state)
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterOscParam()
{
    _state = VTStates::OscParam;
    _trace.TraceStateChange(L"OscParam");
}

// Routine Description:
// - Moves the state machine into the OscString state.
//   This state is entered:
//   1. When a delimiter character (';') is seen in the OSC Param state.
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::_EnterOscString()
{
    _state = VTStates::OscString;
    _trace.TraceStateChange(L"OscString");
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the Ground state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Print all other characters
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventGround(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"Ground");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else
    {
        _ActionPrint(wch);
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the Escape state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Enter Control Sequence state
//   5. Dispatch an Escape action.
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventEscape(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"Escape");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionCollect(wch);
        _EnterEscapeIntermediate();
    }
    else if (s_IsCsiIndicator(wch))
    {
        _EnterCsiEntry();
    }
    else if (s_IsOscIndicator(wch))
    {
        _EnterOscParam();
    }
    else
    {
        _ActionEscDispatch(wch);
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the EscapeIntermediate state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Dispatch an Escape action.
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventEscapeIntermediate(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"EscapeIntermediate");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionCollect(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else
    {
        _ActionEscDispatch(wch);
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the CsiEntry state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Begin to ignore all remaining parameters when an invalid character is detected (CsiIgnore)
//   5. Store parameter data
//   6. Collect Control Sequence Private markers
//   7. Dispatch a control sequence with parameters for action
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventCsiEntry(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"CsiEntry");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionCollect(wch);
        _EnterCsiIntermediate();
    }
    else if (s_IsCsiInvalid(wch))
    {
        _EnterCsiIgnore();
    }
    else if (s_IsCsiParamValue(wch) || s_IsCsiDelimiter(wch))
    {
        _ActionParam(wch);
        _EnterCsiParam();
    }
    else if (s_IsCsiPrivateMarker(wch))
    {
        _ActionCollect(wch);
        _EnterCsiParam();
    }
    else
    {
        _ActionCsiDispatch(wch);
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the CsiIntermediate state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Begin to ignore all remaining parameters when an invalid character is detected (CsiIgnore)
//   5. Dispatch a control sequence with parameters for action
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventCsiIntermediate(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"CsiIntermediate");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionCollect(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsCsiParamValue(wch) || s_IsCsiInvalid(wch) || s_IsCsiDelimiter(wch) || s_IsCsiPrivateMarker(wch))
    {
        _EnterCsiIgnore();
    }
    else
    {
        _ActionCsiDispatch(wch);
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the CsiIgnore state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Begin to ignore all remaining parameters when an invalid character is detected (CsiIgnore)
//   5. Return to Ground
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventCsiIgnore(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"CsiIgnore");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsCsiParamValue(wch) || s_IsCsiInvalid(wch) || s_IsCsiDelimiter(wch) || s_IsCsiPrivateMarker(wch))
    {
        _ActionIgnore();
    }
    else
    {
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the CsiParam state.
//   Events in this state will:
//   1. Execute C0 control characters
//   2. Ignore Delete characters
//   3. Collect Intermediate characters
//   4. Begin to ignore all remaining parameters when an invalid character is detected (CsiIgnore)
//   5. Store parameter data
//   6. Dispatch a control sequence with parameters for action
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventCsiParam(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"CsiParam");
    if (s_IsC0Code(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsDelete(wch))
    {
        _ActionIgnore();
    }
    else if (s_IsCsiParamValue(wch) || s_IsCsiDelimiter(wch))
    {
        _ActionParam(wch);
    }
    else if (s_IsIntermediate(wch))
    {
        _ActionCollect(wch);
        _EnterCsiIntermediate();
    }
    else if (s_IsCsiInvalid(wch) || s_IsCsiPrivateMarker(wch))
    {
        _EnterCsiIgnore();
    }
    else
    {
        _ActionCsiDispatch(wch);
        _EnterGround();
    }
}

// Routine Description:
// - Processes a character event into an Action that occurs while in the OscParam state.
//   Events in this state will:
//   1. Collect numeric values into an Osc Param
//   2. Move to the OscString state on a delimiter
//   3. Ignore everything else.
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventOscParam(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"OscParam");
    if (s_IsOscTerminator(wch))
    {
        _EnterGround();
    }
    else if (s_IsOscParamValue(wch))
    {
        _ActionOscParam(wch);
    }
    else if (s_IsOscDelimiter(wch))
    {
        _EnterOscString();
    }
    else
    {
        _ActionIgnore();
    }
}

// Routine Description:
// - Processes a character event into a Action that occurs while in the OscParam state.
//   Events in this state will:
//   1. Trigger the OSC action associated with the param on an OscTerminator
//   2. Ignore OscInvalid characters.
//   3. Collect everything else into the OscString
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventOscString(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"OscString");
    if (s_IsOscTerminator(wch))
    {
        _ActionOscDispatch(wch);
        _EnterGround();
    }
    else if (s_IsOscInvalid(wch))
    {
        _ActionIgnore();
    }
    else
    {
        // add this character to our OSC string
        _ActionOscPut(wch);
    }
}

// Routine Description:
// - Retrieves the listed graphics options to be applied in order to the "font style" of the next characters inserted into the buffer.
// Arguments:
// - rgGraphicsOptions - Pointer to array space (expected 16 max, the max number of params this can generate) that will be filled with valid options from the GraphicsOptions enum
// - pcOptions - Pointer to the length of rgGraphicsOptions on the way in, and the count of the array used on the way out.
// Return Value:
// - True if we successfully retrieved an array of valid graphics options from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetGraphicsOptions(_Out_writes_(*pcOptions) TermDispatch::GraphicsOptions* rgGraphicsOptions, _Inout_ size_t* pcOptions) const
{
    bool fSuccess = true;

    if (_cParams == 0)
    {
        if (*pcOptions >= 1)
        {
            rgGraphicsOptions[0] = s_defaultGraphicsOption;
            *pcOptions = 1;
        }
        else
        {
            fSuccess = false; // not enough space in buffer to hold response.
        }
    }
    else
    {
        if (*pcOptions >= _cParams)
        {
            for (size_t i = 0; i < _cParams; i++)
            {
                // No memcpy. The parameters are shorts. The graphics options are unsigned ints.
                rgGraphicsOptions[i] = (TermDispatch::GraphicsOptions)_rgusParams[i];
            }

            *pcOptions = _cParams;
        }
        else
        {
            fSuccess = false; // not enough space in buffer to hold response.
        }
    }

    return fSuccess; 
}

// Routine Description:
// - Retrieves the erase type parameter for an upcoming operation.
// Arguments:
// - pEraseType - Memory location to receive the erase type parameter
// Return Value:
// - True if we successfully pulled an erase type from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetEraseOperation(_Out_ TermDispatch::EraseType* const pEraseType) const
{
    bool fSuccess = false; // If we have too many parameters or don't know what to do with the given value, return false.
    *pEraseType = s_defaultEraseType; // if we fail, just put the default type in.

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        *pEraseType = s_defaultEraseType;
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, attempt to match it to the values we accept.
        unsigned short const usParam = _rgusParams[0];

        switch (usParam)
        {
        case 0:
            *pEraseType = TermDispatch::EraseType::ToEnd;
            fSuccess = true;
            break;
        case 1:
            *pEraseType = TermDispatch::EraseType::FromBeginning;
            fSuccess = true;
            break;
        case 2:
            *pEraseType = TermDispatch::EraseType::All;
            fSuccess = true;
            break;
        }
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves a distance for a cursor operation from the parameter pool stored during Param actions.
// Arguments:
// - puiDistance - Memory location to receive the distance
// Return Value:
// - True if we successfully pulled the cursor distance from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetCursorDistance(_Out_ unsigned int* const puiDistance) const
{
    bool fSuccess = false;
    *puiDistance = s_uiDefaultCursorDistance;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, use it.
        *puiDistance = _rgusParams[0];
        fSuccess = true;
    }

    // Distances of 0 should be changed to 1. 
    if (*puiDistance == 0)
    {
        *puiDistance = s_uiDefaultCursorDistance;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves a distance for a scroll operation from the parameter pool stored during Param actions.
// Arguments:
// - puiDistance - Memory location to receive the distance
// Return Value:
// - True if we successfully pulled the scroll distance from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetScrollDistance(_Out_ unsigned int* const puiDistance) const
{
    bool fSuccess = false;
    *puiDistance = s_uiDefaultScrollDistance;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, use it.
        *puiDistance = _rgusParams[0];
        fSuccess = true;
    }

    // Distances of 0 should be changed to 1. 
    if (*puiDistance == 0)
    {
        *puiDistance = s_uiDefaultScrollDistance;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves a width for the console window from the parameter pool stored during Param actions.
// Arguments:
// - puiConsoleWidth - Memory location to receive the width
// Return Value:
// - True if we successfully pulled the width from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetConsoleWidth(_Out_ unsigned int* const puiConsoleWidth) const
{
    bool fSuccess = false;
    *puiConsoleWidth = s_uiDefaultConsoleWidth;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, use it.
        *puiConsoleWidth = _rgusParams[0];
        fSuccess = true;
    }

    // Distances of 0 should be changed to 80. 
    if (*puiConsoleWidth == 0)
    {
        *puiConsoleWidth = s_uiDefaultConsoleWidth;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves an X/Y coordinate pair for a cursor operation from the parameter pool stored during Param actions.
// Arguments:
// - puiLine - Memory location to receive the Y/Line/Row position
// - puiColumn - Memory location to receive the X/Column position
// Return Value:
// - True if we successfully pulled the cursor coordinates from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetXYPosition(_Out_ unsigned int* const puiLine, _Out_ unsigned int* const puiColumn) const
{
    bool fSuccess = false;
    *puiLine = s_uiDefaultLine;
    *puiColumn = s_uiDefaultColumn;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's only one param, leave the default for the column, and retrieve the specified row.
        *puiLine = _rgusParams[0];
        fSuccess = true;
    }
    else if (_cParams == 2)
    {
        // If there are exactly two parameters, use them.
        *puiLine = _rgusParams[0];
        *puiColumn = _rgusParams[1];
        fSuccess = true;
    }

    // Distances of 0 should be changed to 1. 
    if (*puiLine == 0)
    {
        *puiLine = s_uiDefaultLine;
    }

    if (*puiColumn == 0)
    {
        *puiColumn = s_uiDefaultColumn;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves a top and bottom pair for setting the margins from the parameter pool stored during Param actions
// Arguments:
// - psTopMargin - Memory location to receive the top margin
// - psBottomMargin - Memory location to receive the bottom margin
// Return Value:
// - True if we successfully pulled the margin settings from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetTopBottomMargins(_Out_ SHORT* const psTopMargin, _Out_ SHORT* const psBottomMargin) const
{
    // Notes:                           (input -> state machine out -> adapter out -> conhost internal)
    // having only a top param is legal         ([3;r   -> 3,0   -> 3,h  -> 3,h,true)
    // having only a bottom param is legal      ([;3r   -> 0,3   -> 1,3  -> 1,3,true)
    // having neither uses the defaults         ([;r [r -> 0,0   -> 3,h  -> 0,0,false)
    // an illegal combo (eg, 3;2r) is ignored

    bool fSuccess = false;
    *psTopMargin = s_sDefaultTopMargin;
    *psBottomMargin = s_sDefaultBottomMargin;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        *psTopMargin = _rgusParams[0];
        fSuccess = true;
    }
    else if (_cParams == 2)
    {
        // If there are exactly two parameters, use them.
        *psTopMargin = _rgusParams[0];
        *psBottomMargin = _rgusParams[1];
        fSuccess = true;
    }

    if (*psBottomMargin > 0 && *psBottomMargin < *psTopMargin)
    {
        fSuccess = false;
    }
    return fSuccess;
}
// Routine Description:
// - Retrieves the status type parameter for an upcoming device query operation
// Arguments:
// - pStatusType - Memory location to receive the Status Type parameter
// Return Value:
// - True if we successfully found a device operation in the parameters stored. False otherwise.
_Success_(return)
bool StateMachine::_GetDeviceStatusOperation(_Out_ TermDispatch::AnsiStatusType* const pStatusType) const
{
    bool fSuccess = false;
    *pStatusType = (TermDispatch::AnsiStatusType)0;

    if (_cParams == 1)
    {
        // If there's one parameter, attempt to match it to the values we accept.
        unsigned short const usParam = _rgusParams[0];

        switch (usParam)
        {
        // This looks kinda silly, but I want the parser to reject (fSuccess = false) any status types we haven't put here.
        case (unsigned short)TermDispatch::AnsiStatusType::CPR_CursorPositionReport:
            *pStatusType = TermDispatch::AnsiStatusType::CPR_CursorPositionReport;
            fSuccess = true;
            break;
        }
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves the listed private mode params be set/reset by DECSET/DECRST
// Arguments:
// - rPrivateModeParams - Pointer to array space (expected 16 max, the max number of params this can generate) that will be filled with valid params from the PrivateModeParams enum
// - pcParams - Pointer to the length of rPrivateModeParams on the way in, and the count of the array used on the way out.
// Return Value:
// - True if we successfully retrieved an array of private mode params from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetPrivateModeParams(_Out_writes_(*pcParams) TermDispatch::PrivateModeParams* rgPrivateModeParams, _Inout_ size_t* pcParams) const
{
    bool fSuccess = true;

    if (_cParams == 0)
    {
        fSuccess = false; // Can't just set nothing at all
    }
    else
    {
        if (*pcParams >= _cParams)
        {
            for (size_t i = 0; i < _cParams; i++)
            {
                // No memcpy. The parameters are shorts. The graphics options are unsigned ints.
                rgPrivateModeParams[i] = (TermDispatch::PrivateModeParams)_rgusParams[i];
            }
            *pcParams = _cParams;
        }
        else
        {
            fSuccess = false; // not enough space in buffer to hold response.
        }
    }
    return fSuccess; 
}

// - Verifies that no parameters were parsed for the current CSI sequence
// Arguments: 
// - <none>
// Return Value:
// - True if there were no parameters. False otherwise.
_Success_(return)
bool StateMachine::_VerifyHasNoParameters() const
{
    return _cParams == 0;
}

// Routine Description:
// - Validates that we received the correct parameter sequence for the Device Attributes command.
// - For DA, we should have received either NO parameters or just one 0 parameter. Anything else is not acceptable.
// Arguments:
// - <none>
// Return Value:
// - True if the DA params were valid. False otherwise.
_Success_(return)
bool StateMachine::_VerifyDeviceAttributesParams() const
{
    bool fSuccess = false;

    if (_cParams == 0)
    {
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        if (_rgusParams[0] == 0)
        {
            fSuccess = true;
        }
    }

    return fSuccess;
}

// Routine Description:
// - Null terminates, then returns, the string that we've collected as part of the OSC string.  
// Arguments: 
// - ppwchTitle - a pointer to point to the Osc String to use as a title.
// - pcchTitleLength - a pointer place the length of ppwchTitle into.
// Return Value:
// - True if there was a title to output. (a title with length=0 is still valid)
_Success_(return)
bool StateMachine::_GetOscTitle(_Outptr_result_buffer_(*pcchTitle) wchar_t** const ppwchTitle, _Out_ unsigned short * pcchTitle)
{
    *ppwchTitle = _pwchOscStringBuffer;
    *pcchTitle = _sOscNextChar;
    if (_pwchOscStringBuffer != nullptr)
    {
        // null terminate the string on the current char
        _pwchOscStringBuffer[_sOscNextChar] = L'\x0';
    }
    return _pwchOscStringBuffer != nullptr;
}

// Routine Description:
// - Retrieves a distance for a tab operation from the parameter pool stored during Param actions.
// Arguments:
// - psDistance - Memory location to receive the distance
// Return Value:
// - True if we successfully pulled the tab distance from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetTabDistance(_Out_ SHORT* const psDistance) const
{
    bool fSuccess = false;
    *psDistance = s_sDefaultTabDistance;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, use it.
        *psDistance = _rgusParams[0];
        fSuccess = true;
    }

    // Distances of 0 should be changed to 1. 
    if (*psDistance == 0)
    {
        *psDistance = s_sDefaultTabDistance;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves the type of tab clearing operation from the parameter pool stored during Param actions.
// Arguments:
// - psClearType - Memory location to receive the clear type
// Return Value:
// - True if we successfully pulled the tab clear type from the parameters we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetTabClearType(_Out_ SHORT* const psClearType) const
{
    bool fSuccess = false;
    *psClearType = s_sDefaultTabClearType;

    if (_cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (_cParams == 1)
    {
        // If there's one parameter, use it.
        *psClearType = _rgusParams[0];
        fSuccess = true;
    }
    return fSuccess;
}

// Routine Description:
// - Retrieves a designate charset type from the intermediate we've stored. False otherwise.
// Arguments:
// - pDesignateType - Memory location to receive the designate type.
// Return Value:
// - True if we successfully pulled the designate type from the intermediate we've stored. False otherwise.
_Success_(return)
bool StateMachine::_GetDesignateType(_Out_ DesignateCharsetTypes* const pDesignateType) const
{
    bool fSuccess = false;
    *pDesignateType = s_DefaultDesignateCharsetType;

    switch(_wchIntermediate)
    {
    case '(':
        *pDesignateType = DesignateCharsetTypes::G0;
        break;
    case ')':
    case '-':
        *pDesignateType = DesignateCharsetTypes::G1;
        break;
    case '*':
    case '.':
        *pDesignateType = DesignateCharsetTypes::G2;
        break;
    case '+':
    case '/':
        *pDesignateType = DesignateCharsetTypes::G3;
        break;
    }

    return fSuccess;
}

// Routine Description:
// - Entry to the state machine. Takes characters one by one and processes them according to the state machine rules.
// Arguments:
// - wch - New character to operate upon
// Return Value:
// - <none>
void StateMachine::ProcessCharacter(_In_ wchar_t const wch)
{
    _trace.TraceCharInput(wch);

    // Process "from anywhere" events first.
    if (wch == AsciiChars::CAN || 
        wch == AsciiChars::SUB) 
    {
        _ActionExecute(wch);
        _EnterGround();
    }
    else if (s_IsEscape(wch))
    {
        _EnterEscape();
    }
    else
    {
        // Then pass to the current state as an event
        switch (_state)
        {
        case VTStates::Ground:
            return _EventGround(wch);
        case VTStates::Escape:
            return _EventEscape(wch);
        case VTStates::EscapeIntermediate:
            return _EventEscapeIntermediate(wch);
        case VTStates::CsiEntry:
            return _EventCsiEntry(wch);
        case VTStates::CsiIntermediate:
            return _EventCsiIntermediate(wch);
        case VTStates::CsiIgnore:
            return _EventCsiIgnore(wch);
        case VTStates::CsiParam:
            return _EventCsiParam(wch);
        case VTStates::OscParam:
            return _EventOscParam(wch);
        case VTStates::OscString:
            return _EventOscString(wch);
        default:
            //assert(false);
            return;
        }
    }
}

// Routine Description:
// - Helper for entry to the state machine. Will take an array of characters 
//     and print as many as it can without encountering a character indicating 
//     a escape sequence, then feed characters into the state machine one at a
//     time until we return to the ground state.
// Arguments:
// - rgwch - Array of new characters to operate upon
// - cch - Count of characters in array
// Return Value:
// - <none>
void StateMachine::ProcessString(_In_reads_(cch) wchar_t * const rgwch, _In_ size_t const cch)
{
    wchar_t* pwchCurr = rgwch;
    wchar_t* pwchStart = rgwch;
    size_t currRunLength = 0;

    // This should be static, because if one string starts a sequence, and the next finishes it,
    //   we want the partial sequence state to persist.
    static bool s_fProcessIndividually = false;

    for(size_t cchCharsRemaining = cch; cchCharsRemaining > 0; cchCharsRemaining--)
    {
        if (s_fProcessIndividually)
        {
            ProcessCharacter(*pwchCurr); // If we're processing characters individually, send it to the state machine.
            pwchCurr++;
            if (_state == VTStates::Ground)  // Then check if we're back at ground. If we are, the next character (pwchCurr)
            {                                //   is the start of the next run of characters that might be printable.
                s_fProcessIndividually = false;
                pwchStart = pwchCurr;
                currRunLength = 0;
            }
        }
        else
        {
            if (s_IsActionableFromGround(*pwchCurr))  // If the current char is the start of an escape sequence, or should be executed in ground state...
            {
                _pDispatch->PrintString(pwchStart, currRunLength); // ... print all the chars leading up to it as part of the run...
                _trace.DispatchPrintRunTrace(pwchStart, currRunLength);
                s_fProcessIndividually = true; // begin processing future characters individually... 
                currRunLength = 0;
                ProcessCharacter(*pwchCurr); // ... Then process the character individually.
                if (_state == VTStates::Ground)  // If the character took us right back to ground, start another run after it.
                {                                
                    s_fProcessIndividually = false;
                    pwchStart = pwchCurr + 1; // pwchCurr still points at the current that went to the state machine. The run starts after it. 
                    currRunLength = 0;
                }
            }
            else
            {
                currRunLength++; // Otherwise, add this char to the current run to be printed.
            }
            pwchCurr++;
        }  
    }

    if (!s_fProcessIndividually && currRunLength > 0) // If we're at the end of the string and have remaining un-printed characters, 
    {
        // print the rest of the characters in the string
        _pDispatch->PrintString(pwchStart, currRunLength);
        _trace.DispatchPrintRunTrace(pwchStart, currRunLength);

    }
}

// Routine Description:
// - Wherever the state machine is, whatever it's going, go back to ground.
//     This is used by conhost to "jiggle the handle" - when VT support is 
//     turned off, we don't want any bad state left over for the next input it's turned on for
// Arguments:
// - <none>
// Return Value:
// - <none>
void StateMachine::ResetState()
{
    _EnterGround();
}
