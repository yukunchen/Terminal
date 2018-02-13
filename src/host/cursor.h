/*++
Copyright (c) Microsoft Corporation

Module Name:
- cursor.h

Abstract:
- This file implements the NT console server cursor routines.

Author:
- Therese Stowell (ThereseS) 5-Dec-1990

Revision History:
- Grouped into class and items made private. (MiNiksa, 2014)
--*/

#pragma once

#include "../inc/conattrs.hpp"
#include "../interactivity/inc/IConsoleWindow.hpp"
#include "../interactivity/inc/IAccessibilityNotifier.hpp"

// the following values are used to create the textmode cursor.
#define CURSOR_SMALL_SIZE 25    // large enough to be one pixel on a six pixel font
class SCREEN_INFORMATION;
typedef SCREEN_INFORMATION *PSCREEN_INFORMATION;

class Cursor sealed
{
public:

    static const unsigned int s_InvertCursorColor = INVALID_COLOR;

    Cursor(_In_ Microsoft::Console::Interactivity::IAccessibilityNotifier *pNotifier, _In_ ULONG const ulSize);
    static NTSTATUS CreateInstance(_In_ ULONG const ulSize, _Outptr_ Cursor ** const ppCursor);
    ~Cursor();

    void UpdateSystemMetrics();
    void SettingsChanged();
    void FocusStart();
    void FocusEnd();

    const BOOLEAN HasMoved() const;
    const BOOLEAN IsVisible() const;
    const BOOLEAN IsOn() const;
    const BOOLEAN IsBlinkingAllowed() const;
    const BOOLEAN IsDouble() const;
    const BOOLEAN IsDoubleWidth() const;
    const BOOLEAN IsConversionArea() const;
    const BOOLEAN IsPopupShown() const;
    const BOOLEAN GetDelay() const;
    const ULONG GetSize() const;
    const COORD GetPosition() const;

    const CursorType GetCursorType() const;
    const bool IsUsingColor() const;
    const COLORREF GetColor() const;

    void StartDeferDrawing();
    void EndDeferDrawing();

    void SetHasMoved(_In_ BOOLEAN const fHasMoved);
    void SetIsVisible(_In_ BOOLEAN const fIsVisible);
    void SetIsOn(_In_ BOOLEAN const fIsOn);
    void SetBlinkingAllowed(_In_ BOOLEAN const fIsOn);
    void SetIsDouble(_In_ BOOLEAN const fIsDouble);
    void SetIsConversionArea(_In_ BOOLEAN const fIsConversionArea);
    void SetIsPopupShown(_In_ BOOLEAN const fIsPopupShown);
    void SetDelay(_In_ BOOLEAN const fDelay);
    void SetSize(_In_ ULONG const ulSize);

    void SetPosition(_In_ COORD const cPosition);
    void SetXPosition(_In_ int const NewX);
    void SetYPosition(_In_ int const NewY);
    void IncrementXPosition(_In_ int const DeltaX);
    void IncrementYPosition(_In_ int const DeltaY);
    void DecrementXPosition(_In_ int const DeltaX);
    void DecrementYPosition(_In_ int const DeltaY);

    void TimerRoutine(_In_ PSCREEN_INFORMATION const ScreenInfo);
    void CopyProperties(_In_ const Cursor* const pOtherCursor);

    void DelayEOLWrap(_In_ const COORD coordDelayedAt);
    void ResetDelayEOLWrap();
    COORD GetDelayedAtPosition() const;
    BOOL IsDelayedEOLWrap() const;

    void SetColor(_In_ const unsigned int color);
    void SetType(_In_ const CursorType type);

private:
    Microsoft::Console::Interactivity::IAccessibilityNotifier *_pAccessibilityNotifier;

    //TODO: seperate the rendering and text placement

    // NOTE: If you are adding a property here, go add it to CopyProperties.

    COORD _cPosition;   // current position on screen (in screen buffer coords).

    BOOLEAN _fHasMoved;
    BOOLEAN _fIsVisible;  // whether cursor is visible (set only through the API)
    BOOLEAN _fIsOn;   // whether blinking cursor is on or not
    BOOLEAN _fIsDouble;   // whether the cursor size should be doubled
    BOOLEAN _fBlinkingAllowed; //Whether or not the cursor is allowed to blink at all. only set through VT (^[[?12h/l)
    BOOLEAN _fDelay;    // don't blink scursor on next timer message
    BOOLEAN _fIsConversionArea; // is attached to a conversion area so it doesn't actually need to display the cursor.
    BOOLEAN _fIsPopupShown; // if a popup is being shown, turn off, stop blinking.

    BOOLEAN _fDelayedEolWrap;    // don't wrap at EOL till the next char comes in.
    COORD _coordDelayedAt;   // coordinate the EOL wrap was delayed at.

    BOOLEAN _fDeferCursorRedraw; // whether we should defer redrawing the cursor or not
    BOOLEAN _fHaveDeferredCursorRedraw; // have we been asked to redraw the cursor while it was being deferred?

    ULONG _ulSize;

    // These use Timer Queues:
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms687003(v=vs.85).aspx
    HANDLE _hCaretBlinkTimer; // timer used to peridically blink the cursor
    HANDLE _hCaretBlinkTimerQueue; // timer queue where the blink timer lives

    void SetCaretTimer();
    void KillCaretTimer();

    void _RedrawCursor();
    void _RedrawCursorAlways();

    UINT _uCaretBlinkTime;

    CursorType _cursorType;
    bool _fUseColor;
    COLORREF _color;
};
