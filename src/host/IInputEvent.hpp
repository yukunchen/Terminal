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

#include <unordered_set>
#include <memory>

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

    virtual ~IInputEvent() = 0;
    virtual INPUT_RECORD ToInputRecord() const = 0;

    virtual InputEventType EventType() const = 0;
};

class KeyEvent : public IInputEvent
{
public:
    KeyEvent(_In_ const KEY_EVENT_RECORD& record);
    ~KeyEvent();
    INPUT_RECORD ToInputRecord() const;
    InputEventType EventType() const;

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
