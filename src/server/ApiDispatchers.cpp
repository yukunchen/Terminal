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
    RETURN_NTSTATUS(SrvReadConsole(m, pbReplyPending));
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
    a->CurrentWindowSize.X = ex.srWindow.Right - ex.srWindow.Left;
    a->CurrentWindowSize.Y = ex.srWindow.Bottom - ex.srWindow.Top;
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

    PVOID Buffer;
    ULONG cbBuffer;
    RETURN_IF_FAILED(m->GetOutputBuffer(&Buffer, &cbBuffer));

    // a->TitleLength contains length in bytes.
    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(Buffer);
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
        char* const psBuffer = reinterpret_cast<char*>(Buffer);
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

    PVOID Buffer;
    ULONG cbOriginalLength;

    RETURN_IF_FAILED(m->GetInputBuffer(&Buffer, &cbOriginalLength));

    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(Buffer);
        size_t const cchBuffer = cbOriginalLength / sizeof(wchar_t);
        return m->_pApiRoutines->SetConsoleTitleWImpl(pwsBuffer,
                                                      cchBuffer);
    }
    else
    {
        char* const psBuffer = reinterpret_cast<char*>(Buffer);
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
    PVOID Buffer;
    ULONG BufferSize;
    RETURN_IF_FAILED(m->GetInputBuffer(&Buffer, &BufferSize));

    PVOID InputTarget = nullptr;
    PVOID InputExeName;
    PVOID InputSource = nullptr;

    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidStringBuffer(a->Unicode,
                                                         Buffer,
                                                         BufferSize,
                                                         3,
                                                         (ULONG)a->ExeLength,
                                                         &InputExeName,
                                                         (ULONG)a->SourceLength,
                                                         &InputSource,
                                                         (ULONG)a->TargetLength,
                                                         &InputTarget));

    if (a->Unicode)
    {
        return m->_pApiRoutines->AddConsoleAliasWImpl((wchar_t*)InputSource,
                                                      a->SourceLength,
                                                      (wchar_t*)InputTarget,
                                                      a->TargetLength,
                                                      (wchar_t*)InputExeName,
                                                      a->ExeLength);
    }
    else
    {
        return m->_pApiRoutines->AddConsoleAliasAImpl((char*)InputSource,
                                                      a->SourceLength,
                                                      (char*)InputTarget,
                                                      a->TargetLength,
                                                      (char*)InputExeName,
                                                      a->ExeLength);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleAlias(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIAS_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAlias, a->Unicode);

    PVOID InputBuffer;
    ULONG InputBufferSize;
    RETURN_IF_FAILED(m->GetInputBuffer(&InputBuffer, &InputBufferSize));

    PVOID InputExe;
    PVOID InputSource = nullptr;
    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidStringBuffer(a->Unicode,
                                                         InputBuffer,
                                                         InputBufferSize,
                                                         2,
                                                         a->ExeLength,
                                                         &InputExe,
                                                         a->SourceLength,
                                                         &InputSource));

    PVOID OutputBuffer;
    ULONG OutputBufferSize;
    RETURN_IF_FAILED(m->GetOutputBuffer(&OutputBuffer, &OutputBufferSize));

    if (a->Unicode)
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasWImpl((wchar_t*)InputSource,
                                                                a->SourceLength,
                                                                (wchar_t*)OutputBuffer,
                                                                &OutputBufferSize,
                                                                (wchar_t*)InputExe,
                                                                a->ExeLength));
    }
    else
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasAImpl((char*)InputSource,
                                                                a->SourceLength,
                                                                (char*)OutputBuffer,
                                                                &OutputBufferSize,
                                                                (char*)InputExe,
                                                                a->ExeLength));
    }

    // TODO: MSFT: 9115192 - figure this out for size_t, etc.
    RETURN_IF_FAILED(ULongToUShort(OutputBufferSize, &a->TargetLength));

    m->SetReplyInformation(a->TargetLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliasesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETALIASESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasesLengthW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasesLength, a->Unicode);

    ULONG ExeNameLength;
    PVOID ExeName;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    if (a->Unicode)
    {
        return m->_pApiRoutines->GetConsoleAliasesLengthWImpl((wchar_t*)ExeName,
                                                              ExeNameLength,
                                                              &a->AliasesLength);
    }
    else
    {
        return m->_pApiRoutines->GetConsoleAliasesLengthAImpl((char*)ExeName,
                                                              ExeNameLength,
                                                              &a->AliasesLength);
    }

}

HRESULT ApiDispatchers::ServerGetConsoleAliasExesLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETALIASEXESLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleAliasExesLengthW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExesLength, a->Unicode);

    if (a->Unicode)
    {
        return m->_pApiRoutines->GetConsoleAliasExesLengthWImpl(&a->AliasExesLength);
    }
    else
    {
        return m->_pApiRoutines->GetConsoleAliasExesLengthAImpl(&a->AliasExesLength);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleAliases(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIASES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasesW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliases, a->Unicode);

    PVOID ExeName;
    ULONG ExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    PVOID OutputBuffer;
    DWORD AliasesBufferLength;
    RETURN_IF_FAILED(m->GetOutputBuffer(&OutputBuffer, &AliasesBufferLength));

    if (a->Unicode)
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesWImpl((wchar_t*)ExeName,
                                                                  ExeNameLength,
                                                                  (wchar_t*)OutputBuffer,
                                                                  &AliasesBufferLength));
    }
    else
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasesAImpl((char*)ExeName,
                                                                  ExeNameLength,
                                                                  (char*)OutputBuffer,
                                                                  &AliasesBufferLength));
    }

    a->AliasesBufferLength = AliasesBufferLength;

    m->SetReplyInformation(a->AliasesBufferLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerGetConsoleAliasExes(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_GETALIASEXES_MSG* const a = &m->u.consoleMsgL3.GetConsoleAliasExesW;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleAliasExes, a->Unicode);

    PVOID Buffer;
    DWORD AliasExesBufferLength;
    RETURN_IF_FAILED(m->GetOutputBuffer(&Buffer, &AliasExesBufferLength));

    if (a->Unicode)
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesWImpl((wchar_t*)Buffer,
                                                                    &AliasExesBufferLength));
    }
    else
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleAliasExesAImpl((char*)Buffer,
                                                                    &AliasExesBufferLength));
    }

    a->AliasExesBufferLength = AliasExesBufferLength;

    m->SetReplyInformation(a->AliasExesBufferLength);

    return S_OK;
}

HRESULT ApiDispatchers::ServerExpungeConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_EXPUNGECOMMANDHISTORY_MSG* const a = &m->u.consoleMsgL3.ExpungeConsoleCommandHistoryW;

    PVOID ExeName;
    ULONG ExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    if (a->Unicode)
    {
        return m->_pApiRoutines->ExpungeConsoleCommandHistoryWImpl((wchar_t*)ExeName,
                                                                   ExeNameLength);
    }
    else
    {
        return m->_pApiRoutines->ExpungeConsoleCommandHistoryAImpl((char*)ExeName,
                                                                   ExeNameLength);
    }
}

HRESULT ApiDispatchers::ServerSetConsoleNumberOfCommands(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    CONSOLE_SETNUMBEROFCOMMANDS_MSG* const a = &m->u.consoleMsgL3.SetConsoleNumberOfCommandsW;
    PVOID ExeName;
    ULONG ExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    if (a->Unicode)
    {
        return m->_pApiRoutines->SetConsoleNumberOfCommandsWImpl((wchar_t*)ExeName,
                                                                 ExeNameLength,
                                                                 a->NumCommands);
    }
    else
    {
        return m->_pApiRoutines->SetConsoleNumberOfCommandsAImpl((char*)ExeName,
                                                                 ExeNameLength,
                                                                 a->NumCommands);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleCommandHistoryLength(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORYLENGTH_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryLengthW;

    PVOID ExeName;
    ULONG ExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    if (a->Unicode)
    {
        return m->_pApiRoutines->GetConsoleCommandHistoryLengthWImpl((wchar_t*)ExeName,
                                                                     ExeNameLength,
                                                                     &a->CommandHistoryLength);
    }
    else
    {
        return m->_pApiRoutines->GetConsoleCommandHistoryLengthAImpl((char*)ExeName,
                                                                     ExeNameLength,
                                                                     &a->CommandHistoryLength);
    }
}

HRESULT ApiDispatchers::ServerGetConsoleCommandHistory(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const /*pbReplyPending*/)
{
    PCONSOLE_GETCOMMANDHISTORY_MSG const a = &m->u.consoleMsgL3.GetConsoleCommandHistoryW;

    PVOID ExeName;
    ULONG ExeNameLength;
    RETURN_IF_FAILED(m->GetInputBuffer(&ExeName, &ExeNameLength));

    PVOID OutputBuffer;
    RETURN_IF_FAILED(m->GetOutputBuffer(&OutputBuffer, &a->CommandBufferLength));

    if (a->Unicode)
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryWImpl((wchar_t*)ExeName,
                                                                         ExeNameLength,
                                                                         (wchar_t*)OutputBuffer,
                                                                         &a->CommandBufferLength));
    }
    else
    {
        RETURN_IF_FAILED(m->_pApiRoutines->GetConsoleCommandHistoryAImpl((char*)ExeName,
                                                                         ExeNameLength,
                                                                         (char*)OutputBuffer,
                                                                         &a->CommandBufferLength));
    }

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
