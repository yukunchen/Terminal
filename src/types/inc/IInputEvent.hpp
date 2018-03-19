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

#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\common.h>
#include <wil\resource.h>

#include <winconp.h>
#include <wtypes.h>

#include <unordered_set>
#include <memory>
#include <deque>
#include <ostream>

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

    [[nodiscard]]
    static HRESULT ToInputRecords(_Inout_ const std::deque<std::unique_ptr<IInputEvent>>& events,
                                  _Out_writes_(cRecords) INPUT_RECORD* const Buffer,
                                  _In_ const size_t cRecords);

    virtual ~IInputEvent() = 0;
    virtual INPUT_RECORD ToInputRecord() const noexcept = 0;

    virtual InputEventType EventType() const noexcept = 0;

#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const IInputEvent* const pEvent);
#endif
};

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const IInputEvent* pEvent);
#endif

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
    constexpr KeyEvent(_In_ const KEY_EVENT_RECORD& record) :
        _keyDown{ !!record.bKeyDown },
        _repeatCount{ record.wRepeatCount },
        _virtualKeyCode{ record.wVirtualKeyCode },
        _virtualScanCode{ record.wVirtualScanCode },
        _charData{ record.uChar.UnicodeChar },
        _activeModifierKeys{ record.dwControlKeyState }
    {
    }

    constexpr KeyEvent(_In_ const bool keyDown,
                       _In_ const WORD repeatCount,
                       _In_ const WORD virtualKeyCode,
                       _In_ const WORD virtualScanCode,
                       _In_ const wchar_t charData,
                       _In_ const DWORD activeModifierKeys) :
        _keyDown{ keyDown },
        _repeatCount{ repeatCount },
        _virtualKeyCode{ virtualKeyCode },
        _virtualScanCode{ virtualScanCode },
        _charData{ charData },
        _activeModifierKeys{ activeModifierKeys }
    {
    }

    constexpr KeyEvent() :
        _keyDown{ 0 },
        _repeatCount{ 0 },
        _virtualKeyCode{ 0 },
        _virtualScanCode{ 0 },
        _charData { 0 },
        _activeModifierKeys{ 0 }
    {
    }

    KeyEvent(const KeyEvent& keyEvent) = default;
    KeyEvent(KeyEvent&& keyEvent) = default;
    ~KeyEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    constexpr bool IsShiftPressed() const noexcept
    {
        return IsFlagSet(_activeModifierKeys, SHIFT_PRESSED);
    }

    constexpr bool IsAltPressed() const noexcept
    {
        return IsAnyFlagSet(_activeModifierKeys, ALT_PRESSED);
    }

    constexpr bool IsCtrlPressed() const noexcept
    {
        return IsAnyFlagSet(_activeModifierKeys, CTRL_PRESSED);
    }

    constexpr bool IsAltGrPressed() const noexcept
    {
        return IsFlagSet(_activeModifierKeys, LEFT_CTRL_PRESSED) &&
               IsFlagSet(_activeModifierKeys, RIGHT_ALT_PRESSED);
    }

    constexpr bool IsModifierPressed() const noexcept
    {
        return IsAnyFlagSet(_activeModifierKeys, MOD_PRESSED);
    }

    constexpr bool IsCursorKey() const noexcept
    {
        // true iff vk in [End, Home, Left, Up, Right, Down]
        return (_virtualKeyCode >= VK_END) && (_virtualKeyCode <= VK_DOWN);
    }

    constexpr bool IsAltNumpadSet() const noexcept
    {
        return IsFlagSet(_activeModifierKeys, ALTNUMPAD_BIT);
    }

    constexpr bool IsKeyDown() const noexcept
    {
        return _keyDown;
    }

    constexpr bool IsPauseKey() const noexcept
    {
        return (_virtualKeyCode == VK_PAUSE);
    }

    constexpr WORD GetRepeatCount() const noexcept
    {
        return _repeatCount;
    }

    constexpr WORD GetVirtualKeyCode() const noexcept
    {
        return _virtualKeyCode;
    }

    constexpr WORD GetVirtualScanCode() const noexcept
    {
        return _virtualScanCode;
    }

    constexpr wchar_t GetCharData() const noexcept
    {
        return _charData;
    }

    constexpr DWORD GetActiveModifierKeys() const noexcept
    {
        return _activeModifierKeys;
    }

    void SetKeyDown(_In_ const bool keyDown) noexcept;
    void SetRepeatCount(_In_ const WORD repeatCount) noexcept;
    void SetVirtualKeyCode(_In_ const WORD virtualKeyCode) noexcept;
    void SetVirtualScanCode(_In_ const WORD virtualScanCode) noexcept;
    void SetCharData(_In_ const wchar_t character) noexcept;

    void SetActiveModifierKeys(_In_ const DWORD activeModifierKeys) noexcept;
    void DeactivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept;
    void ActivateModifierKey(_In_ const ModifierKeyState modifierKey) noexcept;
    bool DoActiveModifierKeysMatch(_In_ const std::unordered_set<ModifierKeyState>& consoleModifiers) const noexcept;
    bool IsCommandLineEditingKey() const noexcept;
    bool IsCommandLinePopupKey() const noexcept;

private:
    bool _keyDown;
    WORD _repeatCount;
    WORD _virtualKeyCode;
    WORD _virtualScanCode;
    wchar_t _charData;
    DWORD _activeModifierKeys;

    friend constexpr bool operator==(const KeyEvent& a, const KeyEvent& b) noexcept;
#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const KeyEvent* const pKeyEvent);
#endif
};

constexpr bool operator==(const KeyEvent& a, const KeyEvent& b) noexcept
{
    return (a._keyDown == b._keyDown &&
            a._repeatCount == b._repeatCount &&
            a._virtualKeyCode == b._virtualKeyCode &&
            a._virtualScanCode == b._virtualScanCode &&
            a._charData == b._charData &&
            a._activeModifierKeys == b._activeModifierKeys);
}

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const KeyEvent* const pKeyEvent);
#endif

class MouseEvent : public IInputEvent
{
public:
    constexpr MouseEvent(_In_ const MOUSE_EVENT_RECORD& record) :
        _position{ record.dwMousePosition },
        _buttonState{ record.dwButtonState },
        _activeModifierKeys{ record.dwControlKeyState },
        _eventFlags{ record.dwEventFlags }
    {
    }

    constexpr MouseEvent(_In_ const COORD position,
                         _In_ const DWORD buttonState,
                         _In_ const DWORD activeModifierKeys,
                         _In_ const DWORD eventFlags) :
        _position{ position },
        _buttonState{ buttonState },
        _activeModifierKeys{ activeModifierKeys },
        _eventFlags{ eventFlags }
    {
    }

    MouseEvent(const MouseEvent& mouseEvent) = default;
    MouseEvent(MouseEvent&& mouseEvent) = default;
    ~MouseEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    constexpr bool IsMouseMoveEvent() const noexcept
    {
        return _eventFlags == MOUSE_MOVED;
    }

    constexpr COORD GetPosition() const noexcept
    {
        return _position;
    }

    constexpr DWORD GetButtonState() const noexcept
    {
        return _buttonState;
    }

    constexpr DWORD GetActiveModifierKeys() const noexcept
    {
        return _activeModifierKeys;
    }

    constexpr DWORD GetEventFlags() const noexcept
    {
        return _eventFlags;
    }

    void SetPosition(_In_ const COORD position) noexcept;
    void SetButtonState(_In_ const DWORD buttonState) noexcept;
    void SetActiveModifierKeys(_In_ const DWORD activeModifierKeys) noexcept;
    void SetEventFlags(_In_ const DWORD eventFlags) noexcept;

private:
    COORD _position;
    DWORD _buttonState;
    DWORD _activeModifierKeys;
    DWORD _eventFlags;

#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const MouseEvent* const pMouseEvent);
#endif
};

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const MouseEvent* const pMouseEvent);
#endif

class WindowBufferSizeEvent : public IInputEvent
{
public:
    constexpr WindowBufferSizeEvent(_In_ const WINDOW_BUFFER_SIZE_RECORD& record) :
        _size{ record.dwSize }
    {
    }

    constexpr WindowBufferSizeEvent(_In_ const COORD size) :
        _size{ size }
    {
    }

    WindowBufferSizeEvent(const WindowBufferSizeEvent& event) = default;
    WindowBufferSizeEvent(WindowBufferSizeEvent&& event) = default;
    ~WindowBufferSizeEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    constexpr COORD GetSize() const noexcept
    {
        return _size;
    }

    void SetSize(_In_ const COORD size) noexcept;

private:
    COORD _size;

#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const WindowBufferSizeEvent* const pEvent);
#endif
};

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const WindowBufferSizeEvent* const pEvent);
#endif

class MenuEvent : public IInputEvent
{
public:
    constexpr MenuEvent(_In_ const MENU_EVENT_RECORD& record) :
        _commandId{ record.dwCommandId }
    {
    }

    constexpr MenuEvent(_In_ const UINT commandId) :
        _commandId{ commandId }
    {
    }

    MenuEvent(const MenuEvent& menuEvent) = default;
    MenuEvent(MenuEvent&& menuEvent) = default;
    ~MenuEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    constexpr UINT MenuEvent::GetCommandId() const noexcept
    {
        return _commandId;
    }

    void SetCommandId(_In_ const UINT commandId) noexcept;

private:
    UINT _commandId;

#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const MenuEvent* const pMenuEvent);
#endif
};

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const MenuEvent* const pMenuEvent);
#endif

class FocusEvent : public IInputEvent
{
public:
    constexpr FocusEvent(_In_ const FOCUS_EVENT_RECORD& record) :
        _focus{ !!record.bSetFocus }
    {
    }

    constexpr FocusEvent(_In_ const bool focus) :
        _focus{ focus }
    {
    }

    FocusEvent(const FocusEvent& focusEvent) = default;
    FocusEvent(FocusEvent&& focusEvent) = default;
    ~FocusEvent();

    INPUT_RECORD ToInputRecord() const noexcept override;
    InputEventType EventType() const noexcept override;

    constexpr bool GetFocus() const noexcept
    {
        return _focus;
    }

    void SetFocus(_In_ const bool focus) noexcept;

private:
    bool _focus;

#ifdef UNIT_TESTING
    friend std::wostream& operator<<(std::wostream& stream, const FocusEvent* const pFocusEvent);
#endif
};

#ifdef UNIT_TESTING
std::wostream& operator<<(std::wostream& stream, const FocusEvent* const pFocusEvent);
#endif
