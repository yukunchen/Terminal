/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <windows.h>
#include "MouseInput.hpp"

#include "strsafe.h"
#include <stdio.h>
#include <stdlib.h>
using namespace Microsoft::Console::VirtualTerminal;


MouseInput::MouseInput(_In_ WriteInputEvents const pfnWriteEvents) :
    _pfnWriteEvents(pfnWriteEvents),
    _fEnabled(false)
{

}

MouseInput::~MouseInput()
{

}

// Routine Description:
// - Determines if the input windows message code describes a button event
//     (left, middle, right button and any of up, down or double click)
// Parameters:
// - uiButton - the message to decode.
// Return value:
// - true iff uiButton is in fact a button.
bool MouseInput::s_IsButtonMsg(_In_ const unsigned int uiButton)
{
    bool fIsButton = false;
    switch (uiButton)
    {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            fIsButton = true;
            break;
    }
    return fIsButton;
}

// Routine Description:
// - Determines if the input windows message code describes a button press 
//     (either down or doubleclick)
// Parameters:
// - uiButton - the message to decode.
// Return value:
// - true iff uiButton is a button down event
bool MouseInput::s_IsButtonDown(_In_ const unsigned int uiButton)
{
    bool fIsButtonDown = false;
    switch (uiButton)
    {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            fIsButtonDown = true;
            break;
    }
    return fIsButtonDown;
}

// Routine Description:
// - translates the input windows mouse message into it's equivalent X11 encoding.
// X Button Encoding:
// |7|6|5|4|3|2|1|0|
// | |W|H|M|C|S|B|B|
//  bits 0 and 1 are used for button:
//      00 - MB1 pressed (left)
//      01 - MB2 pressed (middle)
//      10 - MB3 pressed (right)
//      11 - released (none)
//  Next three bits indicate modifier keys: (These are ignored for now, MSFT:8329546)
//      0x04 - shift (This never makes it through, as our emulator is skipped when shift is pressed.)
//      0x08 - ctrl
//      0x10 - meta
//  32 (x20) is added for "hover" events: (These are ignored for now, MSFT:8329546)
//     "For example, motion into cell x,y with button 1 down is reported as `CSI M @ CxCy`.
//          ( @  = 32 + 0 (button 1) + 32 (motion indicator) ).
//      Similarly, motion with button 3 down is reported as `CSI M B CxCy`.  
//          ( B  = 32 + 2 (button 3) + 32 (motion indicator) ).
//  64 (x40) is added for wheel events.  (These are ignored for now, MSFT:8329546)
//      so wheel up? is 64, and wheel down? is 65. 
//
// Parameters:
// - uiButton - the message to decode.
// Return value:
// - the int representing the equivalent X button encoding.
int MouseInput::s_WindowsButtonToXEncoding(_In_ const unsigned int uiButton)
{
    int iXValue = 0;
    switch (uiButton)
    {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            iXValue = 0;
            break;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            iXValue = 3;
            break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            iXValue = 2;
            break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            iXValue = 1;
            break;
        // These will be added, MSFT:8329546
        // case WM_MOUSEMOVE:
        // case WM_MOUSEWHEEL:
        // case WM_MOUSEHWHEEL:
        // default:
        //     break;
    }
    return iXValue;
}

// Routine Description:
// - Attempt to handle the given mouse coordinates and windows button as a VT-style mouse event.
//     If the event should be transmitted in the selected mouse mode, then we'll try and 
//     encode the event according to the rules of the selected ExtendedMode, and insert those characters into the input buffer. 
// Parameters:
// - coordMousePosition - The windows coordinates (top,left = 0,0) of the mouse event
// - uiButton - the message to decode.
// Return value:
// - true if the event was handled and we should stop event propagation to the default window handler.
bool MouseInput::HandleMouse(_In_ const COORD coordMousePosition, _In_ const unsigned int uiButton) const
{
    bool fSuccess = _fEnabled && s_IsButtonMsg(uiButton);
    if (fSuccess)
    {
        wchar_t* pwchSequence = nullptr;
        size_t cchSequenceLength = 0;
        switch (_ExtendedMode)
        {
            case ExtendedMode::None:
                fSuccess = _GenerateDefaultSequence(coordMousePosition, uiButton, &pwchSequence, &cchSequenceLength);
                break;
            case ExtendedMode::Utf8:
                fSuccess = _GenerateUtf8Sequence(coordMousePosition, uiButton, &pwchSequence, &cchSequenceLength);
                break;
            case ExtendedMode::Sgr:
                fSuccess = _GenerateSGRSequence(coordMousePosition, uiButton, &pwchSequence, &cchSequenceLength);
                break;
            case ExtendedMode::Urxvt:
            default:
                fSuccess = false;
                break;
        }
        if (fSuccess)
        {
            _SendInputSequence(pwchSequence, cchSequenceLength);
            delete[] pwchSequence;
            fSuccess = true;
        }
    }
    return fSuccess;

}

// Routine Description:
// - Generates a sequence encoding the mouse event according to the default scheme.
//     see http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
// Parameters:
// - coordMousePosition - The windows coordinates (top,left = 0,0) of the mouse event
// - uiButton - the message to decode.
// - ppwchSequence - On success, where to put the pointer to the generated sequence
// - pcchLength - On success, where to put the length of the generated sequence
// Return value:
// - true if we were able to successfully generate a sequence.
// On success, caller is responsible for delete[]ing *ppwchSequence.
bool MouseInput::_GenerateDefaultSequence(_In_ const COORD coordMousePosition,
                                          _In_ const unsigned int uiButton,
                                          _Out_ wchar_t** const ppwchSequence,
                                          _Out_ size_t* const pcchLength) const
{
    bool fSuccess = false;

    // In the default, non-extended encoding scheme, coordinates above 94 shouldn't be supported,
    //   because (95+32+1)=128, which is not an ASCII character.
    // There are more details in _GenerateUtf8Sequence, but basically, we can't put anything above x80 into the input
    //   stream without bash.exe trying to convert it into utf8, and generating extra bytes in the process.
    if (coordMousePosition.X <= MouseInput::s_MaxDefaultCoordinate && coordMousePosition.Y <= MouseInput::s_MaxDefaultCoordinate)
    {
        const COORD coordVTCoords = s_WinToVTCoord(coordMousePosition);
        const short sEncodedX = s_EncodeDefaultCoordinate(coordVTCoords.X);
        const short sEncodedY = s_EncodeDefaultCoordinate(coordVTCoords.Y);
		wchar_t* pwchFormat = new wchar_t[7]{ L"\x1b[Mbxy" };
        if (pwchFormat != nullptr)
        {
            pwchFormat[3] = ' ' + (short)s_WindowsButtonToXEncoding(uiButton);
            pwchFormat[4] = sEncodedX;
            pwchFormat[5] = sEncodedY;

            *ppwchSequence = pwchFormat;
            *pcchLength = 7;
            fSuccess = true;
        }
    }

    return fSuccess;
}

// Routine Description:
// - Generates a sequence encoding the mouse event according to the UTF8 Extended scheme.
//     see http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Extended-coordinates
// Parameters:
// - coordMousePosition - The windows coordinates (top,left = 0,0) of the mouse event
// - uiButton - the message to decode.
// - ppwchSequence - On success, where to put the pointer to the generated sequence
// - pcchLength - On success, where to put the length of the generated sequence
// Return value:
// - true if we were able to successfully generate a sequence.
// On success, caller is responsible for delete[]ing *ppwchSequence.
bool MouseInput::_GenerateUtf8Sequence(_In_ const COORD coordMousePosition,
                                       _In_ const unsigned int uiButton,
                                       _Out_ wchar_t** const ppwchSequence,
                                       _Out_ size_t* const pcchLength) const
{
    bool fSuccess = false;

    // So we have some complications here.
    // The windows input stream is typically encoded as UTF16.
    // Bash.exe knows this, and converts the utf16 input, character by character, into utf8, to send to wsl.
    // So, if we want to emit a char > x80 here, great. bash.exe will convert the x80 into xC280 and pass that along, which is great.
    //   The *nix application was expecting a utf8 stream, and it got one.
    // However, a normal windows program asks for utf8 mode, then it gets the utf16 encoded result. This is not what it wanted.
    //   It was looking for \x1b[M#\xC280y and got \x1b[M#\x0080y
    // Now, I'd argue that in requesting utf8 mode, the application should be enlightened enough to not want the utf16 input stream,
    //   and convert it the same way bash.exe does. 
    // Though, the point could be made to place the utf8 bytes into the input, and read them that way.
    //   However, if we did this, bash.exe would translate those bytes thinking they're utf16, and x80->xC280->xC382C280
    //   So bash would also need to change, but how could it tell the difference between them? no real good way.
    // I'm going to emit a utf16 encoded value for now. Besides, if a windows program really wants it, just use the SGR mode, which is unambiguous.
    // TODO: Followup once the UTF-8 input stack is ready, MSFT:8509613
    if (coordMousePosition.X <= (SHORT_MAX - 33) &&  coordMousePosition.Y <= (SHORT_MAX - 33))
    {
        const COORD coordVTCoords = s_WinToVTCoord(coordMousePosition);
        const short sEncodedX = s_EncodeDefaultCoordinate(coordVTCoords.X);
        const short sEncodedY = s_EncodeDefaultCoordinate(coordVTCoords.Y);
		wchar_t* pwchFormat = new wchar_t[7]{ L"\x1b[Mbxy" };
        if (pwchFormat != nullptr)
        {
            pwchFormat[3] = ' ' + (short)s_WindowsButtonToXEncoding(uiButton);
            pwchFormat[4] = sEncodedX;
            pwchFormat[5] = sEncodedY;

            *ppwchSequence = pwchFormat;
            *pcchLength = 7;
            fSuccess = true;
        }
    }

    return fSuccess;
}

// Routine Description:
// - Generates a sequence encoding the mouse event according to the SGR Extended scheme.
//     see http://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Extended-coordinates
// Parameters:
// - coordMousePosition - The windows coordinates (top,left = 0,0) of the mouse event
// - uiButton - the message to decode.
// - ppwchSequence - On success, where to put the pointer to the generated sequence
// - pcchLength - On success, where to put the length of the generated sequence
// Return value:
// - true if we were able to successfully generate a sequence.
// On success, caller is responsible for delete[]ing *ppwchSequence.
bool MouseInput::_GenerateSGRSequence(_In_ const COORD coordMousePosition,
                                      _In_ const unsigned int uiButton,
                                      _Out_ wchar_t** const ppwchSequence,
                                      _Out_ size_t* const pcchLength) const
{
    // Format for SGR events is:
    // "\x1b[<%d;%d;%d;%c", xButton, x+1, y+1, fButtonDown? 'M' : 'm'
    bool fSuccess = false;
    const int iXButton = s_WindowsButtonToXEncoding(uiButton);
	#pragma warning( push )
	#pragma warning( disable: 4996 )
	// Disable 4996 - The _s version of _snprintf doesn't return the cch if the buffer is null, and we need the cch
    int iNeededChars = _snprintf(nullptr, 0, "\x1b[<%d;%d;%d%c", 
                                iXButton, coordMousePosition.X+1, coordMousePosition.Y+1, s_IsButtonDown(uiButton)? 'M' : 'm');


	#pragma warning( pop )
	iNeededChars += 1; // for null 

    wchar_t* pwchFormat = new wchar_t[iNeededChars];
    if (pwchFormat != nullptr)
    {
        int iTakenChars = _snwprintf_s(pwchFormat, iNeededChars, iNeededChars, L"\x1b[<%d;%d;%d%c",
                                    iXButton, coordMousePosition.X+1, coordMousePosition.Y+1, s_IsButtonDown(uiButton)? 'M' : 'm');
        if (iTakenChars == iNeededChars-1) // again, adjust for null
        {
            *ppwchSequence = pwchFormat;
            *pcchLength = iTakenChars;
            fSuccess = true;   
        }
        else 
        {
            delete[] pwchFormat;
        }
    }
    return fSuccess;
}

// Routine Description:
// - Either enables or disables UTF-8 extended mode encoding. This *should* cause 
//      the coordinates of a mouse event to be encoded as a UTF-8 byte stream, however, because windows' input is 
//      typically UTF-16 encoded, it emits a UTF-16 stream.
//   Does NOT enable or disable mouse mode by itself. This matches the behavior I found in Ubuntu terminals.
// Parameters:
// - fEnable - either enable or disable.
// Return value:
// <none>
void MouseInput::SetUtf8ExtendedMode(_In_ const bool fEnable)
{
    _ExtendedMode = fEnable? ExtendedMode::Utf8 : ExtendedMode::None;
}

// Routine Description:
// - Either enables or disables SGR extended mode encoding. This causes the 
//      coordinates of a mouse event to be emitted in a human readable format, 
//      eg, x,y=203,504 -> "^[[<B;203;504M". This way, applications don't need to worry about character encoding.
//   Does NOT enable or disable mouse mode by itself. This matches the behavior I found in Ubuntu terminals.
// Parameters:
// - fEnable - either enable or disable.
// Return value:
// <none>
void MouseInput::SetSGRExtendedMode(_In_ const bool fEnable)
{
    _ExtendedMode = fEnable? ExtendedMode::Sgr : ExtendedMode::None;
}

// Routine Description:
// - Either enables or disables mouse mode handling. Leaves the extended mode alone, 
//      so if we disable then re-enable mouse mode without toggling an extended mode, the mode will persist.
// Parameters:
// - fEnable - either enable or disable.
// Return value:
// <none>
void MouseInput::Enable(_In_ const bool fEnable)
{
    _fEnabled = fEnable;
}

// Routine Description:
// - Sends the given sequence into the input callback specified by _pfnWriteEvents.
//      Typically, this inserts the characters into the input buffer as KeyDown KEY_EVENTs.
// Parameters:
// - pwszSequence - sequence to send to _pfnWriteEvents
// - cchLength - the length of pwszSequence
// Return value:
// <none>
void MouseInput::_SendInputSequence(_In_reads_(cchLength) const wchar_t* const pwszSequence, _In_ const size_t cchLength) const
{
    size_t cch = 0;
    // + 1 to max sequence length for null terminator count which is required by StringCchLengthW
    if (SUCCEEDED(StringCchLengthW(pwszSequence, cchLength + 1, &cch)) && cch > 0 && cch < DWORD_MAX)
    {        
        INPUT_RECORD* rgInput = new INPUT_RECORD[cchLength];
        if (rgInput != nullptr)
        {
            for (size_t i = 0; i < cch; i++)
            {
                rgInput[i].EventType = KEY_EVENT;
                rgInput[i].Event.KeyEvent.bKeyDown = TRUE;
                rgInput[i].Event.KeyEvent.dwControlKeyState = 0;
                rgInput[i].Event.KeyEvent.wRepeatCount = 1;
                rgInput[i].Event.KeyEvent.wVirtualKeyCode = 0;
                rgInput[i].Event.KeyEvent.wVirtualScanCode = 0;

                rgInput[i].Event.KeyEvent.uChar.UnicodeChar = pwszSequence[i];
            }
            _pfnWriteEvents(rgInput, (DWORD)cch);
            delete[] rgInput;    
        }
    }
}

// Routine Description:
// - Translates the given coord from windows coordinate space (origin=0,0) to VT space (origin=1,1)
// Parameters:
// - coordWinCoordinate - the coordinate to translate
// Return value:
// - the translated coordinate.
COORD MouseInput::s_WinToVTCoord(_In_ const COORD coordWinCoordinate)
{
    return {coordWinCoordinate.X + 1, coordWinCoordinate.Y + 1};
}

// Routine Description:
// - Encodes the given value as a default (or utf-8) encoding value.
//     32 is added so that the value 0 can be emitted as the printable characher ' '.
// Parameters:
// - sCoordinateValue - the value to encode.
// Return value:
// - the encoded value.
short MouseInput::s_EncodeDefaultCoordinate(_In_ const short sCoordinateValue)
{
    return sCoordinateValue + 32;
}