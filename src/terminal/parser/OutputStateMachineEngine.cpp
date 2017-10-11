/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "stateMachine.hpp"
#include "OutputStateMachineEngine.hpp"

#include "ascii.hpp"
using namespace Microsoft::Console::VirtualTerminal;


OutputStateMachineEngine::OutputStateMachineEngine(_In_ TermDispatch* const pDispatch) :
    _pDispatch(pDispatch)
{
}

OutputStateMachineEngine::~OutputStateMachineEngine()
{

}

// Routine Description:
// - Triggers the Execute action to indicate that the listener should 
//      immediately respond to a C0 control character.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - true iff we successfully dispatched the sequence.
bool OutputStateMachineEngine::ActionExecute(_In_ wchar_t const wch)
{
    _pDispatch->Execute(wch);
    return true;
}

// Routine Description:
// - Triggers the Print action to indicate that the listener should render the
//      character given.
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - true iff we successfully dispatched the sequence.
bool OutputStateMachineEngine::ActionPrint(_In_ wchar_t const wch)
{
    _pDispatch->Print(wch); // call print
    return true;
}

// Routine Description:
// - Triggers the Print action to indicate that the listener should render the 
//      string of characters given.
// Arguments:
// - rgwch - string to dispatch.
// - cch - length of rgwch
// Return Value:
// - true iff we successfully dispatched the sequence.
bool OutputStateMachineEngine::ActionPrintString(_Inout_updates_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    _pDispatch->PrintString(rgwch, cch); // call print
    return true;
}

// Routine Description:
// - Triggers the EscDispatch action to indicate that the listener should handle
//      a simple escape sequence. These sequences traditionally start with ESC 
//      and a simple letter. No complicated parameters.
// Arguments:
// - wch - Character to dispatch.
// - cIntermediate - Number of "Intermediate" characters found - such as '!', '?'
// - wchIntermediate - Intermediate character in the sequence, if there was one.
// Return Value:
// - true iff we successfully dispatched the sequence.
bool OutputStateMachineEngine::ActionEscDispatch(_In_ wchar_t const wch, 
                                                 _In_ const unsigned short cIntermediate,
                                                 _In_ const wchar_t wchIntermediate)
{
    bool fSuccess = false;

    // no intermediates.
    if (cIntermediate == 0)
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
        case VTActionCodes::RIS_ResetToInitialState:
            fSuccess = _pDispatch->HardReset();
            TermTelemetry::Instance().Log(TermTelemetry::Codes::RIS);
            break;
        default:
            // If no functions to call, overall dispatch was a failure.
            fSuccess = false;
            break;
        }
    }
    else if (cIntermediate == 1)
    {
        DesignateCharsetTypes designateType = s_DefaultDesignateCharsetType;
        fSuccess = _GetDesignateType(wchIntermediate, &designateType);
        if (fSuccess)
        {
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
            default:
                // If no functions to call, overall dispatch was a failure.
                fSuccess = false;
                break;
            }
        }
    }
    return fSuccess;
}

// Routine Description:
// - Triggers the CsiDispatch action to indicate that the listener should handle
//      a control sequence. These sequences perform various API-type commands 
//      that can include many parameters.
// Arguments:
// - wch - Character to dispatch.
// - cIntermediate - Number of "Intermediate" characters found - such as '!', '?'
// - wchIntermediate - Intermediate character in the sequence, if there was one.
// - rgusParams - set of numeric parameters collected while pasring the sequence.
// - cParams - number of parameters found.
// Return Value:
// - true iff we successfully dispatched the sequence.
bool OutputStateMachineEngine::ActionCsiDispatch(_In_ wchar_t const wch, 
                                                 _In_ const unsigned short cIntermediate,
                                                 _In_ const wchar_t wchIntermediate,
                                                 _In_ const unsigned short* const rgusParams,
                                                 _In_ const unsigned short cParams)
{
    bool fSuccess = false;
    unsigned int uiDistance = 0;
    unsigned int uiLine = 0;
    unsigned int uiColumn = 0;
    SHORT sTopMargin = 0;
    SHORT sBottomMargin = 0;
    SHORT sNumTabs = 0;
    SHORT sClearType = 0;
    unsigned int uiFunction = 0;
    TermDispatch::EraseType eraseType = TermDispatch::EraseType::ToEnd;
    TermDispatch::GraphicsOptions rgGraphicsOptions[StateMachine::s_cParamsMax];
    size_t cOptions = ARRAYSIZE(rgGraphicsOptions);
    TermDispatch::AnsiStatusType deviceStatusType = (TermDispatch::AnsiStatusType)-1; // there is no default status type.
    
    if (cIntermediate == 0)
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
            fSuccess = _GetCursorDistance(rgusParams, cParams, &uiDistance);
            break;
        case VTActionCodes::HVP_HorizontalVerticalPosition:
        case VTActionCodes::CUP_CursorPosition:
            fSuccess = _GetXYPosition(rgusParams, cParams, &uiLine, &uiColumn);
            break;
        case VTActionCodes::DECSTBM_SetScrollingRegion:
            fSuccess = _GetTopBottomMargins(rgusParams, cParams, &sTopMargin, &sBottomMargin);
            break;
        case VTActionCodes::ED_EraseDisplay:
        case VTActionCodes::EL_EraseLine:
            fSuccess = _GetEraseOperation(rgusParams, cParams, &eraseType);
            break;
        case VTActionCodes::SGR_SetGraphicsRendition:
            fSuccess = _GetGraphicsOptions(rgusParams, cParams, rgGraphicsOptions, &cOptions);
            break;
        case VTActionCodes::DSR_DeviceStatusReport:
            fSuccess = _GetDeviceStatusOperation(rgusParams, cParams, &deviceStatusType);
            break;
        case VTActionCodes::DA_DeviceAttributes:
            fSuccess = _VerifyDeviceAttributesParams(rgusParams, cParams);
            break;
        case VTActionCodes::SU_ScrollUp:
        case VTActionCodes::SD_ScrollDown:
            fSuccess = _GetScrollDistance(rgusParams, cParams, &uiDistance);
            break;
        case VTActionCodes::ANSISYSSC_CursorSave:
        case VTActionCodes::ANSISYSRC_CursorRestore:
            fSuccess = _VerifyHasNoParameters(cParams);
            break;
        case VTActionCodes::IL_InsertLine:
        case VTActionCodes::DL_DeleteLine:
            fSuccess = _GetScrollDistance(rgusParams, cParams, &uiDistance);
            break;
        case VTActionCodes::CHT_CursorForwardTab:
        case VTActionCodes::CBT_CursorBackTab:
            fSuccess = _GetTabDistance(rgusParams, cParams, &sNumTabs);
            break;
        case VTActionCodes::TBC_TabClear:
            fSuccess = _GetTabClearType(rgusParams, cParams, &sClearType);
            break;
        case VTActionCodes::DTTERM_WindowManipulation:
            fSuccess = _GetWindowManipulationFunction(rgusParams, cParams, &uiFunction);
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
            case VTActionCodes::DTTERM_WindowManipulation:
                fSuccess = _pDispatch->WindowManipulation(static_cast<TermDispatch::WindowManipulationFunction>(uiFunction), rgusParams+1, cParams-1);
                TermTelemetry::Instance().Log(TermTelemetry::Codes::DTTERM_WM);
                break;
            default:
                // If no functions to call, overall dispatch was a failure.
                fSuccess = false;
                break;
            }
        }
    }
    else if (cIntermediate == 1)
    {
        switch (wchIntermediate)
        {
        case L'?':
            fSuccess = _IntermediateQuestionMarkDispatch(wch, rgusParams, cParams);
            break;
        case L'!':
            fSuccess = _IntermediateExclamationDispatch(wch);
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
// - Handles actions that have postfix params on an intermediate '?', such as DECTCEM, DECCOLM, ATT610
// Arguments:
// - wch - Character to dispatch.
// Return Value:
// - True if handled successfully. False otherwise.
bool OutputStateMachineEngine::_IntermediateQuestionMarkDispatch(_In_ wchar_t const wchAction, _In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams)
{
    bool fSuccess = false;

    TermDispatch::PrivateModeParams rgPrivateModeParams[StateMachine::s_cParamsMax];
    size_t cOptions = ARRAYSIZE(rgPrivateModeParams);
    // Ensure that there was the right number of params
    switch (wchAction)
    {
        case VTActionCodes::DECSET_PrivateModeSet:
        case VTActionCodes::DECRST_PrivateModeReset:
            fSuccess = _GetPrivateModeParams(rgusParams, cParams, rgPrivateModeParams, &cOptions);
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
bool OutputStateMachineEngine::_IntermediateExclamationDispatch(_In_ wchar_t const wchAction)
{
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
// - Triggers the Clear action to indicate that the state machine should erase 
//      all internal state.
// Arguments:
// - <none>
// Return Value:
// - <none>
bool OutputStateMachineEngine::ActionClear()
{
    // do nothing.
    return true;
}

// Routine Description:
// - Triggers the Ignore action to indicate that the state machine should eat 
//      this character and say nothing.
// Arguments:
// - <none>
// Return Value:
// - <none>
bool OutputStateMachineEngine::ActionIgnore()
{
    // do nothing.
    return true;
}

// Routine Description:
// - Triggers the OscDispatch action to indicate that the listener should handle a control sequence.
//   These sequences perform various API-type commands that can include many parameters.
// Arguments:
// - wch - Character to dispatch. This will be a BEL or ST char.
// - sOscParam - identifier of the OSC action to perform
// - pwchOscStringBuffer - OSC string we've collected. NOT null terminated.
// - cchOscString - length of pwchOscStringBuffer
// Return Value:
// - true if we handled the dsipatch.
bool OutputStateMachineEngine::ActionOscDispatch(_In_ wchar_t const wch, _In_ const unsigned short sOscParam, _Inout_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString)
{
    // The wch here is just the string terminator for the OSC string
    UNREFERENCED_PARAMETER(wch);
    bool fSuccess = false;
    wchar_t* pwchTitle = nullptr;  
    unsigned short sCchTitleLength = 0;

    switch (sOscParam)
    {
    case OscActionCodes::SetIconAndWindowTitle:
    case OscActionCodes::SetWindowIcon:
    case OscActionCodes::SetWindowTitle:
        fSuccess = _GetOscTitle(pwchOscStringBuffer, cchOscString, &pwchTitle, &sCchTitleLength);
        break;
    default:
        // If no functions to call, overall dispatch was a failure.
        fSuccess = false;
        break;
    }  
    if (fSuccess)
    {
        switch (sOscParam)
        {
        case OscActionCodes::SetIconAndWindowTitle:
        case OscActionCodes::SetWindowIcon:
        case OscActionCodes::SetWindowTitle:
            fSuccess = _pDispatch->SetWindowTitle(pwchTitle, sCchTitleLength);
            TermTelemetry::Instance().Log(TermTelemetry::Codes::OSCWT);
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
// - Retrieves the listed graphics options to be applied in order to the "font style" of the next characters inserted into the buffer.
// Arguments:
// - rgGraphicsOptions - Pointer to array space (expected 16 max, the max number of params this can generate) that will be filled with valid options from the GraphicsOptions enum
// - pcOptions - Pointer to the length of rgGraphicsOptions on the way in, and the count of the array used on the way out.
// Return Value:
// - True if we successfully retrieved an array of valid graphics options from the parameters we've stored. False otherwise.
_Success_(return)
bool OutputStateMachineEngine::_GetGraphicsOptions(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_writes_(*pcOptions) TermDispatch::GraphicsOptions* rgGraphicsOptions, _Inout_ size_t* pcOptions) const
{
    bool fSuccess = false;

    if (cParams == 0)
    {
        if (*pcOptions >= 1)
        {
            rgGraphicsOptions[0] = s_defaultGraphicsOption;
            *pcOptions = 1;
            fSuccess = true;
        }
        else
        {
            fSuccess = false; // not enough space in buffer to hold response.
        }
    }
    else
    {
        if (*pcOptions >= cParams)
        {
            for (size_t i = 0; i < cParams; i++)
            {
                // No memcpy. The parameters are shorts. The graphics options are unsigned ints.
                rgGraphicsOptions[i] = (TermDispatch::GraphicsOptions)rgusParams[i];
            }

            *pcOptions = cParams;
            fSuccess = true;
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
bool OutputStateMachineEngine::_GetEraseOperation(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ TermDispatch::EraseType* const pEraseType) const
{
    bool fSuccess = false; // If we have too many parameters or don't know what to do with the given value, return false.
    *pEraseType = s_defaultEraseType; // if we fail, just put the default type in.

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        *pEraseType = s_defaultEraseType;
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, attempt to match it to the values we accept.
        unsigned short const usParam = rgusParams[0];

        switch (usParam)
        {
        case TermDispatch::EraseType::ToEnd:
        case TermDispatch::EraseType::FromBeginning:
        case TermDispatch::EraseType::All:
        case TermDispatch::EraseType::Scrollback:
            *pEraseType = (TermDispatch::EraseType) usParam;
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
bool OutputStateMachineEngine::_GetCursorDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiDistance) const
{
    bool fSuccess = false;
    *puiDistance = s_uiDefaultCursorDistance;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, use it.
        *puiDistance = rgusParams[0];
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
bool OutputStateMachineEngine::_GetScrollDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiDistance) const
{
    bool fSuccess = false;
    *puiDistance = s_uiDefaultScrollDistance;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, use it.
        *puiDistance = rgusParams[0];
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
bool OutputStateMachineEngine::_GetConsoleWidth(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiConsoleWidth) const
{
    bool fSuccess = false;
    *puiConsoleWidth = s_uiDefaultConsoleWidth;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, use it.
        *puiConsoleWidth = rgusParams[0];
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
bool OutputStateMachineEngine::_GetXYPosition(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ unsigned int* const puiLine, _Out_ unsigned int* const puiColumn) const
{
    bool fSuccess = false;
    *puiLine = s_uiDefaultLine;
    *puiColumn = s_uiDefaultColumn;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's only one param, leave the default for the column, and retrieve the specified row.
        *puiLine = rgusParams[0];
        fSuccess = true;
    }
    else if (cParams == 2)
    {
        // If there are exactly two parameters, use them.
        *puiLine = rgusParams[0];
        *puiColumn = rgusParams[1];
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
bool OutputStateMachineEngine::_GetTopBottomMargins(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psTopMargin, _Out_ SHORT* const psBottomMargin) const
{
    // Notes:                           (input -> state machine out -> adapter out -> conhost internal)
    // having only a top param is legal         ([3;r   -> 3,0   -> 3,h  -> 3,h,true)
    // having only a bottom param is legal      ([;3r   -> 0,3   -> 1,3  -> 1,3,true)
    // having neither uses the defaults         ([;r [r -> 0,0   -> 3,h  -> 0,0,false)
    // an illegal combo (eg, 3;2r) is ignored

    bool fSuccess = false;
    *psTopMargin = s_sDefaultTopMargin;
    *psBottomMargin = s_sDefaultBottomMargin;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        *psTopMargin = rgusParams[0];
        fSuccess = true;
    }
    else if (cParams == 2)
    {
        // If there are exactly two parameters, use them.
        *psTopMargin = rgusParams[0];
        *psBottomMargin = rgusParams[1];
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
bool OutputStateMachineEngine::_GetDeviceStatusOperation(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ TermDispatch::AnsiStatusType* const pStatusType) const
{
    bool fSuccess = false;
    *pStatusType = (TermDispatch::AnsiStatusType)0;

    if (cParams == 1)
    {
        // If there's one parameter, attempt to match it to the values we accept.
        unsigned short const usParam = rgusParams[0];

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
bool OutputStateMachineEngine::_GetPrivateModeParams(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_writes_(*pcParams) TermDispatch::PrivateModeParams* rgPrivateModeParams, _Inout_ size_t* pcParams) const
{
    bool fSuccess = false;
    // Can't just set nothing at all
    if (cParams > 0)
    {
        if (*pcParams >= cParams)
        {
            for (size_t i = 0; i < cParams; i++)
            {
                // No memcpy. The parameters are shorts. The graphics options are unsigned ints.
                rgPrivateModeParams[i] = (TermDispatch::PrivateModeParams)rgusParams[i];
            }
            *pcParams = cParams;
            fSuccess = true;
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
bool OutputStateMachineEngine::_VerifyHasNoParameters(_In_ const unsigned short cParams) const
{
    return cParams == 0;
}

// Routine Description:
// - Validates that we received the correct parameter sequence for the Device Attributes command.
// - For DA, we should have received either NO parameters or just one 0 parameter. Anything else is not acceptable.
// Arguments:
// - <none>
// Return Value:
// - True if the DA params were valid. False otherwise.
_Success_(return)
bool OutputStateMachineEngine::_VerifyDeviceAttributesParams(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams) const
{
    bool fSuccess = false;

    if (cParams == 0)
    {
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        if (rgusParams[0] == 0)
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
bool OutputStateMachineEngine::_GetOscTitle(_Inout_ wchar_t* const pwchOscStringBuffer, _In_ const unsigned short cchOscString, _Outptr_result_buffer_(*pcchTitle) wchar_t** const ppwchTitle, _Out_ unsigned short * pcchTitle)
{
    *ppwchTitle = pwchOscStringBuffer;
    *pcchTitle = cchOscString;
    if (pwchOscStringBuffer != nullptr)
    {
        // null terminate the string on the current char
        pwchOscStringBuffer[cchOscString] = L'\x0';
    }
    return pwchOscStringBuffer != nullptr;
}

// Routine Description:
// - Retrieves a distance for a tab operation from the parameter pool stored during Param actions.
// Arguments:
// - psDistance - Memory location to receive the distance
// Return Value:
// - True if we successfully pulled the tab distance from the parameters we've stored. False otherwise.
_Success_(return)
bool OutputStateMachineEngine::_GetTabDistance(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psDistance) const
{
    bool fSuccess = false;
    *psDistance = s_sDefaultTabDistance;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, use it.
        *psDistance = rgusParams[0];
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
bool OutputStateMachineEngine::_GetTabClearType(_In_reads_(cParams) const unsigned short* const rgusParams, _In_ const unsigned short cParams, _Out_ SHORT* const psClearType) const
{
    bool fSuccess = false;
    *psClearType = s_sDefaultTabClearType;

    if (cParams == 0)
    {
        // Empty parameter sequences should use the default
        fSuccess = true;
    }
    else if (cParams == 1)
    {
        // If there's one parameter, use it.
        *psClearType = rgusParams[0];
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
bool OutputStateMachineEngine::_GetDesignateType(_In_ const wchar_t wchIntermediate, _Out_ DesignateCharsetTypes* const pDesignateType) const
{
    bool fSuccess = false;
    *pDesignateType = s_DefaultDesignateCharsetType;

    switch(wchIntermediate)
    {
    case '(':
        *pDesignateType = DesignateCharsetTypes::G0;
        fSuccess = true;
        break;
    case ')':
    case '-':
        *pDesignateType = DesignateCharsetTypes::G1;
        fSuccess = true;
        break;
    case '*':
    case '.':
        *pDesignateType = DesignateCharsetTypes::G2;
        fSuccess = true;
        break;
    case '+':
    case '/':
        *pDesignateType = DesignateCharsetTypes::G3;
        fSuccess = true;
        break;
    }

    return fSuccess;
}

// Routine Description:
// - Returns true if the engine should dispatch on the last charater of a string 
//      always, even if the sequence hasn't normally dispatched.
//   If this is false, the engine will persist it's state across calls to 
//      ProcessString, and dispatch only at the end of the sequence.
// Return Value:
// - True iff we should manually dispatch on the last character of a string.
bool OutputStateMachineEngine::FlushAtEndOfString() const
{
    return false;
}

// Method Description:
// - Retrieves the type of window manipulation operation from the parameter pool
//      stored during Param actions.
// Arguments:
// - rgusParams - Array of parameters collected
// - cParams - Number of parameters we've collected
// - puiFunction - Memory location to receive the function type
// Return Value:
// - True iff we successfully pulled the function type from the parameters
bool OutputStateMachineEngine::_GetWindowManipulationFunction(_In_reads_(cParams) const unsigned short* const rgusParams,
                                                              _In_ const unsigned short cParams,
                                                              _Out_ unsigned int* const puiFunction) const
{
    bool fSuccess = false;
    *puiFunction = s_DefaulWindowManipulationFunction;

    if (cParams > 0)
    {
        switch(rgusParams[0])
        {
            case TermDispatch::WindowManipulationFunction::ResizeWindowInCharacters:
                *puiFunction = TermDispatch::WindowManipulationFunction::ResizeWindowInCharacters;
                fSuccess = true;
                break;
        }
    }

    return fSuccess;
}
