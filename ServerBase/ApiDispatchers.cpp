#include "stdafx.h"
#include "ApiDispatchers.h"

NTSTATUS ApiDispatchers::ServeGetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCP_MSG* const a = &m->u.consoleMsgL1.GetConsoleCP;

    NTSTATUS Result = 0;

    if (a->Output)
    {
        Result = pResponders->GetConsoleOutputCodePageImpl(&a->CodePage);
    }
    else
    {
        Result = pResponders->GetConsoleInputCodePageImpl(&a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.GetConsoleMode;
    NTSTATUS Result = 0;

    ConsoleObjectType const Type = m->GetObjectType();

    if (SUCCEEDED(Result))
    {
        if (ConsoleObjectType::Output == Type)
        {
            IConsoleOutputObject* obj;
            Result = m->GetOutputObject(GENERIC_READ, &obj);

            if (SUCCEEDED(Result))
            {
                Result = pResponders->GetConsoleOutputModeImpl(obj, &a->Mode);
            }
        }
        else if (ConsoleObjectType::Input == Type)
        {
            IConsoleInputObject* obj;
            Result = m->GetInputObject(GENERIC_READ, &obj);

            if (SUCCEEDED(Result))
            {
                Result = pResponders->GetConsoleInputModeImpl(obj, &a->Mode);
            }
        }
        else
        {
            Result = STATUS_UNSUCCESSFUL;
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.SetConsoleMode;
    NTSTATUS Result = 0;

    ConsoleObjectType const Type = m->GetObjectType();
    if (ConsoleObjectType::Output == Type)
    {
        IConsoleOutputObject* obj;
        Result = m->GetOutputObject(GENERIC_WRITE, &obj);

        if (SUCCEEDED(Result))
        {
            Result = pResponders->SetConsoleOutputModeImpl(obj, a->Mode);
        }
    }
    else if (ConsoleObjectType::Input == Type)
    {
        IConsoleInputObject* obj;
        Result = m->GetInputObject(GENERIC_WRITE, &obj);

        if (SUCCEEDED(Result))
        {
            Result = pResponders->SetConsoleInputModeImpl(obj, a->Mode);
        }
    }
    else
    {
        Result = STATUS_UNSUCCESSFUL;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetNumberOfInputEvents(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETNUMBEROFINPUTEVENTS_MSG* const a = &m->u.consoleMsgL1.GetNumberOfConsoleInputEvents;
    NTSTATUS Result = 0;

    IConsoleInputObject* obj;
    Result = m->GetInputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->GetNumberOfConsoleInputEventsImpl(obj, &a->ReadyEvents);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEINPUT_MSG* const a = &m->u.consoleMsgL1.GetConsoleInput;

    NTSTATUS Result = 0;
    DWORD Written = 0;

    bool const IsPeek = ((a->Flags & CONSOLE_READ_NOREMOVE) != 0 &&
                         (a->Flags & CONSOLE_READ_NOWAIT) != 0);

    IConsoleInputObject* obj;
    Result = m->GetInputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        if (IsPeek)
        {
            if (a->Unicode)
            {
                Result = pResponders->ReadConsoleInputWImpl(obj, nullptr, 0, &Written);
            }
            else
            {
                Result = pResponders->ReadConsoleInputAImpl(obj, nullptr, 0, &Written);
            }
        }
        else
        {
            if (a->Unicode)
            {
                Result = pResponders->ReadConsoleInputWImpl(obj, nullptr, 0, &Written);
            }
            else
            {
                Result = pResponders->ReadConsoleInputAImpl(obj, nullptr, 0, &Written);
            }
        }
    }

    a->NumRecords = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLE_MSG* const a = &m->u.consoleMsgL1.ReadConsoleW;

    NTSTATUS Result = 0;
    DWORD Read = 0;

    IConsoleInputObject* obj;
    Result = m->GetInputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        CONSOLE_READCONSOLE_CONTROL Control;
        Control.dwControlKeyState = a->ControlKeyState;
        Control.dwCtrlWakeupMask = a->CtrlWakeupMask;
        Control.nInitialChars = a->InitialNumBytes;
        Control.nLength = a->NumBytes;

        if (a->Unicode)
        {
            wchar_t* Buffer;
            ULONG BufferSize;
            m->GetOutputBuffer(&Buffer, &BufferSize);

            Result = pResponders->ReadConsoleWImpl(obj, Buffer, BufferSize, &Read, &Control);
        }
        else
        {
            char* Buffer;
            ULONG BufferSize;
            m->GetOutputBuffer(&Buffer, &BufferSize);

            Result = pResponders->ReadConsoleAImpl(obj, Buffer, BufferSize, &Read, &Control);
        }
    }

    a->NumBytes = SUCCEEDED(Result) ? Read : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsole(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLE_MSG* const a = &m->u.consoleMsgL1.WriteConsoleW;

    NTSTATUS Result = 0;
    DWORD Written = 0;

    // ensure we have the input buffer
    if (m->State.InputBuffer == nullptr)
    {
        Result = STATUS_INVALID_PARAMETER;
    }

    if (SUCCEEDED(Result))
    {
        IConsoleOutputObject* obj;
        Result = m->GetOutputObject(GENERIC_WRITE, &obj);

        if (SUCCEEDED(Result))
        {
            if (a->Unicode)
            {
                wchar_t* Buffer;
                ULONG BufferSize;
                m->GetInputBuffer(&Buffer, &BufferSize);

                Result = pResponders->WriteConsoleWImpl(obj, Buffer, BufferSize, &Written);
            }
            else
            {
                char* Buffer;
                ULONG BufferSize;
                m->GetInputBuffer(&Buffer, &BufferSize);

                Result = pResponders->WriteConsoleAImpl(obj, Buffer, BufferSize, &Written);
            }
        }
    }

    a->NumBytes = SUCCEEDED(Result) ? Written : 0;

    return Result;
}

#define CP_USA                 1252
#define CP_KOREAN              949
#define CP_JAPANESE            932
#define CP_CHINESE_SIMPLIFIED  936
#define CP_CHINESE_TRADITIONAL 950

NTSTATUS ApiDispatchers::ServeGetConsoleLangId(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_LANGID_MSG* const a = &m->u.consoleMsgL1.GetConsoleLangId;
    NTSTATUS Result = 0;

    DWORD CodePage;

    Result = pResponders->GetConsoleOutputCodePageImpl(&CodePage);

    if (SUCCEEDED(Result))
    {
        LANGID LangId;

        // convert output codepage to LangID.
        switch (CodePage)
        {
        case CP_JAPANESE:
            LangId = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
            break;
        case CP_KOREAN:
            LangId = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
            break;
        case CP_CHINESE_SIMPLIFIED:
            LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
            break;
        case CP_CHINESE_TRADITIONAL:
            LangId = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
            break;
        default:
            LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            break;
        }

        a->LangId = LangId;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeFillConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_FILLCONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.FillConsoleOutput;

    NTSTATUS Result = 0;
    DWORD ElementsWritten = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        switch (a->ElementType)
        {
        case CONSOLE_ATTRIBUTE:
        {
            Result = pResponders->FillConsoleOutputAttributeImpl(obj, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
            break;
        }
        case CONSOLE_ASCII:
        {
            Result = pResponders->FillConsoleOutputCharacterAImpl(obj, (char)a->Element, a->Length, a->WriteCoord, &ElementsWritten);
            break;
        }
        case CONSOLE_REAL_UNICODE:
        case CONSOLE_FALSE_UNICODE:
        {
            Result = pResponders->FillConsoleOutputCharacterWImpl(obj, a->Element, a->Length, a->WriteCoord, &ElementsWritten);
            break;
        }
        default:
        {
            Result = STATUS_UNSUCCESSFUL;
            break;
        }
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
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleActiveScreenBufferImpl(obj);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeFlushConsoleInputBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    NTSTATUS Result = 0;

    IConsoleInputObject* obj;
    Result = m->GetInputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->FlushConsoleInputBuffer(obj);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleCP(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCP_MSG* const a = &m->u.consoleMsgL2.SetConsoleCP;

    NTSTATUS Result = 0;

    if (a->Output)
    {
        Result = pResponders->SetConsoleOutputCodePageImpl(a->CodePage);
    }
    else
    {
        Result = pResponders->SetConsoleInputCodePageImpl(a->CodePage);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->GetConsoleCursorInfoImpl(obj, &a->CursorSize, &a->Visible);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorInfo;

    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleCursorInfoImpl(obj, a->CursorSize, a->Visible);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->GetConsoleScreenBufferInfoExImpl(obj, &ex);

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
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferInfo;
    NTSTATUS Result = 0;

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

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        pResponders->SetConsoleScreenBufferInfoExImpl(obj, &ex);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleScreenBufferSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETSCREENBUFFERSIZE_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferSize;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleScreenBufferSizeImpl(obj, &a->Size);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleCursorPosition(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETCURSORPOSITION_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorPosition;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleCursorPositionImpl(obj, &a->CursorPosition);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetLargestConsoleWindowSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETLARGESTWINDOWSIZE_MSG* const a = &m->u.consoleMsgL2.GetLargestConsoleWindowSize;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        pResponders->GetLargestConsoleWindowSizeImpl(obj, &a->Size);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeScrollConsoleScreenBuffer(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SCROLLSCREENBUFFER_MSG* const a = &m->u.consoleMsgL2.ScrollConsoleScreenBufferW;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        SMALL_RECT* pClipRectangle = a->Clip ? &a->ClipRectangle : nullptr;

        if (a->Unicode)
        {
            Result = pResponders->ScrollConsoleScreenBufferWImpl(obj, &a->ScrollRectangle, &a->DestinationOrigin, pClipRectangle, &a->Fill);
        }
        else
        {
            Result = pResponders->ScrollConsoleScreenBufferAImpl(obj, &a->ScrollRectangle, &a->DestinationOrigin, pClipRectangle, &a->Fill);
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTextAttribute(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETTEXTATTRIBUTE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTextAttribute;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleTextAttributeImpl(obj, a->Attributes);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleWindowInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETWINDOWINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleWindowInfo;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleWindowInfoImpl(obj, a->Absolute, &a->Window);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLEOUTPUTSTRING_MSG* const a = &m->u.consoleMsgL2.ReadConsoleOutputString;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        switch (a->StringType)
        {
        case CONSOLE_ATTRIBUTE:
        {
            Result = pResponders->ReadConsoleOutputAttributeImpl(obj, &a->ReadCoord, nullptr, 0, &a->NumRecords);
            break;
        }
        case CONSOLE_ASCII:
        {
            Result = pResponders->ReadConsoleOutputCharacterAImpl(obj, &a->ReadCoord, nullptr, 0, &a->NumRecords);
            break;
        }
        case CONSOLE_REAL_UNICODE:
        case CONSOLE_FALSE_UNICODE:
        {
            Result = pResponders->ReadConsoleOutputCharacterWImpl(obj, &a->ReadCoord, nullptr, 0, &a->NumRecords);
            break;
        }
        default:
        {
            Result = STATUS_UNSUCCESSFUL;
            break;
        }
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleInput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEINPUT_MSG* const a = &m->u.consoleMsgL2.WriteConsoleInputW;

    NTSTATUS Result = 0;
    DWORD Read = 0;

    IConsoleInputObject* obj;
    Result = m->GetInputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        if (a->Unicode)
        {
            Result = pResponders->WriteConsoleInputWImpl(obj, nullptr, 0, &Read);
        }
        else
        {
            Result = pResponders->WriteConsoleInputAImpl(obj, nullptr, 0, &Read);
        }
    }

    a->NumRecords = SUCCEEDED(Result) ? Read : 0;

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.WriteConsoleOutputW;
    NTSTATUS Result = 0;

    a->CharRegion = { 0, 0, -1, -1 };

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        COORD TextBufferSize = { 0 };
        COORD TextBufferSourceOrigin = { 0 };
        SMALL_RECT TargetRectangle = { 0 };
        SMALL_RECT AffectedRectangle;

        if (a->Unicode)
        {
            Result = pResponders->WriteConsoleOutputWImpl(obj, nullptr, &TextBufferSize, &TextBufferSourceOrigin, &TargetRectangle, &AffectedRectangle);
        }
        else
        {
            Result = pResponders->WriteConsoleOutputAImpl(obj, nullptr, &TextBufferSize, &TextBufferSourceOrigin, &TargetRectangle, &AffectedRectangle);
        }

        if (SUCCEEDED(Result))
        {
            a->CharRegion = AffectedRectangle;
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeWriteConsoleOutputString(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_WRITECONSOLEOUTPUTSTRING_MSG* const a = &m->u.consoleMsgL2.WriteConsoleOutputString;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        switch (a->StringType)
        {
        case CONSOLE_ATTRIBUTE:
        {
            Result = pResponders->WriteConsoleOutputAttributeImpl(obj, nullptr, 0, &a->WriteCoord, &a->NumRecords);
            break;
        }
        case CONSOLE_ASCII:
        {
            Result = pResponders->WriteConsoleOutputCharacterAImpl(obj, nullptr, 0, &a->WriteCoord, &a->NumRecords);
            break;
        }
        case CONSOLE_REAL_UNICODE:
        case CONSOLE_FALSE_UNICODE:
        {
            Result = pResponders->WriteConsoleOutputCharacterWImpl(obj, nullptr, 0, &a->WriteCoord, &a->NumRecords);
            break;
        }
        default:
        {
            Result = STATUS_UNSUCCESSFUL;
            break;
        }
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeReadConsoleOutput(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_READCONSOLEOUTPUT_MSG* const a = &m->u.consoleMsgL2.ReadConsoleOutputW;
    NTSTATUS Result = 0;

    a->CharRegion = { 0, 0, -1, -1 };

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {

        COORD TextBufferSize = { 0 };
        COORD TextBufferTargetOrigin = { 0 };
        SMALL_RECT SourceRectangle = { 0 };
        SMALL_RECT ReadRectangle;

        if (a->Unicode)
        {
            Result = pResponders->ReadConsoleOutputW(obj, nullptr, &TextBufferSize, &TextBufferTargetOrigin, &SourceRectangle, &ReadRectangle);
        }
        else
        {
            Result = pResponders->ReadConsoleOutputA(obj, nullptr, &TextBufferSize, &TextBufferTargetOrigin, &SourceRectangle, &ReadRectangle);
        }

        if (SUCCEEDED(Result))
        {
            a->CharRegion = ReadRectangle;
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETTITLE_MSG* const a = &m->u.consoleMsgL2.GetConsoleTitleW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        if (a->Original)
        {
            Result = pResponders->GetConsoleOriginalTitleWImpl(nullptr, 0);
        }
        else
        {
            Result = pResponders->GetConsoleTitleWImpl(nullptr, 0);
        }
    }
    else
    {
        if (a->Original)
        {
            Result = pResponders->GetConsoleOriginalTitleAImpl(nullptr, 0);
        }
        else
        {
            Result = pResponders->GetConsoleTitleAImpl(nullptr, 0);
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleTitle(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETTITLE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTitleW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->SetConsoleTitleWImpl(nullptr, 0);
    }
    else
    {
        Result = pResponders->SetConsoleTitleAImpl(nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleMouseInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETMOUSEINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleMouseInfo;

    return pResponders->GetNumberOfConsoleMouseButtonsImpl(&a->NumButtons);
}

NTSTATUS ApiDispatchers::ServeGetConsoleFontSize(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETFONTSIZE_MSG* const a = &m->u.consoleMsgL3.GetConsoleFontSize;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->GetConsoleFontSizeImpl(obj, a->FontIndex, &a->FontSize);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.GetCurrentConsoleFont;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        CONSOLE_FONT_INFOEX FontInfo = { 0 };
        FontInfo.cbSize = sizeof(FontInfo);

        Result = pResponders->GetCurrentConsoleFontExImpl(obj, a->MaximumWindow, &FontInfo);

        if (SUCCEEDED(Result))
        {
            CopyMemory(a->FaceName, FontInfo.FaceName, RTL_NUMBER_OF_V2(a->FaceName));
            a->FontFamily = FontInfo.FontFamily;
            a->FontIndex = FontInfo.nFont;
            a->FontSize = FontInfo.dwFontSize;
            a->FontWeight = FontInfo.FontWeight;
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_SETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.SetConsoleDisplayMode;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->SetConsoleDisplayModeImpl(obj, a->dwFlags, &a->ScreenBufferDimensions);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleDisplayMode(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.GetConsoleDisplayMode;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_READ, &obj);

    if (SUCCEEDED(Result))
    {
        Result = pResponders->GetConsoleDisplayModeImpl(obj, &a->ModeFlags);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeAddConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_ADDALIAS_MSG* const a = &m->u.consoleMsgL3.AddConsoleAliasW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->AddConsoleAliasWImpl(nullptr, 0, nullptr, 0, nullptr, 0);
    }
    else
    {
        Result = pResponders->AddConsoleAliasAImpl(nullptr, 0, nullptr, 0, nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAlias(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIAS_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasW;

    NTSTATUS Result = 0;

    // Check if input and output buffers are available to us
    if (!m->IsInputBufferAvailable() || !m->IsOutputBufferAvailable())
    {
        Result = STATUS_INVALID_PARAMETER;
    }

    if (SUCCEEDED(Result))
    {
        // The buffers we created were measured in byte values but could be holding either Unicode (UCS-2, a subset of UTF-16) 
        //   or ASCII (which includes single and multibyte codepages and UTF-8) strings.
        // Cast and recalculate lengths to convert from raw byte length into appropriate string types/lengths.

        if (a->Unicode)
        {
            // Divide input buffer into the two input strings
            wchar_t* ExeBuffer;
            ULONG ExeBufferSize = a->ExeLength;
            wchar_t* SourceBuffer;
            ULONG SourceBufferSize = a->SourceLength;

            Result = m->UnpackInputBuffer<wchar_t>(2, &ExeBufferSize, &ExeBuffer, &SourceBufferSize, &SourceBuffer);

            if (SUCCEEDED(Result))
            {
                // Get pointers to the output result buffer
                wchar_t* OutBuffer;
                ULONG OutBufferSize;
                m->GetOutputBuffer(&OutBuffer, &OutBufferSize);

                Result = pResponders->GetConsoleAliasWImpl(SourceBuffer,
                                                           SourceBufferSize,
                                                           OutBuffer,
                                                           OutBufferSize,
                                                           ExeBuffer,
                                                           ExeBufferSize);
            }
        }
        else
        {
            // Divide input buffer into the two input strings
            char* ExeBuffer;
            ULONG ExeBufferSize = a->ExeLength;
            char* SourceBuffer;
            ULONG SourceBufferSize = a->SourceLength;

            Result = m->UnpackInputBuffer<char>(2, &ExeBufferSize, &ExeBuffer, &SourceBufferSize, &SourceBuffer);

            if (SUCCEEDED(Result))
            {
                // Get pointers to the output result buffer
                char* OutBuffer;
                ULONG OutBufferSize;
                m->GetOutputBuffer(&OutBuffer, &OutBufferSize);

                Result = pResponders->GetConsoleAliasAImpl(SourceBuffer,
                                                           SourceBufferSize,
                                                           OutBuffer,
                                                           OutBufferSize,
                                                           ExeBuffer,
                                                           ExeBufferSize);
            }
        }
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASESLENGTH_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesLengthW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasesLengthWImpl(nullptr, 0, &a->AliasesLength);
    }
    else
    {
        Result = pResponders->GetConsoleAliasesLengthAImpl(nullptr, 0, &a->AliasesLength);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExesLength(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASEXESLENGTH_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesLengthW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasExesLengthWImpl(&a->AliasExesLength);
    }
    else
    {
        Result = pResponders->GetConsoleAliasExesLengthAImpl(&a->AliasExesLength);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliases(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasesWImpl(nullptr, 0, nullptr, 0);
    }
    else
    {
        Result = pResponders->GetConsoleAliasesAImpl(nullptr, 0, nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleAliasExes(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETALIASEXES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesW;

    NTSTATUS Result = 0;

    if (a->Unicode)
    {
        Result = pResponders->GetConsoleAliasExesWImpl(nullptr, 0);
    }
    else
    {
        Result = pResponders->GetConsoleAliasExesAImpl(nullptr, 0);
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeGetConsoleWindow(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETCONSOLEWINDOW_MSG* const a = &m->u.consoleMsgL3.GetConsoleWindow;

    return pResponders->GetConsoleWindowImpl(&a->hwnd);
}

NTSTATUS ApiDispatchers::ServeGetConsoleSelectionInfo(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_GETSELECTIONINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleSelectionInfo;

    return pResponders->GetConsoleSelectionInfoImpl(&a->SelectionInfo);
}

NTSTATUS ApiDispatchers::ServeGetConsoleProcessList(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    UNREFERENCED_PARAMETER(pResponders);

    CONSOLE_GETCONSOLEPROCESSLIST_MSG* const a = &m->u.consoleMsgL3.GetConsoleProcessList;

    // This function can be serviced from the server base.

    // Get output buffer we were given.
    DWORD* pProcessList;
    m->GetOutputBuffer<DWORD>(&pProcessList, &a->dwProcessCount);

    // Get the number of processes we know are attached
    size_t const ProcessCount = ConsoleProcessHandle::s_KnownProcesses.size();

    // If it can fit in the space we were granted, fill the buffer we were given.
    if (ProcessCount <= a->dwProcessCount)
    {
        // For every handle in the list
        for (auto const& value : ConsoleProcessHandle::s_KnownProcesses)
        {
            // Fill the output buffer.
            *pProcessList++ = value->GetProcessId();
        }
    }

    // Fill the process count on output with the number we have whether we could fill the buffer or not 
    // so the caller knows how much space to allocate if it couldn't fit.
    a->dwProcessCount = (ULONG)ProcessCount;

    // TODO: Set reply information?

    return STATUS_SUCCESS; // It's always a success even if we can't fill so the client can receive the count properly.
}
          
NTSTATUS ApiDispatchers::ServeGetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.GetConsoleHistory;

    CONSOLE_HISTORY_INFO Info;
    Info.cbSize = sizeof(Info);

    NTSTATUS Result = pResponders->GetConsoleHistoryInfoImpl(&Info);

    if (SUCCEEDED(Result))
    {
        a->HistoryBufferSize = Info.HistoryBufferSize;
        a->NumberOfHistoryBuffers = Info.NumberOfHistoryBuffers;
        a->dwFlags = Info.dwFlags;
    }

    return Result;
}

NTSTATUS ApiDispatchers::ServeSetConsoleHistory(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.SetConsoleHistory;

    CONSOLE_HISTORY_INFO Info;
    Info.cbSize = sizeof(Info);
    Info.dwFlags = a->dwFlags;
    Info.HistoryBufferSize = a->HistoryBufferSize;
    Info.NumberOfHistoryBuffers = a->NumberOfHistoryBuffers;

    return pResponders->SetConsoleHistoryInfoImpl(&Info);
}

NTSTATUS ApiDispatchers::ServeSetConsoleCurrentFont(_In_ IApiResponders* const pResponders, _Inout_ CONSOLE_API_MSG* const m)
{
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.SetCurrentConsoleFont;
    NTSTATUS Result = 0;

    IConsoleOutputObject* obj;
    Result = m->GetOutputObject(GENERIC_WRITE, &obj);

    if (SUCCEEDED(Result))
    {
        CONSOLE_FONT_INFOEX Info;
        Info.cbSize = sizeof(Info);
        Info.dwFontSize = a->FontSize;
        CopyMemory(Info.FaceName, a->FaceName, RTL_NUMBER_OF_V2(Info.FaceName));
        Info.FontFamily = a->FontFamily;
        Info.FontWeight = a->FontWeight;

        Result = pResponders->SetCurrentConsoleFontExImpl(obj, a->MaximumWindow, &Info);
    }

    return Result;
}
