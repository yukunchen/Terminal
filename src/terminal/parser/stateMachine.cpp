/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "stateMachine.hpp"

#include "ascii.hpp"

using namespace Microsoft::Console::VirtualTerminal;

//Takes ownership of the pEngine.
StateMachine::StateMachine(_In_ IStateMachineEngine* const pEngine) :
    _pEngine(THROW_IF_NULL_ALLOC(pEngine)),
    _state(VTStates::Ground),
    _trace(Microsoft::Console::VirtualTerminal::ParserTracing())
{
    _ActionClear();
}

StateMachine::~StateMachine()
{
    if (_pEngine != nullptr)
    {
        delete _pEngine;
    }
}

// Routine Description:
// - Determines if a character indicates an action that should be taken in the ground state - 
//     These are C0 characters and the C1 [single-character] CSI.
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsActionableFromGround(_In_ wchar_t const wch)
{
    return (wch <= AsciiChars::US) || s_IsC1Csi(wch) || s_IsDelete(wch);
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
// - Determines if a character is a C1 CSI (Control Sequence Introducer)
//   This is a single-character way to start a control sequence, as opposed to "ESC[".
//
//   Not all single-byte codepages support C1 control codes--in some, the range that would
//   be used for C1 codes are instead used for additional graphic characters.
//
//   However, we do not need to worry about confusion whether a single byte \x9b in a
//   single-byte stream represents a C1 CSI or some other glyph, because by the time we
//   get here, everything is Unicode. Knowing whether a single-byte \x9b represents a
//   single-character C1 CSI or some other glyph is handled by MultiByteToWideChar before
//   we get here (if the stream was not already UTF-16). For instance, in CP_ACP, if a
//   \x9b shows up, it will get converted to \x203a. So, if we get here, and have a
//   \x009b, we know that it unambiguously represents a C1 CSI.
//
// Arguments:
// - wch - Character to check.
// Return Value:
// - True if it is. False if it isn't.
bool StateMachine::s_IsC1Csi(_In_ wchar_t const wch)
{
    return wch == L'\x9b';
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
    _pEngine->ActionExecute(wch);

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
    _pEngine->ActionPrint(wch);
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
    
    bool fSuccess = _pEngine->ActionEscDispatch(wch, _cIntermediate, _wchIntermediate);
    
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
    
    bool fSuccess = _pEngine->ActionCsiDispatch(wch, _cIntermediate, _wchIntermediate, _rgusParams, _cParams);

    // Trace the result.
    _trace.DispatchSequenceTrace(fSuccess);

    if (!fSuccess)
    {
        // Suppress it and log telemetry on failed cases
        TermTelemetry::Instance().LogFailed(wch);
    }
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

    _pEngine->ActionClear();

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
    
    bool fSuccess = _pEngine->ActionOscDispatch(wch, _sOscParam, _pwchOscStringBuffer, _sOscNextChar);

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
//   2. Handle a C1 Control Sequence Introducer
//   3. Print all other characters
// Arguments:
// - wch - Character that triggered the event
// Return Value:
// - <none>
void StateMachine::_EventGround(_In_ wchar_t const wch)
{
    _trace.TraceOnEvent(L"Ground");
    if (s_IsC0Code(wch) || s_IsDelete(wch))
    {
        _ActionExecute(wch);
    }
    else if (s_IsC1Csi(wch))
    {
        _EnterCsiEntry();
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
    wchar_t* pwchSequenceStart = rgwch;
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
                pwchSequenceStart = pwchCurr;
                currRunLength = 0;
            }
        }
        else
        {
            if (s_IsActionableFromGround(*pwchCurr))  // If the current char is the start of an escape sequence, or should be executed in ground state...
            {
                _pEngine->ActionPrintString(pwchStart, currRunLength); // ... print all the chars leading up to it as part of the run...
                _trace.DispatchPrintRunTrace(pwchStart, currRunLength);
                s_fProcessIndividually = true; // begin processing future characters individually... 
                currRunLength = 0;
                pwchSequenceStart = pwchCurr;
                ProcessCharacter(*pwchCurr); // ... Then process the character individually.
                if (_state == VTStates::Ground)  // If the character took us right back to ground, start another run after it.
                {                                
                    s_fProcessIndividually = false;
                    pwchStart = pwchCurr + 1; // pwchCurr still points at the current that went to the state machine. The run starts after it. 
                    pwchSequenceStart = pwchCurr+1;
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

    // If we're at the end of the string and have remaining un-printed characters, 
    if (!s_fProcessIndividually && currRunLength > 0) 
    {
        // print the rest of the characters in the string
        _pEngine->ActionPrintString(pwchStart, currRunLength);
        _trace.DispatchPrintRunTrace(pwchStart, currRunLength);

    }
    else if (s_fProcessIndividually)
    {
        if (_pEngine->FlushAtEndOfString())
        {
            // Reset our state, and put all but the last char in again. 
            ResetState();
            // Chars to flush are [pwchSequenceStart, pwchCurr)
            wchar_t* pwch = pwchSequenceStart;
            for (pwch = pwchSequenceStart; pwch < pwchCurr-1; pwch++)
            {
                ProcessCharacter(*pwch);
            }
            // Manually execute the last char [pwchCurr]
            switch (_state)
            {
            case VTStates::Ground:
                return _ActionExecute(*pwch);
            case VTStates::Escape:
            case VTStates::EscapeIntermediate:
                return _ActionEscDispatch(*pwch);
            case VTStates::CsiEntry:
            case VTStates::CsiIntermediate:
            case VTStates::CsiIgnore:
            case VTStates::CsiParam:
                return _ActionCsiDispatch(*pwch);
            case VTStates::OscParam:
            case VTStates::OscString:
                return _ActionOscDispatch(*pwch);
            default:
                //assert(false);
                return;
            }

        }
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
