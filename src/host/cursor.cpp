/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "cursor.h"

#include "dbcs.h"
#include "output.h"
#include "window.hpp"
#include "userprivapi.hpp"

#pragma hdrstop

#define CURSOR_TIMER 1

// Routine Description:
// - Constructor to set default properties for Cursor
// Arguments:
// - ulSize - The height of the cursor within this buffer
Cursor::Cursor(_In_ ULONG const ulSize) :
    _ulSize(ulSize),
    _fHasMoved(FALSE),
    _fIsVisible(TRUE),
    _fIsOn(TRUE),
    _fBlinkingAllowed(TRUE),
    _fIsDouble(FALSE),
    _fIsConversionArea(FALSE),
    _fDelay(FALSE),
    _fDelayedEolWrap(FALSE),
    _fDeferCursorRedraw(FALSE),
    _fHaveDeferredCursorRedraw(FALSE)
{
    _cPosition = {0};
    _coordDelayedAt = {0};
}

Cursor::~Cursor()
{

}

// Routine Description:
// - Creates a new instance of Cursor
// Arguments:
// - ulSize - The height of the cursor within this buffer
// - ppCursor - Pointer to accept the instance of the newly created Cursor
// Return Value:
// - Success or a relevant error status (usually out of memory).
NTSTATUS Cursor::CreateInstance(_In_ ULONG const ulSize, _Outptr_ Cursor ** const ppCursor)
{
    *ppCursor = nullptr;

    Cursor* pCursor = new Cursor(ulSize);
    NTSTATUS status = NT_TESTNULL(pCursor);

    if (NT_SUCCESS(status))
    {
        *ppCursor = pCursor;
    }

    return status;
}

const COORD Cursor::GetPosition() const
{
    return _cPosition;
}

const BOOLEAN Cursor::HasMoved() const
{
    return _fHasMoved;
}

const BOOLEAN Cursor::IsVisible() const
{
    return _fIsVisible;
}

const BOOLEAN Cursor::IsOn() const
{
    return _fIsOn;
}

const BOOLEAN Cursor::IsBlinkingAllowed() const
{
    return _fBlinkingAllowed;
}

const BOOLEAN Cursor::IsDouble() const
{
    return _fIsDouble;
}

const BOOLEAN Cursor::IsDoubleWidth() const
{
    // Check with the current screen buffer to see if the character under the cursor is double-width.
    return !!IsCharFullWidth(g_ciConsoleInformation.CurrentScreenBuffer->TextInfo->GetRowByOffset(_cPosition.Y)->CharRow.Chars[_cPosition.X]);
}

const BOOLEAN Cursor::IsConversionArea() const
{
    return _fIsConversionArea;
}

const BOOLEAN Cursor::IsPopupShown() const
{
    return _fIsPopupShown;
}

const BOOLEAN Cursor::GetDelay() const
{
    return _fDelay;
}

const ULONG Cursor::GetSize() const
{
    return _ulSize;
}

void Cursor::SetHasMoved(_In_ BOOLEAN const fHasMoved)
{
    _fHasMoved = fHasMoved;
}

void Cursor::SetIsVisible(_In_ BOOLEAN const fIsVisible)
{
    _fIsVisible = fIsVisible;
    _RedrawCursor();
}

void Cursor::SetIsOn(_In_ BOOLEAN const fIsOn)
{
    _fIsOn = fIsOn;
    _RedrawCursorAlways();
}

void Cursor::SetBlinkingAllowed(_In_ BOOLEAN const fBlinkingAllowed)
{
    _fBlinkingAllowed = fBlinkingAllowed;
    _RedrawCursorAlways();
}

void Cursor::SetIsDouble(_In_ BOOLEAN const fIsDouble)
{
    _fIsDouble = fIsDouble;
    _RedrawCursor();
}

void Cursor::SetIsConversionArea(_In_ BOOLEAN const fIsConversionArea)
{
    _fIsConversionArea = fIsConversionArea;
    _RedrawCursorAlways();
}

void Cursor::SetIsPopupShown(_In_ BOOLEAN const fIsPopupShown)
{
    _fIsPopupShown = fIsPopupShown;
    _RedrawCursorAlways();
}

void Cursor::SetDelay(_In_ BOOLEAN const fDelay)
{
    _fDelay = fDelay;
}

void Cursor::SetSize(_In_ ULONG const ulSize)
{
    _ulSize = ulSize;
    _RedrawCursor();
}

// Routine Description:
// - Sends a redraw message to the renderer only if the cursor is currently on.
// - NOTE: For use with most methods in this class.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Cursor::_RedrawCursor()
{
    // Only trigger the redraw if we're on.
    // Don't draw the cursor if this was triggered from a conversion area.
    // (Conversion areas have cursors to mark the insertion point internally, but the user's actual cursor is the one on the primary screen buffer.)
    if (IsOn() && !IsConversionArea())
    {
        if (_fDeferCursorRedraw)
        {
            _fHaveDeferredCursorRedraw = TRUE;
        }
        else
        {
            _RedrawCursorAlways();
        }
    }
}

// Routine Description:
// - Sends a redraw message to the renderer no matter what.
// - NOTE: For use with the method that turns the cursor on and off to force a refresh
//         and clear the ON cursor from the screen. Not for use with other methods.
//         They should use the other method so refreshes are suppressed while the cursor is off.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Cursor::_RedrawCursorAlways()
{
    if (g_pRender != nullptr)
    {
        // Always trigger update of the cursor as one character width
        g_pRender->TriggerRedraw(&_cPosition);

        // In case of a double width character, we need to invalidate the spot one to the right of the cursor as well.
        if (IsDoubleWidth())
        {
            COORD cExtra = _cPosition;
            cExtra.X++;
            g_pRender->TriggerRedraw(&cExtra);
        }
    }
}

void Cursor::SetPosition(_In_ COORD const cPosition)
{
    _RedrawCursor();
    _cPosition.X = cPosition.X;
    _cPosition.Y = cPosition.Y;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::SetXPosition(_In_ int const NewX)
{
    _RedrawCursor();
    _cPosition.X = (SHORT)NewX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::SetYPosition(_In_ int const NewY)
{
    _RedrawCursor();
    _cPosition.Y = (SHORT)NewY;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::IncrementXPosition(_In_ int const DeltaX)
{
    _RedrawCursor();
    _cPosition.X += (SHORT)DeltaX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::IncrementYPosition(_In_ int const DeltaY)
{
    _RedrawCursor();
    _cPosition.Y += (SHORT)DeltaY;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::DecrementXPosition(_In_ int const DeltaX)
{
    _RedrawCursor();
    _cPosition.X -= (SHORT)DeltaX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::DecrementYPosition(_In_ int const DeltaY)
{
    _RedrawCursor();
    _cPosition.Y -= (SHORT)DeltaY;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

///////////////////////////////////////////////////////////////////////////////
// Routine Description:
// - Copies properties from another cursor into this one.
// - This is primarily to copy properties that would otherwise not be specified during CreateInstance
// - NOTE: As of now, this function is specifically used to handle the ResizeWithReflow operation.
//         It will need modification for other future users.
// Arguments:
// - pOtherCursor - The cursor to copy properties from
// Return Value:
// - <none>
void Cursor::CopyProperties(_In_ const Cursor* const pOtherCursor)
{
    // We shouldn't copy the position as it will be already rearranged by the resize operation.
    //this->_cPosition                    = pOtherCursor->_cPosition;

    this->_fHasMoved                    = pOtherCursor->_fHasMoved;
    this->_fIsVisible                   = pOtherCursor->_fIsVisible;
    this->_fIsOn                        = pOtherCursor->_fIsOn;
    this->_fIsDouble                    = pOtherCursor->_fIsDouble;
    this->_fBlinkingAllowed             = pOtherCursor->_fBlinkingAllowed;
    this->_fDelay                       = pOtherCursor->_fDelay;
    this->_fIsConversionArea            = pOtherCursor->_fIsConversionArea;

    // A resize operation should invalidate the delayed end of line status, so do not copy.
    //this->_fDelayedEolWrap              = pOtherCursor->_fDelayedEolWrap;
    //this->_coordDelayedAt               = pOtherCursor->_coordDelayedAt;

    this->_fDeferCursorRedraw           = pOtherCursor->_fDeferCursorRedraw;
    this->_fHaveDeferredCursorRedraw    = pOtherCursor->_fHaveDeferredCursorRedraw;

    // Size will be handled seperately in the resize operation.
    //this->_ulSize                       = pOtherCursor->_ulSize;
}

UINT Cursor::s_uCaretBlinkTime = INFINITE; // default to no blink

// Routine Description:
// - This routine is called when the timer in the console with the focus goes off.  It blinks the cursor.
// Arguments:
// - ScreenInfo - pointer to screen info structure.
// Return Value:
// - <none>
void Cursor::TimerRoutine(_In_ PSCREEN_INFORMATION const ScreenInfo)
{
    if ((g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS) == 0)
    {
        return;
    }

    // Update the cursor pos in USER so accessibility will work.
    if (this->HasMoved())
    {
        this->SetHasMoved(FALSE);

        CONSOLE_CARET_INFO ConsoleCaretInfo;
        ConsoleCaretInfo.hwnd = g_ciConsoleInformation.hWnd;
        ConsoleCaretInfo.rc.left = (this->GetPosition().X - ScreenInfo->GetBufferViewport().Left) * ScreenInfo->GetScreenFontSize().X;
        ConsoleCaretInfo.rc.top = (this->GetPosition().Y - ScreenInfo->GetBufferViewport().Top) * ScreenInfo->GetScreenFontSize().Y;
        ConsoleCaretInfo.rc.right = ConsoleCaretInfo.rc.left + ScreenInfo->GetScreenFontSize().X;
        ConsoleCaretInfo.rc.bottom = ConsoleCaretInfo.rc.top + ScreenInfo->GetScreenFontSize().Y;
        UserPrivApi::s_ConsoleControl(UserPrivApi::CONSOLECONTROL::ConsoleSetCaretInfo, &ConsoleCaretInfo, sizeof(ConsoleCaretInfo));

        // Send accessibility information
        {
            DWORD dwFlags = 0;

            // Flags is expected to be 2, 1, or 0. 2 in selecting (whether or not visible), 1 if just visible, 0 if invisible/noselect.
            if (IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_SELECTING))
            {
                dwFlags = CONSOLE_CARET_SELECTION;
            }
            else if (this->IsVisible())
            {
                dwFlags = CONSOLE_CARET_VISIBLE;
            }

            NotifyWinEvent(EVENT_CONSOLE_CARET, g_ciConsoleInformation.hWnd, dwFlags, PACKCOORD(this->GetPosition()));
        }
    }

    // If the DelayCursor flag has been set, wait one more tick before toggle.
    // This is used to guarantee the cursor is on for a finite period of time
    // after a move and off for a finite period of time after a WriteString.
    if (this->GetDelay())
    {
        this->SetDelay(FALSE);
        return;
    }

    // Don't blink the cursor for remote sessions.
    if ((!GetSystemMetrics(SM_CARETBLINKINGENABLED) || s_uCaretBlinkTime == -1 || (!this->IsBlinkingAllowed())) && this->IsOn())
    {
        return;
    }

    // Blink only if the cursor isn't turned off via the API
    if (this->IsVisible())
    {
        this->SetIsOn(!this->IsOn());
    }
}

void Cursor::DelayEOLWrap(_In_ const COORD coordDelayedAt)
{
    _coordDelayedAt = coordDelayedAt;
    _fDelayedEolWrap = TRUE;
}

void Cursor::ResetDelayEOLWrap()
{
    _coordDelayedAt = {0};
    _fDelayedEolWrap = FALSE;
}

COORD Cursor::GetDelayedAtPosition() const
{
    return _coordDelayedAt;
}

BOOL Cursor::IsDelayedEOLWrap() const
{
    return _fDelayedEolWrap;
}

void Cursor::StartDeferDrawing()
{
    _fDeferCursorRedraw = TRUE;
}

void Cursor::EndDeferDrawing()
{
    if (_fHaveDeferredCursorRedraw)
    {
        _RedrawCursorAlways();
    }
    _fDeferCursorRedraw = FALSE;
}

UINT Cursor::s_GetCaretBlinkTime()
{
    return s_uCaretBlinkTime;
}

void Cursor::s_UpdateSystemMetrics()
{
    // This can be -1 in a TS session
    s_uCaretBlinkTime = GetCaretBlinkTime();
}

void Cursor::s_SettingsChanged(_In_ HWND const hWnd)
{
    DWORD const dwCaretBlinkTime = GetCaretBlinkTime();

    if (dwCaretBlinkTime != s_uCaretBlinkTime)
    {
        s_KillCaretTimer(hWnd);
        s_uCaretBlinkTime = dwCaretBlinkTime;
        s_SetCaretTimer(hWnd);
    }
}

void Cursor::s_FocusEnd(_In_ HWND const hWnd)
{
    s_KillCaretTimer(hWnd);
}

void Cursor::s_FocusStart(_In_ HWND const hWnd)
{
    s_SetCaretTimer(hWnd);
}

// Routine Description:
// - If guCaretBlinkTime is -1, we don't want to blink the caret. However, we
//   need to make sure it gets drawn, so we'll set a short timer. When that
//   goes off, we'll hit CursorTimerRoutine, and it'll do the right thing if
//   guCaretBlinkTime is -1.
void Cursor::s_SetCaretTimer(_In_ HWND const hWnd)
{
    static const DWORD dwDefTimeout = 0x212;

    SetTimer(hWnd, CURSOR_TIMER, s_uCaretBlinkTime == -1 ? dwDefTimeout : s_uCaretBlinkTime, nullptr);
}

void Cursor::s_KillCaretTimer(_In_ HWND const hWnd)
{
    KillTimer(hWnd, CURSOR_TIMER);
}
