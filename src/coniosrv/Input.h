/*++
Copyright (c) Microsoft Corporation

Module Name:
- Input.h

Abstract:
- These routines process raw input from the keyboard, and dispatch focus events
  to client applications when one becomes the background application and another
  becomes the foreground (active) one.
- Keycode processing code comes from \minkernel\minwin\console\conkbd\conkbd.c.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

#define MAX_CHARS_FROM_1_KEYSTROKE 6

VOID
InpProcessKeycode(
    PKEYBOARD_INPUT_DATA Keycode
);

NTSTATUS
InpDispatchFocusEvent(
    _In_ PCIS_CLIENT ClientContext,
    _In_ BOOLEAN IsActive
);
