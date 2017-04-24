/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrv.h"

#include "Api.h"
#include "Client.h"
#include "Input.h"
#include "Server.h"

#pragma region Forward Declarations

DWORD
WINAPI
CisServerCommunicationPortThreadProc(
    _In_ LPVOID lpParam
);

#pragma endregion

#pragma region Server Lifecycle Methods

NTSTATUS
CisInitializePortSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS Status;

    PSID WorldSid = NULL;

    PACL PortDacl = NULL;
    ULONG PortDaclLength = 0;

    //
    // Well-known SID's.
    //

    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Initialize one SID as:
    // World Authority -> World Principal
    //

    Status = RtlAllocateAndInitializeSid(&WorldAuthority ,
                                         1,
                                         SECURITY_WORLD_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &WorldSid);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Allocate a Discretionary Access Control List.
    //
    
    PortDaclLength = sizeof(ACL) +
                     (1 * sizeof(ACCESS_ALLOWED_ACE)) + // 1 for one principal
                     RtlLengthSid(WorldSid);
    PortDacl = RtlAllocateHeap(RtlProcessHeap(),
                               0,
                               PortDaclLength);
    Status = NT_ALLOCSUCCESS(PortDacl); 
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Initialize the Security Descriptor.
    //

    Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Initialize the DACL.
    //

    Status = RtlCreateAcl(PortDacl,
                          PortDaclLength,
                          ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Add the World SID.
    //
    
    Status = RtlAddAccessAllowedAce(PortDacl,
                                    ACL_REVISION,
                                    PORT_CONNECT,
                                    WorldSid);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }
    
    //
    // Set the DACL into the Security Descriptor.
    //

    Status = RtlSetDaclSecurityDescriptor(SecurityDescriptor,
                                          TRUE,
                                          PortDacl,
                                          FALSE);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

Exit:
    if (!NT_SUCCESS(Status))
    {
        if (PortDacl)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, PortDacl);
        }
    }

    if (WorldSid)
    {
        RtlFreeSid(WorldSid);
    }

    return Status;
}

VOID
CisFreePortSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PACL Dacl;
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;

    NTSTATUS Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                                   &DaclPresent,
                                                   &Dacl,
                                                   &DaclDefaulted);
    if (NT_SUCCESS(Status) && DaclPresent != FALSE && Dacl != NULL)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, Dacl);
    }
}

NTSTATUS
CisInitializeServerGlobals(
    VOID
    )
{
    NTSTATUS Status;

    g_CisContext = (PCIS_CONTEXT)RtlAllocateHeap(RtlProcessHeap(),
                                                 0,
                                                 sizeof(CIS_CONTEXT));
    Status = NT_ALLOCSUCCESS(g_CisContext);

    if (NT_SUCCESS(Status))
    {
        g_CisContext->ServerPort = INVALID_HANDLE_VALUE;

        g_CisContext->ActiveClient = NULL;

        g_CisContext->Display.Handle = NULL;
        g_CisContext->Display.Height = 0;
        g_CisContext->Display.Width = 0;

        RtlInitializeSRWLock(&g_CisContext->ContextLock);
    }

    return Status;
}

NTSTATUS
CisInitializeServerConnectionPort(
    VOID
    )
{
    BOOLEAN Ret;
    NTSTATUS Status;

    HANDLE ServerPort = NULL;
    UNICODE_STRING PortName = { 0 };

    SECURITY_DESCRIPTOR PortSecurityDescriptor = { 0 };

    //
    // Initialize the server port name.
    //

    Ret = RtlCreateUnicodeString(&PortName, CIS_ALPC_PORT_NAME);
    if (Ret == FALSE)
    {
        Status = STATUS_NO_MEMORY;

        goto Exit;
    }

    //
    // Initialize the security descriptor for the port.
    //

    Status = CisInitializePortSecurityDescriptor(&PortSecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Initialize the attributes of the port object.
    //

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                               &PortName,
                               0,
                               NULL,
                               &PortSecurityDescriptor);

    const SECURITY_QUALITY_OF_SERVICE DefaultQoS = {
        sizeof(SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_DYNAMIC_TRACKING,
        FALSE
    };

    //
    // Initialize the ALPC port attributes.
    //

    ALPC_PORT_ATTRIBUTES PortAttributes;
    PortAttributes.Flags = 0;
    PortAttributes.MaxMessageLength = sizeof(CIS_MSG);
    PortAttributes.MaxPoolUsage = 0x4000;
    PortAttributes.MaxSectionSize = CIS_SHARED_VIEW_SIZE;
    PortAttributes.MaxTotalSectionSize = PortAttributes.MaxSectionSize;
    PortAttributes.MaxViewSize = PortAttributes.MaxSectionSize;
    PortAttributes.MemoryBandwidth = 0;
    PortAttributes.SecurityQos = DefaultQoS;
    PortAttributes.DupObjectTypes = 0;

    //
    // Create the request port.
    //

    Status = NtAlpcCreatePort(&ServerPort,
                              &ObjectAttributes,
                              &PortAttributes);
    if (NT_SUCCESS(Status))
    {
        g_CisContext->ServerPort = ServerPort;
    }

Exit:
    CisFreePortSecurityDescriptor(&PortSecurityDescriptor);

    if (PortName.Buffer)
    {
        RtlFreeUnicodeString(&PortName);
    }

    return Status;
}

NTSTATUS
CisInitializeServer(
    VOID
    )
{
    NTSTATUS Status;
    HANDLE Thread;

    Status = CisInitializeServerConnectionPort();
    if (NT_SUCCESS(Status))
    {
        Thread = CreateThread(NULL,
                                0,
                                &CisServerCommunicationPortThreadProc,
                                NULL,
                                0,
                                NULL);
        Status = NT_ALLOCSUCCESS(Thread);
    }

    return Status;
}

#pragma endregion

#pragma region ALPC Message Handlers

NTSTATUS
CisServiceConnectionRequest(
    _In_ PCIS_MSG ReceiveMessage,
    _Inout_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes
    )
{
    BOOL Ret;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PCIS_CLIENT ClientContext = NULL;
    HANDLE ClientProcessHandle = NULL;
    HANDLE ServerCommunicationPort = NULL;
    BOOLEAN AcceptConnection = FALSE;

    HANDLE PipeReadHandle = NULL;
    HANDLE PipeWriteHandle = NULL;

    ALPC_HANDLE SectionHandle = NULL;
    SIZE_T SectionSize = CIS_SHARED_VIEW_SIZE;
    
    PALPC_HANDLE_ATTR HandleAttributes = NULL;
    PALPC_DATA_VIEW_ATTR ViewAttributes = NULL;

    //
    // Retrieve a handle to the client process.
    //
    // N.B.: This must take place before accepting the connection, at which
    //       point the owner of the message will change to be us (CSRSS) and
    //       trying to open the "client" process will no longer work.
    //
    
    ClientProcessHandle = CisOpenClientProcess(ReceiveMessage);

    //
    // If we succeeded in opening the client process, and the process is
    // eligible as a client, then go on and process the connection request.
    //

    if (ClientProcessHandle && CisCanRegisterClient(ClientProcessHandle))
    {
        //
        // Retrieve a pointer to the view attributes within the attributes of the
        // receive message buffer.
        //

        ViewAttributes = ALPC_GET_DATAVIEW_ATTRIBUTES(ReceiveMessageAttributes);

        //
        // Same with the handle attributes.
        //

        HandleAttributes = ALPC_GET_HANDLE_ATTRIBUTES(ReceiveMessageAttributes);

        //
        // Create a memory section and associate it with the server
        // connection port. The final size of the section may be rounded up
        // by the memory manager.
        //

        Status = NtAlpcCreatePortSection(g_CisContext->ServerPort,  // Server Connection Port
                                         0,                         // Creation Flags
                                         NULL,                      // Existing Section Handle
                                         SectionSize,               // Requested Section Size
                                         &SectionHandle,            // Pointer to new ALPC Section Handle
                                         &SectionSize);             // Actual Section Size
        if (NT_SUCCESS(Status))
        {
            ViewAttributes->Flags = 0;                      // Must be zero as per the ALPC spec.
            ViewAttributes->ViewBase = NULL;                // Filled in by NtAlpcCreateSectionView.
            ViewAttributes->ViewSize = SectionSize;
            ViewAttributes->SectionHandle = SectionHandle;

            //
            // Map the section into our address space.
            //

            Status = NtAlpcCreateSectionView(g_CisContext->ServerPort,  // Server Connection Port
                                             0,                         // Must be zero as per the ALPC spec.
                                             ViewAttributes);           // Receive Message Attributes
            if (NT_SUCCESS(Status))
            {
                
                //
                // Mark the Data View attribute as valid since we have
                // successfully allocated a section and mapped the view.
                //

                ReceiveMessageAttributes->ValidAttributes |= ALPC_FLG_MSG_DATAVIEW_ATTR;

                //
                // Create an anonymous pipe. We keep a reference to the write
                // handle, and pass the read handle to the client via ALPC.
                // Then, whenever we need to dispatch events to the client, we
                // do so over the pipe.
                //

                Ret = CreatePipe(&PipeReadHandle,
                                 &PipeWriteHandle,
                                 NULL,
                                 0);
                if (Ret != FALSE)
                {
                    HandleAttributes->Flags = 0;
                    HandleAttributes->ObjectType = 0;
                    HandleAttributes->Handle = PipeReadHandle;
                    HandleAttributes->DesiredAccess = GENERIC_READ;

                    //
                    // Mark the Handle Attribute as valid since we have
                    // successfully allocated a pipe.
                    //

                    ReceiveMessageAttributes->ValidAttributes |= ALPC_FLG_MSG_HANDLE_ATTR;

                    //
                    // Register the client and its associated data.
                    //
                    // N.B.: We want to set the client context structure for this
                    //       new client as the port context inside the ALPC context
                    //       message attribute. As such, we must allocate and
                    //       initialize the structure before accepting the 
                    //       connection, since it is in that call that we pass the
                    //       context pointer.
                    //

                    Status = CisRegisterClient(ReceiveMessage,
                                               ClientProcessHandle,
                                               PipeReadHandle,
                                               PipeWriteHandle,
                                               SectionHandle,
                                               SectionSize,
                                               ViewAttributes->ViewBase,
                                               &ClientContext);
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // If all is well, accept the connection.
                        //

                        AcceptConnection = TRUE;
                    }
                    else
                    {
                        //
                        // Otherwise, close the pipe, unmap the view, and close
                        // the shared section.
                        //

                        NtClose(PipeReadHandle);
                        NtClose(PipeWriteHandle);
                        NtAlpcDeleteSectionView(g_CisContext->ServerPort,
                                                0,
                                                ViewAttributes->ViewBase);
                        NtAlpcDeletePortSection(g_CisContext->ServerPort,
                                                0,
                                                SectionHandle);
                    }
                }
                else
                {
                    //
                    // On error, unmap the view and close the shared section.
                    //

                    NtAlpcDeleteSectionView(g_CisContext->ServerPort,
                                            0,
                                            ViewAttributes->ViewBase);
                    
                    NtAlpcDeletePortSection(g_CisContext->ServerPort,
                                            0,
                                            SectionHandle);
                }
            }
            else
            {
                //
                // Delete the section if we failed to map the view.
                //

                NtAlpcDeletePortSection(g_CisContext->ServerPort,
                                        0,
                                        SectionHandle);
            }
        }

        //
        // As per the ALPC spec., this method must always be called, whether we
        // accept the connection or not (makes sense since it unblocks the
        // client).
        //

        Status = NtAlpcAcceptConnectPort(&ServerCommunicationPort,       // Server Communication Port for this client
                                         g_CisContext->ServerPort,       // Server Connection Port
                                         0,                              // Flags
                                         NULL,                           // Port Object Attributes
                                         NULL,                           // ALPC Port Attributes
                                         ClientContext,                  // Port Context (-> To Client Context)
                                         (PPORT_MESSAGE)ReceiveMessage,  // Connection Request Message
                                         ReceiveMessageAttributes,       // Connection Request Message Attributes
                                         AcceptConnection);              // Accept or Reject this connection
        if (NT_SUCCESS(Status))
        {
            if (AcceptConnection)
            {
                //
                // The client context was allocated and initialized prior to
                // accepting the connection, so now is the time to set the
                // server communication port.
                //
                
                ClientContext->ServerCommunicationPort = ServerCommunicationPort;
            }
        }
        else
        {
            if (PipeReadHandle)
            {
                NtClose(PipeReadHandle);
            }

            if (PipeWriteHandle)
            {
                NtClose(PipeWriteHandle);
            }

            if (ViewAttributes->ViewBase)
            {
                NtAlpcDeleteSectionView(g_CisContext->ServerPort,
                                        0,
                                        ViewAttributes->ViewBase);
            }

            if (SectionHandle)
            {
                NtAlpcDeletePortSection(g_CisContext->ServerPort,
                                        0,
                                        SectionHandle);
            }

            if (ServerCommunicationPort)
            {
                NtClose(ServerCommunicationPort);
            }

            if (ClientContext)
            {
                if (CisIsClientRegistered(ClientContext->ProcessHandle))
                {
                    CisDeregisterClient(ClientContext);
                }

                RtlFreeHeap(RtlProcessHeap(), 0, ClientContext);
            }
        }
    }

    return Status;
}

VOID
CisServiceApiRequest(
    _Inout_ PCIS_MSG ReceiveReplyMessage,
    _In_ PCIS_CLIENT ClientContext
)
{
    if (ClientContext && CisIsClientRegistered(ClientContext->ProcessHandle))
    {
        switch (ReceiveReplyMessage->Type)
        {
        case CIS_MSG_TYPE_MAPVIRTUALKEY:
        {
            UINT ReturnValue = ApiMapVirtualKey(ReceiveReplyMessage->MapVirtualKeyParams.Code,
                ReceiveReplyMessage->MapVirtualKeyParams.MapType);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->MapVirtualKeyParams.ReturnValue = ReturnValue;
        }
        break;

        case CIS_MSG_TYPE_VKKEYSCAN:
        {
            SHORT ReturnValue = ApiVkKeyScan(ReceiveReplyMessage->VkKeyScanParams.Character);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->VkKeyScanParams.ReturnValue = ReturnValue;
        }
        break;

        case CIS_MSG_TYPE_GETKEYSTATE:
        {
            SHORT ReturnValue = ApiGetKeyState(ReceiveReplyMessage->GetKeyStateParams.VirtualKey);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->GetKeyStateParams.ReturnValue = ReturnValue;
        }
        break;

        case CIS_MSG_TYPE_GETDISPLAYSIZE:
        {
            NTSTATUS ReturnValue = ApiGetDisplaySize(&ReceiveReplyMessage->GetDisplaySizeParams.DisplaySize);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->GetDisplaySizeParams.ReturnValue = ReturnValue;
        }
        break;

        case CIS_MSG_TYPE_GETFONTSIZE:
        {
            NTSTATUS ReturnValue = ApiGetFontSize(&ReceiveReplyMessage->GetFontSizeParams.FontSize);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->GetFontSizeParams.ReturnValue = ReturnValue;
        }
        break;

        case CIS_MSG_TYPE_SETCURSOR:
        {
            NTSTATUS ReturnValue = ApiSetCursor(ReceiveReplyMessage->SetCursorParams.CursorInformation, ClientContext);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->SetCursorParams.ReturnValue = ReturnValue;
        }

        case CIS_MSG_TYPE_UPDATEDISPLAY:
        {
            NTSTATUS ReturnValue = ApiUpdateDisplay(ReceiveReplyMessage, ClientContext);

            ReceiveReplyMessage->IsSuccessful = TRUE;
            ReceiveReplyMessage->SetCursorParams.ReturnValue = ReturnValue;
        }
        break;

        default:
            ReceiveReplyMessage->IsSuccessful = FALSE;
        break;
        }
    }
}

NTSTATUS
CisServiceClientTermination(
    _In_ PCIS_CLIENT ClientContext
    )
{
    NTSTATUS DeleteViewStatus;
    NTSTATUS DeleteSectionStatus;
    NTSTATUS DisconnectPortStatus;
    NTSTATUS ClosePortStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    if (ClientContext && CisIsClientRegistered(ClientContext->ProcessHandle))
    {
        //
        // Remove the client from the list of clients.
        //

        CisDeregisterClient(ClientContext);

        //
        // Send an unfocus event to the previous client, and a focus event to
        // the next active one, if any.
        //

        InpDispatchFocusEvent(ClientContext, FALSE);
        if (g_CisContext->ActiveClient)
        {
            InpDispatchFocusEvent(g_CisContext->ActiveClient, TRUE);
        }

        //
        // Unmap the view, close the shared section, close the pipe, disconnect
        // and close the port.
        //

        NtClose(ClientContext->PipeReadHandle);

        NtClose(ClientContext->PipeWriteHandle);

        DeleteViewStatus = NtAlpcDeleteSectionView(g_CisContext->ServerPort,
                                                   0,
                                                   ClientContext->SharedSection.ViewBase);

        DeleteSectionStatus = NtAlpcDeletePortSection(g_CisContext->ServerPort,
                                                      0,
                                                      ClientContext->SharedSection.Handle);

        DisconnectPortStatus = NtAlpcDisconnectPort(ClientContext->ServerCommunicationPort,
                                                    0);

        ClosePortStatus = NtClose(ClientContext->ServerCommunicationPort);

        //
        // Free the client context structure's memory.
        //

        RtlFreeHeap(RtlProcessHeap(),
                    0,
                    ClientContext);

        //
        // In case of failure, set the return status as the earliest failure.
        //
        
        if (!NT_SUCCESS(ClosePortStatus))
        {
            Status = ClosePortStatus;
        }
        if (!NT_SUCCESS(DisconnectPortStatus))
        {
            Status = DisconnectPortStatus;
        }
        if (!NT_SUCCESS(DeleteSectionStatus))
        {
            Status = DeleteSectionStatus;
        }
        if (!NT_SUCCESS(DeleteViewStatus))
        {
            Status = DeleteViewStatus;
        }
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

#pragma endregion

#pragma region ALPC Message Loop

NTSTATUS
CisServiceServerConnectionPort(
    VOID
    )
{
    NTSTATUS Status;

    //
    // For the server connection port, we don't require the client to send
    // anything back after our reply, so ALPC does not need to keep messages
    // around.
    //

    ULONG SendReceiveFlags = ALPC_MSGFLG_RELEASE_MESSAGE;

    //
    // ReceiveMessage holds the contents of any one message the client sends to
    // us.
    //

    CIS_MSG ReceiveMessage = { 0 };
    SIZE_T ActualReceiveMessageLength = sizeof(CIS_MSG);

    //
    // ReplyMessage points to the message contents to send back to the client
    // after the client sends a request to us. Not all requests warrant a
    // reply, so this may sometimes be null.
    //
    // N.B.: This variable is a pointer because we reuse the contents of
    //       ReceiveMessage as the reply message.
    //

    PCIS_MSG ReplyMessage = NULL;

    //
    // ALPC allows consumers to send and receive additional data with messages
    // which ALPC has special provisions for. For intance, for large messages,
    // ALPC allows client and server to share a view (a chunk of memory). This
    // information is contained in a message attribute. A message can have more
    // than one attribute, so we need to create a buffer to hold the data for
    // these attributes (in our case, only one for views) and the header that
    // describes the attributes we use.
    //

    SIZE_T ReceiveMessageAttributesBufferLength;
    UCHAR ReceiveMessageAttributesBuffer[CIS_MSG_ATTR_BUFFER_SIZE];
    PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes
        = (PALPC_MESSAGE_ATTRIBUTES)&ReceiveMessageAttributesBuffer;

    //
    // Initialize the structure's contents.
    //

    Status = AlpcInitializeMessageAttribute(CIS_MSG_ATTR_FLAGS,
                                            ReceiveMessageAttributes,
                                            CIS_MSG_ATTR_BUFFER_SIZE,
                                            &ReceiveMessageAttributesBufferLength);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    //
    // Main Receive/Reply loop.
    //

    while (TRUE)
    {
        Status = NtAlpcSendWaitReceivePort(g_CisContext->ServerPort,        // Server Communication Port
                                           SendReceiveFlags,                // Flags
                                           (PPORT_MESSAGE)ReplyMessage,     // Reply Message (if any)
                                           NULL,                            // Reply Message Attributes
                                           (PPORT_MESSAGE)&ReceiveMessage,  // Receive Message
                                           &ActualReceiveMessageLength,     // Receive Message Length
                                           ReceiveMessageAttributes,        // Receive Message Attributes
                                           0);                              // Receive Wait Timeout (0 = Infinite)

        //
        // By default, we do not have a reply message for when the loop loops
        // back in.
        //

        ReplyMessage = NULL;

        //
        // If we fail to Send/Receive, ignore the message and do not reply.
        //

        if (!NT_SUCCESS(Status))
        {
            continue;
        }

        //
        // Extract the context attribute from the message.
        //

        PALPC_CONTEXT_ATTR ContextAttribute = ALPC_GET_CONTEXT_ATTRIBUTES(ReceiveMessageAttributes);

        //
        // We use the context attribute to hold a pointer to the client context
        // structure instead of having to look it up every time.
        //
        // N.B.: This is NULL for connection requests.
        //

        PCIS_CLIENT ClientContext = (PCIS_CLIENT)ContextAttribute->PortContext;

        //
        // Lock the server's context whenever servicing a request.
        //

        RtlAcquireSRWLockExclusive(&g_CisContext->ContextLock);
        
        //
        // Determine what type of message we have just received.
        //

        USHORT MessageType = ALPC_GET_MESSAGE_TYPE((PPORT_MESSAGE)&ReceiveMessage);
        switch (MessageType)
        {
        case LPC_CONNECTION_REQUEST:
            Status = CisServiceConnectionRequest(&ReceiveMessage,
                                                 ReceiveMessageAttributes);

            break;

        case LPC_REQUEST:
            CisServiceApiRequest(&ReceiveMessage, ClientContext);
            ReplyMessage = &ReceiveMessage;

            break;
        
        case LPC_PORT_CLOSED:
        case LPC_CLIENT_DIED:
            CisServiceClientTermination(ClientContext);

            break;
        }

        //
        // Release the lock during the wait until a new request arrives.
        //

        RtlReleaseSRWLockExclusive(&g_CisContext->ContextLock);        
    }

Exit:
    return Status;
}

DWORD
WINAPI
CisServerCommunicationPortThreadProc(
    _In_ LPVOID lpParam
    )
{
    UNREFERENCED_PARAMETER(lpParam);

    CisServiceServerConnectionPort();

    return 0;
}

#pragma endregion
