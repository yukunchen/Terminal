/*++
Copyright (c) Microsoft Corporation

Module Name:
- Display.h

Abstract:
- Console driver display object management routines.

Author:
- HeGatta Mar.16.2017
--*/

#pragma once

NTSTATUS
DspInitializeDisplay(
    VOID
);

NTSTATUS
DspShutDownDisplay(
    VOID
);