/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ConIoSrv.h"
#include "Display.h"

#define CONSOLE_DISPLAY_OBJECT          L"\\Device\\ConDrv\\Display"
#define FILE_SHARE_VALID_FLAGS          0x00000007

NTSTATUS
ApiGetDisplaySize(
    _Inout_ CD_IO_DISPLAY_SIZE* const pDisplaySize
    )
{
    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    DWORD cbWritten = 0;
    CD_IO_DISPLAY_SIZE DisplaySize;
    
    Ret = DeviceIoControl(g_CisContext->Display.Handle,
                          IOCTL_CONDRV_GET_DISPLAY_SIZE,
                          NULL,
                          0,
                          &DisplaySize,
                          sizeof(DisplaySize),
                          &cbWritten,
                          NULL);
    
    if (Ret != FALSE)
    {
        *pDisplaySize = DisplaySize;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    
    return Status;
}

NTSTATUS
ApiGetFontSize(
    _Inout_ CD_IO_FONT_SIZE* const pFontSize
    )
{
    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    DWORD cbWritten = 0;
    CD_IO_FONT_SIZE FontSize;
    
    Ret = DeviceIoControl(g_CisContext->Display.Handle,
                          IOCTL_CONDRV_GET_FONT_SIZE,
                          NULL,
                          0,
                          &FontSize,
                          sizeof(FontSize),
                          &cbWritten,
                          NULL);
    
    if (Ret != FALSE)
    {
        *pFontSize = FontSize;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    
    return Status;
}

NTSTATUS
ApiSetCursor(
    _In_ CD_IO_CURSOR_INFORMATION CursorInformation,
    _In_ PCIS_CLIENT ClientContext
    )
{
    BOOL Ret;
    DWORD cbWritten = 0;
    
    if (g_CisContext->ActiveClient == ClientContext)
    {
        Ret = DeviceIoControl(g_CisContext->Display.Handle,
                              IOCTL_CONDRV_SET_CURSOR,
                              &CursorInformation,
                              sizeof(CursorInformation),
                              NULL,
                              0,
                              &cbWritten,
                              NULL);
    }
    else
    {
        Ret = FALSE;
    }
    
    return Ret != FALSE ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
ApiUpdateDisplay(
    _In_ PCIS_MSG Message,
    _In_ PCIS_CLIENT ClientContext
    )
{
    //
    // Note about PCIS_CLIENT->IsNextDisplayUpdateFocusRedraw:
    //
    // The console driver's UpdateDisplay IOCTL takes in three parameters:
    //
    // 1. The index of the row being rendered (top-to-bottom, zero-based);
    // 2. The previous (Old) state of that row (characters + attributes);
    // 3. The current (New) state of that row (idem).
    //    -> Old is what is currently on the screen prior to our rendering, and
    //       New is what we are going to replace it with.
    //
    // The console driver goes through every character in the row and compares
    // Old and New. If they are the same, it doesn't do anything. Note! The
    // console driver decides whether a character has changed, either in actual
    // character, attribute, or both, based on what WE give to it; it keeps no
    // internal state whatsoever. If we make the console driver believe that the
    // screen has not changed with say Old[index] = { A, 0xF } and
    // New[index] = { A, 0xF } while on screen there in fact is { B, 0xD },
    // nothing will happen, and { B, 0xD } will remain on screen.
    //
    // The problem here originates from swtiching from one console application
    // to another. Consider the following sequence of events:
    //
    // 1. Console A starts with an empty Old buffer, puts some characters into
    //    the New buffer, and asks to be rendered. Old != New and the changes
    //    are rendered. To keep state for the next time it needs to render,
    //    Console A sets Old = New such that the current state will be previous
    //    state the next time around;
    // 2. Console B starts and becomes the foreground application. It does the
    //    same as Console A. On screen there are now Console B's contents;
    // 3. The user switches to Console A, and the Console IO Server asks it to
    //    render itself;
    // 4. Console A goes on to render and finds that a few characters have
    //    changed with respect to its Old buffer. It proceeds as before and asks
    //    the Console IO Server to render its New buffer, using its Old buffer
    //    to base the differential rendering off of. However, this Old buffer no
    //    longer reflects what is on screen because Console B had come along and
    //    changed what is actually on screen. At this point, what is truly on
    //    screen and Console A's Old buffer are no longer in sync. By the time
    //    it's done rendering itself, the screen is corrupted.
    //
    // The solution is to force wiping out the Old buffer of Console A after the
    // switch such that when the console driver diffs Old and New for Console A,
    // all characters are different, and everything gets rendered. That is what
    // this flag is here for: we read it, and, if it is set, we clear our
    // client's Old buffer and reset it when done with the display update. We
    // could just clear the Old buffer every time, but we would lose the
    // performance improvement.
    //
    // N.B.: The general design of this routine assumes interlocked behavior
    //       with respect to the data in the shared view; the client must block
    //       until we are done (which is currently the case). Neither client nor
    //       server can access the shared view simultaneously (for read or
    //       write); all access must be serialized. Failing this, we'd have to
    //       capture the buffer into local storage.
    //

    UNREFERENCED_PARAMETER(Message);

    BOOL Ret;
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG_PTR ViewBase;
    ULONG_PTR RunLength;
    ULONG_PTR BytesToRead;

    PVOID OldRunBase;
    PVOID NewRunBase;
    
    PCD_IO_CHARACTER OldRun;
    PCD_IO_CHARACTER NewRun;

    CD_IO_ROW_INFORMATION RowInformation;

    DWORD cbWritten = 0;

    if (g_CisContext->ActiveClient == ClientContext)
    {
        ViewBase = (ULONG_PTR)ClientContext->SharedSection.ViewBase;
        RunLength = sizeof(CD_IO_CHARACTER) * g_CisContext->Display.Width;
        BytesToRead = 2 * RunLength * g_CisContext->Display.Height;

        if (BytesToRead > ClientContext->SharedSection.Size)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            for (SHORT i = 0 ; i < (SHORT)g_CisContext->Display.Height ; i++)
            {
                OldRunBase = (PVOID)(ViewBase + (i * 2 * RunLength));
                NewRunBase = (PVOID)(ViewBase + (i * 2 * RunLength) + RunLength);

                OldRun = (PCD_IO_CHARACTER)OldRunBase;
                NewRun = (PCD_IO_CHARACTER)NewRunBase;

                if (ClientContext->IsNextDisplayUpdateFocusRedraw)
                {
                    memset(OldRun, 0, RunLength);
                }

                RowInformation.Old = OldRun;
                RowInformation.New = NewRun;
                RowInformation.Index = i;
                
                Ret = DeviceIoControl(g_CisContext->Display.Handle,
                                      IOCTL_CONDRV_UPDATE_DISPLAY,
                                      &RowInformation,
                                      sizeof(RowInformation),
                                      NULL,
                                      0,
                                      &cbWritten,
                                      NULL);
                if (Ret == FALSE)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }

            ClientContext->IsNextDisplayUpdateFocusRedraw = FALSE;
        }
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    
    return Status;
}

NTSTATUS
DspInitializeDisplay(
    VOID
    )
{
    NTSTATUS Status;

    UNICODE_STRING Name;
    RtlInitUnicodeString(&Name, CONSOLE_DISPLAY_OBJECT);

    HANDLE DisplayHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    IO_STATUS_BLOCK IoStatusBlock = { 0 };

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&DisplayHandle,
                        FILE_WRITE_DATA | FILE_READ_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_VALID_FLAGS,
                        0);
    
    if (NT_SUCCESS(Status))
    {
        g_CisContext->Display.Handle = DisplayHandle;

        CD_IO_DISPLAY_SIZE DisplaySize;
        Status = ApiGetDisplaySize(&DisplaySize);
        
        if (NT_SUCCESS(Status))
        {
            g_CisContext->Display.Height = DisplaySize.Height;
            g_CisContext->Display.Width  = DisplaySize.Width;
        }
        else
        {
            NtClose(DisplayHandle);
        }
    }
    
    return Status;
}

NTSTATUS
DspShutDownDisplay(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (g_CisContext->Display.Handle)
    {
        Status = NtClose(g_CisContext->Display.Handle);

        if (NT_SUCCESS(Status))
        {
            g_CisContext->Display.Handle = NULL;
            g_CisContext->Display.Height = 0;
            g_CisContext->Display.Width = 0;
        }
    }
    
    return Status;
}
