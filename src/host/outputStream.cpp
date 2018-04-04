/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "outputStream.hpp"

#include "_stream.h"
#include "getset.h"
#include "directio.h"

#pragma hdrstop
using namespace Microsoft::Console;

WriteBuffer::WriteBuffer(_In_ Microsoft::Console::IIoProvider* const pIo) :
    _pIo(THROW_IF_NULL_ALLOC(pIo))
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

    _pIo->GetActiveOutputBuffer()->TextInfo->GetCursor()->SetIsOn(true);

    _ntstatus = WriteCharsLegacy(_pIo->GetActiveOutputBuffer(),
                                 buffer,
                                 buffer,
                                 buffer,
                                 &dwNumBytes,
                                 nullptr,
                                 _pIo->GetActiveOutputBuffer()->TextInfo->GetCursor()->GetPosition().X,
                                 WC_NONDESTRUCTIVE_TAB | WC_DELAY_EOL_WRAP,
                                 nullptr);
}

ConhostInternalGetSet::ConhostInternalGetSet(_In_ IIoProvider* const pIo) :
    _pIo(THROW_IF_NULL_ALLOC(pIo))
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
    DoSrvGetConsoleScreenBufferInfo(_pIo->GetActiveOutputBuffer(), pConsoleScreenBufferInfoEx);
    return TRUE;
}

// Routine Description:
// - Connects the SetConsoleScreenBufferInfoEx API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleScreenBufferInfoEx - Pointer to structure containing screen buffer information like the public API call.
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleScreenBufferInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const
{
    DoSrvSetScreenBufferInfo(_pIo->GetActiveOutputBuffer(), pConsoleScreenBufferInfoEx);
    return TRUE;
}

// Routine Description:
// - Connects the SetConsoleCursorPosition API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - coordCursorPosition - new cursor position to set like the public API call.
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleCursorPosition). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleCursorPosition(_In_ COORD const coordCursorPosition)
{
    return SUCCEEDED(DoSrvSetConsoleCursorPosition(_pIo->GetActiveOutputBuffer(), &coordCursorPosition));
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

    DoSrvGetConsoleCursorInfo(_pIo->GetActiveOutputBuffer(), &dwSize, &bVisible);
    pConsoleCursorInfo->bVisible = bVisible;
    pConsoleCursorInfo->dwSize = dwSize;
    return TRUE;
}

// Routine Description:
// - Connects the SetConsoleCursorInfo API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pConsoleCursorInfo - Updated size/visibility information to modify the cursor rendering behavior.
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleCursorInfo). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo)
{
    return SUCCEEDED(DoSrvSetConsoleCursorInfo(_pIo->GetActiveOutputBuffer(), pConsoleCursorInfo->dwSize, !!pConsoleCursorInfo->bVisible));
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

    BOOL fSuccess = NT_SUCCESS(DoSrvFillConsoleOutput(_pIo->GetActiveOutputBuffer(), &msg));

    if (fSuccess)
    {
        *pNumberWritten = msg.Length; // the length value is replaced when exiting with the number written.
    }

    return fSuccess;
}

// Routine Description:
// - Connects the SetConsoleTextAttribute API call directly into our Driver Message servicing call inside Conhost.exe
//     Sets BOTH the FG and the BG component of the attributes.
// Arguments:
// - wAttr - new color/graphical attributes to apply as default within the console text buffer
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleTextAttribute). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleTextAttribute(_In_ WORD const wAttr)
{
    return SUCCEEDED(DoSrvSetConsoleTextAttribute(_pIo->GetActiveOutputBuffer(), wAttr));
}

// Routine Description:
// - Connects the PrivateSetLegacyAttributes API call directly into our Driver Message servicing call inside Conhost.exe
//     Sets only the components of the attributes requested with the fForeground, fBackground, and fMeta flags.
// Arguments:
// - wAttr - new color/graphical attributes to apply as default within the console text buffer
// - fForeground - The new attributes contain an update to the foreground attributes
// - fBackground - The new attributes contain an update to the background attributes
// - fMeta - The new attributes contain an update to the meta attributes
// Return Value:
// - TRUE if successful (see DoSrvVtSetLegacyAttributes). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateSetLegacyAttributes(_In_ WORD const wAttr,
                                                       _In_ const bool fForeground,
                                                       _In_ const bool fBackground,
                                                       _In_ const bool fMeta)
{
    DoSrvPrivateSetLegacyAttributes(_pIo->GetActiveOutputBuffer(), wAttr, fForeground, fBackground, fMeta);
    return TRUE;
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
    DoSrvPrivateSetConsoleXtermTextAttribute(_pIo->GetActiveOutputBuffer(), iXtermTableEntry, fIsForeground);
    return TRUE;
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
    DoSrvPrivateSetConsoleRGBTextAttribute(_pIo->GetActiveOutputBuffer(), rgbColor, fIsForeground);
    return TRUE;
}

// Routine Description:
// - Connects the WriteConsoleInput API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - events - the input events to be copied into the head of the input
// buffer for the underlying attached process
// - eventsWritten - on output, the number of events written
// Return Value:
// - TRUE if successful (see DoSrvWriteConsoleInput). FALSE otherwise.
BOOL ConhostInternalGetSet::WriteConsoleInputW(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                               _Out_ size_t& eventsWritten)
{
    eventsWritten = 0;
    return SUCCEEDED(DoSrvWriteConsoleInput(_pIo->GetActiveInputBuffer(),
                                            events,
                                            eventsWritten,
                                            true, // unicode
                                            true)); // append
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
    return SUCCEEDED(DoSrvScrollConsoleScreenBufferW(_pIo->GetActiveOutputBuffer(),
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
    return SUCCEEDED(DoSrvSetConsoleWindowInfo(_pIo->GetActiveOutputBuffer(), !!bAbsolute, lpConsoleWindow));
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
    DoSrvPrivateAllowCursorBlinking(_pIo->GetActiveOutputBuffer(), fEnable);
    return TRUE;
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
    return NT_SUCCESS(DoSrvPrivateSetScrollingRegion(_pIo->GetActiveOutputBuffer(), psrScrollMargins));
}

// Routine Description:
// - Connects the PrivateReverseLineFeed call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateReverseLineFeed is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on out public API surface.
// Return Value:
// - TRUE if successful (see DoSrvPrivateReverseLineFeed). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateReverseLineFeed()
{
    return NT_SUCCESS(DoSrvPrivateReverseLineFeed(_pIo->GetActiveOutputBuffer()));
}

// Routine Description:
// - Connects the SetConsoleTitleW API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - pwchWindowTitle - The null-terminated string to set as the window title
// - sCchTitleLength - the number of characters in the title
// Return Value:
// - TRUE if successful (see DoSrvSetConsoleTitle). FALSE otherwise.
BOOL ConhostInternalGetSet::SetConsoleTitleW(_In_reads_(sCchTitleLength) const wchar_t* const pwchWindowTitle, _In_ unsigned short sCchTitleLength)
{
    return SUCCEEDED(DoSrvSetConsoleTitleW(pwchWindowTitle, sCchTitleLength));
}

// Routine Description:
// - Connects the PrivateUseAlternateScreenBuffer call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateUseAlternateScreenBuffer is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on out public API surface.
// Return Value:
// - TRUE if successful (see DoSrvPrivateUseAlternateScreenBuffer). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateUseAlternateScreenBuffer()
{
    return NT_SUCCESS(DoSrvPrivateUseAlternateScreenBuffer(_pIo->GetActiveOutputBuffer()));
}

// Routine Description:
// - Connects the PrivateUseMainScreenBuffer call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateUseMainScreenBuffer is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on out public API surface.
// Return Value:
// - TRUE if successful (see DoSrvPrivateUseMainScreenBuffer). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateUseMainScreenBuffer()
{
    DoSrvPrivateUseMainScreenBuffer(_pIo->GetActiveOutputBuffer());
    return TRUE;
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
    DoSrvPrivateTabClear(fClearAll);
    return TRUE;
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
    DoSrvPrivateEnableVT200MouseMode(fEnabled);
    return TRUE;
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
    DoSrvPrivateEnableUTF8ExtendedMouseMode(fEnabled);
    return TRUE;
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
    DoSrvPrivateEnableSGRExtendedMouseMode(fEnabled);
    return TRUE;
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
    DoSrvPrivateEnableButtonEventMouseMode(fEnabled);
    return TRUE;
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
    DoSrvPrivateEnableAnyEventMouseMode(fEnabled);
    return TRUE;
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
    DoSrvPrivateEnableAlternateScroll(fEnabled);
    return TRUE;
}

// Routine Description:
// - Connects the PrivateEraseAll call directly into our Driver Message servicing call inside Conhost.exe
//   PrivateEraseAll is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on our public API surface.
// Arguments:
// <none>
// Return Value:
// - TRUE if successful (see DoSrvPrivateEraseAll). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateEraseAll()
{
    return NT_SUCCESS(DoSrvPrivateEraseAll(_pIo->GetActiveOutputBuffer()));
}

// Routine Description:
// - Connects the SetCursorStyle call directly into our Driver Message servicing call inside Conhost.exe
//   SetCursorStyle is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on our public API surface.
// Arguments:
// - cursorType: The style of cursor to change the cursor to.
// Return Value:
// - TRUE if successful (see DoSrvSetCursorStyle). FALSE otherwise.
BOOL ConhostInternalGetSet::SetCursorStyle(_In_ CursorType const cursorType)
{
    DoSrvSetCursorStyle(_pIo->GetActiveOutputBuffer(), cursorType);
    return TRUE;
}

// Routine Description:
// - Retrieves the default color attributes information for the active screen buffer.
// - This function is used to optimize SGR calls in lieu of calling GetConsoleScreenBufferInfoEx.
// Arguments:
// - pwAttributes - Pointer to space to receive color attributes data
// Return Value:
// - TRUE if successful. FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateGetConsoleScreenBufferAttributes(_Out_ WORD* const pwAttributes)
{
    return NT_SUCCESS(DoSrvPrivateGetConsoleScreenBufferAttributes(_pIo->GetActiveOutputBuffer(), pwAttributes));
}

// Routine Description:
// - Connects the PrivatePrependConsoleInput API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - events - the input events to be copied into the head of the input
// buffer for the underlying attached process
// - eventsWritten - on output, the number of events written
// Return Value:
// - TRUE if successful (see DoSrvPrivatePrependConsoleInput). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivatePrependConsoleInput(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                                       _Out_ size_t& eventsWritten)
{
    return SUCCEEDED(DoSrvPrivatePrependConsoleInput(_pIo->GetActiveInputBuffer(),
                                                     events,
                                                     eventsWritten));
}

// Routine Description:
// - Connects the PrivatePrependConsoleInput API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - <none>
// Return Value:
// - TRUE if successful (see DoSrvPrivateRefreshWindow). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateRefreshWindow()
{
    DoSrvPrivateRefreshWindow(_pIo->GetActiveOutputBuffer());
    return TRUE;
}

// Routine Description:
// - Connects the PrivateWriteConsoleControlInput API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - key - a KeyEvent representing a special type of keypress, typically Ctrl-C
// Return Value:
// - TRUE if successful (see DoSrvPrivateWriteConsoleControlInput). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateWriteConsoleControlInput(_In_ KeyEvent key)
{
    return SUCCEEDED(DoSrvPrivateWriteConsoleControlInput(_pIo->GetActiveInputBuffer(),
                                                          key));
}

// Routine Description:
// - Connects the GetConsoleOutputCP API call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - puiOutputCP - recieves the outputCP of the console.
// Return Value:
// - TRUE if successful (see DoSrvPrivateWriteConsoleControlInput). FALSE otherwise.
BOOL ConhostInternalGetSet::GetConsoleOutputCP(_Out_ unsigned int* const puiOutputCP)
{
    DoSrvGetConsoleOutputCodePage(puiOutputCP);
    return TRUE;
}

// Routine Description:
// - Connects the PrivateSuppressResizeRepaint API call directly into our Driver
//      Message servicing call inside Conhost.exe
// Arguments:
// - <none>
// Return Value:
// - TRUE if successful (see DoSrvPrivateSuppressResizeRepaint). FALSE otherwise.
BOOL ConhostInternalGetSet::PrivateSuppressResizeRepaint()
{
    return SUCCEEDED(DoSrvPrivateSuppressResizeRepaint());
}

// Routine Description:
// - Connects the SetCursorStyle call directly into our Driver Message servicing call inside Conhost.exe
//   SetCursorStyle is an internal-only "API" call that the vt commands can execute,
//     but it is not represented as a function call on our public API surface.
// Arguments:
// - cursorColor: The color to change the cursor to. INVALID_COLOR will revert
//      it to the legacy inverting behavior.
// Return Value:
// - TRUE if successful (see DoSrvSetCursorStyle). FALSE otherwise.
BOOL ConhostInternalGetSet::SetCursorColor(_In_ COLORREF const cursorColor)
{
    DoSrvSetCursorColor(_pIo->GetActiveOutputBuffer(), cursorColor);
    return TRUE;
}

// Routine Description:
// - Connects the IsConsolePty call directly into our Driver Message servicing call inside Conhost.exe
// Arguments:
// - isPty: recieves the bool indicating whether or not we're in pty mode.
// Return Value:
// - TRUE if successful (see DoSrvIsConsolePty). FALSE otherwise.
BOOL ConhostInternalGetSet::IsConsolePty(_Out_ bool* const pIsPty) const
{
    DoSrvIsConsolePty(pIsPty);
    return TRUE;
}
