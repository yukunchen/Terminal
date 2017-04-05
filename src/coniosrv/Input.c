/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrv.h"

#include "Input.h"
#include "xlate.h"


NTSTATUS
InpDispatchInputRecord(
    _In_ INPUT_RECORD Record
    )
{
    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // This ensures that no clients are added or removed while we operate on
    // the client list.
    //

    RtlAcquireSRWLockShared(&g_CisContext->ContextLock);

    if (g_CisContext->ActiveClient)
    {
        CIS_EVENT Event = { 0 };

        Event.Type = CIS_EVENT_TYPE_INPUT;
        Event.InputEvent.Record = Record;

        
        Ret = WriteFile(g_CisContext->ActiveClient->PipeWriteHandle,
                        (LPCVOID)&Event,
                        sizeof(CIS_EVENT),
                        NULL,
                        NULL);
        if (Ret == FALSE)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    RtlReleaseSRWLockShared(&g_CisContext->ContextLock);

    return Status;
}

NTSTATUS
InpDispatchFocusEvent(
    _In_ PCIS_CLIENT ClientContext,
    _In_ BOOLEAN IsActive
    )
/*++
    This routine should be called with the server context lock held in at least
    shared mode.
--*/
{
    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    CIS_EVENT Event = { 0 };

    Event.Type = CIS_EVENT_TYPE_FOCUS;
    Event.FocusEvent.IsActive = IsActive;

    Ret = WriteFile(ClientContext->PipeWriteHandle,
                    (LPCVOID)&Event,
                    sizeof(CIS_EVENT),
                    NULL,
                    NULL);
    if (Ret != FALSE)
    {
        ClientContext->IsNextDisplayUpdateFocusRedraw = IsActive;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
InpHandleAltTab(
    _In_ BOOLEAN CycleBackwards
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PCIS_CLIENT ActiveClientContext;
    PCIS_CLIENT NextClientContext;

    RtlAcquireSRWLockExclusive(&g_CisContext->ContextLock);

    NextClientContext = NULL;
    ActiveClientContext = g_CisContext->ActiveClient;

    if (ActiveClientContext)
    {
        NextClientContext = CycleBackwards
            ? (PCIS_CLIENT)ActiveClientContext->List.Blink
            : (PCIS_CLIENT)ActiveClientContext->List.Flink;

        if (NextClientContext != ActiveClientContext)
        {
            g_CisContext->ActiveClient = NextClientContext;

            InpDispatchFocusEvent(ActiveClientContext, FALSE);
            InpDispatchFocusEvent(NextClientContext, TRUE);
        }
    }

    RtlReleaseSRWLockExclusive(&g_CisContext->ContextLock);

    return Status;
}

VOID
InpProcessKeycode(
    _In_ PKEYBOARD_INPUT_DATA Keycode
    )
{
    KE KeyData;
    BYTE bPrefix, VK;
    DWORD flags;
    INT cChar;
    WCHAR awch[MAX_CHARS_FROM_1_KEYSTROKE];
    INPUT_RECORD Record[MAX_CHARS_FROM_1_KEYSTROKE];
    INT Index;
    ULONG ControlKeyState;

    //
    // Parse out the OEM-specific prefix. These are akin to namespaces for
    // additional scancodes added by manufacturers as extra features to a
    // keyboard. They are also sometimes used to distinguish key handedness
    // (left-vs-right control, etc.)
    //
    
    if (Keycode->Flags & KEY_E0)
    {
        bPrefix = 0xE0;
    }
    else if (Keycode->Flags & KEY_E1)
    {
        bPrefix = 0xE1;
    }
    else
    {
        bPrefix = 0;
    }

    KeyData.bScanCode = (BYTE)(Keycode->MakeCode & 0x7F);

    VK = VKFromVSC(&KeyData, bPrefix);

    UpdateRawKeyState(VK, Keycode->Flags & KEY_BREAK);

    //
    // Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
    // ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
    //
    if ((VK >= VK_LSHIFT) && (VK <= VK_RMENU))
    {
        UpdateRawKeyState((BYTE)((VK - VK_LSHIFT) / 2 + VK_SHIFT), Keycode->Flags & KEY_BREAK);
    }

    if (!(Keycode->Flags & KEY_BREAK))
    {
        if (TestRawKeyDown(VK_MENU))
        {
            switch (VK)
            {
            case VK_TAB:
                InpHandleAltTab(TestRawKeyDown(VK_SHIFT));

                return;
            }
        }
    }

    //
    // If the KEY_TERMSRV_VKPACKET flag is set, the MakeCode is already a
    // Unicode character.  Otherwise, we need to translate the virtual key code
    // to Unicode. 
    //
    ControlKeyState = GetControlKeyState();

    if (Keycode->Flags & KEY_TERMSRV_VKPACKET)
    {
        cChar = 1;

        Record[0].EventType = KEY_EVENT;
        Record[0].Event.KeyEvent.bKeyDown = (Keycode->Flags & KEY_BREAK) == 0;
        Record[0].Event.KeyEvent.wRepeatCount = 1;
        Record[0].Event.KeyEvent.uChar.UnicodeChar = Keycode->MakeCode;
        Record[0].Event.KeyEvent.dwControlKeyState = ControlKeyState;
    }
    else
    {
        cChar = xxxInternalToUnicode(VK,
                                     Keycode->MakeCode,
                                     (const PBYTE)&gafRawKeyState,
                                     awch,
                                     sizeof(awch) / sizeof(awch[0]),
                                     0,
                                     &flags);
        if (cChar <= 0)
        {
            cChar = 1;
            awch[0] = UNICODE_NULL;
        }
        else if (cChar > RTL_NUMBER_OF(awch))
        {
            cChar = RTL_NUMBER_OF(awch);
        }

        for (Index = 0; Index < cChar; Index += 1)
        {
            Record[Index].EventType = KEY_EVENT;
            Record[Index].Event.KeyEvent.bKeyDown = (Keycode->Flags & KEY_BREAK) == 0;
            Record[Index].Event.KeyEvent.wRepeatCount = 1;
            Record[Index].Event.KeyEvent.wVirtualKeyCode = VK;
            Record[Index].Event.KeyEvent.wVirtualScanCode = KeyData.bScanCode;
            Record[Index].Event.KeyEvent.uChar.UnicodeChar = awch[Index];
            Record[Index].Event.KeyEvent.dwControlKeyState = ControlKeyState;
        }
    }

    for (Index = 0; Index < cChar; Index++)
    {
        InpDispatchInputRecord(Record[Index]);
    }
}
