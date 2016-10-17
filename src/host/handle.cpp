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
    FreeInputBuffer(g_ciConsoleInformation.pInputBuffer);
    
ErrorExit3:
    ASSERT(!NT_SUCCESS(Status));
    return Status;
}

PCONSOLE_PROCESS_HANDLE AllocProcessData(_In_ CLIENT_ID const * const ClientId,
                                         _In_ ULONG const ulProcessGroupId,
                                         _In_opt_ PCONSOLE_PROCESS_HANDLE pParentProcessData)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    PCONSOLE_PROCESS_HANDLE ProcessData = FindProcessInList(ClientId->UniqueProcess);
    if (ProcessData != nullptr)
    {
        // In the GenerateConsoleCtrlEvent it's OK for this process to already have a ProcessData object. However, the other case is someone
        // connecting to our LPC port and they should only do that once, so we fail subsequent connection attempts.
        if (pParentProcessData == nullptr)
        {
            return nullptr;
        }
        else
        {
            return ProcessData;
        }
    }

    ProcessData = new CONSOLE_PROCESS_HANDLE();
    if (ProcessData != nullptr)
    {
        ProcessData->ClientId = *ClientId;
        ProcessData->ProcessGroupId = ulProcessGroupId;

        #pragma warning(push)
        // pointer truncation due to using the HANDLE type to store a DWORD process ID.
        // We're using the HANDLE type in the public ClientId field to store the process ID when we should
        // be using a more appropriate type. This should be collected and replaced with the server refactor.
        // TODO - MSFT:9115192
        #pragma warning(disable:4311 4302) 
        ProcessData->ProcessHandle = OpenProcess(MAXIMUM_ALLOWED,
                                                 FALSE,
                                                 (DWORD)ProcessData->ClientId.UniqueProcess);
        #pragma warning(pop)

        // Link this ProcessData ptr into the global list.
        InsertHeadList(&g_ciConsoleInformation.ProcessHandleList, &ProcessData->ListLink);

        if (ProcessData->ProcessHandle != nullptr)
        {
            Telemetry::Instance().LogProcessConnected(ProcessData->ProcessHandle);
        }
    }

    return ProcessData;
}

// Routine Description:
// - This routine frees any per-process data allocated by the console.
// Arguments:
// - ProcessData - Pointer to the per-process data structure.
// Return Value:
void FreeProcessData(_In_ PCONSOLE_PROCESS_HANDLE pProcessData)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    if (pProcessData->InputHandle != nullptr)
    {
        pProcessData->InputHandle->CloseHandle();
    }

    if (pProcessData->OutputHandle != nullptr)
    {
        pProcessData->OutputHandle->CloseHandle();
    }

    pProcessData->WaitBlockQueue.FreeBlocks();

    if (pProcessData->ProcessHandle != nullptr)
    {
        CloseHandle(pProcessData->ProcessHandle);
    }

    RemoveEntryList(&pProcessData->ListLink);
    delete pProcessData;
}
