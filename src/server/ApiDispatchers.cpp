/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "ApiDispatchers.h"

#include "..\host\directio.h"
#include "..\host\getset.h"
#include "..\host\stream.h"
#include "..\host\srvinit.h"
#include "..\host\telemetry.hpp"

HRESULT ApiDispatchers::ServerGetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETCP_MSG* const a = &m->u.consoleMsgL1.GetConsoleCP;

    if (a->Output)
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleOutputCP);
        return m->_pApiRoutines->GetConsoleOutputCodePageImpl(&a->CodePage);
    }
    else
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleCP);
        return m->_pApiRoutines->GetConsoleInputCodePageImpl(&a->CodePage);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleMode);
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.GetConsoleMode;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);
    if (pObjectHandle->IsInputHandle())
    {
        INPUT_INFORMATION* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_READ, &pObj));
        return m->_pApiRoutines->GetConsoleInputModeImpl(pObj, &a->Mode);
    }
    else
    {
        SCREEN_INFORMATION* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));
        return m->_pApiRoutines->GetConsoleOutputModeImpl(pObj, &a->Mode);
    }
}

HRESULT ApiDispatchers::ServerSetConsoleMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleMode);
    CONSOLE_MODE_MSG* const a = &m->u.consoleMsgL1.SetConsoleMode;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);
    if (pObjectHandle->IsInputHandle())
    {
        INPUT_INFORMATION* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_WRITE, &pObj));
        return m->_pApiRoutines->SetConsoleInputModeImpl(pObj, a->Mode);
    }
    else
    {
        SCREEN_INFORMATION* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));
        return m->_pApiRoutines->SetConsoleOutputModeImpl(pObj, a->Mode);
    }
}

HRESULT ApiDispatchers::ServerGetNumberOfInputEvents(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetNumberOfConsoleInputEvents);
    CONSOLE_GETNUMBEROFINPUTEVENTS_MSG* const a = &m->u.consoleMsgL1.GetNumberOfConsoleInputEvents;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    INPUT_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_READ, &pObj));

    return m->_pApiRoutines->GetNumberOfConsoleInputEventsImpl(pObj, &a->ReadyEvents);
}

HRESULT ApiDispatchers::ServerGetConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvGetConsoleInput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerReadConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    NTSTATUS Status = SrvReadConsole(m, pbReplyPending);
    if (Status != CONSOLE_STATUS_WAIT)
    {
        RETURN_NTSTATUS(Status);
    }
    else
    {
        return HRESULT_FROM_NT(Status);
    }
}

HRESULT ApiDispatchers::ServerWriteConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsole(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerFillConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvFillConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerSetConsoleActiveScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleActiveScreenBuffer);
    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleActiveScreenBufferImpl(pObj);
}

HRESULT ApiDispatchers::ServerFlushConsoleInputBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FlushConsoleInputBuffer);
    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    INPUT_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->FlushConsoleInputBuffer(pObj);
}

HRESULT ApiDispatchers::ServerSetConsoleCP(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_SETCP_MSG* const a = &m->u.consoleMsgL2.SetConsoleCP;

    if (a->Output)
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleOutputCP);
        return m->_pApiRoutines->SetConsoleOutputCodePageImpl(a->CodePage);
    }
    else
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleCP);
        return m->_pApiRoutines->SetConsoleInputCodePageImpl(a->CodePage);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleCursorInfo);
    CONSOLE_GETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleCursorInfo;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->GetConsoleCursorInfoImpl(pObj, &a->CursorSize, &a->Visible);
}

HRESULT ApiDispatchers::ServerSetConsoleCursorInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleCursorInfo);
    CONSOLE_SETCURSORINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorInfo;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleCursorInfoImpl(pObj, a->CursorSize, a->Visible);
}

HRESULT ApiDispatchers::ServerGetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleScreenBufferInfoEx);
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.GetConsoleScreenBufferInfo;

    CONSOLE_SCREEN_BUFFER_INFOEX ex = { 0 };
    ex.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));

    RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleScreenBufferInfoExImpl(pObj, &ex));

    a->FullscreenSupported = !!ex.bFullscreenSupported;
    size_t const ColorTableSizeInBytes = RTL_NUMBER_OF_V2(ex.ColorTable) * sizeof(*ex.ColorTable);
    CopyMemory(a->ColorTable, ex.ColorTable, ColorTableSizeInBytes);
    a->CursorPosition = ex.dwCursorPosition;
    a->MaximumWindowSize = ex.dwMaximumWindowSize;
    a->Size = ex.dwSize;
    a->ScrollPosition.X = ex.srWindow.Left;
    a->ScrollPosition.Y = ex.srWindow.Top;
    a->CurrentWindowSize.X = ex.srWindow.Right - ex.srWindow.Left + 1;
    a->CurrentWindowSize.Y = ex.srWindow.Bottom - ex.srWindow.Top + 1;
    a->Attributes = ex.wAttributes;
    a->PopupAttributes = ex.wPopupAttributes;

    return S_OK;
}

HRESULT ApiDispatchers::ServerSetConsoleScreenBufferInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleScreenBufferInfoEx);
    CONSOLE_SCREENBUFFERINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferInfo;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

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

    return m->_pApiRoutines->SetConsoleScreenBufferInfoExImpl(pObj, &ex);
}

HRESULT ApiDispatchers::ServerSetConsoleScreenBufferSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleScreenBufferSize);
    CONSOLE_SETSCREENBUFFERSIZE_MSG* const a = &m->u.consoleMsgL2.SetConsoleScreenBufferSize;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleScreenBufferSizeImpl(pObj, &a->Size);
}

HRESULT ApiDispatchers::ServerSetConsoleCursorPosition(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleCursorPosition);
    CONSOLE_SETCURSORPOSITION_MSG* const a = &m->u.consoleMsgL2.SetConsoleCursorPosition;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleCursorPositionImpl(pObj, &a->CursorPosition);
}

HRESULT ApiDispatchers::ServerGetLargestConsoleWindowSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetLargestConsoleWindowSize);
    CONSOLE_GETLARGESTWINDOWSIZE_MSG* const a = &m->u.consoleMsgL2.GetLargestConsoleWindowSize;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->GetLargestConsoleWindowSizeImpl(pObj, &a->Size);
}

HRESULT ApiDispatchers::ServerScrollConsoleScreenBuffer(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_SCROLLSCREENBUFFER_MSG* const a = &m->u.consoleMsgL2.ScrollConsoleScreenBuffer;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ScrollConsoleScreenBuffer, a->Unicode);

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    if (a->Unicode)
    {
        return m->_pApiRoutines->ScrollConsoleScreenBufferWImpl(pObj,
                                                                &a->ScrollRectangle,
                                                                &a->DestinationOrigin,
                                                                a->Clip ? &a->ClipRectangle : nullptr,
                                                                a->Fill.Char.UnicodeChar,
                                                                a->Fill.Attributes);
    }
    else
    {
        return m->_pApiRoutines->ScrollConsoleScreenBufferAImpl(pObj,
                                                                &a->ScrollRectangle,
                                                                &a->DestinationOrigin,
                                                                a->Clip ? &a->ClipRectangle : nullptr,
                                                                a->Fill.Char.AsciiChar,
                                                                a->Fill.Attributes);
    }
}

HRESULT ApiDispatchers::ServerSetConsoleTextAttribute(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleTextAttribute);
    CONSOLE_SETTEXTATTRIBUTE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTextAttribute;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleTextAttributeImpl(pObj, a->Attributes);
}

HRESULT ApiDispatchers::ServerSetConsoleWindowInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleWindowInfo);
    CONSOLE_SETWINDOWINFO_MSG* const a = &m->u.consoleMsgL2.SetConsoleWindowInfo;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleWindowInfoImpl(pObj, a->Absolute, &a->Window);
}

HRESULT ApiDispatchers::ServerReadConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvReadConsoleOutputString(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerWriteConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleInput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerWriteConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerWriteConsoleOutputString(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvWriteConsoleOutputString(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerReadConsoleOutput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    RETURN_NTSTATUS(SrvReadConsoleOutput(m, pbReplyPending));
}

HRESULT ApiDispatchers::ServerGetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETTITLE_MSG const a = &m->u.consoleMsgL2.GetConsoleTitle;
    Telemetry::Instance().LogApiCall(a->Original ? Telemetry::ApiCall::GetConsoleOriginalTitle : Telemetry::ApiCall::GetConsoleTitle, a->Unicode);

    PVOID pvBuffer;
    ULONG cbBuffer;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvBuffer, &cbBuffer));

    // a->TitleLength contains length in bytes.
    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchBuffer = cbBuffer / sizeof(wchar_t);
        size_t cchWritten;
        if (a->Original)
        {
            RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleOriginalTitleWImpl(pwsBuffer,
                                                                            cchBuffer,
                                                                            &cchWritten));
        }
        else
        {
            RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleTitleWImpl(pwsBuffer,
                                                                    cchBuffer,
                                                                    &cchWritten));
        }

        // We must return the character length of the string in a->TitleLength
        RETURN_IF_FAILED(SizeTToULong(cchWritten, &a->TitleLength));

        // Number of bytes written + the trailing null.
        m->SetReplyInformation((cchWritten + 1) * sizeof(wchar_t));
    }
    else
    {
        char* const psBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchBuffer = cbBuffer;
        size_t cchWritten;
        if (a->Original)
        {
            RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleOriginalTitleAImpl(psBuffer,
                                                                            cchBuffer,
                                                                            &cchWritten));
        }
        else
        {
            RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleTitleAImpl(psBuffer,
                                                                    cchBuffer,
                                                                    &cchWritten));
        }

        // We must return the character length of the string in a->TitleLength
        RETURN_IF_FAILED(SizeTToULong(cchWritten, &a->TitleLength));

        // Number of bytes written + the trailing null.
        m->SetReplyInformation((cchWritten + 1) * sizeof(char));
    }

    return S_OK;
}

HRESULT ApiDispatchers::ServerSetConsoleTitle(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_SETTITLE_MSG* const a = &m->u.consoleMsgL2.SetConsoleTitle;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleTitle, a->Unicode);

    PVOID pvBuffer;
    ULONG cbOriginalLength;

    RETURN_IF_FAILED(m->GetInputBuffer(&pvBuffer, &cbOriginalLength));

    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchBuffer = cbOriginalLength / sizeof(wchar_t);
        return m->_pApiRoutines->SetConsoleTitleWImpl(pwsBuffer,
                                                      cchBuffer);
    }
    else
    {
        char* const psBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchBuffer = cbOriginalLength;
        return m->_pApiRoutines->SetConsoleTitleAImpl(psBuffer,
                                                      cchBuffer);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleMouseInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetNumberOfConsoleMouseButtons);
    CONSOLE_GETMOUSEINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleMouseInfo;

    return m->_pApiRoutines->GetNumberOfConsoleMouseButtonsImpl(&a->NumButtons);
}

HRESULT ApiDispatchers::ServerGetConsoleFontSize(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleFontSize);
    CONSOLE_GETFONTSIZE_MSG* const a = &m->u.consoleMsgL3.GetConsoleFontSize;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));

    return m->_pApiRoutines->GetConsoleFontSizeImpl(pObj, a->FontIndex, &a->FontSize);
}

HRESULT ApiDispatchers::ServerGetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetCurrentConsoleFontEx);
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.GetCurrentConsoleFont;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));

    CONSOLE_FONT_INFOEX FontInfo = { 0 };
    FontInfo.cbSize = sizeof(FontInfo);

    RETURN_IF_FAILED(m->_pApiRoutines->GetCurrentConsoleFontExImpl(pObj, a->MaximumWindow, &FontInfo));

    CopyMemory(a->FaceName, FontInfo.FaceName, RTL_NUMBER_OF_V2(a->FaceName));
    a->FontFamily = FontInfo.FontFamily;
    a->FontIndex = FontInfo.nFont;
    a->FontSize = FontInfo.dwFontSize;
    a->FontWeight = FontInfo.FontWeight;

    return S_OK;
}

HRESULT ApiDispatchers::ServerSetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleDisplayMode);
    CONSOLE_SETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.SetConsoleDisplayMode;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    return m->_pApiRoutines->SetConsoleDisplayModeImpl(pObj, a->dwFlags, &a->ScreenBufferDimensions);
}

HRESULT ApiDispatchers::ServerGetConsoleDisplayMode(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleDisplayMode);
    CONSOLE_GETDISPLAYMODE_MSG* const a = &m->u.consoleMsgL3.GetConsoleDisplayMode;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));

    return m->_pApiRoutines->GetConsoleDisplayModeImpl(pObj, &a->ModeFlags);
}

// TODO: MSFT: 9115192 - remove extern and fetch from cmdline.cpp
BOOLEAN IsValidStringBuffer(_In_ BOOLEAN Unicode, _In_reads_bytes_(Size) PVOID Buffer, _In_ ULONG Size, _In_ ULONG Count, ...);

HRESULT ApiDispatchers::ServerAddConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_ADDALIAS_MSG* const a = &m->u.consoleMsgL3.AddConsoleAliasW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::AddConsoleAlias, a->Unicode);

    // Read the input buffer and validate the strings.
    PVOID pvBuffer;
    ULONG cbBufferSize;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvBuffer, &cbBufferSize));

    PVOID pvInputTarget;
    ULONG const cbInputTarget = a->TargetLength;
    PVOID pvInputExeName;
    ULONG const cbInputExeName = a->ExeLength;
    PVOID pvInputSource;
    ULONG const cbInputSource = a->SourceLength;
    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidStringBuffer(a->Unicode,
                                                         pvBuffer,
                                                         cbBufferSize,
                                                         3,
                                                         cbInputExeName,
                                                         &pvInputExeName,
                                                         cbInputSource,
                                                         &pvInputSource,
                                                         cbInputTarget,
                                                         &pvInputTarget));

    if (a->Unicode)
    {
        wchar_t* const pwsInputSource = reinterpret_cast<wchar_t*>(pvInputSource);
        size_t const cchInputSource = cbInputSource / sizeof(wchar_t);
        wchar_t* const pwsInputTarget = reinterpret_cast<wchar_t*>(pvInputTarget);
        size_t const cchInputTarget = cbInputTarget / sizeof(wchar_t);
        wchar_t* const pwsInputExeName = reinterpret_cast<wchar_t*>(pvInputExeName);
        size_t const cchInputExeName = cbInputExeName / sizeof(wchar_t);

        return m->_pApiRoutines->AddConsoleAliasWImpl(pwsInputSource,
                                                      cchInputSource,
                                                      pwsInputTarget,
                                                      cchInputTarget,
                                                      pwsInputExeName,
                                                      cchInputExeName);
    }
    else
    {
        char* const psInputSource = reinterpret_cast<char*>(pvInputSource);
        size_t const cchInputSource = cbInputSource;
        char* const psInputTarget = reinterpret_cast<char*>(pvInputTarget);
        size_t const cchInputTarget = cbInputTarget;
        char* const psInputExeName = reinterpret_cast<char*>(pvInputExeName);
        size_t const cchInputExeName = cbInputExeName;


        return m->_pApiRoutines->AddConsoleAliasAImpl(psInputSource,
                                                      cchInputSource,
                                                      psInputTarget,
                                                      cchInputTarget,
                                                      psInputExeName,
                                                      cchInputExeName);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIAS_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAlias, a->Unicode);

    PVOID pvInputBuffer;
    ULONG cbInputBufferSize;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvInputBuffer, &cbInputBufferSize));

    PVOID pvInputExe;
    ULONG const cbInputExe = a->ExeLength;
    PVOID pvInputSource;
    ULONG const cbInputSource = a->SourceLength;
    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidStringBuffer(a->Unicode,
                                                         pvInputBuffer,
                                                         cbInputBufferSize,
                                                         2,
                                                         cbInputExe,
                                                         &pvInputExe,
                                                         cbInputSource,
                                                         &pvInputSource));

    PVOID pvOutputBuffer;
    ULONG cbOutputBufferSize;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvOutputBuffer, &cbOutputBufferSize));

    size_t cbWritten;
    if (a->Unicode)
    {
        const wchar_t* const pwsInputExe = reinterpret_cast<wchar_t*>(pvInputExe);
        size_t const cchInputExe = cbInputExe / sizeof(wchar_t);
        const wchar_t* const pwsInputSource = reinterpret_cast<wchar_t*>(pvInputSource);
        size_t const cchInputSource = cbInputSource / sizeof(wchar_t);

        wchar_t* const pwsOutputBuffer = reinterpret_cast<wchar_t*>(pvOutputBuffer);
        size_t const cchOutputBuffer = cbOutputBufferSize / sizeof(wchar_t);
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasWImpl(pwsInputSource,
                                                                cchInputSource,
                                                                pwsOutputBuffer,
                                                                cchOutputBuffer,
                                                                &cchWritten,
                                                                pwsInputExe,
                                                                cchInputExe));

        // We must set the reply length in bytes. Convert back from characters.
        RETURN_IF_FAILED(SizeTMult(cchWritten, sizeof(wchar_t), &cbWritten));
    }
    else
    {
        const char* const psInputExe = reinterpret_cast<char*>(pvInputExe);
        size_t const cchInputExe = cbInputExe;
        const char* const psInputSource = reinterpret_cast<char*>(pvInputSource);
        size_t const cchInputSource = cbInputSource;

        char* const psOutputBuffer = reinterpret_cast<char*>(pvOutputBuffer);
        size_t const cchOutputBuffer = cbOutputBufferSize;
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasAImpl(psInputSource,
                                                                cchInputSource,
                                                                psOutputBuffer,
                                                                cchOutputBuffer,
                                                                &cchWritten,
                                                                psInputExe,
                                                                cchInputExe));

        cbWritten = cchWritten;
    }

    // We must return the byte length of the written data in the message
    RETURN_IF_FAILED(SizeTToUShort(cbWritten, &a->TargetLength));

    m->SetReplyInformation(a->TargetLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETALIASESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasesLengthW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasesLength, a->Unicode);

    ULONG cbExeNameLength;
    PVOID pvExeName;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    size_t cbAliasesLength;
    if (a->Unicode)
    {
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);
        size_t cchAliasesLength;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesLengthWImpl(pwsExeName, cchExeName, &cchAliasesLength));

        RETURN_IF_FAILED(SizeTMult(cchAliasesLength, sizeof(wchar_t), &cbAliasesLength));
    }
    else
    {
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;
        size_t cchAliasesLength;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesLengthAImpl(psExeName, cchExeName, &cchAliasesLength));

        cbAliasesLength = cchAliasesLength;
    }

    RETURN_IF_FAILED(SizeTToULong(cbAliasesLength, &a->AliasesLength));

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETALIASEXESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasExesLengthW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExesLength, a->Unicode);

    size_t cbAliasExesLength;
    if (a->Unicode)
    {
        size_t cchAliasExesLength;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesLengthWImpl(&cchAliasExesLength));
        cbAliasExesLength = cchAliasExesLength /= sizeof(wchar_t);
    }
    else
    {
        size_t cchAliasExesLength;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesLengthAImpl(&cchAliasExesLength));
        cbAliasExesLength = cchAliasExesLength;
    }

    RETURN_IF_FAILED(SizeTToULong(cbAliasExesLength, &a->AliasExesLength));

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliases(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIASES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliases, a->Unicode);

    PVOID pvExeName;
    ULONG cbExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    PVOID pvOutputBuffer;
    DWORD cbAliasesBufferLength;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvOutputBuffer, &cbAliasesBufferLength));

    size_t cbWritten;
    if (a->Unicode)
    {
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);

        wchar_t* const pwsAliasesBuffer = reinterpret_cast<wchar_t*>(pvOutputBuffer);
        size_t const cchAliasesBuffer = cbAliasesBufferLength / sizeof(wchar_t);
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesWImpl(pwsExeName,
                                                                  cchExeName,
                                                                  pwsAliasesBuffer,
                                                                  cchAliasesBuffer,
                                                                  &cchWritten));

        // We must set the reply length in bytes. Convert back from characters.
        RETURN_IF_FAILED(SizeTMult(cchWritten, sizeof(wchar_t), &cbWritten));
    }
    else
    {
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;

        char* const psAliasesBuffer = reinterpret_cast<char*>(pvOutputBuffer);
        size_t const cchAliasesBuffer = cbAliasesBufferLength;
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesAImpl(psExeName,
                                                                  cchExeName,
                                                                  psAliasesBuffer,
                                                                  cchAliasesBuffer,
                                                                  &cchWritten));
        cbWritten = cchWritten;
    }

    RETURN_IF_FAILED(SizeTToULong(cbWritten, &a->AliasesBufferLength));

    m->SetReplyInformation(a->AliasesBufferLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIASEXES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExes, a->Unicode);

    PVOID pvBuffer;
    ULONG cbAliasExesBufferLength;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvBuffer, &cbAliasExesBufferLength));

    size_t cbWritten;
    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchBuffer = cbAliasExesBufferLength / sizeof(wchar_t);
        size_t cchWritten;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesWImpl(pwsBuffer,
                                                                    cchBuffer,
                                                                    &cchWritten));

        RETURN_IF_FAILED(SizeTMult(cchWritten, sizeof(wchar_t), &cbWritten));
    }
    else
    {
        char* const psBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchBuffer = cbAliasExesBufferLength;
        size_t cchWritten;
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesAImpl(psBuffer,
                                                                    cchBuffer,
                                                                    &cchWritten));

        cbWritten = cchWritten;
    }

    // We must return the byte length of the written data in the message
    RETURN_IF_FAILED(SizeTToULong(cbWritten, &a->AliasExesBufferLength));

    m->SetReplyInformation(a->AliasExesBufferLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_EXPUNGECOMMANDHISTORY_MSG* const a = &m->u.consoleMsgL3.ExpungeConsoleCommandHistoryW;

    PVOID pvExeName;
    ULONG cbExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    if (a->Unicode)
    {
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);

        return m->_pApiRoutines->ExpungeConsoleCommandHistoryWImpl(pwsExeName, cchExeName);
    }
    else
    {
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;

        return m->_pApiRoutines->ExpungeConsoleCommandHistoryAImpl(psExeName, cchExeName);
    }
}

HRESULT ApiDispatchers::ServerSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_SETNUMBEROFCOMMANDS_MSG* const a = &m->u.consoleMsgL3.SetConsoleNumberOfCommandsW;
    PVOID pvExeName;
    ULONG cbExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    size_t const NumberOfCommands = a->NumCommands;
    if (a->Unicode)
    {
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);

        return m->_pApiRoutines->SetConsoleNumberOfCommandsWImpl(pwsExeName,
                                                                 cchExeName,
                                                                 NumberOfCommands);
    }
    else
    {
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;

        return m->_pApiRoutines->SetConsoleNumberOfCommandsAImpl(psExeName,
                                                                 cchExeName,
                                                                 NumberOfCommands);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORYLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryLengthW;

    PVOID pvExeName;
    ULONG cbExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    size_t cbCommandHistoryLength;
    if (a->Unicode)
    {
        size_t cchCommandHistoryLength;
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryLengthWImpl(pwsExeName, cchExeName, &cchCommandHistoryLength));

        // We must set the reply length in bytes. Convert back from characters.
        RETURN_IF_FAILED(SizeTMult(cchCommandHistoryLength, sizeof(wchar_t), &cbCommandHistoryLength));
    }
    else
    {
        size_t cchCommandHistoryLength;
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryLengthAImpl(psExeName, cchExeName, &cchCommandHistoryLength));

        cbCommandHistoryLength = cchCommandHistoryLength;
    }

    // Fit return value into structure memory size
    RETURN_IF_FAILED(SizeTToULong(cbCommandHistoryLength, &a->CommandHistoryLength));

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORY_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryW;

    PVOID pvExeName;
    ULONG cbExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&pvExeName, &cbExeNameLength));

    PVOID pvOutputBuffer;
    ULONG cbOutputBuffer;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvOutputBuffer, &cbOutputBuffer));

    size_t cbWritten;
    if (a->Unicode)
    {
        const wchar_t* const pwsExeName = reinterpret_cast<wchar_t*>(pvExeName);
        size_t const cchExeName = cbExeNameLength / sizeof(wchar_t);

        wchar_t* const pwsOutput = reinterpret_cast<wchar_t*>(pvOutputBuffer);
        size_t const cchOutput = cbOutputBuffer / sizeof(wchar_t);
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryWImpl(pwsExeName,
                                                                         cchExeName,
                                                                         pwsOutput,
                                                                         cchOutput,
                                                                         &cchWritten));

        // We must set the reply length in bytes. Convert back from characters.
        RETURN_IF_FAILED(SizeTMult(cchWritten, sizeof(wchar_t), &cbWritten));
    }
    else
    {
        const char* const psExeName = reinterpret_cast<char*>(pvExeName);
        size_t const cchExeName = cbExeNameLength;

        char* const psOutput = reinterpret_cast<char*>(pvOutputBuffer);
        size_t const cchOutput = cbOutputBuffer;
        size_t cchWritten;

        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryAImpl(psExeName,
                                                                         cchExeName,
                                                                         psOutput,
                                                                         cchOutput,
                                                                         &cchWritten));

        cbWritten = cchWritten;
    }

    // Fit return value into structure memory size.
    RETURN_IF_FAILED(SizeTToULong(cbWritten, &a->CommandBufferLength));

    m->SetReplyInformation(a->CommandBufferLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleWindow(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleWindow);
    CONSOLE_GETCONSOLEWINDOW_MSG* const a = &m->u.consoleMsgL3.GetConsoleWindow;

    return m->_pApiRoutines->GetConsoleWindowImpl(&a->hwnd);
}

HRESULT ApiDispatchers::ServerGetConsoleSelectionInfo(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleSelectionInfo);
    CONSOLE_GETSELECTIONINFO_MSG* const a = &m->u.consoleMsgL3.GetConsoleSelectionInfo;

    return m->_pApiRoutines->GetConsoleSelectionInfoImpl(&a->SelectionInfo);
}

HRESULT ApiDispatchers::ServerGetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.GetConsoleHistory;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleHistoryInfo);

    CONSOLE_HISTORY_INFO info;
    info.cbSize = sizeof(info);

    RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleHistoryInfoImpl(&info));

    a->dwFlags = info.dwFlags;
    a->HistoryBufferSize = info.HistoryBufferSize;
    a->NumberOfHistoryBuffers = info.NumberOfHistoryBuffers;

    return S_OK;
}

HRESULT ApiDispatchers::ServerSetConsoleHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_HISTORY_MSG* const a = &m->u.consoleMsgL3.SetConsoleHistory;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetConsoleHistoryInfo);

    CONSOLE_HISTORY_INFO info;
    info.cbSize = sizeof(info);
    info.dwFlags = a->dwFlags;
    info.HistoryBufferSize = a->HistoryBufferSize;
    info.NumberOfHistoryBuffers = a->NumberOfHistoryBuffers;

    return m->_pApiRoutines->SetConsoleHistoryInfoImpl(&info);
}

HRESULT ApiDispatchers::ServerSetConsoleCurrentFont(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::SetCurrentConsoleFontEx);
    CONSOLE_CURRENTFONT_MSG* const a = &m->u.consoleMsgL3.SetCurrentConsoleFont;

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));

    CONSOLE_FONT_INFOEX Info;
    Info.cbSize = sizeof(Info);
    Info.dwFontSize = a->FontSize;
    CopyMemory(Info.FaceName, a->FaceName, RTL_NUMBER_OF_V2(Info.FaceName));
    Info.FontFamily = a->FontFamily;
    Info.FontWeight = a->FontWeight;

    return m->_pApiRoutines->SetCurrentConsoleFontExImpl(pObj, a->MaximumWindow, &Info);
}
