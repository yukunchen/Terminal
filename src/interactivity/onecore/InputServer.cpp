/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "InputServer.hpp"

#include "ConIoSrv.h"

#include "..\..\host\input.h"
#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

// For details on the mechanisms employed in this class, read the comments in
// ConIoSrv.h, included above. For security-related considerations, see Trust.h
// in the ConIoSrv directory.

using namespace Microsoft::Console::Interactivity::OneCore;

#pragma region Communication

NTSTATUS InputServer::Connect()
{
    BOOL Ret = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;

    // Initialize the server port name.
    UNICODE_STRING PortName;
    Ret = RtlCreateUnicodeString(&PortName, CIS_ALPC_PORT_NAME);
    if (Ret == FALSE)
    {
        return STATUS_NO_MEMORY;
    }

    // Initialize the attributes of the port object.
    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    SIZE_T ReceiveMessageAttributesBufferLength;
    UCHAR ReceiveMessageAttributesBuffer[CIS_MSG_ATTR_BUFFER_SIZE];
    PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes
        = (PALPC_MESSAGE_ATTRIBUTES)&ReceiveMessageAttributesBuffer;
    
    Status = AlpcInitializeMessageAttribute(CIS_MSG_ATTR_FLAGS,
                                            ReceiveMessageAttributes,
                                            CIS_MSG_ATTR_BUFFER_SIZE,
                                            &ReceiveMessageAttributesBufferLength);

    const SECURITY_QUALITY_OF_SERVICE DefaultQoS = {
        sizeof (SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_DYNAMIC_TRACKING,
        FALSE
    };

    ALPC_PORT_ATTRIBUTES PortAttributes;
    PortAttributes.Flags = ALPC_PORFLG_ACCEPT_DUP_HANDLES;
    PortAttributes.MaxMessageLength = sizeof(CIS_MSG);
    PortAttributes.MaxPoolUsage = 0x4000;
    PortAttributes.MaxSectionSize = 0;
    PortAttributes.MaxTotalSectionSize = 0;
    PortAttributes.MaxViewSize = 0;
    PortAttributes.MemoryBandwidth = 0;
    PortAttributes.SecurityQos = DefaultQoS;
    PortAttributes.DupObjectTypes = OB_FILE_OBJECT_TYPE;

    HANDLE PortHandle;
    Status = NtAlpcConnectPort(&PortHandle,
                               &PortName,
                               NULL,
                               &PortAttributes,
                               ALPC_MSGFLG_SYNC_REQUEST,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               ReceiveMessageAttributes,
                               0);
    if (NT_SUCCESS(Status))
    {
        PALPC_HANDLE_ATTR HandleAttributes = ALPC_GET_HANDLE_ATTRIBUTES(ReceiveMessageAttributes);
        PALPC_DATA_VIEW_ATTR ViewAttributes = ALPC_GET_DATAVIEW_ATTRIBUTES(ReceiveMessageAttributes);

        _pipeReadHandle = HandleAttributes->Handle;

        _alpcClientCommunicationPort = PortHandle;
        _alpcSharedViewSize = ViewAttributes->ViewSize;
        _alpcSharedViewBase = ViewAttributes->ViewBase;

        memset(_alpcSharedViewBase, 0, _alpcSharedViewSize);
    }

    return Status;
}

NTSTATUS InputServer::ServiceInputPipe()
{
    BOOL Ret;
    NTSTATUS Status;

    CIS_EVENT Event = { 0 };

    while (TRUE)
    {
        Ret = ReadFile(_pipeReadHandle,
                       &Event,
                       sizeof(CIS_EVENT),
                       NULL,
                       NULL);

        if (Ret != FALSE)
        {
            switch (Event.Type)
            {
            case CIS_EVENT_TYPE_INPUT:
                HandleGenericKeyEvent(Event.InputEvent.Record, FALSE);

            case CIS_EVENT_TYPE_FOCUS:
                if (Event.FocusEvent.IsActive)
                {
                    ServiceLocator::LocateGlobals()->pRender->TriggerRedrawAll();
                }
            }
        }
        else
        {
            // If we get disconnected, terminate.
            TerminateProcess(GetCurrentProcess(), GetLastError());
        }
    }

    return Status;
}

NTSTATUS InputServer::SendRequestReceiveReply(PCIS_MSG Message)
{
    NTSTATUS Status;

    Message->AlpcHeader.MessageId = 0;
    Message->AlpcHeader.u2.ZeroInit = 0;

    Message->AlpcHeader.u1.s1.TotalLength = sizeof(CIS_MSG);
    Message->AlpcHeader.u1.s1.DataLength = sizeof(CIS_MSG) - sizeof(PORT_MESSAGE);

    Message->AlpcHeader.ClientId.UniqueProcess = 0;
    Message->AlpcHeader.ClientId.UniqueThread = 0;

    SIZE_T ActualReceiveMessageLength = sizeof(CIS_MSG);

    Status = NtAlpcSendWaitReceivePort(_alpcClientCommunicationPort,
                                       0,
                                       (PPORT_MESSAGE)Message,
                                       NULL,
                                       (PPORT_MESSAGE)Message,
                                       &ActualReceiveMessageLength,
                                       NULL,
                                       0);
    
    return Status;
}

#pragma endregion

#pragma region Request Methods

NTSTATUS InputServer::RequestGetDisplaySize(_Inout_ PCD_IO_DISPLAY_SIZE pCdDisplaySize)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_GETDISPLAYSIZE;
    
    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        *pCdDisplaySize = Message.GetDisplaySizeParams.DisplaySize;
        Status = Message.GetDisplaySizeParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestGetFontSize(_Inout_ PCD_IO_FONT_SIZE pCdFontSize)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_GETFONTSIZE;
    
    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        *pCdFontSize = Message.GetFontSizeParams.FontSize;
        Status = Message.GetFontSizeParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestSetCursor(_In_ CD_IO_CURSOR_INFORMATION* const pCdCursorInformation)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_SETCURSOR;
    Message.SetCursorParams.CursorInformation = *pCdCursorInformation;
    
    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        Status = Message.SetCursorParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestUpdateDisplay(_In_ SHORT RowIndex)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_UPDATEDISPLAY;
    Message.UpdateDisplayParams.RowIndex = RowIndex;

    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        Status = Message.UpdateDisplayParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestMapVirtualKey(_In_ UINT uCode, _In_ UINT uMapType, _Out_ UINT* puReturnValue)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_MAPVIRTUALKEY;
    Message.MapVirtualKeyParams.Code = uCode;
    Message.MapVirtualKeyParams.MapType = uMapType;

    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        *puReturnValue = Message.MapVirtualKeyParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestVkKeyScan(_In_ WCHAR wCharacter, _Out_ SHORT* psReturnValue)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_VKKEYSCAN;
    Message.VkKeyScanParams.Character = wCharacter;

    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        *psReturnValue = Message.VkKeyScanParams.ReturnValue;
    }

    return Status;
}

NTSTATUS InputServer::RequestGetKeyState(_In_ int iVirtualKey, _Out_ SHORT *psReturnValue)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_GETKEYSTATE;
    Message.GetKeyStateParams.VirtualKey = iVirtualKey;

    Status = SendRequestReceiveReply(&Message);
    if (NT_SUCCESS(Status))
    {
        *psReturnValue = Message.GetKeyStateParams.ReturnValue;
    }

    return Status;
}

PVOID InputServer::GetSharedViewBase()
{
    return _alpcSharedViewBase;
}

#pragma endregion

#pragma region IInputServices Members

UINT InputServer::MapVirtualKeyW(UINT uCode, UINT uMapType)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UINT ReturnValue;
    Status = RequestMapVirtualKey(uCode, uMapType, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
    }

    return ReturnValue;
}

SHORT InputServer::VkKeyScanW(WCHAR ch)
{
    NTSTATUS Status = STATUS_SUCCESS;

    SHORT ReturnValue;
    Status = RequestVkKeyScan(ch, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
    }

    return ReturnValue;
}

SHORT InputServer::GetKeyState(int nVirtKey)
{
    NTSTATUS Status = STATUS_SUCCESS;

    SHORT ReturnValue;
    Status = RequestGetKeyState(nVirtKey, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
    }

    return ReturnValue;
}

BOOL InputServer::TranslateCharsetInfo(DWORD * lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(lpSrc);
    UNREFERENCED_PARAMETER(lpCs);
    UNREFERENCED_PARAMETER(dwFlags);

    return FALSE;
}

#pragma endregion
