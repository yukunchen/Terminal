#include "stdafx.h"
#include "ApiDispatchers.h"

HANDLE h = INVALID_HANDLE_VALUE; // look up handle again based on what's in message

NTSTATUS ApiDispatchers::ServeGetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCP_MSG* const a = &m->u.consoleMsgL1.GetConsoleCP;

    DWORD Result = 0;

    if (a->Output)
    {
        Result = pResponders->GetConsoleOutputCodePageImpl(h, &a->CodePage);
    }
    else
    {
        Result = pResponders->GetConsoleInputCodePageImpl(h, &a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.GetConsoleMode;

    bool IsOutputMode = true; // this needs to check the handle type in the handle data to determine which one to operate on. I haven't done handle extraction yet.

    DWORD Result = 0;

    if (IsOutputMode)
    {
        Result = pResponders->GetConsoleOutputModeImpl(h, &a->Mode);
    }
    else
    {
        Result = pResponders->GetConsoleInputModeImpl(h, &a->Mode);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.SetConsoleMode;

    bool IsOutputMode = true; // this needs to check the handle type in the handle data to determine which one to operate on. I haven't done handle extraction yet.

    DWORD Result = 0;

    if (IsOutputMode)
    {
        Result = pResponders->SetConsoleOutputModeImpl(h, a->Mode);
    }
    else
    {
        Result = pResponders->SetConsoleInputModeImpl(h, a->Mode);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetNumberOfInputEvents(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETNUMBEROFINPUTEVENTS_MSG* const a = &m->u.consoleMsgL1.GetNumberOfConsoleInputEvents;

    return pResponders->GetNumberOfConsoleInputEventsImpl(h, &a->ReadyEvents);
}

NTSTATUS ApiDispatchers::ServeGetConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEINPUT_MSG* const a = &m->u.consoleMsgL1.GetConsoleInput;

    DWORD Result = 0;
    DWORD Written = 0;

    if (a->Unicode)
    {
        Result = pResponders->ReadConsoleInputWImpl(h, nullptr, 0, &Written);
    }
    else
    {
        Result = pResponders->ReadConsoleInputAImpl(h, nullptr, 0, &Written);
    }

    a->NumRecords = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLE_MSG* const a = &m->u.consoleMsgL1.ReadConsoleW;

    DWORD Result = 0;
    DWORD Read = 0;

    CONSOLE_READCONSOLE_CONTROL Control;
    Control.dwControlKeyState = a->ControlKeyState;
    Control.dwCtrlWakeupMask = a->CtrlWakeupMask;
    Control.nInitialChars = a->InitialNumBytes;
    Control.nLength = a->NumBytes;

    if (a->Unicode)
    {
        Result = pResponders->ReadConsoleWImpl(h, nullptr, 0, &Read, &Control);
    }
    else
    {
        Result = pResponders->ReadConsoleAImpl(h, nullptr, 0, &Read, &Control);
    }

    a->NumBytes = SUCCEEDED(Result) ? Read : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLE_MSG* const a = &m->u.consoleMsgL1.WriteConsoleW;

    DWORD Result = 0;
    DWORD Written = 0;

    if (a->Unicode)
    {
        Result = pResponders->WriteConsoleWImpl(h, nullptr, 0, &Written);
    }
    else
    {
        Result = pResponders->WriteConsoleAImpl(h, nullptr, 0, &Written);
    }

    a->NumBytes = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleLangId(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_LANGID_MSG* const a = &m->u.consoleMsgL1.GetConsoleLangId;

    // TODO: Consider replacing with just calling the Output CP API and converting to LangID here.
    return pResponders->GetConsoleLangId(h, &a->LangId);
}

NTSTATUS ApiDispatchers::ServeFillConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_FILLCONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.FillConsoleOutput;

    DWORD Result = 0;
    DWORD ElementsWritten = 0;

    switch (a->ElementType)
    {
    case CONSOLE_ATTRIBUTE:
    {
        Result = pResponders->FillConsoleOutputAttributeImpl(h, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
        break;
    }
    case CONSOLE_ASCII:
    {
        Result = pResponders->FillConsoleOutputCharacterAImpl(h, (char)a->Element, a->Length, a->WriteCoord, &ElementsWritten);
        break;
    }
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
    {
        Result = pResponders->FillConsoleOutputCharacterWImpl(h, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
    }
    }

    a->Length = SUCCEEDED(Result) ? ElementsWritten : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeGenerateConsoleCtrlEvent(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CTRLEVENT_MSG* const a = &m->u.consoleMsgL2.GenerateConsoleCtrlEvent;

    return pResponders->GenerateConsoleCtrlEventImpl(a->ProcessGroupId, a->CtrlEvent);
}

NTSTATUS ApiDispatchers::ServeSetConsoleActiveScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return pResponders->SetConsoleActiveScreenBufferImpl(h);
}

NTSTATUS ApiDispatchers::ServeFlushConsoleInputBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return pResponders->FlushConsoleInputBuffer(h);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCP_MSG* const a = &m->u.consoleMsgL2.SetConsoleCP;

    DWORD Result = 0;

    if (a->Output)
    {
        Result = pResponders->SetConsoleOutputCodePageImpl(h, a->CodePage);
    }
    else
    {
        Result = pResponders->SetConsoleInputCodePageImpl(h, a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    return pResponders->GetConsoleCursorInfoImpl(h, &a->CursorSize, &a->Visible);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorInfo;

    return pResponders->SetConsoleCursorInfoImpl(h, a->CursorSize, a->Visible);
}

NTSTATUS ApiDispatchers::ServeGetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
        
    DWORD Result = pResponders->GetConsoleScreenBufferInfoExImpl(h, &ex);

    if (SUCCEEDED(Result))
    {
        a->FullscreenSupported = ex.bFullscreenSupported;
        size_t const ColorTableSizeInBytes = RTL_NUMBER_OF_V2(ex.ColorTable) * sizeof(*ex.ColorTable);
        CopyMemory(a->ColorTable, ex.ColorTable, ColorTableSizeInBytes);
        a->CursorPosition = ex.dwCursorPosition;
        a->MaximumWindowSize = ex.dwMaximumWindowSize;
        a->Size = ex.dwSize;
        a->ScrollPosition.X = ex.srWindow.Left;
        a->ScrollPosition.Y = ex.srWindow.Top;
        a->CurrentWindowSize.X = ex.srWindow.Right - ex.srWindow.Left;
        a->CurrentWindowSize.Y = ex.srWindow.Bottom - ex.srWindow.Top;
        a->Attributes = ex.wAttributes;
        a->PopupAttributes = ex.wPopupAttributes;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    ex.bFullscreenSupported = a->FullscreenSupported;
    size_t const ColorTableSizeInBytes = RTL_NUMBER_OF_V2(ex.ColorTable) * sizeof(*ex.ColorTable);
    CopyMemory(ex.ColorTable, a->ColorTable, ColorTableSizeInBytes);
    ex.dwCursorPosition = a->CursorPosition;
    ex.dwMaximumWindowSize = a->MaximumWindowSize;
    ex.dwSize = a->Size;
    ex.srWindow = { 0 };
    ex.srWindow.Left = a->ScrollPosition.X;
    ex.srWindow.Top = a->ScrollPosition.Y;
    ex.srWindow.Right = ex.srWindow.Left + a->CurrentWindowSize.X;
    ex.srWindow.Bottom = ex.srWindow.Top + a->CurrentWindowSize.Y;
    ex.wAttributes = a->Attributes;
    ex.wPopupAttributes = a->PopupAttributes;

    return pResponders->SetConsoleScreenBufferInfoExImpl(h, &ex);
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorPosition(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetLargestConsoleWindowSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeScrollConsoleScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTextAttribute(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleWindowInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMouseInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleFontSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeAddConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliases(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExes(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleWindow(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleSelectionInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleProcessList(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeGetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}

NTSTATUS ApiDispatchers::ServeSetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    return 0;
}
