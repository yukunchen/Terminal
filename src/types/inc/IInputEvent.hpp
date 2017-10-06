/*++
Copyright (c) Microsoft Corporation

Module Name:
- IInputEvent.hpp

Abstract:
- Internal representation of public INPUT_RECORD struct.

Author:
- Austin Diviness (AustDi) 18-Aug-2017
--*/

#pragma once

#include <winconp.h>
#include <wtypes.h>

#include <unordered_set>
#include <memory>
#include <deque>


enum class InputEventType
{
    KeyEvent,
    MouseEvent,
    WindowBufferSizeEvent,
    MenuEvent,
    FocusEvent
};

class IInputEvent
{
public:
    static std::unique_ptr<IInputEvent> Create(_In_ const INPUT_RECORD& record);
    static std::deque<std::unique_ptr<IInputEvent>> Create(_In_reads_(cRecords) const INPUT_RECORD* const pRecords,
                                                           _In_ const size_t cRecords);
    static std::deque<std::unique_ptr<IInputEvent>> Create(_In_ const std::deque<INPUT_RECORD>& records);

    static HRESULT ToInputRecords(_Inout_ const std::deque<std::unique_ptr<IInputEvent>>& events,
                                  _Out_writes_(cRecords) INPUT_RECORD* const Buffer,
                                  _In_ const size_t cRecords);

    virtual ~IInputEvent() = 0;
    virtual INPUT_RECORD ToInputRecord() const = 0;

    virtual InputEventType EventType() const = 0;

};

#define ALT_PRESSED     (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTRL_PRESSED    (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define MOD_PRESSED     (SHIFT_PRESSED | ALT_PRESSED | CTRL_PRESSED)

#define CTRL_BUT_NOT_ALT(n) \
        (((n) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && \
        !((n) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))

// Note taken from VkKeyScan docs (https://msdn.microsoft.com/en-us/library/windows/desktop/ms646329(v=vs.85).aspx):
// For keyboard layouts that use the right-hand ALT key as a shift key
// (for example, the French keyboard layout), the shift state is
// represented by the value 6, because the right-hand ALT key is
// converted internally into CTRL+ALT.
struct VkKeyScanModState
{
    static const byte None = 0;
    static const byte ShiftPressed = 1;
    static const byte CtrlPressed = 2;
    static const byte ShiftAndCtrlPressed = ShiftPressed | CtrlPressed;
    static const byte AltPressed = 4;
    static const byte ShiftAndAltPressed = ShiftPressed | AltPressed;
    static const byte CtrlAndAltPressed = CtrlPressed | AltPressed;
    static const byte ModPressed = ShiftPressed | CtrlPressed | AltPressed;
};


class KeyEvent : public IInputEvent
{
public:
    KeyEvent(_In_ const KEY_EVENT_RECORD& record);
    KeyEvent(_In_ const int keyDown,
             _In_ const WORD repeatCount,
             _In_ const WORD virtualKeyCode,
             _In_ const WORD virtualScanCode,
             _In_ const wchar_t charData,
             _In_ const DWORD activeModifierKeys);
    KeyEvent();
    ~KeyEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    bool IsPauseKey() const;
    bool IsCommandLineEditingKey() const;
    bool IsCommandLinePopupKey() const;

    const ExtKeySubst* const ParseEditKeyInfo();
    const ExtKeySubst* const GetKeySubst() const;

    int _keyDown;
    WORD _repeatCount;
    WORD _virtualKeyCode;
    WORD _virtualScanCode;
    wchar_t _charData;
    DWORD _activeModifierKeys;
};

bool operator==(const KeyEvent& a, const KeyEvent& b);

class MouseEvent : public IInputEvent
{
public:
    MouseEvent(_In_ const MOUSE_EVENT_RECORD& record);
    MouseEvent(_In_ const COORD mousePosition,
               _In_ const DWORD buttonState,
               _In_ const DWORD activeModifierKeys,
               _In_ const DWORD eventFlags);
    ~MouseEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    COORD _mousePosition;
    DWORD _buttonState;
    DWORD _activeModifierKeys;
    DWORD _eventFlags;
};

class WindowBufferSizeEvent : public IInputEvent
{
public:
    WindowBufferSizeEvent(_In_ const WINDOW_BUFFER_SIZE_RECORD& record);
    WindowBufferSizeEvent(_In_ const COORD);
    ~WindowBufferSizeEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    COORD _size;
};

class MenuEvent : public IInputEvent
{
public:
    MenuEvent(_In_ const MENU_EVENT_RECORD& record);
    MenuEvent(_In_ const UINT commandId);
    ~MenuEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    UINT _commandId;
};

class FocusEvent : public IInputEvent
{
public:
    FocusEvent(_In_ const FOCUS_EVENT_RECORD& record);
    FocusEvent(_In_ const int setFocus);
    ~FocusEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    int _setFocus;
};

enum class ControlKeyState
{
    RightAlt,
    LeftAlt,
    RightCtrl,
    LeftCtrl,
    Shift,
    NumLock,
    ScrollLock,
    CapsLock,
    EnhancedKey,
    NlsDbcsChar,
    NlsAlphanumeric,
    NlsKatakana,
    NlsHiragana,
    NlsRoman,
    NlsImeConversion,
    NlsImeDisable
};

std::unordered_set<ControlKeyState> ExpandControlKeyStateFlags(_In_ const DWORD flags);
