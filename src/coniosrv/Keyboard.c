/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrv.h"

#include "Input.h"
#include "Keyboard.h"

#pragma region Keyboard Structures and Data

typedef struct {
    HANDLE KeyboardHandle;

    // State for PnpInputApc
    KEYBOARD_INPUT_DATA KeyCode;
    IO_STATUS_BLOCK IoStatusBlock;

    // State for PNP callbacks
    BOOLEAN KeyboardRemoved;
    HANDLE InputApcDoneEvent;
    WCHAR SymbolicLink[ANYSIZE_ARRAY];
} KBD_PNP_KEYBOARD_CONTEXT, *PKBD_PNP_KEYBOARD_CONTEXT;

HCMNOTIFICATION PnpKeyboardNotification = NULL;

#pragma endregion

#pragma region Forward Declarations

DWORD
WINAPI
KbdInitializeKeyboardInput(
    PVOID lpParam
);

NTSTATUS
KbdInitializeKeyboards(
    VOID
);

NTSTATUS
KbdInitializeKeyboard(
    _In_ PCWSTR SymbolicLink
);

DWORD
KbdRemoveKeyboardCallback(
    _In_ HCMNOTIFICATION Notification,
    _In_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD EventDataSize
);

VOID
WINAPI
KbdKeyboardInputApcCallback(
    _In_ PKBD_PNP_KEYBOARD_CONTEXT ApcContext,
    _In_opt_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
);

VOID
KbdUnregisterNotification(
    _In_ HCMNOTIFICATION Notification
);

VOID
KbdRegisterKeyboardDeviceEvents(
    VOID
);

#pragma endregion

#pragma region Initialization

NTSTATUS
KbdInitializeKeyboardInputAsync(
    VOID
    )
{
    return RtlCreateUserThread(NtCurrentProcess(),
                               NULL,
                               FALSE,
                               0,
                               0,
                               1024,
                               (PUSER_THREAD_START_ROUTINE)KbdInitializeKeyboardInput,
                               NULL,
                               NULL,
                               NULL);
}

DWORD
WINAPI
KbdInitializeKeyboardInput(
    PVOID lpParam
    )
{
    UNREFERENCED_PARAMETER(lpParam);
    
    KbdInitializeKeyboards();
    KbdRegisterKeyboardDeviceEvents();

    while (TRUE)
    {
        //
        // Sleep alertable to receive APC callbacks.
        //
        SleepEx(INFINITE, TRUE);
    }
}

NTSTATUS
KbdInitializeKeyboards(
    VOID
    )
{
    CONFIGRET cr = CR_SUCCESS;
    PZZWSTR KbdList = NULL;
    ULONG KbdListLength = 0;

    cr = CM_Get_Device_Interface_List_SizeW(&KbdListLength,
                                            (LPGUID)&GUID_DEVINTERFACE_KEYBOARD,
                                            NULL,
                                            CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KbdList = (PWSTR)RtlAllocateHeap(RtlProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     KbdListLength * sizeof(WCHAR));
    if (KbdList == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    cr = CM_Get_Device_Interface_ListW((LPGUID)&GUID_DEVINTERFACE_KEYBOARD,
                                       NULL,
                                       KbdList,
                                       KbdListLength,
                                       CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cr != CR_SUCCESS)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, KbdList);

        return STATUS_UNSUCCESSFUL;
    }

    PWSTR CurrentKbd = KbdList;
    while (*CurrentKbd)
    {
        KbdInitializeKeyboard(CurrentKbd);
        CurrentKbd += wcslen(CurrentKbd) + 1;
    }

    RtlFreeHeap(RtlProcessHeap(), 0, KbdList);

    return STATUS_SUCCESS;
}

NTSTATUS 
KbdInitializeKeyboard(
    _In_ PCWSTR SymbolicLink
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    CONFIGRET cr = CR_SUCCESS;

    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING KeyboardName;
    OBJECT_ATTRIBUTES Obja;
    CM_NOTIFY_FILTER NotifyFilter = { 0 };
    HCMNOTIFICATION Notify = INVALID_HANDLE_VALUE;
    PKBD_PNP_KEYBOARD_CONTEXT LocalKeyboardContext = NULL;
    size_t StrLen = 0;
    DWORD RequiredSize = 0;

    // 
    // Theoretically, device interface path lengths can be longer than MAX_PATH.  
    // In practice, they seldom are, but it is a possibility.
    // Calculate the actual length of SymbolicLink (in Bytes)
    //
    if (FAILED(StringCchLengthW(SymbolicLink, STRSAFE_MAX_CCH, &StrLen)))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (FAILED(ULongMult((ULONG)(StrLen + 1), sizeof(WCHAR), &RequiredSize)))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }
    
    LocalKeyboardContext = RtlAllocateHeap(RtlProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           sizeof(KBD_PNP_KEYBOARD_CONTEXT) + RequiredSize);

    if (LocalKeyboardContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // Convert the prefix \\?\ to \??\ for use with NtCreateFile
    //
    CopyMemory(LocalKeyboardContext->SymbolicLink, SymbolicLink, RequiredSize);
    LocalKeyboardContext->SymbolicLink[1] = L'?';

    //
    // Open a handle to the new keyboard
    //
    RtlInitUnicodeString(&KeyboardName, LocalKeyboardContext->SymbolicLink);
    InitializeObjectAttributes(&Obja, &KeyboardName, 0, NULL, NULL);
    Status = NtCreateFile(&LocalKeyboardContext->KeyboardHandle,
                          FILE_READ_DATA | SYNCHRONIZE,
                          &Obja,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ,
                          FILE_OPEN_IF,
                          FILE_DIRECTORY_FILE,
                          NULL,
                          0);

    //
    // Failed to open the handle
    //
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Set KeyboardRemoved to FALSE initially. A value of TRUE indicates 
    // that PnpInputApc must exit
    //
    LocalKeyboardContext->KeyboardRemoved = FALSE;

    //
    // Initialize the InputApcDoneEvent event. This is used by 
    // PnpInputApc to acknowledge that it has stopped accessing the 
    // context. When set, PNP callbacks can safely deallocate the context.
    //
    Status = NtCreateEvent(&LocalKeyboardContext->InputApcDoneEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }
        
    //
    // Setup removal notification for this device handle
    //
    NotifyFilter.cbSize = sizeof(NotifyFilter);
    NotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE;
    NotifyFilter.u.DeviceHandle.hTarget = LocalKeyboardContext->KeyboardHandle;

    //
    // Register for device removal event for this handle
    //
    cr = CM_Register_Notification(&NotifyFilter,
                                  (PVOID)LocalKeyboardContext,
                                  KbdRemoveKeyboardCallback,
                                  &Notify);

    //
    // Failed to register for device removal
    //
    if (cr != CR_SUCCESS)
    {
        
        Status = NTSTATUS_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_INVALID_DATA));
        goto Exit;
    }

    //
    // Start reading from the new keyboard
    //
    KbdKeyboardInputApcCallback(LocalKeyboardContext, NULL, 0);

Exit:
    if (!NT_SUCCESS(Status))
    {

        if (LocalKeyboardContext != NULL)
        {
            if (LocalKeyboardContext->KeyboardHandle != NULL)
            {
                NtClose(LocalKeyboardContext->KeyboardHandle);
            }

            if (LocalKeyboardContext->InputApcDoneEvent != NULL)
            {
                NtClose(LocalKeyboardContext->InputApcDoneEvent);
            }

            RtlFreeHeap(RtlProcessHeap(), 0, LocalKeyboardContext);
        }
    }

    return Status;
}

#pragma endregion

#pragma region Device Notification Callbacks

//
// Handle keyboard interface arrival
//
DWORD
KbdAddKeyboardCallback(
    _In_ HCMNOTIFICATION Notification,
    _In_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD EventDataSize
)
{
    UNREFERENCED_PARAMETER(Notification);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(EventDataSize);

    if (Action == CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL) {

        KbdInitializeKeyboard(EventData->u.DeviceInterface.SymbolicLink);
    }

    return ERROR_SUCCESS;
}

DWORD
KbdRemoveKeyboardCallback(
    _In_ HCMNOTIFICATION Notification,
    _In_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_ PCM_NOTIFY_EVENT_DATA EventData,
    _In_ DWORD EventDataSize
    )
{
    UNREFERENCED_PARAMETER(EventData);
    UNREFERENCED_PARAMETER(EventDataSize);

    NTSTATUS Status = STATUS_SUCCESS;

    PKBD_PNP_KEYBOARD_CONTEXT LocalKeyboardContext = (PKBD_PNP_KEYBOARD_CONTEXT)Context;
    
    //
    // Reference: https://msdn.microsoft.com/en-us/library/windows/hardware/dn858592(v=vs.85).aspx
    //
    switch(Action)
    {

    //
    // Close the handle to allow remove to complete. PnpInputApc gets notified 
    // when the handle is closed. Specifically, the IoStatusBlock comes back 
    // with STATUS_CANCELLED. Setting KeyboardRemoved to TRUE causes 
    // PnpInputApc to exit from read loop. Note that deallocation of the Keyboard 
    // Context is deferred to CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE
    //
    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVE:

        LocalKeyboardContext->KeyboardRemoved = TRUE;

        NtClose(LocalKeyboardContext->KeyboardHandle);
        LocalKeyboardContext->KeyboardHandle = INVALID_HANDLE_VALUE;

        break;

    //
    // Case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE: Unregister notification now that 
    // device remove is complete. If the handle is still valid, close the handle
    // Finally free the Keyboard Context
    //
    // Case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING: The MSDN link above recommends that 
    // we handle RemovePending like RemoveComplete. We first unregister from callbacks 
    // so no further notification is received. Then it is safe to close the handle 
    // and deallocate the context.

    case CM_NOTIFY_ACTION_DEVICEREMOVEPENDING:
    case CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE:
         
        KbdUnregisterNotification(Notification);

        if(LocalKeyboardContext->KeyboardHandle != INVALID_HANDLE_VALUE)
        {
            LocalKeyboardContext->KeyboardRemoved = TRUE;
            NtClose(LocalKeyboardContext->KeyboardHandle);
            LocalKeyboardContext->KeyboardHandle = INVALID_HANDLE_VALUE;
        }

        //
        // Wait for PnpInputApc to be done before freeing the context
        // An APC event can abort the wait. So make sure to come back 
        // to the wait to avoid closing the handle before PnpInputApc
        // is completely done with the event object
        //
        do
        {
            Status = NtWaitForSingleObject(LocalKeyboardContext->InputApcDoneEvent, TRUE, NULL);
        }
        while (Status == STATUS_USER_APC);
        NtClose(LocalKeyboardContext->InputApcDoneEvent);

        //
        // Safe to free the context now
        //
        RtlFreeHeap(RtlProcessHeap(), 0, LocalKeyboardContext);

        break;

    //
    // Device remove has failed. We need to reset our structures 
    // First we unregister from the old notification
    // Then open a new handle and register 
    // Finally free any old context structures
    //
    case CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED:

        KbdUnregisterNotification(Notification);

        //
        // Query remove should have closed the keyboard handle before we 
        // get to removefailed.
        //
        NT_ASSERT(LocalKeyboardContext->KeyboardHandle == INVALID_HANDLE_VALUE);
        
        //
        // Reopen the handle and allocate a fresh context
        // and start reading from it.
        //
        KbdInitializeKeyboard(LocalKeyboardContext->SymbolicLink);

        //
        // Cleanup the old context
        // Wait for PnpInputApc before deallocating the old context.
        // An APC event can abort the wait. So make sure to come back 
        // to the wait to avoid closing the handle before PnpInputApc
        // is completely done with the event object
        //
        do
        {
            Status = NtWaitForSingleObject(LocalKeyboardContext->InputApcDoneEvent, TRUE, NULL);
        }
        while (Status == STATUS_USER_APC);

        NtClose(LocalKeyboardContext->InputApcDoneEvent);
        RtlFreeHeap(RtlProcessHeap(), 0, LocalKeyboardContext);

        break;
    }

    return ERROR_SUCCESS;
}

VOID
WINAPI
KbdKeyboardInputApcCallback(
    _In_ PKBD_PNP_KEYBOARD_CONTEXT ApcContext,
    _In_opt_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    )
{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    ByteOffset.QuadPart = 0;

    UNREFERENCED_PARAMETER(Reserved);

    //
    // KeyboardRemoved flag detected. This Keyboard device
    // is being removed. Stop processing input and return.
    //
    if (ApcContext->KeyboardRemoved == TRUE)
    {
        SetEvent(ApcContext->InputApcDoneEvent);
        return;
    }

    if (IoStatusBlock && IoStatusBlock->Information)
    {
        InpProcessKeycode(&ApcContext->KeyCode);
    }

    Status = NtReadFile(ApcContext->KeyboardHandle,
                        NULL,
                        KbdKeyboardInputApcCallback,
                        ApcContext,
                        &ApcContext->IoStatusBlock,
                        &ApcContext->KeyCode,
                        sizeof(ApcContext->KeyCode),
                        &ByteOffset,
                        NULL);

    NT_ASSERT(NT_SUCCESS(Status));
}

#pragma endregion

#pragma region Notification (Un)Registration

VOID
KbdRegisterKeyboardDeviceEvents(
    VOID
    )
{
    CONFIGRET cr = CR_SUCCESS;
    CM_NOTIFY_FILTER NotifyFilter = { 0 };

    //
    // Set initial state
    //
    PnpKeyboardNotification = NULL;

    NotifyFilter.cbSize = sizeof(NotifyFilter);
    NotifyFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    NotifyFilter.u.DeviceInterface.ClassGuid = GUID_DEVINTERFACE_KEYBOARD;
    NotifyFilter.Reserved = 0;

    cr = CM_Register_Notification(&NotifyFilter,
                                  NULL,
                                  KbdAddKeyboardCallback,
                                  &PnpKeyboardNotification);

    // 
    // Check if we failed to register for notifications
    //
    if (cr != CR_SUCCESS)
    {
        PnpKeyboardNotification = NULL;
    }
}

//
// Unregister and stop listening for 
// GUID_DEVINTERFACE_KEYBOARD events
//
VOID
KbdUnregisterKeyboardDeviceEvents(
    VOID
    )
{
    if (PnpKeyboardNotification != NULL)
    {
        CM_Unregister_Notification(PnpKeyboardNotification);

        PnpKeyboardNotification = NULL;
    }
}

// 
// Calls to unregister a PNP callback must be deferred to a different thread
// We cannot call unregister from the same thread as the notification callback
//
VOID
CALLBACK
KbdExecuteDeferredNotificationDeregistration(
    _In_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(Instance);

    CM_Unregister_Notification((HCMNOTIFICATION)Context);
}

VOID
KbdUnregisterNotification(
    _In_ HCMNOTIFICATION Notification
    )
{
    TrySubmitThreadpoolCallback(KbdExecuteDeferredNotificationDeregistration,
                                (PVOID)Notification,
                                NULL);
}

#pragma endregion
