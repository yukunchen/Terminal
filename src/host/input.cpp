/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "input.h"

#include "dbcs.h"
#include "stream.h"

#include "..\terminal\adapter\terminalInput.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

#define CTRL_BUT_NOT_ALT(n) \
        (((n) & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) && \
        !((n) & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)))

#define KEY_ENHANCED 0x01000000

bool IsInProcessedInputMode()
{
    return (ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->InputMode & ENABLE_PROCESSED_INPUT) != 0;
}

bool IsInVirtualTerminalInputMode()
{
    return IsFlagSet(ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->InputMode, ENABLE_VIRTUAL_TERMINAL_INPUT);
}

BOOL IsSystemKey(_In_ WORD const wVirtualKeyCode)
{
    switch (wVirtualKeyCode)
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_MENU:
        case VK_PAUSE:
        case VK_CAPITAL:
        case VK_LWIN:
        case VK_RWIN:
        case VK_NUMLOCK:
        case VK_SCROLL:
            return TRUE;
    }
    return FALSE;
}

ULONG GetControlKeyState(_In_ const LPARAM lParam)
{
    ULONG ControlKeyState = 0;

    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_LMENU) & KEY_PRESSED)
    {
        ControlKeyState |= LEFT_ALT_PRESSED;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_RMENU) & KEY_PRESSED)
    {
        ControlKeyState |= RIGHT_ALT_PRESSED;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_LCONTROL) & KEY_PRESSED)
    {
        ControlKeyState |= LEFT_CTRL_PRESSED;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_RCONTROL) & KEY_PRESSED)
    {
        ControlKeyState |= RIGHT_CTRL_PRESSED;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_SHIFT) & KEY_PRESSED)
    {
        ControlKeyState |= SHIFT_PRESSED;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_NUMLOCK) & KEY_TOGGLED)
    {
        ControlKeyState |= NUMLOCK_ON;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_SCROLL) & KEY_TOGGLED)
    {
        ControlKeyState |= SCROLLLOCK_ON;
    }
    if (ServiceLocator::LocateInputServices()->GetKeyState(VK_CAPITAL) & KEY_TOGGLED)
    {
        ControlKeyState |= CAPSLOCK_ON;
    }
    if (lParam & KEY_ENHANCED)
    {
        ControlKeyState |= ENHANCED_KEY;
    }

    ControlKeyState |= (lParam & ALTNUMPAD_BIT);

    return ControlKeyState;
}

// Routine Description:
// - returns true if we're in a mode amenable to us taking over keyboard shortcuts
bool ShouldTakeOverKeyboardShortcuts()
{
    return !ServiceLocator::LocateGlobals()->getConsoleInformation()->GetCtrlKeyShortcutsDisabled() && IsInProcessedInputMode();
}



// Routine Description:
// - handles key events without reference to Win32 elements.
void HandleGenericKeyEvent(INPUT_RECORD InputEvent, BOOL bGenerateBreak)
{
    BOOLEAN ContinueProcessing = TRUE;
    ULONG EventsWritten;
    
    // if (HandleTerminalKeyEvent(&InputEvent))
    // {
    //     return;
    // }

    if (CTRL_BUT_NOT_ALT(InputEvent.Event.KeyEvent.dwControlKeyState) && InputEvent.Event.KeyEvent.bKeyDown)
    {
        // check for ctrl-c, if in line input mode.
        if (InputEvent.Event.KeyEvent.wVirtualKeyCode == 'C' && IsInProcessedInputMode())
        {
            HandleCtrlEvent(CTRL_C_EVENT);
            if (ServiceLocator::LocateGlobals()->getConsoleInformation()->PopupCount == 0)
            {
                ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->TerminateRead(WaitTerminationReason::CtrlC);
            }

            if (!(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags & CONSOLE_SUSPENDED))
            {
                ContinueProcessing = FALSE;
            }
        }

        // check for ctrl-break.
        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL)
        {
            ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->Flush();
            HandleCtrlEvent(CTRL_BREAK_EVENT);
            if (ServiceLocator::LocateGlobals()->getConsoleInformation()->PopupCount == 0)
            {
                ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->TerminateRead(WaitTerminationReason::CtrlBreak);
            }

            if (!(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags & CONSOLE_SUSPENDED))
            {
                ContinueProcessing = FALSE;
            }
        }

        // don't write ctrl-esc to the input buffer
        else if (InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
        {
            ContinueProcessing = FALSE;
        }
    }
    else if ((InputEvent.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) &&
             InputEvent.Event.KeyEvent.bKeyDown &&
             InputEvent.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
    {
        ContinueProcessing = FALSE;
    }

    if (ContinueProcessing)
    {
        EventsWritten = ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
        if (EventsWritten && bGenerateBreak)
        {
            InputEvent.Event.KeyEvent.bKeyDown = FALSE;
            ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
        }
    }
}

// Routine Description:
// - Handler for detecting whether a key-press event can be appropriately converted into a terminal sequence.
//   Will only trigger when virtual terminal input mode is set via STDIN handle
// Arguments:
// - pInputRecord - Input record event from the general input event handler
// Return Value:
// - True if the modes were appropriate for converting to a terminal sequence AND there was a matching terminal sequence for this key. False otherwise.
bool HandleTerminalKeyEvent(_In_ const INPUT_RECORD* const pInputRecord)
{
    // If the modes don't align, this is unhandled by default.
    bool fWasHandled = false;

    // Virtual terminal input mode
    if (IsInVirtualTerminalInputMode())
    {
        fWasHandled = ServiceLocator::LocateGlobals()->getConsoleInformation()->termInput.HandleKey(pInputRecord);
    }

    return fWasHandled;
}

void HandleFocusEvent(_In_ const BOOL fSetFocus)
{
    INPUT_RECORD InputEvent;
    InputEvent.EventType = FOCUS_EVENT;
    InputEvent.Event.FocusEvent.bSetFocus = fSetFocus;

#pragma prefast(suppress:28931, "EventsWritten is not unused. Used by assertions.")
    ULONG const EventsWritten = ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
    EventsWritten; // shut the fre build up.
    ASSERT(EventsWritten == 1);
}

void HandleMenuEvent(_In_ const DWORD wParam)
{
    INPUT_RECORD InputEvent;
    InputEvent.EventType = MENU_EVENT;
    InputEvent.Event.MenuEvent.dwCommandId = wParam;

#pragma prefast(suppress:28931, "EventsWritten is not unused. Used by assertions.")
    ULONG const EventsWritten = ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer->WriteInputBuffer(&InputEvent, 1);
    EventsWritten; // shut the fre build up.
#if DBG
    if (EventsWritten != 1)
    {
        RIPMSG0(RIP_WARNING, "PutInputInBuffer: EventsWritten != 1, 1 expected");
    }
#endif
}

void HandleCtrlEvent(_In_ const DWORD EventType)
{
    switch (EventType)
    {
        case CTRL_C_EVENT:
            ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags |= CONSOLE_CTRL_C_FLAG;
            break;
        case CTRL_BREAK_EVENT:
            ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags |= CONSOLE_CTRL_BREAK_FLAG;
            break;
        case CTRL_CLOSE_EVENT:
            ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags |= CONSOLE_CTRL_CLOSE_FLAG;
            break;
        default:
            RIPMSG1(RIP_ERROR, "Invalid EventType: 0x%x", EventType);
    }
}

void ProcessCtrlEvents()
{
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags == 0)
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
        return;
    }

    // Make our own copy of the console process handle list.
    DWORD const LimitingProcessId = ServiceLocator::LocateGlobals()->getConsoleInformation()->LimitingProcessId;
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LimitingProcessId = 0;

    ConsoleProcessTerminationRecord* rgProcessHandleList;
    size_t cProcessHandleList;

    HRESULT hr = ServiceLocator::LocateGlobals()
        ->getConsoleInformation()
        ->ProcessHandleList
        .GetTerminationRecordsByGroupId(LimitingProcessId,
                                        IsFlagSet(ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags,
                                                  CONSOLE_CTRL_CLOSE_FLAG),
                                        &rgProcessHandleList,
                                        &cProcessHandleList);

    if (FAILED(hr) || cProcessHandleList == 0)
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
        return;
    }

    // Copy ctrl flags.
    ULONG CtrlFlags = ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags;
    ASSERT(!((CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG | CONSOLE_CTRL_C_FLAG)) && (CtrlFlags & (CONSOLE_CTRL_LOGOFF_FLAG | CONSOLE_CTRL_SHUTDOWN_FLAG))));

    ServiceLocator::LocateGlobals()->getConsoleInformation()->CtrlFlags = 0;

    ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();

    // the ctrl flags could be a combination of the following
    // values:
    //
    //        CONSOLE_CTRL_C_FLAG
    //        CONSOLE_CTRL_BREAK_FLAG
    //        CONSOLE_CTRL_CLOSE_FLAG
    //        CONSOLE_CTRL_LOGOFF_FLAG
    //        CONSOLE_CTRL_SHUTDOWN_FLAG

    DWORD EventType = (DWORD) - 1;
    switch (CtrlFlags & (CONSOLE_CTRL_CLOSE_FLAG | CONSOLE_CTRL_BREAK_FLAG | CONSOLE_CTRL_C_FLAG | CONSOLE_CTRL_LOGOFF_FLAG | CONSOLE_CTRL_SHUTDOWN_FLAG))
    {

        case CONSOLE_CTRL_CLOSE_FLAG:
            EventType = CTRL_CLOSE_EVENT;
            break;

        case CONSOLE_CTRL_BREAK_FLAG:
            EventType = CTRL_BREAK_EVENT;
            break;

        case CONSOLE_CTRL_C_FLAG:
            EventType = CTRL_C_EVENT;
            break;

        case CONSOLE_CTRL_LOGOFF_FLAG:
            EventType = CTRL_LOGOFF_EVENT;
            break;

        case CONSOLE_CTRL_SHUTDOWN_FLAG:
            EventType = CTRL_SHUTDOWN_EVENT;
            break;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    for (size_t i = 0; i < cProcessHandleList; i++)
    {
        /*
         * Status will be non-successful if a process attached to this console
         * vetos shutdown. In that case, we don't want to try to kill any more
         * processes, but we do need to make sure we continue looping so we
         * can close any remaining process handles. The exception is if the
         * process is inaccessible, such that we can't even open a handle for
         * query. In this case, use best effort to send the close event but
         * ignore any errors.
         */
        if (NT_SUCCESS(Status))
        {
            Status = ServiceLocator::LocateConsoleControl()
                ->EndTask((HANDLE)rgProcessHandleList[i].dwProcessID,
                          EventType,
                          CtrlFlags);
            if (rgProcessHandleList[i].hProcess == nullptr) {
                Status = STATUS_SUCCESS;
            }
        }

        if (rgProcessHandleList[i].hProcess != nullptr)
        {
            CloseHandle(rgProcessHandleList[i].hProcess);
        }
    }

    delete[] rgProcessHandleList;
}
