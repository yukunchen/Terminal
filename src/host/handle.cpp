/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "handle.h"

#include "misc.h"
#include "output.h"
#include "srvinit.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

void LockConsole()
{
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
}

void UnlockConsole()
{
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetCSRecursionCount() == 1)
    {
        ProcessCtrlEvents();
    }
    else
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    }
}

NTSTATUS RevalidateConsole(_Out_ CONSOLE_INFORMATION ** const ppConsole)
{
    LockConsole();

    *ppConsole = ServiceLocator::LocateGlobals()->getConsoleInformation();

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine allocates and initialized a console and its associated
//   data - input buffer and screen buffer.
// - NOTE: Will read global ServiceLocator::LocateGlobals()->getConsoleInformation expecting Settings to already be filled.
// Arguments:
// - Title - Window Title to display
// - TitleLength - Length of Window Title string
// Return Value:
// - <none> - Updates global ServiceLocator::LocateGlobals()->getConsoleInformation()->
NTSTATUS AllocateConsole(_In_reads_bytes_(cbTitle) const WCHAR * const pwchTitle, _In_ const DWORD cbTitle)
{
    // Synchronize flags
    SetFlagIf(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_AUTO_POSITION, !!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetAutoPosition());
    SetFlagIf(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_QUICK_EDIT_MODE, !!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetQuickEdit());
    SetFlagIf(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_HISTORY_NODUP, !!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetHistoryNoDup());

    Selection* const pSelection = &Selection::Instance();
    pSelection->SetLineSelection(!!ServiceLocator::LocateGlobals()->getConsoleInformation()->GetLineSelection());

    SetConsoleCPInfo(TRUE);
    SetConsoleCPInfo(FALSE);

    // Initialize input buffer.
    try
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer = new InputBuffer();
        if (ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer == nullptr)
        {
            return STATUS_NO_MEMORY;
        }
    }
    catch(...)
    {
        return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
    }

    NTSTATUS Status;
    // Byte count + 1 so dividing by 2 always rounds up. +1 more for trailing null guard.
    ServiceLocator::LocateGlobals()->getConsoleInformation()->Title = new WCHAR[((cbTitle + 1) / sizeof(WCHAR)) + 1];
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->Title == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit2;
    }

    #pragma prefast(suppress:26035, "If this fails, we just display an empty title, which is ok.")
    StringCbCopyW(ServiceLocator::LocateGlobals()->getConsoleInformation()->Title, cbTitle + sizeof(WCHAR), pwchTitle);

    ServiceLocator::LocateGlobals()->getConsoleInformation()->OriginalTitle =
        TranslateConsoleTitle(ServiceLocator::LocateGlobals()->getConsoleInformation()->Title, TRUE, FALSE);
    if (ServiceLocator::LocateGlobals()->getConsoleInformation()->OriginalTitle == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit1;
    }

    Status = DoCreateScreenBuffer();
    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit1b;
    }

    ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer =
        ServiceLocator::LocateGlobals()->getConsoleInformation()->ScreenBuffers;

    ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->ScrollScale =
        ServiceLocator::LocateGlobals()->getConsoleInformation()->GetScrollScale();

    ServiceLocator::LocateGlobals()->getConsoleInformation()->ConsoleIme.RefreshAreaAttributes();

    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    RIPMSG1(RIP_WARNING, "Console init failed with status 0x%x", Status);

    delete ServiceLocator::LocateGlobals()->getConsoleInformation()->ScreenBuffers;
    ServiceLocator::LocateGlobals()->getConsoleInformation()->ScreenBuffers = nullptr;

ErrorExit1b:
    delete[] ServiceLocator::LocateGlobals()->getConsoleInformation()->Title;
    ServiceLocator::LocateGlobals()->getConsoleInformation()->Title = nullptr;

ErrorExit1:
    delete[] ServiceLocator::LocateGlobals()->getConsoleInformation()->OriginalTitle;
    ServiceLocator::LocateGlobals()->getConsoleInformation()->OriginalTitle = nullptr;

ErrorExit2:
    delete ServiceLocator::LocateGlobals()->getConsoleInformation()->pInputBuffer;

    ASSERT(!NT_SUCCESS(Status));
    return Status;
}
