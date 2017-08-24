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
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->LockConsole();
}

void UnlockConsole()
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    if (gci->GetCSRecursionCount() == 1)
    {
        ProcessCtrlEvents();
    }
    else
    {
        gci->UnlockConsole();
    }
}

NTSTATUS RevalidateConsole(_Out_ CONSOLE_INFORMATION ** const ppConsole)
{
    LockConsole();

    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    *ppConsole = gci;

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
// - <none> - Updates global gci->
NTSTATUS AllocateConsole(_In_reads_bytes_(cbTitle) const WCHAR * const pwchTitle, _In_ const DWORD cbTitle)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    // Synchronize flags
    SetFlagIf(gci->Flags, CONSOLE_AUTO_POSITION, !!gci->GetAutoPosition());
    SetFlagIf(gci->Flags, CONSOLE_QUICK_EDIT_MODE, !!gci->GetQuickEdit());
    SetFlagIf(gci->Flags, CONSOLE_HISTORY_NODUP, !!gci->GetHistoryNoDup());

    Selection* const pSelection = &Selection::Instance();
    pSelection->SetLineSelection(!!gci->GetLineSelection());

    SetConsoleCPInfo(TRUE);
    SetConsoleCPInfo(FALSE);

    // Initialize input buffer.
    try
    {
        gci->pInputBuffer = new InputBuffer();
        if (gci->pInputBuffer == nullptr)
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
    gci->Title = new WCHAR[((cbTitle + 1) / sizeof(WCHAR)) + 1];
    if (gci->Title == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit2;
    }

    #pragma prefast(suppress:26035, "If this fails, we just display an empty title, which is ok.")
    StringCbCopyW(gci->Title, cbTitle + sizeof(WCHAR), pwchTitle);

    gci->OriginalTitle =
        TranslateConsoleTitle(gci->Title, TRUE, FALSE);
    if (gci->OriginalTitle == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit1;
    }

    Status = DoCreateScreenBuffer();
    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit1b;
    }

    gci->CurrentScreenBuffer =
        gci->ScreenBuffers;

    gci->CurrentScreenBuffer->ScrollScale =
        gci->GetScrollScale();

    gci->ConsoleIme.RefreshAreaAttributes();

    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    RIPMSG1(RIP_WARNING, "Console init failed with status 0x%x", Status);

    delete gci->ScreenBuffers;
    gci->ScreenBuffers = nullptr;

ErrorExit1b:
    delete[] gci->Title;
    gci->Title = nullptr;

ErrorExit1:
    delete[] gci->OriginalTitle;
    gci->OriginalTitle = nullptr;

ErrorExit2:
    delete gci->pInputBuffer;

    ASSERT(!NT_SUCCESS(Status));
    return Status;
}
