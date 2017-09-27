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
    static HRESULT ToInputRecords(_Inout_ const std::deque<std::unique_ptr<IInputEvent>>& events,
                                  _Out_writes_(cRecords) INPUT_RECORD* const Buffer,
                                  _In_ const size_t cRecords);

    virtual ~IInputEvent() = 0;
    virtual INPUT_RECORD ToInputRecord() const = 0;

    virtual InputEventType EventType() const = 0;

};

class KeyEvent : public IInputEvent
{
public:
    KeyEvent(_In_ const KEY_EVENT_RECORD& record);
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

class MouseEvent : public IInputEvent
{
public:
    MouseEvent(_In_ const MOUSE_EVENT_RECORD& record);
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
    ~MenuEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

    UINT _commandId;
};

class FocusEvent : public IInputEvent
{
public:
    FocusEvent(_In_ const FOCUS_EVENT_RECORD& record);
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
