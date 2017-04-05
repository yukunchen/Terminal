/*++
Copyright (c) Microsoft Corporation

Module Name:
- Keyboard.h

Abstract:
- Discover keyboard devices, read raw keyboard device input, and listen for
  keyboard device arrival and removal events via Windows' Configuration Manager.
- Code comes from \minkernel\minwin\console\conkbd\conkbd.c.

Author:
- RichP Mar.10.2004

Revision History:
- Added PnP notification support. (PoNagpal, 2016)
- Moved to ConIoSrv. (HeGatta, 2017)
--*/

#pragma once

NTSTATUS
KbdInitializeKeyboardInputAsync(
    VOID
);
