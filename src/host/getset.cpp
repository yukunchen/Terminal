/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "getset.h"

#include "_output.h"
#include "_stream.h"
#include "output.h"
#include "cursor.h"
#include "dbcs.h"
#include "handle.h"
#include "icon.hpp"
#include "misc.h"
#include "srvinit.h"
#include "tracing.hpp"
#include "window.hpp"

#pragma hdrstop

// The following mask is used to test for valid text attributes.
#define VALID_TEXT_ATTRIBUTES (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY | \
COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE | COMMON_LVB_GRID_HORIZONTAL | COMMON_LVB_GRID_LVERTICAL | COMMON_LVB_GRID_RVERTICAL | COMMON_LVB_REVERSE_VIDEO | COMMON_LVB_UNDERSCORE )

#define INPUT_MODES (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT)
#define OUTPUT_MODES (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_LVB_GRID_WORLDWIDE)
#define PRIVATE_MODES (ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_EXTENDED_FLAGS)

NTSTATUS SrvGetConsoleMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_MODE_MSG const a = (PCONSOLE_MODE_MSG)& m->u.consoleMsgL1.GetConsoleMode;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleMode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    // Check handle type and access.
    if (HandleData->IsInputHandle())
    {
        INPUT_INFORMATION* pInputInfo;
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetInputBuffer(GENERIC_READ, &pInputInfo));
        if (NT_SUCCESS(Status))
        {
            a->Mode = pInputInfo->InputMode;

            if ((g_ciConsoleInformation.Flags & CONSOLE_USE_PRIVATE_FLAGS) != 0)
            {
                a->Mode |= ENABLE_EXTENDED_FLAGS;

                if (g_ciConsoleInformation.GetInsertMode())
                {
                    a->Mode |= ENABLE_INSERT_MODE;
                }

                if (g_ciConsoleInformation.Flags & CONSOLE_QUICK_EDIT_MODE)
                {
                    a->Mode |= ENABLE_QUICK_EDIT_MODE;
                }

                if (g_ciConsoleInformation.Flags & CONSOLE_AUTO_POSITION)
                {
                    a->Mode |= ENABLE_AUTO_POSITION;
                }
            }
        }
    }
    else
    {
        SCREEN_INFORMATION* pScreenInfo;
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
        if (NT_SUCCESS(Status))
        {
            a->Mode = pScreenInfo->GetActiveBuffer()->OutputMode;
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetConsoleNumberOfInputEvents(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETNUMBEROFINPUTEVENTS_MSG const a = &m->u.consoleMsgL1.GetNumberOfConsoleInputEvents;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetNumberOfConsoleInputEvents);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    INPUT_INFORMATION* pInputInfo;

    Status = NTSTATUS_FROM_HRESULT(HandleData->GetInputBuffer(GENERIC_READ, &pInputInfo));
    if (NT_SUCCESS(Status))
    {
        GetNumberOfReadyEvents(pInputInfo, &a->ReadyEvents);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetConsoleScreenBufferInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SCREENBUFFERINFO_MSG const a = &m->u.consoleMsgL2.GetConsoleScreenBufferInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleScreenBufferInfoEx);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvGetConsoleScreenBufferInfo(pScreenInfo->GetActiveBuffer(), a);
    }

    Tracing::s_TraceApi(Status, a, false);

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SCREENBUFFERINFO_MSG* pMsg)
{
    NTSTATUS Status = pScreenInfo->GetScreenBufferInformation(&pMsg->Size,
                                                              &pMsg->CursorPosition,
                                                              &pMsg->ScrollPosition,
                                                              &pMsg->Attributes,
                                                              &pMsg->CurrentWindowSize,
                                                              &pMsg->MaximumWindowSize,
                                                              &pMsg->PopupAttributes,
                                                              pMsg->ColorTable);

    pMsg->FullscreenSupported = FALSE; // traditional full screen with the driver support is no longer supported.

    return Status;
}

NTSTATUS SrvGetConsoleCursorInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETCURSORINFO_MSG const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleCursorInfo);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvGetConsoleCursorInfo(pScreenInfo->GetActiveBuffer(), a);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_GETCURSORINFO_MSG* pMsg)
{
    pMsg->CursorSize = pScreenInfo->TextInfo->GetCursor()->GetSize();
    pMsg->Visible = (BOOLEAN)pScreenInfo->TextInfo->GetCursor()->IsVisible();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleSelectionInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETSELECTIONINFO_MSG const a = &m->u.consoleMsgL3.GetConsoleSelectionInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleSelectionInfo);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (NT_SUCCESS(Status))
    {
        const Selection* const pSelection = &Selection::Instance();
        if (pSelection->IsInSelectingState())
        {
            pSelection->GetPublicSelectionFlags(&a->SelectionInfo.dwFlags);

            // we should never have failed to set the CONSOLE_SELECTION_IN_PROGRESS flag....
            ASSERT((a->SelectionInfo.dwFlags & CONSOLE_SELECTION_IN_PROGRESS) != 0);
            a->SelectionInfo.dwFlags |= CONSOLE_SELECTION_IN_PROGRESS; // ... but if we did, set it anyway in release mode.

            pSelection->GetSelectionAnchor(&a->SelectionInfo.dwSelectionAnchor);
            pSelection->GetSelectionRectangle(&a->SelectionInfo.srSelection);
        }
        else
        {
            ZeroMemory(&a->SelectionInfo, sizeof(a->SelectionInfo));
        }

        UnlockConsole();
    }

    return Status;
}

NTSTATUS SrvGetConsoleMouseInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETMOUSEINFO_MSG const a = &m->u.consoleMsgL3.GetConsoleMouseInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetNumberOfConsoleMouseButtons);
    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->NumButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetConsoleFontSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETFONTSIZE_MSG const a = &m->u.consoleMsgL3.GetConsoleFontSize;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleFontSize);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        if (a->FontIndex == 0)
        {
            // As of the November 2015 renderer system, we only have a single font at index 0.
            a->FontSize = pScreenInfo->GetActiveBuffer()->TextInfo->GetCurrentFont()->GetUnscaledSize();
        }
        else
        {
            // Invalid font is 0,0 with STATUS_INVALID_PARAMETER
            a->FontSize.X = 0;
            a->FontSize.Y = 0;
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetConsoleCurrentFont(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_CURRENTFONT_MSG a = &m->u.consoleMsgL3.GetCurrentConsoleFont;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetCurrentConsoleFontEx);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        const SCREEN_INFORMATION* const psi = pScreenInfo->GetActiveBuffer();

        COORD WindowSize;
        if (a->MaximumWindow)
        {
            WindowSize = psi->GetMaxWindowSizeInCharacters();
        }
        else
        {
            WindowSize = psi->TextInfo->GetCurrentFont()->GetUnscaledSize();
        }
        a->FontSize = WindowSize;

        a->FontIndex = 0;

        const FontInfo* const pfi = psi->TextInfo->GetCurrentFont();
        a->FontFamily = pfi->GetFamily();
        a->FontWeight = pfi->GetWeight();

        StringCchCopyW(a->FaceName, ARRAYSIZE(a->FaceName), pfi->GetFaceName());
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvSetConsoleCurrentFont(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_CURRENTFONT_MSG const a = &m->u.consoleMsgL3.SetCurrentConsoleFont;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetCurrentConsoleFontEx);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        SCREEN_INFORMATION* const psi = pScreenInfo->GetActiveBuffer();

        a->FaceName[ARRAYSIZE(a->FaceName) - 1] = UNICODE_NULL;

        FontInfo fi(a->FaceName, static_cast<BYTE>(a->FontFamily), a->FontWeight, a->FontSize, g_ciConsoleInformation.OutputCP);

        psi->UpdateFont(&fi);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvSetConsoleMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_MODE_MSG const a = &m->u.consoleMsgL1.SetConsoleMode;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleMode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    
    if (HandleData->IsInputHandle())
    {
        INPUT_INFORMATION* pInputInfo;
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetInputBuffer(GENERIC_WRITE, &pInputInfo));
        if (NT_SUCCESS(Status))
        {
            BOOL PreviousInsertMode;

            if (a->Mode & ~(INPUT_MODES | PRIVATE_MODES))
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else if ((a->Mode & (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) == ENABLE_ECHO_INPUT)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else if (a->Mode & PRIVATE_MODES)
            {
                g_ciConsoleInformation.Flags |= CONSOLE_USE_PRIVATE_FLAGS;

                if (a->Mode & ENABLE_QUICK_EDIT_MODE)
                {
                    g_ciConsoleInformation.Flags |= CONSOLE_QUICK_EDIT_MODE;
                }
                else
                {
                    g_ciConsoleInformation.Flags &= ~CONSOLE_QUICK_EDIT_MODE;
                }

                if (a->Mode & ENABLE_AUTO_POSITION)
                {
                    g_ciConsoleInformation.Flags |= CONSOLE_AUTO_POSITION;
                }
                else
                {
                    g_ciConsoleInformation.Flags &= ~CONSOLE_AUTO_POSITION;
                }

                PreviousInsertMode = g_ciConsoleInformation.GetInsertMode();
                if (a->Mode & ENABLE_INSERT_MODE)
                {
                    g_ciConsoleInformation.SetInsertMode(TRUE);
                }
                else
                {
                    g_ciConsoleInformation.SetInsertMode(FALSE);
                }

                if (g_ciConsoleInformation.GetInsertMode() != PreviousInsertMode)
                {
                    g_ciConsoleInformation.CurrentScreenBuffer->SetCursorDBMode(FALSE);
                    if (g_ciConsoleInformation.lpCookedReadData != nullptr)
                    {
                        g_ciConsoleInformation.lpCookedReadData->InsertMode = !!g_ciConsoleInformation.GetInsertMode();
                    }
                }
            }
            else
            {
                g_ciConsoleInformation.Flags &= ~CONSOLE_USE_PRIVATE_FLAGS;
            }

            pInputInfo->InputMode = a->Mode & ~PRIVATE_MODES;
        }
    }
    else
    {
        SCREEN_INFORMATION* pScreenInfo;
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
        if (NT_SUCCESS(Status))
        {
            if ((a->Mode & ~OUTPUT_MODES) == 0)
            {
                pScreenInfo = pScreenInfo->GetActiveBuffer();
                const DWORD dwOldMode = pScreenInfo->OutputMode;
                const DWORD dwNewMode = a->Mode;

                pScreenInfo->OutputMode = dwNewMode;

                // if we're moving from VT on->off
                if (!IsFlagSet(dwNewMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) && IsFlagSet(dwOldMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
                {
                    // jiggle the handle
                    pScreenInfo->GetStateMachine()->ResetState();
                }
                g_ciConsoleInformation.SetVirtTermLevel(IsFlagSet(dwNewMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 1 : 0);
                g_ciConsoleInformation.SetAutomaticReturnOnNewline(IsFlagSet(pScreenInfo->OutputMode, DISABLE_NEWLINE_AUTO_RETURN) ? false : true);
                g_ciConsoleInformation.SetGridRenderingAllowedWorldwide(IsFlagSet(pScreenInfo->OutputMode, ENABLE_LVB_GRID_WORLDWIDE));
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS GetProcessParentId(_Inout_ PULONG ProcessId)
{
    // TODO: Get Parent current not really available without winternl + NtQueryInformationProcess. http://osgvsowi/8394495

    /*OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, nullptr, 0, nullptr, nullptr);

    CLIENT_ID ClientId;
    ClientId.UniqueProcess = UlongToHandle(*ProcessId);
    ClientId.UniqueThread = 0;

    HANDLE ProcessHandle;
    NTSTATUS Status = NtOpenProcess(&ProcessHandle, PROCESS_QUERY_LIMITED_INFORMATION, &oa, &ClientId);

    PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
        NtClose(ProcessHandle);
    }

    if (!NT_SUCCESS(Status))
    {
        *ProcessId = 0;
        return Status;
    }

    *ProcessId = (ULONG) BasicInfo.InheritedFromUniqueProcessId;
    return STATUS_SUCCESS;*/

    *ProcessId = 0;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS SrvGenerateConsoleCtrlEvent(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_CTRLEVENT_MSG const a = &m->u.consoleMsgL2.GenerateConsoleCtrlEvent;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GenerateConsoleCtrlEvent);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // Make sure the process group id is valid.
    if (a->ProcessGroupId)
    {
        PCONSOLE_PROCESS_HANDLE ProcessHandle;

        ProcessHandle = ConsoleProcessList::s_FindProcessByGroupId(a->ProcessGroupId);
        if (ProcessHandle == nullptr)
        {
            ULONG ProcessId = a->ProcessGroupId;

            // We didn't find a process with that group ID.
            // Let's see if the process with that ID exists and has a parent that is a member of this console.
            Status = GetProcessParentId(&ProcessId);
            if (NT_SUCCESS(Status))
            {
                ProcessHandle = ConsoleProcessList::s_FindProcessInList(UlongToHandle(ProcessId));
                if (ProcessHandle == nullptr)
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    CLIENT_ID ClientId;

                    ClientId.UniqueProcess = UlongToHandle(a->ProcessGroupId);
                    ClientId.UniqueThread = 0;
                    if (ConsoleProcessList::s_AllocProcessData(&ClientId, a->ProcessGroupId, ProcessHandle) == nullptr)
                    {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                    else
                    {
                        Status = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        g_ciConsoleInformation.LimitingProcessId = a->ProcessGroupId;
        HandleCtrlEvent(a->CtrlEvent);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvSetConsoleActiveScreenBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleActiveScreenBuffer);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = DoSrvSetConsoleActiveScreenBuffer(GetMessageObject(m));

    UnlockConsole();
    return Status;
}

// Most other Srv calls do some other processing of the msg once they get the screen buffer out of the message - 
//  SetConsoleActiveScreenBuffer just sets it. So there's not a lot more to do here.
NTSTATUS DoSrvSetConsoleActiveScreenBuffer(_In_ ConsoleHandleData* hScreenBufferHandle)
{
    ConsoleHandleData* HandleData = hScreenBufferHandle;
    SCREEN_INFORMATION* pScreenInfo;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = SetActiveScreenBuffer(pScreenInfo->GetActiveBuffer());
    }
    return Status;
}

NTSTATUS SrvFlushConsoleInputBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FlushConsoleInputBuffer);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    INPUT_INFORMATION* pInputInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetInputBuffer(GENERIC_WRITE, &pInputInfo));
    if (NT_SUCCESS(Status))
    {
        FlushInputBuffer(pInputInfo);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetLargestConsoleWindowSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETLARGESTWINDOWSIZE_MSG const a = &m->u.consoleMsgL2.GetLargestConsoleWindowSize;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetLargestConsoleWindowSize);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        pScreenInfo = pScreenInfo->GetActiveBuffer();
        a->Size = pScreenInfo->GetLargestWindowSizeInCharacters();
    }

    Tracing::s_TraceApi(Status, a);

    UnlockConsole();
    return Status;
}

NTSTATUS SrvSetConsoleScreenBufferSize(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETSCREENBUFFERSIZE_MSG const a = &m->u.consoleMsgL2.SetConsoleScreenBufferSize;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleScreenBufferSize);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        pScreenInfo = pScreenInfo->GetActiveBuffer();

        COORD const coordMin = pScreenInfo->GetMinWindowSizeInCharacters();

        if (a->Size.X < pScreenInfo->GetScreenWindowSizeX() ||
            a->Size.Y < pScreenInfo->GetScreenWindowSizeY() ||
            a->Size.Y < coordMin.Y ||
            a->Size.X < coordMin.X)
        {
            // Make sure requested screen buffer size isn't smaller than the window.
            Status = STATUS_INVALID_PARAMETER;
        }
        else if (a->Size.X == SHORT_MAX || a->Size.Y == SHORT_MAX)
        {
            // Ensure the requested size isn't larger than we can handle in our data type.
            Status = STATUS_INVALID_PARAMETER;
        }
        else if (a->Size.X == pScreenInfo->ScreenBufferSize.X && a->Size.Y == pScreenInfo->ScreenBufferSize.Y)
        {
            // If we're not actually changing the size, then quickly return success
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = pScreenInfo->ResizeScreenBuffer(a->Size, TRUE);
        }
    }

    Tracing::s_TraceApi(Status, a);

    UnlockConsole();
    return Status;
}

NTSTATUS SrvSetScreenBufferInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SCREENBUFFERINFO_MSG const a = &m->u.consoleMsgL2.SetConsoleScreenBufferInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleScreenBufferInfoEx);

    NTSTATUS Status = STATUS_SUCCESS;
    if (a->Size.X == 0 || a->Size.Y == 0 || a->Size.X == 0x7FFF || a->Size.Y == 0x7FFF)
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(Status))
    {
        CONSOLE_INFORMATION *Console;
        Status = RevalidateConsole(&Console);
        if (NT_SUCCESS(Status))
        {
            ConsoleHandleData* HandleData = GetMessageObject(m);
            SCREEN_INFORMATION* pScreenInfo;
            Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
            if (NT_SUCCESS(Status))
            {
                Status = DoSrvSetScreenBufferInfo(pScreenInfo->GetActiveBuffer(), a);
            }
        }
    }

    Tracing::s_TraceApi(Status, a, true);

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvSetScreenBufferInfo(_In_ PSCREEN_INFORMATION const ScreenInfo, _In_ PCONSOLE_SCREENBUFFERINFO_MSG const a)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (a->Size.X != ScreenInfo->ScreenBufferSize.X || (a->Size.Y != ScreenInfo->ScreenBufferSize.Y))
    {
        CommandLine* const pCommandLine = &CommandLine::Instance();

        pCommandLine->Hide(FALSE);

        ScreenInfo->ResizeScreenBuffer(a->Size, TRUE);

        pCommandLine->Show();
    }

    g_ciConsoleInformation.SetColorTable(a->ColorTable, ARRAYSIZE(a->ColorTable));
    SetScreenColors(ScreenInfo, a->Attributes, a->PopupAttributes, TRUE);

    COORD NewSize;
    NewSize.X = min(a->CurrentWindowSize.X, a->MaximumWindowSize.X);
    NewSize.Y = min(a->CurrentWindowSize.Y, a->MaximumWindowSize.Y);

    // If wrap text is on, then the window width must be the same size as the buffer width
    if (g_ciConsoleInformation.GetWrapText())
    {
        NewSize.X = ScreenInfo->ScreenBufferSize.X;
    }

    if (NewSize.X != ScreenInfo->GetScreenWindowSizeX() || NewSize.Y != ScreenInfo->GetScreenWindowSizeY())
    {
        g_ciConsoleInformation.pWindow->UpdateWindowSize(NewSize);
    }

    return Status;
}

NTSTATUS SrvSetConsoleCursorPosition(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETCURSORPOSITION_MSG const a = &m->u.consoleMsgL2.SetConsoleCursorPosition;

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvSetConsoleCursorPosition(pScreenInfo->GetActiveBuffer(), a);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETCURSORPOSITION_MSG* pMsg)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleCursorPosition);

    NTSTATUS Status = STATUS_SUCCESS;

    if (pMsg->CursorPosition.X >= pScreenInfo->ScreenBufferSize.X ||
        pMsg->CursorPosition.Y >= pScreenInfo->ScreenBufferSize.Y ||
        pMsg->CursorPosition.X < 0 ||
        pMsg->CursorPosition.Y < 0)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        Status = pScreenInfo->SetCursorPosition(pMsg->CursorPosition, TRUE);
    }

    if (NT_SUCCESS(Status))
    {
        ConsoleImeResizeCompStrView();

        COORD WindowOrigin;
        WindowOrigin.X = 0;
        WindowOrigin.Y = 0;
        if (pScreenInfo->BufferViewport.Left > pMsg->CursorPosition.X)
        {
            WindowOrigin.X = pMsg->CursorPosition.X - pScreenInfo->BufferViewport.Left;
        }
        else if (pScreenInfo->BufferViewport.Right < pMsg->CursorPosition.X)
        {
            WindowOrigin.X = pMsg->CursorPosition.X - pScreenInfo->BufferViewport.Right;
        }

        if (pScreenInfo->BufferViewport.Top > pMsg->CursorPosition.Y)
        {
            WindowOrigin.Y = pMsg->CursorPosition.Y - pScreenInfo->BufferViewport.Top;
        }
        else if (pScreenInfo->BufferViewport.Bottom < pMsg->CursorPosition.Y)
        {
            WindowOrigin.Y = pMsg->CursorPosition.Y - pScreenInfo->BufferViewport.Bottom;
        }

        Status = pScreenInfo->SetViewportOrigin(FALSE, WindowOrigin);
    }

    return Status;
}

NTSTATUS SrvSetConsoleCursorInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETCURSORINFO_MSG const a = &m->u.consoleMsgL2.SetConsoleCursorInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleCursorInfo);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvSetConsoleCursorInfo(pScreenInfo->GetActiveBuffer(), a);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETCURSORINFO_MSG* pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pMsg->CursorSize > 100 || pMsg->CursorSize == 0)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        Status = pScreenInfo->SetCursorInformation(pMsg->CursorSize, pMsg->Visible);
    }

    return Status;
}

NTSTATUS SrvSetConsoleWindowInfo(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETWINDOWINFO_MSG const a = &m->u.consoleMsgL2.SetConsoleWindowInfo;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleWindowInfo);

    // Backup the incoming message for tracing purposes
    CONSOLE_SETWINDOWINFO_MSG const aOriginal = *a;

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (NT_SUCCESS(Status))
    {
        ConsoleHandleData* HandleData = GetMessageObject(m);
        SCREEN_INFORMATION* pScreenInfo;
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
        if (NT_SUCCESS(Status))
        {
            Status = DoSrvSetConsoleWindowInfo(pScreenInfo->GetActiveBuffer(), a);
        }

        UnlockConsole();
    }

    Tracing::s_TraceApi(Status, &aOriginal);

    return Status;
}

NTSTATUS DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETWINDOWINFO_MSG* pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pMsg->Absolute)
    {
        pMsg->Window.Left += pScreenInfo->BufferViewport.Left;
        pMsg->Window.Right += pScreenInfo->BufferViewport.Right;
        pMsg->Window.Top += pScreenInfo->BufferViewport.Top;
        pMsg->Window.Bottom += pScreenInfo->BufferViewport.Bottom;
    }

    if (pMsg->Window.Right < pMsg->Window.Left || pMsg->Window.Bottom < pMsg->Window.Top)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        COORD NewWindowSize;
        NewWindowSize.X = (SHORT)(CalcWindowSizeX(&pMsg->Window));
        NewWindowSize.Y = (SHORT)(CalcWindowSizeY(&pMsg->Window));

        COORD const coordMax = pScreenInfo->GetMaxWindowSizeInCharacters();

        if (NewWindowSize.X > coordMax.X || NewWindowSize.Y > coordMax.Y)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            // Even if it's the same size, we need to post an update in case the scroll bars need to go away.
            Status = pScreenInfo->SetViewportRect(&pMsg->Window);
            if (pScreenInfo->IsActiveScreenBuffer())
            {
                pScreenInfo->PostUpdateWindowSize();
                WriteToScreen(pScreenInfo, &pScreenInfo->BufferViewport);
            }
        }
    }
    return Status;
}

NTSTATUS SrvScrollConsoleScreenBuffer(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SCROLLSCREENBUFFER_MSG const a = &m->u.consoleMsgL2.ScrollConsoleScreenBuffer;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ScrollConsoleScreenBuffer, a->Unicode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvScrollConsoleScreenBuffer(pScreenInfo->GetActiveBuffer(), a);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvScrollConsoleScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo, _Inout_ CONSOLE_SCROLLSCREENBUFFER_MSG* const pMsg)
{
    PSMALL_RECT ClipRect;
    if (pMsg->Clip)
    {
        ClipRect = &pMsg->ClipRectangle;
    }
    else
    {
        ClipRect = nullptr;
    }

    if (!pMsg->Unicode)
    {
        pMsg->Fill.Char.UnicodeChar = CharToWchar(&pMsg->Fill.Char.AsciiChar, 1);
    }

    return ScrollRegion(pScreenInfo, &pMsg->ScrollRectangle, ClipRect, pMsg->DestinationOrigin, pMsg->Fill);
}

// Routine Description:
// - This routine is called when the user changes the screen/popup colors.
// - It goes through the popup structures and changes the saved contents to reflect the new screen/popup colors.
VOID UpdatePopups(IN WORD NewAttributes, IN WORD NewPopupAttributes, IN WORD OldAttributes, IN WORD OldPopupAttributes)
{
    WORD const InvertedOldPopupAttributes = (WORD)(((OldPopupAttributes << 4) & 0xf0) | ((OldPopupAttributes >> 4) & 0x0f));
    WORD const InvertedNewPopupAttributes = (WORD)(((NewPopupAttributes << 4) & 0xf0) | ((NewPopupAttributes >> 4) & 0x0f));
    PLIST_ENTRY const HistoryListHead = &g_ciConsoleInformation.CommandHistoryList;
    PLIST_ENTRY HistoryListNext = HistoryListHead->Blink;
    while (HistoryListNext != HistoryListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(HistoryListNext, COMMAND_HISTORY, ListLink);
        HistoryListNext = HistoryListNext->Blink;
        if (History->Flags & CLE_ALLOCATED && !CLE_NO_POPUPS(History))
        {
            PLIST_ENTRY const PopupListHead = &History->PopupList;
            PLIST_ENTRY PopupListNext = PopupListHead->Blink;
            while (PopupListNext != PopupListHead)
            {
                PCLE_POPUP const Popup = CONTAINING_RECORD(PopupListNext, CLE_POPUP, ListLink);
                PopupListNext = PopupListNext->Blink;
                PCHAR_INFO OldContents = Popup->OldContents;
                for (SHORT i = Popup->Region.Left; i <= Popup->Region.Right; i++)
                {
                    for (SHORT j = Popup->Region.Top; j <= Popup->Region.Bottom; j++)
                    {
                        if (OldContents->Attributes == OldAttributes)
                        {
                            OldContents->Attributes = NewAttributes;
                        }
                        else if (OldContents->Attributes == OldPopupAttributes)
                        {
                            OldContents->Attributes = NewPopupAttributes;
                        }
                        else if (OldContents->Attributes == InvertedOldPopupAttributes)
                        {
                            OldContents->Attributes = InvertedNewPopupAttributes;
                        }

                        OldContents++;
                    }
                }
            }
        }
    }
}

NTSTATUS SetScreenColors(_In_ PSCREEN_INFORMATION ScreenInfo, _In_ WORD Attributes, _In_ WORD PopupAttributes, _In_ BOOL UpdateWholeScreen)
{
    WORD const DefaultAttributes = ScreenInfo->GetAttributes()->GetLegacyAttributes();
    WORD const DefaultPopupAttributes = ScreenInfo->GetPopupAttributes()->GetLegacyAttributes();
    TextAttribute NewPrimaryAttributes = TextAttribute(Attributes);
    TextAttribute NewPopupAttributes = TextAttribute(PopupAttributes);
    ScreenInfo->SetAttributes(&NewPrimaryAttributes);
    ScreenInfo->SetPopupAttributes(&NewPopupAttributes);
    g_ciConsoleInformation.ConsoleIme.RefreshAreaAttributes();

    if (UpdateWholeScreen)
    {
        // TODO: MSFT 9354902: Fix this up to be clearer with less magic bit shifting and less magic numbers. http://osgvsowi/9354902
        WORD const InvertedOldPopupAttributes = (WORD)(((DefaultPopupAttributes << 4) & 0xf0) | ((DefaultPopupAttributes >> 4) & 0x0f));
        WORD const InvertedNewPopupAttributes = (WORD)(((PopupAttributes << 4) & 0xf0) | ((PopupAttributes >> 4) & 0x0f));

        // change all chars with default color
        for (SHORT i = 0; i < ScreenInfo->ScreenBufferSize.Y; i++)
        {
            ROW* const Row = &ScreenInfo->TextInfo->Rows[i];
            Row->AttrRow.ReplaceLegacyAttrs(DefaultAttributes, Attributes);
            Row->AttrRow.ReplaceLegacyAttrs(DefaultPopupAttributes, PopupAttributes);
            Row->AttrRow.ReplaceLegacyAttrs(InvertedOldPopupAttributes, InvertedNewPopupAttributes);
        }

        if (g_ciConsoleInformation.PopupCount)
        {
            UpdatePopups(Attributes, PopupAttributes, DefaultAttributes, DefaultPopupAttributes);
        }

        // force repaint of entire line
        WriteToScreen(ScreenInfo, &ScreenInfo->BufferViewport);
    }

    return STATUS_SUCCESS;
}

NTSTATUS SrvSetConsoleTextAttribute(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETTEXTATTRIBUTE_MSG CONST a = &m->u.consoleMsgL2.SetConsoleTextAttribute;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleTextAttribute);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        Status = DoSrvSetConsoleTextAttribute(pScreenInfo->GetActiveBuffer(), a);
    }
    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_SETTEXTATTRIBUTE_MSG* pMsg)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (pMsg->Attributes & ~VALID_TEXT_ATTRIBUTES)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        Status = SetScreenColors(pScreenInfo, pMsg->Attributes, pScreenInfo->GetPopupAttributes()->GetLegacyAttributes(), FALSE);
    }

    return Status;
}

NTSTATUS DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ int const iXtermTableEntry, _In_ bool fIsForeground)
{
    TextAttribute NewAttributes;
    NewAttributes.SetFrom(pScreenInfo->GetAttributes());

    COLORREF rgbColor;
    if (iXtermTableEntry < COLOR_TABLE_SIZE)
    {
        //Convert the xterm index to the win index
        bool fRed = (iXtermTableEntry & 0x01) > 0;
        bool fGreen = (iXtermTableEntry & 0x02) > 0;
        bool fBlue = (iXtermTableEntry & 0x04) > 0;
        bool fBright = (iXtermTableEntry & 0x08) > 0;
        WORD iWinEntry = (fRed ? 0x4 : 0x0) | (fGreen ? 0x2 : 0x0) | (fBlue ? 0x1 : 0x0) | (fBright ? 0x8 : 0x0);

        rgbColor = g_ciConsoleInformation.GetColorTableEntry(iWinEntry);
    }
    else
    {
        rgbColor = g_ciConsoleInformation.GetColorTableEntry(iXtermTableEntry);
    }

    NewAttributes.SetColor(rgbColor, fIsForeground);

    pScreenInfo->SetAttributes(&NewAttributes);

    return STATUS_SUCCESS;
}

NTSTATUS DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ COLORREF const rgbColor, _In_ bool fIsForeground)
{
    TextAttribute NewAttributes;
    NewAttributes.SetFrom(pScreenInfo->GetAttributes());
    NewAttributes.SetColor(rgbColor, fIsForeground);
    pScreenInfo->SetAttributes(&NewAttributes);

    return STATUS_SUCCESS;
}

NTSTATUS SrvSetConsoleCP(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETCP_MSG const a = &m->u.consoleMsgL2.SetConsoleCP;

    Telemetry::Instance().LogApiCall(a->Output ? Telemetry::ApiCall::SetConsoleOutputCP : Telemetry::ApiCall::SetConsoleCP);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!IsValidCodePage(a->CodePage))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto SrvSetConsoleCPFailure;
    }

    if ((a->Output && g_ciConsoleInformation.OutputCP != a->CodePage) || (!a->Output && g_ciConsoleInformation.CP != a->CodePage))
    {
        UINT CodePage;

        if (a->Output)
        {
            // Backup old code page
            CodePage = g_ciConsoleInformation.OutputCP;

            // Set new code page
            g_ciConsoleInformation.OutputCP = a->CodePage;

            SetConsoleCPInfo(a->Output);
        }
        else
        {
            // Backup old code page
            CodePage = g_ciConsoleInformation.CP;

            // Set new code page
            g_ciConsoleInformation.CP = a->CodePage;

            SetConsoleCPInfo(a->Output);
        }
    }

SrvSetConsoleCPFailure:
    UnlockConsole();
    return Status;
}

NTSTATUS SrvGetConsoleCP(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETCP_MSG const a = &m->u.consoleMsgL1.GetConsoleCP;

    Telemetry::Instance().LogApiCall(a->Output ? Telemetry::ApiCall::GetConsoleOutputCP : Telemetry::ApiCall::GetConsoleCP);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (a->Output)
    {
        a->CodePage = g_ciConsoleInformation.OutputCP;
    }
    else
    {
        a->CodePage = g_ciConsoleInformation.CP;
    }

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleWindow(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETCONSOLEWINDOW_MSG const a = &m->u.consoleMsgL3.GetConsoleWindow;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleWindow);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->hwnd = g_ciConsoleInformation.hWnd;

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvGetConsoleProcessList(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETCONSOLEPROCESSLIST_MSG const a = &m->u.consoleMsgL3.GetConsoleProcessList;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleProcessList);

    DWORD dwProcessCount = 0;

    PVOID Buffer;
    ULONG BufferSize;
    NTSTATUS Status = GetOutputBuffer(m, &Buffer, &BufferSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->dwProcessCount = BufferSize / sizeof(ULONG);

    CONSOLE_INFORMATION *Console = nullptr;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    LPDWORD lpdwProcessList = (PDWORD)Buffer;

    /*
     * Run through the console's process list to determine if the user-supplied
     * buffer is big enough to contain them all. This is requires that we make
     * two passes over the data, but it allows this function to have the same
     * semantics as GetProcessHeaps().
     */

    PLIST_ENTRY const ListHead = &g_ciConsoleInformation.ProcessHandleList;
    PLIST_ENTRY ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        ++dwProcessCount;
        ListNext = ListNext->Flink;
    }

    // At this point we can't fail, so set the status accordingly.
    Status = STATUS_SUCCESS;

    /*
     * There's not enough space in the array to hold all the pids, so we'll
     * inform the user of that by returning a number > than a->dwProcessCount
     * (but we still return STATUS_SUCCESS).
     */
    if (dwProcessCount > a->dwProcessCount)
    {
        goto Cleanup;
    }

    // Loop over the list of processes again and fill in the caller's buffer.
    ListNext = ListHead->Flink;
    while (ListNext != ListHead)
    {
        PCONSOLE_PROCESS_HANDLE const ProcessHandleRecord = CONTAINING_RECORD(ListNext, CONSOLE_PROCESS_HANDLE, ListLink);
        *lpdwProcessList++ = HandleToUlong(ProcessHandleRecord->ClientId.UniqueProcess);
        ListNext = ListNext->Flink;
    }

    SetReplyInformation(m, dwProcessCount * sizeof(ULONG));

Cleanup:
    a->dwProcessCount = dwProcessCount;

    if (Console != nullptr)
    {
        UnlockConsole();
    }

    return Status;
}

NTSTATUS SrvGetConsoleHistory(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_HISTORY_MSG const a = &m->u.consoleMsgL3.GetConsoleHistory;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleHistoryInfo);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->HistoryBufferSize = g_ciConsoleInformation.GetHistoryBufferSize();
    a->NumberOfHistoryBuffers = g_ciConsoleInformation.GetNumberOfHistoryBuffers();
    if (g_ciConsoleInformation.Flags & CONSOLE_HISTORY_NODUP)
    {
        a->dwFlags = HISTORY_NO_DUP_FLAG;
    }
    else
    {
        a->dwFlags = 0;
    }

    UnlockConsole();
    return STATUS_SUCCESS;
}

NTSTATUS SrvSetConsoleHistory(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_HISTORY_MSG const a = &m->u.consoleMsgL3.SetConsoleHistory;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleHistoryInfo);

    if (a->HistoryBufferSize > SHORT_MAX || a->NumberOfHistoryBuffers > SHORT_MAX || (a->dwFlags & CHI_VALID_FLAGS) != a->dwFlags)
    {
        RIPMSG3(RIP_WARNING, "Invalid Parameter: 0x%x, 0x%x, 0x%x", a->HistoryBufferSize, a->NumberOfHistoryBuffers, a->dwFlags);
        return STATUS_INVALID_PARAMETER;
    }

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ResizeCommandHistoryBuffers(a->HistoryBufferSize);
    g_ciConsoleInformation.SetNumberOfHistoryBuffers(a->NumberOfHistoryBuffers);

    if (a->dwFlags & HISTORY_NO_DUP_FLAG)
    {
        g_ciConsoleInformation.Flags |= CONSOLE_HISTORY_NODUP;
    }
    else
    {
        g_ciConsoleInformation.Flags &= ~CONSOLE_HISTORY_NODUP;
    }

    UnlockConsole();
    return STATUS_SUCCESS;
}

// NOTE: This was in private.c, but turns out to be a public API: http://msdn.microsoft.com/en-us/library/windows/desktop/ms683164(v=vs.85).aspx
NTSTATUS SrvGetConsoleDisplayMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_GETDISPLAYMODE_MSG const a = &m->u.consoleMsgL3.GetConsoleDisplayMode;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleDisplayMode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (NT_SUCCESS(Status))
    {
        // Initialize flags portion of structure
        a->ModeFlags = 0;

        if (g_ciConsoleInformation.pWindow != nullptr && g_ciConsoleInformation.pWindow->IsInFullscreen())
        {
            a->ModeFlags |= CONSOLE_FULLSCREEN_MODE;
        }

        UnlockConsole();
    }

    return Status;
}

// Routine Description:
// - This routine sets the console display mode for an output buffer.
// - This API is only supported on x86 machines.
// Parameters:
// - hConsoleOutput - Supplies a console output handle.
// - dwFlags - Specifies the display mode. Options are:
//      CONSOLE_FULLSCREEN_MODE - data is displayed fullscreen
//      CONSOLE_WINDOWED_MODE - data is displayed in a window
// - lpNewScreenBufferDimensions - On output, contains the new dimensions of the screen buffer.  The dimensions are in rows and columns for textmode screen buffers.
// Return value:
// - TRUE - The operation was successful.
// - FALSE/nullptr - The operation failed. Extended error status is available using GetLastError.
// NOTE:
// - This was in private.c, but turns out to be a public API:
// - See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686028(v=vs.85).aspx
NTSTATUS SrvSetConsoleDisplayMode(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_SETDISPLAYMODE_MSG const a = (PCONSOLE_SETDISPLAYMODE_MSG)& m->u.consoleMsgL3.SetConsoleDisplayMode;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleDisplayMode);

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = GetMessageObject(m);
    SCREEN_INFORMATION* pScreenInfo;
    Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    if (NT_SUCCESS(Status))
    {
        pScreenInfo = pScreenInfo->GetActiveBuffer();
        a->ScreenBufferDimensions = pScreenInfo->ScreenBufferSize;

        if (!pScreenInfo->IsActiveScreenBuffer())
        {
            Status = STATUS_INVALID_PARAMETER;
            goto SrvSetConsoleDisplayModeFailure;
        }

        // SetIsFullscreen() below ultimately calls SetwindowLong, which ultimately calls SendMessage(). If we retain
        // the console lock, we'll deadlock since ConsoleWindowProc takes the lock before processing messages. Instead,
        // we'll release early.
        UnlockConsole();
        if (a->dwFlags == CONSOLE_FULLSCREEN_MODE)
        {
            g_ciConsoleInformation.pWindow->SetIsFullscreen(true);
            Status = STATUS_SUCCESS;
        }
        else if (a->dwFlags == CONSOLE_WINDOWED_MODE)
        {
            g_ciConsoleInformation.pWindow->SetIsFullscreen(false);
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        LockConsole();
    }

SrvSetConsoleDisplayModeFailure:
    UnlockConsole();
    return Status;
}

// Routine Description:
// - A private API call for changing the cursor keys input mode between normal and application mode.
//     The cursor keys are the arrows, plus Home and End.
// Parameters:
// - fApplicationMode - set to true to enable Application Mode Input, false for Numeric Mode Input.
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode)
{
    g_ciConsoleInformation.termInput.ChangeCursorKeysMode(fApplicationMode);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for changing the keypad input mode between numeric and application mode.
//     This controls what the keys on the numpad translate to.
// Parameters:
// - fApplicationMode - set to true to enable Application Mode Input, false for Numeric Mode Input.
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode)
{
    g_ciConsoleInformation.termInput.ChangeKeypadMode(fApplicationMode);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling or disabling the cursor blinking. 
// Parameters:
// - fEnable - set to true to enable blinking, false to disable
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ bool fEnable)
{
    pScreenInfo->TextInfo->GetCursor()->SetBlinkingAllowed(fEnable);
    pScreenInfo->TextInfo->GetCursor()->SetIsOn(!fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for setting the top and bottom scrolling margins for 
//     the current page. This creates a subsection of the screen that scrolls 
//     when input reaches the end of the region, leaving the rest of the screen
//     untouched.
//  Currently only accessible through the use of ANSI sequence DECSTBM
// Parameters:
// - psrScrollMargins - A rect who's Top and Bottom members will be used to set
//     the new values of the top and bottom margins. If (0,0), then the margins
//     will be disabled. NOTE: This is a rect in the case that we'll need the
//     left and right margins in the future.
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateSetScrollingRegion(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const SMALL_RECT* const psrScrollMargins)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (psrScrollMargins->Top > psrScrollMargins->Bottom)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    if (NT_SUCCESS(Status))
    {
        SMALL_RECT srViewport = pScreenInfo->BufferViewport;
        SMALL_RECT srScrollMargins = pScreenInfo->GetScrollMargins();
        srScrollMargins.Top = psrScrollMargins->Top;
        srScrollMargins.Bottom = psrScrollMargins->Bottom;
        pScreenInfo->SetScrollMargins(&srScrollMargins);

        COORD newCursorPosition = { 0 };
        Status = pScreenInfo->SetCursorPosition(newCursorPosition, TRUE);
    }

    return Status;
}

// Routine Description:
// - A private API call for performing a "Reverse line feed", essentially, the opposite of '\n'.
//    Moves the cursor up one line, and tries to keep its position in the line
// Parameters:
// - pScreenInfo - a pointer to the screen buffer that should perform the reverse line feed
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateReverseLineFeed(_In_ SCREEN_INFORMATION* pScreenInfo)
{
    COORD newCursorPosition = pScreenInfo->TextInfo->GetCursor()->GetPosition();
    newCursorPosition.Y -= 1;
    return AdjustCursorPosition(pScreenInfo, newCursorPosition, TRUE, nullptr);
}

// Routine Description:
// - A private API call for swaping to the alternate screen buffer. In virtual terminals, there exists both a "main"
//     screen buffer and an alternate. ASBSET creates a new alternate, and switches to it. If there is an already 
//     existing alternate, it is discarded. 
// Parameters:
// - psiCurr - a pointer to the screen buffer that should use an alternate buffer
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr)
{
    return psiCurr->UseAlternateScreenBuffer();
}

// Routine Description:
// - A private API call for swaping to the main screen buffer. From the 
//     alternate buffer, returns to the main screen buffer. From the main 
//     screen buffer, does nothing. The alternate is discarded.
// Parameters:
// - psiCurr - a pointer to the screen buffer that should use the main buffer
// Return value:
// - True if handled successfully. False otherwise.
NTSTATUS DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr)
{
    return psiCurr->UseMainScreenBuffer();
}

// Routine Description:
// - A private API call for setting a VT tab stop in the cursor's current column.
// Parameters:
// <none>
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoSrvPrivateHorizontalTabSet()
{
    SCREEN_INFORMATION* const pScreenBuffer = g_ciConsoleInformation.CurrentScreenBuffer;

    const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
    return pScreenBuffer->AddTabStop(cursorPos.X);
}

// Routine Description:
// - A private helper for excecuting a number of tabs.
// Parameters:
// sNumTabs - The number of tabs to execute
// fForward - whether to tab forward or backwards
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoPrivateTabHelper(_In_ SHORT const sNumTabs, _In_ bool fForward)
{
    SCREEN_INFORMATION* const pScreenBuffer = g_ciConsoleInformation.CurrentScreenBuffer;

    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(sNumTabs >= 0);
    for (SHORT sTabsExecuted = 0; sTabsExecuted < sNumTabs && NT_SUCCESS(Status); sTabsExecuted++)
    {
        const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
        COORD cNewPos = (fForward) ? pScreenBuffer->GetForwardTab(cursorPos) : pScreenBuffer->GetReverseTab(cursorPos);
        // GetForwardTab is smart enough to move the cursor to the next line if 
        // it's at the end of the current one already. AdjustCursorPos shouldn't
        // to be doing anything funny, just moving the cursor to the location GetForwardTab returns
        Status = AdjustCursorPosition(pScreenBuffer, cNewPos, TRUE, nullptr);
    }
    return Status;
}

// Routine Description:
// - A private API call for performing a forwards tab. This will take the 
//     cursor to the tab stop following its current location. If there are no
//     more tabs in this row, it will take it to the right side of the window.
//     If it's already in the last column of the row, it will move it to the next line.
// Parameters:
// - sNumTabs - The number of tabs to perform.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoSrvPrivateForwardTab(_In_ SHORT const sNumTabs)
{
    return DoPrivateTabHelper(sNumTabs, true);
}

// Routine Description:
// - A private API call for performing a backwards tab. This will take the 
//     cursor to the tab stop previous to its current location. It will not reverse line feed.
// Parameters:
// - sNumTabs - The number of tabs to perform.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoSrvPrivateBackwardsTab(_In_ SHORT const sNumTabs)
{
    return DoPrivateTabHelper(sNumTabs, false);
}

// Routine Description:
// - A private API call for clearing the VT tabs that have been set.
// Parameters:
// - fClearAll - If false, only clears the tab in the current column (if it exists)
//      otherwise clears all set tabs. (and reverts to lecacy 8-char tabs behavior.)
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoSrvPrivateTabClear(_In_ bool const fClearAll)
{
    SCREEN_INFORMATION* const pScreenBuffer = g_ciConsoleInformation.CurrentScreenBuffer;
    if (fClearAll)
    {
        pScreenBuffer->ClearTabStops();
    }
    else
    {
        const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
        pScreenBuffer->ClearTabStop(cursorPos.X);
    }
    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling VT200 style mouse mode.
// Parameters:
// - fEnable - true to enable default tracking mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableVT200MouseMode(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.EnableDefaultTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling utf8 style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.SetUtf8ExtendedMode(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling SGR style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableSGRExtendedMouseMode(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.SetSGRExtendedMode(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling button-event mouse mode.
// Parameters:
// - fEnable - true to enable button-event mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableButtonEventMouseMode(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.EnableButtonEventTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling any-event mouse mode.
// Parameters:
// - fEnable - true to enable any-event mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableAnyEventMouseMode(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.EnableAnyEventTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling alternate scroll mode
// Parameters:
// - fEnable - true to enable alternate scroll mode, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableAlternateScroll(_In_ bool const fEnable)
{
    g_ciConsoleInformation.terminalMouseInput.EnableAlternateScroll(fEnable);

    return STATUS_SUCCESS;
}
