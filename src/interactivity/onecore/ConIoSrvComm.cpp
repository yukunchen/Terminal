/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrvComm.hpp"
#include "ConIoSrv.h"

#include "..\..\host\input.h"
#include "..\..\renderer\wddmcon\wddmconrenderer.hpp"

#include "..\inc\ServiceLocator.hpp"

#pragma hdrstop

// For details on the mechanisms employed in this class, read the comments in
// ConIoSrv.h, included above. For security-related considerations, see Trust.h
// in the ConIoSrv directory.

using namespace Microsoft::Console::Interactivity::OneCore;

ConIoSrvComm::ConIoSrvComm()
    : _pipeReadHandle(INVALID_HANDLE_VALUE),
      _pipeWriteHandle(INVALID_HANDLE_VALUE),
      _alpcClientCommunicationPort(INVALID_HANDLE_VALUE),
      _alpcSharedViewSize(0),
      _alpcSharedViewBase(NULL),
      _displayMode(USHORT_MAX),
      _fIsInputInitialized(false)
{

}

#pragma region Communication

NTSTATUS ConIoSrvComm::Connect()
{
    BOOL Ret = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;

    // Port handle and name.
    HANDLE PortHandle;
    UNICODE_STRING PortName;

    // Generic Object Manager attributes for the port object and ALPC-specific
    // port attributes.
    OBJECT_ATTRIBUTES ObjectAttributes;
    ALPC_PORT_ATTRIBUTES PortAttributes;

    // Connection message.
    CIS_MSG ConnectionMessage;
    SIZE_T ConnectionMessageLength;

    // Connection message attributes.
    SIZE_T ConnectionMessageAttributesBufferLength;
    UCHAR ConnectionMessageAttributesBuffer[CIS_MSG_ATTR_BUFFER_SIZE];

    // Type-specific pointers into the connection message attributes.
    PALPC_HANDLE_ATTR HandleAttributes;
    PALPC_DATA_VIEW_ATTR ViewAttributes;

    // Structure used to iterate over the handles given to us by the server.
    ALPC_MESSAGE_HANDLE_INFORMATION HandleInfo;

    // Initialize the server port name.
    Ret = RtlCreateUnicodeString(&PortName, CIS_ALPC_PORT_NAME);
    if (Ret == FALSE)
    {
        return STATUS_NO_MEMORY;
    }

    // Initialize the attributes of the port object.
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    // Initialize the connection message attributes.
    PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes
        = (PALPC_MESSAGE_ATTRIBUTES)&ConnectionMessageAttributesBuffer;

    Status = AlpcInitializeMessageAttribute(CIS_MSG_ATTR_FLAGS,
                                            ConnectionMessageAttributes,
                                            CIS_MSG_ATTR_BUFFER_SIZE,
                                            &ConnectionMessageAttributesBufferLength);

    // Set up the default security QoS descriptor.
    const SECURITY_QUALITY_OF_SERVICE DefaultQoS = {
        sizeof (SECURITY_QUALITY_OF_SERVICE),
        SecurityAnonymous,
        SECURITY_DYNAMIC_TRACKING,
        FALSE
    };

    // Set up the port attributes.
    PortAttributes.Flags = ALPC_PORFLG_ACCEPT_DUP_HANDLES |
                           ALPC_PORFLG_ACCEPT_INDIRECT_HANDLES;
    PortAttributes.MaxMessageLength = sizeof(CIS_MSG);
    PortAttributes.MaxPoolUsage = 0x4000;
    PortAttributes.MaxSectionSize = 0;
    PortAttributes.MaxTotalSectionSize = 0;
    PortAttributes.MaxViewSize = 0;
    PortAttributes.MemoryBandwidth = 0;
    PortAttributes.SecurityQos = DefaultQoS;
    PortAttributes.DupObjectTypes = OB_FILE_OBJECT_TYPE;

    // Initialize the connection message structure.
    ConnectionMessage.AlpcHeader.MessageId = 0;
    ConnectionMessage.AlpcHeader.u2.ZeroInit = 0;

    ConnectionMessage.AlpcHeader.u1.s1.TotalLength = sizeof(CIS_MSG);
    ConnectionMessage.AlpcHeader.u1.s1.DataLength = sizeof(CIS_MSG) - sizeof(PORT_MESSAGE);

    ConnectionMessage.AlpcHeader.ClientId.UniqueProcess = 0;
    ConnectionMessage.AlpcHeader.ClientId.UniqueThread = 0;

    // Request to connect to the server.
    ConnectionMessageLength = sizeof(CIS_MSG);
    Status = NtAlpcConnectPort(&PortHandle,
                               &PortName,
                               NULL,
                               &PortAttributes,
                               ALPC_MSGFLG_SYNC_REQUEST,
                               NULL,
                               (PPORT_MESSAGE)&ConnectionMessage,
                               &ConnectionMessageLength,
                               NULL,
                               ConnectionMessageAttributes,
                               0);
    if (NT_SUCCESS(Status))
    {
        ViewAttributes = ALPC_GET_DATAVIEW_ATTRIBUTES(ConnectionMessageAttributes);
        HandleAttributes = ALPC_GET_HANDLE_ATTRIBUTES(ConnectionMessageAttributes);

        // We must have exactly two handles, one for read, and one for write for
        // the pipe.
        if (HandleAttributes->HandleCount != 2)
        {
            Status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(Status))
        {
            // Get each handle out. ALPC does not allow to pass indirect handles
            // all at once; they must be retrieved one by one.
            for (ULONG Index = 0 ; Index < HandleAttributes->HandleCount ; Index++)
            {
                HandleInfo.Index = Index;

                Status = NtAlpcQueryInformationMessage(PortHandle,
                                                       (PPORT_MESSAGE)&ConnectionMessage,
                                                       AlpcMessageHandleInformation,
                                                       &HandleInfo,
                                                       sizeof(HandleInfo),
                                                       NULL);
                if (NT_SUCCESS(Status))
                {
                    if (Index == 0)
                    {
                        _pipeReadHandle = ULongToHandle(HandleInfo.Handle);
                    }
                    else if (Index == 1)
                    {
                        _pipeWriteHandle = ULongToHandle(HandleInfo.Handle);
                    }
                }
            }

            // Keep the shared view information.
            _alpcSharedViewSize = ViewAttributes->ViewSize;
            _alpcSharedViewBase = ViewAttributes->ViewBase;

            // As well as a pointer to our communication port.
            _alpcClientCommunicationPort = PortHandle;

            // Zero out the view.
            memset(_alpcSharedViewBase, 0, _alpcSharedViewSize);
        }
    }

    return Status;
}

NTSTATUS ConIoSrvComm::EnsureConnection()
{
    NTSTATUS Status;

    if (_alpcClientCommunicationPort == INVALID_HANDLE_VALUE)
    {
        Status = Connect();
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS ConIoSrvComm::ServiceInputPipe()
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
            break;

            case CIS_EVENT_TYPE_FOCUS:
                HandleFocusEvent(&Event);
            break;
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

NTSTATUS ConIoSrvComm::SendRequestReceiveReply(PCIS_MSG Message) const
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

VOID ConIoSrvComm::HandleFocusEvent(PCIS_EVENT Event)
{
    BOOL Ret;
    NTSTATUS Status;

    USHORT DisplayMode;

    IRenderer *Renderer;
    WddmConEngine *WddmEngine;

    CIS_EVENT ReplyEvent;
    
    Status = RequestGetDisplayMode(&DisplayMode);

    if (NT_SUCCESS(Status))
    {
        Renderer = ServiceLocator::LocateGlobals()->pRender;

        switch (DisplayMode)
        {
            case CIS_DISPLAY_MODE_BGFX:
                if (Event->FocusEvent.IsActive)
                {
                    // Allow the renderer to paint (this has an effect only on
                    // the first call).
                    Renderer->EnablePainting();

                    // Force a complete redraw.
                    Renderer->TriggerRedrawAll();
                }
            break;

            case CIS_DISPLAY_MODE_DIRECTX:
                WddmEngine = (WddmConEngine *)ServiceLocator::LocateGlobals()->pRenderEngine;

                if (Event->FocusEvent.IsActive)
                {
                    HRESULT hr = S_OK;

                    // Lazy-initialize the WddmCon engine.
                    //
                    // This is necessary because the engine cannot be allowed to
                    // request ownership of the display before whatever instance
                    // of conhost was using it before has relinquished it.
                    if (!WddmEngine->IsInitialized())
                    {
                        hr = WddmEngine->Initialize();
                        LOG_IF_FAILED(hr);
                    }
                    
                    if (SUCCEEDED(hr))
                    {
                        // Allow acquiring device resources before drawing.
                        hr = WddmEngine->Enable();
                        LOG_IF_FAILED(hr);
                        if (SUCCEEDED(hr))
                        {
                            // Allow the renderer to paint.
                            Renderer->EnablePainting();

                            // Force a complete redraw.
                            Renderer->TriggerRedrawAll();
                        }
                    }
                }
                else
                {
                    if (WddmEngine->IsInitialized())
                    {
                        // Wait for the currently running paint operation, if any,
                        // and prevent further attempts to render.
                        Renderer->WaitForPaintCompletionAndDisable(1000);

                        // Relinquish control of the graphics device (only one
                        // DirectX application may control the device at any one
                        // time).
                        LOG_IF_FAILED(WddmEngine->Disable());

                        // Let the Console IO Server that we have relinquished
                        // control of the display.
                        ReplyEvent.Type = CIS_EVENT_TYPE_FOCUS_ACK;
                        Ret = WriteFile(_pipeWriteHandle,
                                        &ReplyEvent,
                                        sizeof(CIS_EVENT),
                                        NULL,
                                        NULL);
                    }
                }
            break;

            case CIS_DISPLAY_MODE_NONE:
                // Focus events have no meaning in a headless environment.
            break;
        }
    }
}

VOID ConIoSrvComm::SignalInputEventIfNecessary()
{
    if (!_fIsInputInitialized)
    {
        // Signal that input is ready to go.
        ServiceLocator::LocateGlobals()->hConsoleInputInitEvent.SetEvent();

        _fIsInputInitialized = true;
    }
}

#pragma endregion

#pragma region Request Methods

NTSTATUS ConIoSrvComm::RequestGetDisplaySize(_Inout_ PCD_IO_DISPLAY_SIZE pCdDisplaySize) const
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

NTSTATUS ConIoSrvComm::RequestGetFontSize(_Inout_ PCD_IO_FONT_SIZE pCdFontSize) const
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

NTSTATUS ConIoSrvComm::RequestSetCursor(_In_ CD_IO_CURSOR_INFORMATION* const pCdCursorInformation) const
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

NTSTATUS ConIoSrvComm::RequestUpdateDisplay(_In_ SHORT RowIndex) const
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

NTSTATUS ConIoSrvComm::RequestMapVirtualKey(_In_ UINT uCode, _In_ UINT uMapType, _Out_ UINT* puReturnValue)
{
    NTSTATUS Status;

    Status = EnsureConnection();
    if (NT_SUCCESS(Status))
    {
        CIS_MSG Message = { 0 };
        Message.Type = CIS_MSG_TYPE_MAPVIRTUALKEY;
        Message.MapVirtualKeyParams.Code = uCode;
        Message.MapVirtualKeyParams.MapType = uMapType;

        Status = SendRequestReceiveReply(&Message);
        if (NT_SUCCESS(Status))
        {
            *puReturnValue = Message.MapVirtualKeyParams.ReturnValue;
        }
    }

    return Status;
}

NTSTATUS ConIoSrvComm::RequestVkKeyScan(_In_ WCHAR wCharacter, _Out_ SHORT* psReturnValue)
{
    NTSTATUS Status;

    Status = EnsureConnection();
    if (NT_SUCCESS(Status))
    {
        CIS_MSG Message = { 0 };
        Message.Type = CIS_MSG_TYPE_VKKEYSCAN;
        Message.VkKeyScanParams.Character = wCharacter;

        Status = SendRequestReceiveReply(&Message);
        if (NT_SUCCESS(Status))
        {
            *psReturnValue = Message.VkKeyScanParams.ReturnValue;
        }
    }

    return Status;
}

NTSTATUS ConIoSrvComm::RequestGetKeyState(_In_ int iVirtualKey, _Out_ SHORT *psReturnValue)
{
    NTSTATUS Status;

    Status = EnsureConnection();
    if (NT_SUCCESS(Status))
    {
        CIS_MSG Message = { 0 };
        Message.Type = CIS_MSG_TYPE_GETKEYSTATE;
        Message.GetKeyStateParams.VirtualKey = iVirtualKey;

        Status = SendRequestReceiveReply(&Message);
        if (NT_SUCCESS(Status))
        {
            *psReturnValue = Message.GetKeyStateParams.ReturnValue;
        }
    }

    return Status;
}

NTSTATUS ConIoSrvComm::RequestGetDisplayMode(_Out_ USHORT *psDisplayMode)
{
    NTSTATUS Status;

    CIS_MSG Message = { 0 };
    Message.Type = CIS_MSG_TYPE_GETDISPLAYMODE;

    // Cache the response (USHORT_MAX = not set yet).
    if (_displayMode == USHORT_MAX)
    {
        Status = SendRequestReceiveReply(&Message);
        if (NT_SUCCESS(Status))
        {
            _displayMode = Message.GetDisplayModeParams.DisplayMode;
            *psDisplayMode = _displayMode;
        }
    }
    else
    {
        *psDisplayMode = _displayMode;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

PVOID ConIoSrvComm::GetSharedViewBase() const
{
    return _alpcSharedViewBase;
}

#pragma endregion

#pragma region IInputServices Members

UINT ConIoSrvComm::MapVirtualKeyW(UINT uCode, UINT uMapType)
{
    NTSTATUS Status = STATUS_SUCCESS;

    UINT ReturnValue;
    Status = RequestMapVirtualKey(uCode, uMapType, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    return ReturnValue;
}

SHORT ConIoSrvComm::VkKeyScanW(WCHAR ch)
{
    NTSTATUS Status = STATUS_SUCCESS;

    SHORT ReturnValue;
    Status = RequestVkKeyScan(ch, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    return ReturnValue;
}

SHORT ConIoSrvComm::GetKeyState(int nVirtKey)
{
    NTSTATUS Status = STATUS_SUCCESS;

    SHORT ReturnValue;
    Status = RequestGetKeyState(nVirtKey, &ReturnValue);
    
    if (!NT_SUCCESS(Status))
    {
        ReturnValue = 0;
        SetLastError(ERROR_PROC_NOT_FOUND);
    }

    return ReturnValue;
}

BOOL ConIoSrvComm::TranslateCharsetInfo(DWORD * lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(lpSrc);
    UNREFERENCED_PARAMETER(lpCs);
    UNREFERENCED_PARAMETER(dwFlags);

    SetLastError(ERROR_PROC_NOT_FOUND);

    return FALSE;
}

#pragma endregion
