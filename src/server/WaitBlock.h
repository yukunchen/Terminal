/*++
Copyright (c) Microsoft Corporation

Module Name:
- WaitBlock.h

Abstract:
- This file defines a queued operation when a console buffer object cannot currently satisfy the request.

Author:
- Michael Niksa (miniksa) 12-Oct-2016

Revision History:
- Adapted from original items in handle.h
--*/

#pragma once

typedef BOOL(*CONSOLE_WAIT_ROUTINE) (_In_ PLIST_ENTRY pWaitQueue,
                                     _In_ PCONSOLE_API_MSG pWaitReplyMessage,
                                     _In_ PVOID pvWaitParameter,
                                     _In_ PVOID pvSatisfyParameter,
                                     _In_ BOOL fThreadDying);

typedef struct _CONSOLE_WAIT_BLOCK
{
    LIST_ENTRY Link;
    LIST_ENTRY ProcessLink;
    PVOID WaitParameter;
    CONSOLE_WAIT_ROUTINE WaitRoutine;
    CONSOLE_API_MSG WaitReplyMessage;
} CONSOLE_WAIT_BLOCK, *PCONSOLE_WAIT_BLOCK;
