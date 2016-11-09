/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "outputStream.hpp"

#include "_stream.h"
#include "stream.h"
#include "_output.h"
#include "misc.h"
#include "getset.h"
#include "directio.h"
#include "cursor.h"

#pragma hdrstop

WriteBuffer::WriteBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo) : _dcsi(pScreenInfo)
{
}

// Routine Description:
// - Handles the print action from the state machine
// Arguments:
// - wch - The character to be printed.
// Return Value:
// - <none>
void WriteBuffer::Print(_In_ wchar_t const wch)
{
    _DefaultCase(wch);
}

// Routine Description:
// - Handles the print action from the state machine
// Arguments:
// - wch - The character to be printed.
// Return Value:
// - <none>
void WriteBuffer::PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    _DefaultStringCase(rgwch, cch);
}

// Routine Description:
// - Handles the execute action from the state machine
// Arguments:
// - wch - The C0 control character to be executed.
// Return Value:
// - <none>
void WriteBuffer::Execute(_In_ wchar_t const wch)
{
    _DefaultCase(wch);
}

// Routine Description:
// - Default text editing/printing handler for all characters that were not routed elsewhere by other state machine intercepts.
// Arguments:
// - wch - The character to be processed by our default text editing/printing mechanisms.
// Return Value:
// - <none>
void WriteBuffer::_DefaultCase(_In_ wchar_t const wch)
{
    _DefaultStringCase(const_cast<wchar_t*>(&wch), 1); // WriteCharsLegacy wants mutable chars, so we'll givve it mutable chars.
}

// Routine Description:
// - Default text editing/printing handler for all characters that were not routed elsewhere by other state machine intercepts.
// Arguments:
// - wch - The character to be processed by our default text editing/printing mechanisms.
// Return Value:
// - <none>
void WriteBuffer::_DefaultStringCase(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch)
{
    PWCHAR buffer = &(rgwch[0]);

    DWORD dwNumBytes = (DWORD)(cch * sizeof(wchar_t));

    _dcsi->TextInfo->GetCursor()->SetIsOn(TRUE);

    _ntstatus = WriteCharsLegacy(_dcsi, 
                                 buffer, 
                                 buffer, 
                                 buffer, 
                                 &dwNumBytes, 
                                 nullptr, 
                                 _dcsi->TextInfo->GetCursor()->GetPosition().X, 
                                 WC_NONDESTRUCTIVE_TAB | WC_DELAY_EOL_WRAP, 
                                 nullptr);
}

// Routine Description:
// - Sets our pointer to the screen buffer we're attached to. This is used by UseAlternateBuffer and UseMainBuffer
//     because they share one InternalGetSet and WriteBuffer
// Return Value: 
// <none>
void WriteBuffer::SetActiveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    this->_dcsi = pScreenInfo;
}

ConhostInternalGetSet::ConhostInternalGetSet(_In_ SCREEN_INFORMATION* const pScreenInfo, 
                                             _In_ INPUT_INFORMATION* const pInputInfo) :
                                             _pScreenInfo(pScreenInfo), 
                                             _pInputInfo(pInputInfo)
{
}

// Routine Description:
// - Connects the GetConsoleScreenBufferInfoEx API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleScreenBufferInfoEx - Pointer to structure to hold screen buffer information like the public API call.
// Return Value:
// - TRUE if successful (see DoSrvGetConsoleScreenBufferInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const 
{
    return SUCCEEDED(DoSrvGetConsoleScreenBufferInfo(_pScreenInfo, pConsoleScreenBufferInfoEx));
}

// Routine Description:
// - Connects the SetConsoleScreenBufferInfoEx API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleScreenBufferInfoEx - Pointer to structure containing screen buffer information like the public API call.
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleScreenBufferInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const 
{
    return SUCCEEDED(DoSrvSetScreenBufferInfo(_pScreenInfo, pConsoleScreenBufferInfoEx));
}

// Routine Description:
// - Connects the SetConsoleCursorPosition API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - coordCursorPosition - new cursor position to set like the public API call.
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleCursorPosition). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleCursorPosition(_In_ COORD const coordCursorPosition)
{
    return SUCCEEDED(DoSrvSetConsoleCursorPosition(_pScreenInfo, &coordCursorPosition));
}

// Routine Description:
// - Connects the GetConsoleCursorInfo API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleCursorInfo - Pointer to structure to receive console cursor rendering info
// Return Value: 
// - TRUE if successful (see DoSrvGetConsoleCursorInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const 
{
    BOOLEAN bVisible;
    DWORD dwSize;

    if (SUCCEEDED(DoSrvGetConsoleCursorInfo(_pScreenInfo, &dwSize, &bVisible)))
    {
        pConsoleCursorInfo->bVisible = bVisible;
        pConsoleCursorInfo->dwSize = dwSize;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Routine Description:
// - Connects the SetConsoleCursorInfo API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleCursorInfo - Updated size/visibility information to modify the cursor rendering behavior.
// Return Value: 
// - TRUE if successful (see DoSrvSetConsoleCursorInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo)
{
    return SUCCEEDED(DoSrvSetConsoleCursorInfo(_pScreenInfo, pConsoleCursorInfo->dwSize, !!pConsoleCursorInfo->bVisible));
}

// Routine Description:
// - Connects the FillConsoleOutputCharacter API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - wch - Character to use for filling the buffer
// - nLength - The length of the fill run in characters (depending on mode, will wrap at the window edge so multiple lines are the sum of the total length)
// - dwWriteCoord - The first fill character's coordinate position in the buffer (writes continue rightward and possibly down from there)
// - pNumberOfCharsWritten - Pointer to memory location to hold the total number of characters written into the buffer
// Return Value: 
// - TRUE if successful (see DoSrvFillConsoleOutput). FALSE otherwise.
BOOL ConhostInternalGetSet::FillConsoleOutputCharacterW(_In_ WCHAR const wch, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfCharsWritten)
{
    return _FillConsoleOutput(wch, CONSOLE_REAL_UNICODE, nLength, dwWriteCoord, pNumberOfCharsWritten);
}

// Routine Description:
// - Connects the FillConsoleOutputAttribute API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - wAttribute - Text attribute (colors/font style) for filling the buffer
// - nLength - The length of the fill run in characters (depending on mode, will wrap at the window edge so multiple lines are the sum of the total length)
// - dwWriteCoord - The first fill character's coordinate position in the buffer (writes continue rightward and possibly down from there)
// - pNumberOfCharsWritten - Pointer to memory location to hold the total number of text attributes written into the buffer
// Return Value: 
// - TRUE if successful (see DoSrvFillConsoleOutput). FALSE otherwise.
BOOL ConhostInternalGetSet::FillConsoleOutputAttribute(_In_ WORD const wAttribute, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberOfAttrsWritten)
{
    return _FillConsoleOutput(wAttribute, CONSOLE_ATTRIBUTE, nLength, dwWriteCoord, pNumberOfAttrsWritten);
}

// Routine Description:
// - Helper to consolidate FillConsole calls into the same internal message/API call pattern.
// Arguments:
// - usElement - Variable element type to fill the console with (characters, attributes)
// - ulElementType - Flag to specify which type of fill to perform with usElement variable.
// - nLength - The length of the fill run in characters (depending on mode, will wrap at the window edge so multiple lines are the sum of the total length)
// - dwWriteCoord - The first fill character's coordinate position in the buffer (writes continue rightward and possibly down from there)
// - pNumberOfCharsWritten - Pointer to memory location to hold the total number of elements written into the buffer
// Return Value: 
// - TRUE if successful (see DoSrvFillConsoleOutput). FALSE otherwise.
BOOL ConhostInternalGetSet::_FillConsoleOutput(_In_ USHORT const usElement, _In_ ULONG const ulElementType, _In_ DWORD const nLength, _In_ COORD const dwWriteCoord, _Out_ DWORD* const pNumberWritten)
{
    CONSOLE_FILLCONSOLEOUTPUT_MSG msg;
    msg.Element = usElement;
    msg.ElementType = ulElementType;
    msg.WriteCoord = dwWriteCoord;
    msg.Length = nLength;

    BOOL fSuccess = NT_SUCCESS(DoSrvFillConsoleOutput(_pScreenInfo, &msg));

    if (fSuccess)
    {
        *pNumberWritten = msg.Length; // the length value is replaced when exiting with the number written.
    }

    return fSuccess;
}

// Routine Description:
// - Connects the SetConsoleTextAttribute API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - wAttr - new color/graphical attributes to apply as default within the console text buffer
// Return Value: 
// - TRUE if successful (see DoSrvSetConsoleTextAttribute). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleTextAttribute(_In_ WORD const wAttr)
{
    return SUCCEEDED(DoSrvSetConsoleTextAttribute(_pScreenInfo, wAttr));
}

// Routine Description:
// - Sets the current attributes of the screen buffer to use the color table entry specified by 
//     the iXtermTableEntry. Sets either the FG or the BG component of the attributes.
// Arguments:
// - iXtermTableEntry - The entry of the xterm table to use.
// - fIsForeground - Whether or not the color applies to the foreground.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateSetConsoleXtermTextAttribute). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleXtermTextAttribute(_In_ int const iXtermTableEntry, _In_ const bool fIsForeground)
{
    return NT_SUCCESS(DoSrvPrivateSetConsoleXtermTextAttribute(_pScreenInfo, iXtermTableEntry, fIsForeground));
}

// Routine Description:
// - Sets the current attributes of the screen buffer to use the given rgb color.
//     Sets either the FG or the BG component of the attributes.
// Arguments:
// - rgbColor - The rgb color to use.
// - fIsForeground - Whether or not the color applies to the foreground.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateSetConsoleRGBTextAttribute). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleRGBTextAttribute(_In_ COLORREF const rgbColor, _In_ const bool fIsForeground)
{
    return NT_SUCCESS(DoSrvPrivateSetConsoleRGBTextAttribute(_pScreenInfo, rgbColor, fIsForeground));
}

// Routine Description:
// - Connects the WriteConsoleInput API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - rgInputRecords - An array of input records to be copied into the the head of the input buffer for the underlying attached process
// - nLength - The number of records in the rgInputRecords array
// - pNumberOfEventsWritten - Pointer to memory location to hold the total number of elements written into the buffer
// Return Value: 
// - TRUE if successful (see DoSrvWriteConsoleInput). FALSE otherwise.
BOOL ConhostInternalGetSet::WriteConsoleInputW(_In_reads_(nLength) INPUT_RECORD* const rgInputRecords, _In_ DWORD const nLength, _Out_ DWORD* const pNumberOfEventsWritten)
{
    CONSOLE_WRITECONSOLEINPUT_MSG msg;
    msg.Append = false;
    msg.NumRecords = nLength;
    msg.Unicode = true;

    BOOL fSuccess = NT_SUCCESS(DoSrvWriteConsoleInput(_pInputInfo, &msg, rgInputRecords));

    *pNumberOfEventsWritten = msg.NumRecords;

    return fSuccess;
}

// Routine Description:
// - Connects the ScrollConsoleScreenBuffer API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pScrollRectangle - The region to "cut" from the existing buffer text
// - pClipRectangle - The bounding rectangle within which all modifications should happen. Any modification outside this RECT should be clipped.
// - coordDestinationOrigin - The top left corner of the "paste" from pScrollREctangle
// - pFill - The text/attribute pair to fill all remaining space behind after the "cut" operation (bounded by clip, of course.)
// Return Value: 
// - TRUE if successful (see DoSrvScrollConsoleScreenBuffer). FALSE otherwise.
BOOL ConhostInternalGetSet::ScrollConsoleScreenBufferW(_In_ const SMALL_RECT* pScrollRectangle, 
                                                       _In_opt_ const SMALL_RECT* pClipRectangle, 
                                                       _In_ COORD coordDestinationOrigin, 
                                                       _In_ const CHAR_INFO* pFill)
{
    return SUCCEEDED(DoSrvScrollConsoleScreenBufferW(_pScreenInfo,
                                                     pScrollRectangle,
                                                     &coordDestinationOrigin,
                                                     pClipRectangle,
                                                     pFill->Char.UnicodeChar,
                                                     pFill->Attributes));
}

// Routine Description:
// - Connects the SetConsoleWindowInfo API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - bAbsolute - Should the window be moved to an absolute position? If false, the movement is relative to the current pos.
// - lpConsoleWindow - Info about how to move the viewport
// Return Value: 
// - TRUE if successful (see DoSrvSetConsoleWindowInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleWindowInfo(_In_ BOOL const bAbsolute, _In_ const SMALL_RECT* const lpConsoleWindow)
{
    return SUCCEEDED(DoSrvSetConsoleWindowInfo(_pScreenInfo, !!bAbsolute, lpConsoleWindow));
}

// Routine Description:
// - Connects the PrivateSetCursorKeysMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateSetCursorKeysMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fApplicationMode - set to true to enable Application Mode Input, false for Normal Mode.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateSetCursorKeysMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateSetCursorKeysMode(_In_ bool const fApplicationMode)
{
    return NT_SUCCESS(DoSrvPrivateSetCursorKeysMode(fApplicationMode));
}

// Routine Description:
// - Connects the PrivateSetKeypadMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateSetKeypadMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fApplicationMode - set to true to enable Application Mode Input, false for Numeric Mode.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateSetKeypadMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateSetKeypadMode(_In_ bool const fApplicationMode)
{
    return NT_SUCCESS(DoSrvPrivateSetKeypadMode(fApplicationMode));
}

// Routine Description:
// - Connects the PrivateAllowCursorBlinking call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateAllowCursorBlinking is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnable - set to true to enable blinking, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateAllowCursorBlinking). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateAllowCursorBlinking(_In_ bool const fEnable)
{
    return NT_SUCCESS(DoSrvPrivateAllowCursorBlinking(_pScreenInfo, fEnable));
}

// Routine Description:
// - Connects the PrivateSetScrollingRegion call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateSetScrollingRegion is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - psrScrollMargins - The bounds of the region to be the scrolling region of the viewport.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateSetScrollingRegion). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateSetScrollingRegion(_In_ const SMALL_RECT* const psrScrollMargins)
{
    return NT_SUCCESS(DoSrvPrivateSetScrollingRegion(_pScreenInfo, psrScrollMargins));
}

// Routine Description:
// - Connects the PrivateReverseLineFeed call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateReverseLineFeed is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateReverseLineFeed). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateReverseLineFeed()
{
    return NT_SUCCESS(DoSrvPrivateReverseLineFeed(_pScreenInfo));
}

// Routine Description:
// - Connects the SetConsoleTitleW API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pwchWindowTitle - The null-terminated string to set as the window title 
// - sCchTitleLength - the number of characters in the title
// Return Value: 
// - TRUE if successful (see DoSrvSetConsoleTitle). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleTitleW(_In_ const wchar_t* const pwchWindowTitle, _In_ unsigned short sCchTitleLength)
{
    ULONG cbOriginalLength;

    BOOL fResult = SUCCEEDED(ULongMult(sCchTitleLength, sizeof(WCHAR), &cbOriginalLength));
    
    if (fResult)
    {
        fResult = SUCCEEDED(DoSrvSetConsoleTitleW(pwchWindowTitle, cbOriginalLength));
    }    

    return fResult;
}

// Routine Description:
// - Connects the PrivateUseAlternateScreenBuffer call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateUseAlternateScreenBuffer is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateUseAlternateScreenBuffer). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateUseAlternateScreenBuffer()
{
    return NT_SUCCESS(DoSrvPrivateUseAlternateScreenBuffer(_pScreenInfo));
}

// Routine Description:
// - Connects the PrivateUseMainScreenBuffer call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateUseMainScreenBuffer is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Return Value: 
// - TRUE if successful (see DoSrvPrivateUseMainScreenBuffer). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateUseMainScreenBuffer()
{
    return NT_SUCCESS(DoSrvPrivateUseMainScreenBuffer(_pScreenInfo));
}

// Routine Description:
// - Sets our pointer to the screen buffer we're attached to. This is used by UseAlternateBuffer and UseMainBuffer
//     because they share one InternalGetSet and WriteBuffer
// Return Value: 
// <none>
void ConhostInternalGetSet::SetActiveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    this->_pScreenInfo = pScreenInfo;
}

// - Connects the PrivateHorizontalTabSet call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateHorizontalTabSet is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// <none>
// Return Value: 
// - TRUE if successful (see PrivateHorizontalTabSet). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateHorizontalTabSet()
{
    return NT_SUCCESS(DoSrvPrivateHorizontalTabSet());    
}

// Routine Description:
// - Connects the PrivateForwardTab call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateForwardTab is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - sNumTabs - the number of tabs to execute
// Return Value: 
// - TRUE if successful (see PrivateForwardTab). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateForwardTab(_In_ SHORT const sNumTabs)
{
    return NT_SUCCESS(DoSrvPrivateForwardTab(sNumTabs));
}

// Routine Description:
// - Connects the PrivateBackwardsTab call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateBackwardsTab is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - sNumTabs - the number of tabs to execute
// Return Value: 
// - TRUE if successful (see PrivateBackwardsTab). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateBackwardsTab(_In_ SHORT const sNumTabs)
{
    return NT_SUCCESS(DoSrvPrivateBackwardsTab(sNumTabs));
}

// Routine Description:
// - Connects the PrivateTabClear call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateTabClear is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fClearAll - set to true to enable blinking, false to disable
// Return Value: 
// - TRUE if successful (see PrivateTabClear). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateTabClear(_In_ bool const fClearAll)
{
    return NT_SUCCESS(DoSrvPrivateTabClear(fClearAll));
}

// Routine Description:
// - Connects the PrivateEnableVT200MouseMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableVT200MouseMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable vt200 mouse mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableVT200MouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableVT200MouseMode(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableVT200MouseMode(fEnabled));
}

// Routine Description:
// - Connects the PrivateEnableUTF8ExtendedMouseMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableUTF8ExtendedMouseMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable utf8 extended mouse mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableUTF8ExtendedMouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableUTF8ExtendedMouseMode(fEnabled));
}

// Routine Description:
// - Connects the PrivateEnableSGRExtendedMouseMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableSGRExtendedMouseMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable SGR extended mouse mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableSGRExtendedMouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableSGRExtendedMouseMode(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableSGRExtendedMouseMode(fEnabled));
}

// Routine Description:
// - Connects the PrivateEnableButtonEventMouseMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableButtonEventMouseMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable button-event mouse mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableButtonEventMouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableButtonEventMouseMode(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableButtonEventMouseMode(fEnabled));
}

// Routine Description:
// - Connects the PrivateEnableAnyEventMouseMode call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableAnyEventMouseMode is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable any-event mouse mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableAnyEventMouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableAnyEventMouseMode(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableAnyEventMouseMode(fEnabled));
}

// Routine Description:
// - Connects the PrivateEnableAlternateScroll call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEnableAlternateScroll is an internal-only "API" call that the vt commands can execute, 
//     but it is not represented as a function call on out public API surface.
// Arguments:
// - fEnabled - set to true to enable alternate scroll mode, false to disable
// Return Value: 
// - TRUE if successful (see DoSrvPrivateEnableAnyEventMouseMode). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEnableAlternateScroll(_In_ bool const fEnabled)
{
    return NT_SUCCESS(DoSrvPrivateEnableAlternateScroll(fEnabled));
}