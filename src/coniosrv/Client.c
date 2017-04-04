/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

//
// This file contains routines to examine client processes and to register them
// with us.
//

#include "precomp.h"

#include "ConIoSrv.h"
#include "Client.h"
#include "Trust.h"

NTSTATUS
CisRegisterClient(
    _In_ PCIS_MSG Message,
    _In_ HANDLE ClientProcessHandle,
    _In_ HANDLE PipeReadHandle,
    _In_ HANDLE PipeWriteHandle,
    _In_ HANDLE AlpcSharedSectionHandle,
    _In_ SIZE_T AlpcSharedSectionSize,
    _In_ PVOID AlpcSharedSectionViewBase,
    _Outptr_ PCIS_CLIENT *ClientContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (CisCanRegisterClient(Message))
    {
        PCIS_CLIENT NewClient = (PCIS_CLIENT)RtlAllocateHeap(RtlProcessHeap(),
                                                             0,
                                                             sizeof(CIS_CLIENT));
        if (NewClient)
        {
            //
            // Initialize client structure.
            //

            InitializeListHead((PLIST_ENTRY)NewClient);

            NewClient->ProcessHandle = ClientProcessHandle;
            NewClient->UniqueProcessId = Message->AlpcHeader.ClientId.UniqueProcess;
            NewClient->ServerCommunicationPort = NULL;  // Set later.
            NewClient->PipeReadHandle = PipeReadHandle;
            NewClient->PipeWriteHandle = PipeWriteHandle;
            NewClient->IsNextDisplayUpdateFocusRedraw = FALSE;
            NewClient->SharedSection.Handle = AlpcSharedSectionHandle;
            NewClient->SharedSection.Size = AlpcSharedSectionSize;
            NewClient->SharedSection.ViewBase = AlpcSharedSectionViewBase;

            //
            // Insert the new client at the tail of our list of clients.
            //

            if (g_CisContext->ActiveClient == NULL)
            {
                g_CisContext->ActiveClient = NewClient;
            }
            else
            {
                InsertTailList((PLIST_ENTRY)g_CisContext->ActiveClient,
                                (PLIST_ENTRY)NewClient);

                //
                // Mark the new client as the active one.
                //

                g_CisContext->ActiveClient = NewClient;
            }


            *ClientContext = NewClient;
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

VOID
CisDeregisterClient(
    _In_ PCIS_CLIENT ClientContext
    )
{
    PLIST_ENTRY CurrentListEntry = (PLIST_ENTRY)ClientContext;

    if (g_CisContext->ActiveClient == ClientContext)
    {
        if (CurrentListEntry == CurrentListEntry->Flink)
        {
            g_CisContext->ActiveClient = NULL;
        }
        else
        {
            g_CisContext->ActiveClient = (PCIS_CLIENT)CurrentListEntry->Flink;
            RemoveEntryList(CurrentListEntry);
        }
    }
    else
    {
        RemoveEntryList(CurrentListEntry);
    }
}

BOOLEAN
CisCanRegisterClient(
    _In_ HANDLE ClientProcessHandle
    )
{
    return !CisIsClientRegistered(ClientProcessHandle);
}

BOOLEAN
CisIsClientRegistered(
    _In_ HANDLE Process
    )
{
    HANDLE CurrentProcess;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY CurrentEntry;

    CurrentEntry =
        ListHead = (PLIST_ENTRY)g_CisContext->ActiveClient;
    if (ListHead)
    {
        while (TRUE)
        {
            CurrentProcess = ((PCIS_CLIENT)CurrentEntry)->ProcessHandle;
            if (CurrentProcess == Process)
            {
                return TRUE;
            }

            if (CurrentEntry->Flink == ListHead)
            {
                break;
            }
            else
            {
                CurrentEntry = CurrentEntry->Flink;
            }
        }
    }

    return FALSE;
}

BOOLEAN
CisIsClientActive(
    _In_ HANDLE Process
    )
{
    return g_CisContext->ActiveClient
        && g_CisContext->ActiveClient->ProcessHandle == Process;
}

HANDLE
CisOpenClientProcess(
    _In_ PCIS_MSG Message
    )
{
    HANDLE RetVal = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    HANDLE ClientProcessHandle;
    Status = NtAlpcOpenSenderProcess(&ClientProcessHandle,
                                     g_CisContext->ServerPort,
                                     &Message->AlpcHeader,
                                     0,
                                     MAXIMUM_ALLOWED,
                                     &ObjectAttributes);
    if (Status == STATUS_ACCESS_DENIED)
    {
        Status = NtAlpcOpenSenderProcess(&ClientProcessHandle,
                                         g_CisContext->ServerPort,
                                         &Message->AlpcHeader,
                                         0,
                                         PROCESS_PROTECTED_ACCESS,
                                         &ObjectAttributes);
    }

    if (NT_SUCCESS(Status))
    {
        RetVal = ClientProcessHandle;
    }

    return RetVal;
}

BOOLEAN
CisIsClientTrusted(
    _In_ HANDLE Process
    )
{
    BOOL bRet;
    BOOLEAN RetVal = FALSE;

    WCHAR ImageFilePath[MAX_PATH];
    DWORD PathStringLength = MAX_PATH;

    bRet = QueryFullProcessImageNameW(Process,
                                      0,                    // Use Win32 Path Format (not native)
                                      ImageFilePath,
                                      &PathStringLength);
    if (bRet != FALSE)
    {
        return TstIsFileTrusted(ImageFilePath);
    }

    return RetVal;
}
