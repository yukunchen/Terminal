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
    virtual INPUT_RECORD ToInputRecord() const noexcept = 0;

    virtual InputEventType EventType() const noexcept = 0;

};

#define ALT_PRESSED     (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)
#define CTRL_PRESSED    (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)
#define MOD_PRESSED     (SHIFT_PRESSED | ALT_PRESSED | CTRL_PRESSED)

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

enum class ModifierKeyState
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
    AltNumpad,
    NlsImeDisable,
    ENUM_COUNT // must be the last element in the enum class
};

std::unordered_set<ModifierKeyState> FromVkKeyScan(_In_ const short vkKeyScanFlags);
std::unordered_set<ModifierKeyState> FromConsoleControlKeyFlags(_In_ const DWORD flags);
DWORD ToConsoleControlKeyFlag(_In_ const ModifierKeyState modifierKey) noexcept;

class KeyEvent : public IInputEvent
{
public:
    KeyEvent(_In_ const KEY_EVENT_RECORD& record);
    KeyEvent(_In_ const bool keyDown,
             _In_ const WORD repeatCount,
             _In_ const WORD virtualKeyCode,
             _In_ const WORD virtualScanCode,
             _In_ const wchar_t charData,
             _In_ const DWORD activeModifierKeys);
    KeyEvent();
    KeyEvent(const KeyEvent& keyEvent) = default;
    KeyEvent(KeyEvent&& keyEvent) = default;
    ~KeyEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    bool IsShiftPressed() const noexcept;
    bool IsAltPressed() const noexcept;
    bool IsCtrlPressed() const noexcept;
    bool IsAltGrPressed() const noexcept;
    bool IsModifierPressed() const noexcept;
    bool IsCursorKey() const noexcept;
    bool IsAltNumpadSet() const noexcept;

    bool IsKeyDown() const noexcept;
    void SetKeyDown(_In_ const bool keyDown) noexcept;

    WORD GetRepeatCount() const noexcept;
    void SetRepeatCount(_In_ const WORD repeatCount) noexcept;

    WORD GetVirtualKeyCode() const noexcept;
    void SetVirtualKeyCode(_In_ const WORD virtualKeyCode) noexcept;

    WORD GetVirtualScanCode() const noexcept;
    void SetVirtualScanCode(_In_ const WORD virtualScanCode) noexcept;

    wchar_t GetCharData() const noexcept;
    void SetCharData(_In_ const wchar_t character) noexcept;

    DWORD GetActiveModifierKeys() const noexcept;
    void SetActiveModifierKeys(_In_ const DWORD activeModifierKeys) noexcept;
    void DeactivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept;
    void ActivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept;
    bool DoActiveModifierKeysMatch(_In_ const std::unordered_set<ModifierKeyState>& consoleModifiers) noexcept;

private:
    bool _keyDown;
    WORD _repeatCount;
    WORD _virtualKeyCode;
    WORD _virtualScanCode;
    wchar_t _charData;
    DWORD _activeModifierKeys;

    friend bool operator==(const KeyEvent& a, const KeyEvent& b) noexcept;
};

bool operator==(const KeyEvent& a, const KeyEvent& b) noexcept;

class MouseEvent : public IInputEvent
{
public:
    MouseEvent(_In_ const MOUSE_EVENT_RECORD& record);
    MouseEvent(_In_ const COORD mousePosition,
               _In_ const DWORD buttonState,
               _In_ const DWORD activeModifierKeys,
               _In_ const DWORD eventFlags);
    MouseEvent(const MouseEvent& mouseEvent) = default;
    MouseEvent(MouseEvent&& mouseEvent) = default;
    ~MouseEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    bool IsMouseMoveEvent() const noexcept;

    COORD GetPosition() const noexcept;
    void SetPosition(_In_ const COORD position) noexcept;

    DWORD GetButtonState() const noexcept;
    void SetButtonState(_In_ const DWORD buttonState) noexcept;

    DWORD GetActiveModifierKeys() const noexcept;
    void SetActiveModifierKeys(_In_ const DWORD activeModifierKeys) noexcept;

    DWORD GetEventFlags() const noexcept;
    void SetEventFlags(_In_ const DWORD eventFlags) noexcept;

private:
    COORD _position;
    DWORD _buttonState;
    DWORD _activeModifierKeys;
    DWORD _eventFlags;
};

class WindowBufferSizeEvent : public IInputEvent
{
public:
    WindowBufferSizeEvent(_In_ const WINDOW_BUFFER_SIZE_RECORD& record);
    WindowBufferSizeEvent(_In_ const COORD);
    WindowBufferSizeEvent(const WindowBufferSizeEvent& event) = default;
    WindowBufferSizeEvent(WindowBufferSizeEvent&& event) = default;
    ~WindowBufferSizeEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    COORD GetSize() const noexcept;
    void SetSize(_In_ const COORD size) noexcept;

private:
    COORD _size;
};

class MenuEvent : public IInputEvent
{
public:
    MenuEvent(_In_ const MENU_EVENT_RECORD& record);
    MenuEvent(_In_ const UINT commandId);
    MenuEvent(const MenuEvent& menuEvent) = default;
    MenuEvent(MenuEvent&& menuEvent) = default;
    ~MenuEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    UINT GetCommandId() const noexcept;
    void SetCommandId(_In_ const UINT commandId) noexcept;

private:
    UINT _commandId;
};

class FocusEvent : public IInputEvent
{
public:
    FocusEvent(_In_ const FOCUS_EVENT_RECORD& record);
    FocusEvent(_In_ const bool focus);
    FocusEvent(const FocusEvent& focusEvent) = default;
    FocusEvent(FocusEvent&& focusEvent) = default;
    ~FocusEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    bool GetFocus() const noexcept;
    void SetFocus(_In_ const bool focus) noexcept;

private:
    bool _focus;
};

std::unordered_set<ModifierKeyState> ExpandModifierKeyStateFlags(_In_ const DWORD flags);
