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
    SetFlagBasedOnBool(!!g_ciConsoleInformation.GetAutoPosition(), &g_ciConsoleInformation.Flags, CONSOLE_AUTO_POSITION);
    SetFlagBasedOnBool(!!g_ciConsoleInformation.GetQuickEdit(), &g_ciConsoleInformation.Flags, CONSOLE_QUICK_EDIT_MODE);
    SetFlagBasedOnBool(!!g_ciConsoleInformation.GetHistoryNoDup(), &g_ciConsoleInformation.Flags, CONSOLE_HISTORY_NODUP);

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

    InitializeObjectHeader(&g_ciConsoleInformation.pInputBuffer->Header);

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

    g_ciConsoleInformation.CurrentScreenBuffer->Header.OpenCount = 1;
    g_ciConsoleInformation.CurrentScreenBuffer->Header.ReaderCount = 1;
    g_ciConsoleInformation.CurrentScreenBuffer->Header.ReadShareCount = 1;
    g_ciConsoleInformation.CurrentScreenBuffer->Header.WriterCount = 1;
    g_ciConsoleInformation.CurrentScreenBuffer->Header.WriteShareCount = 1;

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

void ConsoleCloseHandle(_In_ const HANDLE hClose)
{
    PCONSOLE_HANDLE_DATA HandleData;
    NTSTATUS Status = DereferenceIoHandleNoCheck(hClose, &HandleData);
    if (NT_SUCCESS(Status))
    {
        if (HandleData->HandleType & CONSOLE_INPUT_HANDLE)
        {
            Status = CloseInputHandle(HandleData, hClose);
        }
        else
        {
            Status = CloseOutputHandle((PSCREEN_INFORMATION) HandleData->ClientPointer, hClose);
        }
    }
}

bool FreeConsoleHandle(_In_ HANDLE hFree)
{
    PCONSOLE_HANDLE_DATA const HandleData = (PCONSOLE_HANDLE_DATA)hFree;

    BOOLEAN const ReadRequested = (HandleData->Access & GENERIC_READ) != 0;
    BOOLEAN const ReadShared = (HandleData->ShareAccess & FILE_SHARE_READ) != 0;

    BOOLEAN const WriteRequested = (HandleData->Access & GENERIC_WRITE) != 0;
    BOOLEAN const WriteShared = (HandleData->ShareAccess & FILE_SHARE_WRITE) != 0;

    PCONSOLE_OBJECT_HEADER const Header = (PCONSOLE_OBJECT_HEADER)HandleData->ClientPointer;

    delete HandleData;

    Header->OpenCount -= 1;

    Header->ReaderCount -= ReadRequested;
    Header->ReadShareCount -= ReadShared;

    Header->WriterCount -= WriteRequested;
    Header->WriteShareCount -= WriteShared;

    return Header->OpenCount == 0;
}

// Routine Description:
// - This routine allocates an input or output handle from the process's handle table.
// - This routine initializes all non-type specific fields in the handle data structure.
// Arguments:
// - ulHandleType - Flag indicating input or output handle.
// - phOut - On return, contains allocated handle.  Handle is an index internally.  When returned to the API caller, it is translated into a handle.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.  The handle is allocated from the per-process handle table.  Holding the console
//   lock serializes both threads within the calling process and any other process that shares the console.
NTSTATUS AllocateIoHandle(_In_ const ULONG ulHandleType,
                          _Out_ HANDLE * const phOut,
                          _Inout_ PCONSOLE_OBJECT_HEADER pObjHeader,
                          _In_ const ACCESS_MASK amDesired,
                          _In_ const ULONG ulShareMode)
{
    // Check the share mode.
    BOOLEAN const ReadRequested = (amDesired & GENERIC_READ) != 0;
    BOOLEAN const ReadShared = (ulShareMode & FILE_SHARE_READ) != 0;

    BOOLEAN const WriteRequested = (amDesired & GENERIC_WRITE) != 0;
    BOOLEAN const WriteShared = (ulShareMode & FILE_SHARE_WRITE) != 0;

    if (((ReadRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->ReadShareCount)) ||
        ((ReadShared == FALSE) && (pObjHeader->ReaderCount > 0)) ||
        ((WriteRequested != FALSE) && (pObjHeader->OpenCount > pObjHeader->WriteShareCount)) || ((WriteShared == FALSE) && (pObjHeader->WriterCount > 0)))
    {
        return STATUS_SHARING_VIOLATION;
    }

    // Allocate all necessary state.
    PCONSOLE_HANDLE_DATA const HandleData = new CONSOLE_HANDLE_DATA();
    if (HandleData == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    if ((ulHandleType & CONSOLE_INPUT_HANDLE) != 0)
    {
        HandleData->pClientInput = new INPUT_READ_HANDLE_DATA();
        if (HandleData->pClientInput == nullptr)
        {
            delete HandleData;
            return STATUS_NO_MEMORY;
        }
    }

    // Update share/open counts and store handle information.
    pObjHeader->OpenCount += 1;

    pObjHeader->ReaderCount += ReadRequested;
    pObjHeader->ReadShareCount += ReadShared;

    pObjHeader->WriterCount += WriteRequested;
    pObjHeader->WriteShareCount += WriteShared;

    HandleData->HandleType = ulHandleType;
    HandleData->ShareAccess = ulShareMode;
    HandleData->Access = amDesired;
    HandleData->ClientPointer = pObjHeader;

    *phOut = (HANDLE)HandleData;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:
NTSTATUS DereferenceIoHandleNoCheck(_In_ HANDLE hIO, _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData)
{
    *ppConsoleData = (PCONSOLE_HANDLE_DATA)hIO;
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine verifies a handle's validity, then returns a pointer to the handle data structure.
// Arguments:
// - ProcessData - Pointer to per process data structure.
// - Handle - Handle to dereference.
// - HandleData - On return, pointer to handle data structure.
// Return Value:
NTSTATUS DereferenceIoHandle(_In_ HANDLE hIO,
                             _In_ const ULONG ulHandleType,
                             _In_ const ACCESS_MASK amRequested,
                             _Out_ PCONSOLE_HANDLE_DATA * const ppConsoleData)
{
    PCONSOLE_HANDLE_DATA const Data = (PCONSOLE_HANDLE_DATA)hIO;

    // Check the type and the granted access.
    if ((hIO == nullptr) || ((Data->Access & amRequested) == 0) || ((ulHandleType != 0) && ((Data->HandleType & ulHandleType) == 0)))
    {
        return STATUS_INVALID_HANDLE;
    }

    *ppConsoleData = Data;

    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine inserts the screen buffer pointer into the console's list of screen buffers.
// Arguments:
// - Console - Pointer to console information structure.
// - ScreenInfo - Pointer to screen information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void InsertScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo)
{
    ASSERT(g_ciConsoleInformation.IsConsoleLocked());

    pScreenInfo->Next = g_ciConsoleInformation.ScreenBuffers;
    g_ciConsoleInformation.ScreenBuffers = pScreenInfo;
}

// Routine Description:
// - This routine removes the screen buffer pointer from the console's list of screen buffers.
// Arguments:
// - Console - Pointer to console information structure.
// - ScreenInfo - Pointer to screen information structure.
// Return Value:
// Note:
// - The console lock must be held when calling this routine.
void RemoveScreenBuffer(_In_ PSCREEN_INFORMATION pScreenInfo)
{
    if (pScreenInfo == g_ciConsoleInformation.ScreenBuffers)
    {
        g_ciConsoleInformation.ScreenBuffers = pScreenInfo->Next;
    }
    else
    {
        PSCREEN_INFORMATION Cur = g_ciConsoleInformation.ScreenBuffers;
        PSCREEN_INFORMATION Prev = Cur;
        while (Cur != nullptr)
        {
            if (pScreenInfo == Cur)
            {
                break;
            }

            Prev = Cur;
            Cur = Cur->Next;
        }

        ASSERT(Cur != nullptr);
        __analysis_assume(Cur != nullptr);
        Prev->Next = Cur->Next;
    }

    if (pScreenInfo == g_ciConsoleInformation.CurrentScreenBuffer && g_ciConsoleInformation.ScreenBuffers != g_ciConsoleInformation.CurrentScreenBuffer)
    {
        if (g_ciConsoleInformation.ScreenBuffers != nullptr)
        {
            SetActiveScreenBuffer(g_ciConsoleInformation.ScreenBuffers);
        }
        else
        {
            g_ciConsoleInformation.CurrentScreenBuffer = nullptr;
        }
    }

    delete pScreenInfo;
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

        InitializeListHead(&ProcessData->WaitBlockQueue);

        ProcessData->ProcessHandle = OpenProcess(MAXIMUM_ALLOWED,
                                                 FALSE,
                                                 (DWORD)ProcessData->ClientId.UniqueProcess);

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
        ConsoleCloseHandle(pProcessData->InputHandle);
    }

    if (pProcessData->OutputHandle != nullptr)
    {
        ConsoleCloseHandle(pProcessData->OutputHandle);
    }

    while (!IsListEmpty(&pProcessData->WaitBlockQueue))
    {
        PCONSOLE_WAIT_BLOCK WaitBlock;

        WaitBlock = CONTAINING_RECORD(pProcessData->WaitBlockQueue.Flink, CONSOLE_WAIT_BLOCK, ProcessLink);

        ConsoleNotifyWaitBlock(WaitBlock, nullptr, nullptr, TRUE);
    }

    if (pProcessData->ProcessHandle != nullptr)
    {
        CloseHandle(pProcessData->ProcessHandle);
    }

    RemoveEntryList(&pProcessData->ListLink);
    delete pProcessData;
}

void InitializeObjectHeader(_Out_ PCONSOLE_OBJECT_HEADER pObjHeader)
{
    ZeroMemory(pObjHeader, sizeof(*pObjHeader));
}

// Routine Description:
// - Retieves the properly typed Input Buffer from the Handle.
// Arguments:
// - HandleData - The HANDLE containing the pointer to the input buffer, typically from an API call.
// Return Value:
// - The Input Buffer that this handle points to
PINPUT_INFORMATION GetInputBufferFromHandle(const CONSOLE_HANDLE_DATA* HandleData)
{
    return ((PINPUT_INFORMATION)(HandleData->ClientPointer));
}

// Routine Description:
// - Retieves the properly typed Screen Buffer from the Handle. If the screen
//     buffer currently has an alternate buffer, as set through ANSI sequence
//     ASBSET, then this returns the alternate, so that it seems as though 
//     the same handle now points to the alternate buffer.
// Arguments:
// - HandleData - The HANDLE containing the pointer to the screen buffer, typically from an API call.
// Return Value:
// - The Screen Buffer that this handle points to.
PSCREEN_INFORMATION GetScreenBufferFromHandle(const CONSOLE_HANDLE_DATA* HandleData)
{
    SCREEN_INFORMATION* psiScreenInfo = ((PSCREEN_INFORMATION)(HandleData->ClientPointer));
    return psiScreenInfo->GetActiveBuffer();
}