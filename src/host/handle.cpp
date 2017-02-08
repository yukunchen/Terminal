/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "handle.h"

#include "misc.h"
#include "input.h"
#include "output.h"
#include "srvinit.h"
#include "stream.h"
#include "utils.hpp"

#pragma hdrstop

void LockConsole()
{
    g_ciConsoleInformation.LockConsole();
}

void UnlockConsole()
{
    if (g_ciConsoleInformation.GetCSRecursionCount() == 1)
    {
        ProcessCtrlEvents();
    }
    else
    {
        g_ciConsoleInformation.UnlockConsole();
    }
}

NTSTATUS RevalidateConsole(_Out_ CONSOLE_INFORMATION ** const ppConsole)
{
    LockConsole();

    *ppConsole = &g_ciConsoleInformation;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine allocates and initialized a console and its associated
//   data - input buffer and screen buffer.
// - NOTE: Will read global g_ciConsoleInformation expecting Settings to already be filled.
// Arguments:
// - Title - Window Title to display
// - TitleLength - Length of Window Title string
// Return Value:
// - <none> - Updates global g_ciConsoleInformation.
NTSTATUS AllocateConsole(_In_reads_bytes_(cbTitle) const WCHAR * const pwchTitle, _In_ const DWORD cbTitle)
{
    // Synchronize flags
    SetFlagIf(g_ciConsoleInformation.Flags, CONSOLE_AUTO_POSITION, !!g_ciConsoleInformation.GetAutoPosition());
    SetFlagIf(g_ciConsoleInformation.Flags, CONSOLE_QUICK_EDIT_MODE, !!g_ciConsoleInformation.GetQuickEdit());
    SetFlagIf(g_ciConsoleInformation.Flags, CONSOLE_HISTORY_NODUP, !!g_ciConsoleInformation.GetHistoryNoDup());

    Selection* const pSelection = &Selection::Instance();
    pSelection->SetLineSelection(!!g_ciConsoleInformation.GetLineSelection());

    SetConsoleCPInfo(TRUE);
    SetConsoleCPInfo(FALSE);

    // Initialize input buffer.
    g_ciConsoleInformation.pInputBuffer = new INPUT_INFORMATION();
    if (g_ciConsoleInformation.pInputBuffer == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    NTSTATUS Status = CreateInputBuffer(g_ciConsoleInformation.GetInputBufferSize(), g_ciConsoleInformation.pInputBuffer);
    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit3;
    }

    // Byte count + 1 so dividing by 2 always rounds up. +1 more for trailing null guard.
    g_ciConsoleInformation.Title = new WCHAR[((cbTitle + 1) / sizeof(WCHAR)) + 1];
    if (g_ciConsoleInformation.Title == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit2;
    }

    #pragma prefast(suppress:26035, "If this fails, we just display an empty title, which is ok.")
    StringCbCopyW(g_ciConsoleInformation.Title, cbTitle + sizeof(WCHAR), pwchTitle);

    g_ciConsoleInformation.OriginalTitle = TranslateConsoleTitle(g_ciConsoleInformation.Title, TRUE, FALSE);
    if (g_ciConsoleInformation.OriginalTitle == nullptr)
    {
        Status = STATUS_NO_MEMORY;
        goto ErrorExit1;
    }

    Status = DoCreateScreenBuffer();
    if (!NT_SUCCESS(Status))
    {
        goto ErrorExit1b;
    }

    g_ciConsoleInformation.CurrentScreenBuffer = g_ciConsoleInformation.ScreenBuffers;

    g_ciConsoleInformation.CurrentScreenBuffer->ScrollScale = g_ciConsoleInformation.GetScrollScale();

    g_ciConsoleInformation.ConsoleIme.RefreshAreaAttributes();

    if (NT_SUCCESS(Status))
    {
        return STATUS_SUCCESS;
    }

    RIPMSG1(RIP_WARNING, "Console init failed with status 0x%x", Status);

    delete g_ciConsoleInformation.ScreenBuffers;
    g_ciConsoleInformation.ScreenBuffers = nullptr;

ErrorExit1b:
    delete[] g_ciConsoleInformation.Title;
    g_ciConsoleInformation.Title = nullptr;

ErrorExit1:
    delete[] g_ciConsoleInformation.OriginalTitle;
    g_ciConsoleInformation.OriginalTitle = nullptr;

ErrorExit2:
    delete g_ciConsoleInformation.pInputBuffer;

ErrorExit3:
    ASSERT(!NT_SUCCESS(Status));
    return Status;
}
