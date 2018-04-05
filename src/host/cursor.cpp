/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "cursor.h"

#include "dbcs.h"
#include "handle.h"
#include "scrolling.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// Routine Description:
// - Constructor to set default properties for Cursor
// Arguments:
// - ulSize - The height of the cursor within this buffer
Cursor::Cursor(IAccessibilityNotifier *pNotifier, const ULONG ulSize) :
    _pAccessibilityNotifier(pNotifier),
    _ulSize(ulSize),
    _fHasMoved(false),
    _fIsVisible(true),
    _fIsOn(true),
    _fBlinkingAllowed(true),
    _fIsDouble(false),
    _fIsConversionArea(false),
    _fDelay(false),
    _fDelayedEolWrap(false),
    _fDeferCursorRedraw(false),
    _fHaveDeferredCursorRedraw(false),
    _hCaretBlinkTimer(INVALID_HANDLE_VALUE),
    _uCaretBlinkTime(INFINITE) // default to no blink
{
    _cPosition = {0};
    _coordDelayedAt = {0};

    _hCaretBlinkTimerQueue = CreateTimerQueue();

    _fUseColor = false;
    _color = s_InvertCursorColor;
    _cursorType = CursorType::Legacy;
}

Cursor::~Cursor()
{
    if (_hCaretBlinkTimerQueue)
    {
        DeleteTimerQueueEx(_hCaretBlinkTimerQueue, INVALID_HANDLE_VALUE);
    }
}

// Routine Description:
// - Creates a new instance of Cursor
// Arguments:
// - ulSize - The height of the cursor within this buffer
// - ppCursor - Pointer to accept the instance of the newly created Cursor
// Return Value:
// - Success or a relevant error status (usually out of memory).
[[nodiscard]]
NTSTATUS Cursor::CreateInstance(const ULONG ulSize, _Outptr_ Cursor ** const ppCursor)
{
    *ppCursor = nullptr;

    NTSTATUS status = STATUS_SUCCESS;

    IAccessibilityNotifier *pNotifier = ServiceLocator::LocateAccessibilityNotifier();
    status = NT_TESTNULL(pNotifier);

    if (NT_SUCCESS(status))
    {
        Cursor* pCursor = new Cursor(pNotifier, ulSize);
        status = NT_TESTNULL(pCursor);

        if (NT_SUCCESS(status))
        {
            *ppCursor = pCursor;
        }
        else
        {
            delete pCursor;
        }
    }

    return status;
}

COORD Cursor::GetPosition() const noexcept
{
    return _cPosition;
}

bool Cursor::HasMoved() const noexcept
{
    return _fHasMoved;
}

bool Cursor::IsVisible() const noexcept
{
    return _fIsVisible;
}

bool Cursor::IsOn() const noexcept
{
    return _fIsOn;
}

bool Cursor::IsBlinkingAllowed() const noexcept
{
    return _fBlinkingAllowed;
}

bool Cursor::IsDouble() const noexcept
{
    return _fIsDouble;
}

bool Cursor::IsDoubleWidth() const
{
    // Check with the current screen buffer to see if the character under the cursor is double-width.
    const auto& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    std::vector<OutputCell> cells = gci.CurrentScreenBuffer->ReadLine(_cPosition.Y, _cPosition.X, 1);
    return !!IsCharFullWidth(cells[0].GetCharData());
}

bool Cursor::IsConversionArea() const noexcept
{
    return _fIsConversionArea;
}

bool Cursor::IsPopupShown() const noexcept
{
    return _fIsPopupShown;
}

bool Cursor::GetDelay() const noexcept
{
    return _fDelay;
}

ULONG Cursor::GetSize() const noexcept
{
    return _ulSize;
}

void Cursor::SetHasMoved(const bool fHasMoved)
{
    _fHasMoved = fHasMoved;
}

void Cursor::SetIsVisible(const bool fIsVisible)
{
    _fIsVisible = fIsVisible;
    _RedrawCursor();
}

void Cursor::SetIsOn(const bool fIsOn)
{
    _fIsOn = fIsOn;
    _RedrawCursorAlways();
}

void Cursor::SetBlinkingAllowed(const bool fBlinkingAllowed)
{
    _fBlinkingAllowed = fBlinkingAllowed;
    _RedrawCursorAlways();
}

void Cursor::SetIsDouble(const bool fIsDouble)
{
    _fIsDouble = fIsDouble;
    _RedrawCursor();
}

void Cursor::SetIsConversionArea(const bool fIsConversionArea)
{
    // Functionally the same as "Hide cursor"
    // Never called with TRUE, it's only used in the creation of a
    //      ConversionAreaInfo, and never changed after that.
    _fIsConversionArea = fIsConversionArea;
    _RedrawCursorAlways();
}

void Cursor::SetIsPopupShown(const bool fIsPopupShown)
{
    // Functionally the same as "Hide cursor"
    _fIsPopupShown = fIsPopupShown;
    _RedrawCursorAlways();
}

void Cursor::SetDelay(const bool fDelay)
{
    _fDelay = fDelay;
}

void Cursor::SetSize(const ULONG ulSize)
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
    if (ServiceLocator::LocateGlobals().pRender != nullptr)
    {
        // Always trigger update of the cursor as one character width
        ServiceLocator::LocateGlobals().pRender->TriggerRedrawCursor(&_cPosition);

        // In case of a double width character, we need to invalidate the spot one to the right of the cursor as well.
        try
        {
            if (IsDoubleWidth())
            {
                COORD cExtra = _cPosition;
                cExtra.X++;
                ServiceLocator::LocateGlobals().pRender->TriggerRedrawCursor(&cExtra);
            }
        }
        CATCH_LOG();
    }
}

void Cursor::SetPosition(const COORD cPosition)
{
    _RedrawCursor();
    _cPosition.X = cPosition.X;
    _cPosition.Y = cPosition.Y;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::SetXPosition(const int NewX)
{
    _RedrawCursor();
    _cPosition.X = (SHORT)NewX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::SetYPosition(const int NewY)
{
    _RedrawCursor();
    _cPosition.Y = (SHORT)NewY;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::IncrementXPosition(const int DeltaX)
{
    _RedrawCursor();
    _cPosition.X += (SHORT)DeltaX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::IncrementYPosition(const int DeltaY)
{
    _RedrawCursor();
    _cPosition.Y += (SHORT)DeltaY;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::DecrementXPosition(const int DeltaX)
{
    _RedrawCursor();
    _cPosition.X -= (SHORT)DeltaX;
    _RedrawCursor();
    ResetDelayEOLWrap();
}

void Cursor::DecrementYPosition(const int DeltaY)
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
void Cursor::CopyProperties(const Cursor* const pOtherCursor)
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

    // This property is set only once on console startup, and might change
    // during the lifetime of the console, but is not set again anywhere when a
    // cursor is replaced during reflow. This wasn't necessary when this
    // property and the cursor timer were static.
    this->_uCaretBlinkTime              = pOtherCursor->_uCaretBlinkTime;

    // If there's a timer on the other one, then it was active for blinking.
    // Make sure we have a timer on our side too.
    if (pOtherCursor->_hCaretBlinkTimer != INVALID_HANDLE_VALUE)
    {
        this->SetCaretTimer();
    }
}

// Routine Description:
// - This routine is called when the timer in the console with the focus goes off.  It blinks the cursor.
// Arguments:
// - ScreenInfo - pointer to screen info structure.
// Return Value:
// - <none>
void Cursor::TimerRoutine(_In_ PSCREEN_INFORMATION const ScreenInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    if (pWindow != nullptr)
    {
        pWindow->SetWindowHasMoved(false);
    }

    if (!IsFlagSet(gci.Flags, CONSOLE_HAS_FOCUS))
    {
        goto DoScroll;
    }

    // Update the cursor pos in USER so accessibility will work.
    if (this->HasMoved())
    {
        this->SetHasMoved(false);

        RECT rc;
        rc.left = (this->GetPosition().X - ScreenInfo->GetBufferViewport().Left) * ScreenInfo->GetScreenFontSize().X;
        rc.top = (this->GetPosition().Y - ScreenInfo->GetBufferViewport().Top) * ScreenInfo->GetScreenFontSize().Y;
        rc.right = rc.left + ScreenInfo->GetScreenFontSize().X;
        rc.bottom = rc.top + ScreenInfo->GetScreenFontSize().Y;

        _pAccessibilityNotifier->NotifyConsoleCaretEvent(rc);

        // Send accessibility information
        {
            IAccessibilityNotifier::ConsoleCaretEventFlags flags = IAccessibilityNotifier::ConsoleCaretEventFlags::CaretInvisible;

            // Flags is expected to be 2, 1, or 0. 2 in selecting (whether or not visible), 1 if just visible, 0 if invisible/noselect.
            if (IsFlagSet(gci.Flags, CONSOLE_SELECTING))
            {
                flags = IAccessibilityNotifier::ConsoleCaretEventFlags::CaretSelection;
            }
            else if (this->IsVisible())
            {
                flags = IAccessibilityNotifier::ConsoleCaretEventFlags::CaretVisible;
            }

            _pAccessibilityNotifier->NotifyConsoleCaretEvent(flags, PACKCOORD(this->GetPosition()));
        }
    }

    // If the DelayCursor flag has been set, wait one more tick before toggle.
    // This is used to guarantee the cursor is on for a finite period of time
    // after a move and off for a finite period of time after a WriteString.
    if (this->GetDelay())
    {
        this->SetDelay(false);
        goto DoScroll;
    }

    // Don't blink the cursor for remote sessions.
    if ((!ServiceLocator::LocateSystemConfigurationProvider()->IsCaretBlinkingEnabled() ||
         _uCaretBlinkTime == -1 ||
        (!this->IsBlinkingAllowed())) &&
       this->IsOn())
    {
        goto DoScroll;
    }

    // Blink only if the cursor isn't turned off via the API
    if (this->IsVisible())
    {
        this->SetIsOn(!this->IsOn());
    }

DoScroll:
    Scrolling::s_ScrollIfNecessary(ScreenInfo);
}

void Cursor::DelayEOLWrap(const COORD coordDelayedAt)
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

void Cursor::UpdateSystemMetrics()
{
    // This can be -1 in a TS session
    _uCaretBlinkTime = ServiceLocator::LocateSystemConfigurationProvider()->GetCaretBlinkTime();
}

void Cursor::SettingsChanged()
{
    DWORD const dwCaretBlinkTime = ServiceLocator::LocateSystemConfigurationProvider()->GetCaretBlinkTime();

    if (dwCaretBlinkTime != _uCaretBlinkTime)
    {
        KillCaretTimer();
        _uCaretBlinkTime = dwCaretBlinkTime;
        SetCaretTimer();
    }
}

void Cursor::FocusEnd()
{
    KillCaretTimer();
}

void Cursor::FocusStart()
{
    SetCaretTimer();
}

void CALLBACK CursorTimerRoutineWrapper(_In_ PVOID /* lpParam */, _In_ BOOL /* TimerOrWaitFired */)
{
    // Suppose the following sequence of events takes place:
    //
    // 1. The user resizes the console;
    // 2. The console acquires the console lock;
    // 3. The current SCREEN_INFORMATION instance is deleted;
    // 4. This causes the current Cursor instance to be deleted, too;
    // 5. The Cursor's destructor is called;
    // => Somewhere between 1 and 5, the timer fires:
    //    Timer queue timer callbacks execute asynchronously with respect to
    //    the UI thread under which the numbered steps are taking place.
    //    Because the callback touches console state, it needs to acquire the
    //    console lock. But what if the timer callback fires at just the right
    //    time such that 2 has already acquired the lock?
    // 6. The Cursor's destructor deletes the timer queue and thereby destroys
    //    the timer queue timer used for blinking. However, because this
    //    timer's callback modifies console state, it is prudent to not
    //    continue the destruction if the callback has already started but has
    //    not yet finished. Therefore, the destructor waits for the callback to
    //    finish executing.
    // => Meanwhile, the callback just happens to be stuck waiting for the
    //    console lock acquired in step 2. Since the destructor is waiting on
    //    the callback to complete, and the callback is waiting on the lock,
    //    which will only be released way after the Cursor instance is deleted,
    //    the console has now deadlocked.
    //
    // As a solution, skip the blink if the console lock is already being held.
    // Note that critical sections to not have a waitable synchronization
    // object unless there readily is contention on it. As a result, if we
    // wanted to wait until the lock became available under the condition of
    // not being destroyed, things get too complicated.
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    if (gci.TryLockConsole() != FALSE)
    {
        Cursor *cursor = gci.CurrentScreenBuffer->TextInfo->GetCursor();
        cursor->TimerRoutine(gci.CurrentScreenBuffer);

        UnlockConsole();
    }
}

// Routine Description:
// - If guCaretBlinkTime is -1, we don't want to blink the caret. However, we
//   need to make sure it gets drawn, so we'll set a short timer. When that
//   goes off, we'll hit CursorTimerRoutine, and it'll do the right thing if
//   guCaretBlinkTime is -1.
void Cursor::SetCaretTimer()
{
    static const DWORD dwDefTimeout = 0x212;

    KillCaretTimer();

    if (_hCaretBlinkTimer == INVALID_HANDLE_VALUE)
    {
        BOOL  bRet = TRUE;
        DWORD dwEffectivePeriod = _uCaretBlinkTime == -1 ? dwDefTimeout : _uCaretBlinkTime;

        bRet = CreateTimerQueueTimer(&_hCaretBlinkTimer,
                                     _hCaretBlinkTimerQueue,
                                     (WAITORTIMERCALLBACKFUNC)CursorTimerRoutineWrapper,
                                     this,
                                     dwEffectivePeriod,
                                     dwEffectivePeriod,
                                     0);

        LOG_LAST_ERROR_IF_FALSE(bRet);
    }
}

void Cursor::KillCaretTimer()
{
    if (_hCaretBlinkTimer != INVALID_HANDLE_VALUE)
    {
        BOOL bRet = TRUE;

        bRet = DeleteTimerQueueTimer(_hCaretBlinkTimerQueue,
                                     _hCaretBlinkTimer,
                                     NULL);

        // According to https://msdn.microsoft.com/en-us/library/windows/desktop/ms682569(v=vs.85).aspx
        // A failure to delete the timer with the LastError being ERROR_IO_PENDING means that the timer is
        // currently in use and will get cleaned up when released. Delete should not be called again.
        // We treat that case as a success.
        if (bRet == FALSE && GetLastError() != ERROR_IO_PENDING)
        {
            LOG_LAST_ERROR();
        }
        else
        {
            _hCaretBlinkTimer = INVALID_HANDLE_VALUE;
        }
    }
}

const CursorType Cursor::GetType() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetCursorType();
}

const bool Cursor::IsUsingColor() const
{
    return GetColor() != INVALID_COLOR;
}

const COLORREF Cursor::GetColor() const
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    return gci.GetCursorColor();
}

void Cursor::SetColor(const unsigned int color)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.SetCursorColor(color);
}

void Cursor::SetType(const CursorType type)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.SetCursorType(type);
}
