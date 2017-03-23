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
    std::wstring handleType = L"unknown";

    auto tracing = wil::ScopeExit([&]()
    {
        Tracing::s_TraceApi(a, handleType);
    });

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);
    if (pObjectHandle->IsInputHandle())
    {
        handleType = L"input handle";
        InputBuffer* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_READ, &pObj));
        RETURN_HR(m->_pApiRoutines->GetConsoleInputModeImpl(pObj, &a->Mode));
    }
    else
    {
        handleType = L"output handle";
        SCREEN_INFORMATION* pObj;
        RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_READ, &pObj));
        RETURN_HR(m->_pApiRoutines->GetConsoleOutputModeImpl(pObj, &a->Mode));
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
        InputBuffer* pObj;
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

    InputBuffer* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetInputBuffer(GENERIC_READ, &pObj));

    return m->_pApiRoutines->GetNumberOfConsoleInputEventsImpl(pObj, &a->ReadyEvents);
}

HRESULT ApiDispatchers::ServerGetConsoleInput(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    *pbReplyPending = FALSE;

    CONSOLE_GETCONSOLEINPUT_MSG* const a = &m->u.consoleMsgL1.GetConsoleInput;
    if (IsFlagSet(a->Flags, CONSOLE_READ_NOREMOVE))
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::PeekConsoleInput, a->Unicode);
    }
    else
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleInput, a->Unicode);
    }

    a->NumRecords = 0;

    // If any flags are set that are not within our enum, it's invalid.
    if (IsAnyFlagSet(a->Flags, ~CONSOLE_READ_VALID))
    {
        return E_INVALIDARG;
    }

    // Make sure we have a valid input buffer.
    ConsoleHandleData* const pHandleData = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pHandleData);
    InputBuffer* pInputBuffer;
    RETURN_IF_FAILED(pHandleData->GetInputBuffer(GENERIC_READ, &pInputBuffer));

    // Get output buffer.
    PVOID pvBuffer;
    ULONG cbBufferSize;
    RETURN_IF_FAILED(m->GetOutputBuffer(&pvBuffer, &cbBufferSize));

    INPUT_RECORD* const rgRecords = reinterpret_cast<INPUT_RECORD*>(pvBuffer);
    size_t const cRecords = cbBufferSize / sizeof(INPUT_RECORD);

    bool const fIsPeek = IsFlagSet(a->Flags, CONSOLE_READ_NOREMOVE);
    bool const fIsWaitAllowed = IsFlagClear(a->Flags, CONSOLE_READ_NOWAIT);

    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = pHandleData->GetClientInput();

    IWaitRoutine* pWaiter = nullptr;
    HRESULT hr;
    size_t cRecordsWritten;
    if (a->Unicode)
    {
        if (fIsPeek)
        {
            hr = m->_pApiRoutines->PeekConsoleInputWImpl(pInputBuffer,
                                                         rgRecords,
                                                         cRecords,
                                                         &cRecordsWritten,
                                                         pInputReadHandleData,
                                                         &pWaiter);
        }
        else
        {
            hr = m->_pApiRoutines->ReadConsoleInputWImpl(pInputBuffer,
                                                         rgRecords,
                                                         cRecords,
                                                         &cRecordsWritten,
                                                         pInputReadHandleData,
                                                         &pWaiter);
        }
    }
    else
    {
        if (fIsPeek)
        {
            hr = m->_pApiRoutines->PeekConsoleInputAImpl(pInputBuffer,
                                                         rgRecords,
                                                         cRecords,
                                                         &cRecordsWritten,
                                                         pInputReadHandleData,
                                                         &pWaiter);
        }
        else
        {
            hr = m->_pApiRoutines->ReadConsoleInputAImpl(pInputBuffer,
                                                         rgRecords,
                                                         cRecords,
                                                         &cRecordsWritten,
                                                         pInputReadHandleData,
                                                         &pWaiter);
        }
    }

    // We must return the number of records in the message payload (to alert the client)
    // as well as in the message headers (below in SetReplyInfomration) to alert the driver.
    LOG_IF_FAILED(SizeTToULong(cRecordsWritten, &a->NumRecords));

    size_t cbWritten;
    LOG_IF_FAILED(SizeTMult(cRecordsWritten, sizeof(INPUT_RECORD), &cbWritten));

    if (nullptr != pWaiter)
    {
        // In some circumstances, the read may have told us to wait because it didn't have data,
        // but the client explicitly asked us to return immediate. In that case, we'll convert the
        // wait request into a "0 bytes found, OK". 

        if (fIsWaitAllowed)
        {
            hr = ConsoleWaitQueue::s_CreateWait(m, pWaiter);
            if (FAILED(hr))
            {
                delete pWaiter;
                pWaiter = nullptr;
            }
            else
            {
                *pbReplyPending = TRUE;
                hr = CONSOLE_STATUS_WAIT;
            }
        }
        else
        {
            // If wait isn't allowed and the routine generated a waiter, delete it and say there was nothing to be retrieved right now.
            delete pWaiter;
            pWaiter = nullptr;

            cbWritten = 0;
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        m->SetReplyInformation(cbWritten);
    }

    return hr;
}

HRESULT ApiDispatchers::ServerReadConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    *pbReplyPending = FALSE;

    CONSOLE_READCONSOLE_MSG* const a = &m->u.consoleMsgL1.ReadConsole;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsole, a->Unicode);

    a->NumBytes = 0; // we return 0 until proven otherwise.

    // Make sure we have a valid input buffer.
    ConsoleHandleData* const HandleData = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, HandleData);
    InputBuffer* pInputBuffer;
    RETURN_IF_FAILED(HandleData->GetInputBuffer(GENERIC_READ, &pInputBuffer));

    // Get output parameter buffer.
    PVOID pvBuffer;
    ULONG cbBufferSize;
    // TODO: This is dumb. We should find out how much we need, not guess.
    // If the request is not in Unicode mode, we must allocate an output buffer that is twice as big as the actual caller buffer.
    RETURN_IF_FAILED(m->GetAugmentedOutputBuffer((a->Unicode != FALSE) ? 1 : 2,
                                                 &pvBuffer,
                                                 &cbBufferSize));

    // TODO: This is also rather strange and will also probably make more sense if we stop guessing that we need 2x buffer to convert.
    // This might need to go on the other side of the fence (inside host) because the server doesn't know what we're going to do with initial num bytes.
    // (This restriction exists because it's going to copy initial into the final buffer, but we don't know that.)
    RETURN_HR_IF(E_INVALIDARG, a->InitialNumBytes > cbBufferSize); 

    // Retrieve input parameters.
    // 1. Exe name making the request
    ULONG const cchExeName = a->ExeNameLength;
    ULONG cbExeName;
    RETURN_IF_FAILED(ULongMult(cchExeName, sizeof(wchar_t), &cbExeName));
    wistd::unique_ptr<wchar_t[]> pwsExeName;
    
    if (cchExeName > 0)
    {
        pwsExeName = wil::make_unique_nothrow<wchar_t[]>(cchExeName);
        RETURN_IF_NULL_ALLOC(pwsExeName);
        RETURN_IF_FAILED(m->ReadMessageInput(0, pwsExeName.get(), cbExeName));
    }

    // 2. Existing data in the buffer that was passed in.
    ULONG const cbInitialData = a->InitialNumBytes;
    ULONG const cchInitialData = cbInitialData / sizeof(wchar_t);
    wistd::unique_ptr<wchar_t[]> pwsInitialData;
    
    if (cbInitialData > 0)
    {
        pwsInitialData = wil::make_unique_nothrow<wchar_t[]>(cbInitialData);
        RETURN_IF_NULL_ALLOC(pwsInitialData);

        // This parameter starts immediately after the exe name so skip by that many bytes.
        RETURN_IF_FAILED(m->ReadMessageInput(cbExeName, pwsInitialData.get(), cbInitialData));
    }

    // ReadConsole needs this to get the command history list associated with an attached process, but it can be an opaque value.
    HANDLE const hConsoleClient = (HANDLE)m->GetProcessHandle();

    // ReadConsole needs this to store context information across "processed reads" e.g. reads on the same handle
    // across multiple calls when we are simulating a command prompt input line for the client application.
    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = HandleData->GetClientInput();

    IWaitRoutine* pWaiter = nullptr;
    size_t cbWritten;

    HRESULT hr;
    if (a->Unicode)
    {
        wchar_t* const pwsOutputBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchOutputBuffer = cbBufferSize / sizeof(wchar_t);
        size_t cchOutputWritten;

        hr = m->_pApiRoutines->ReadConsoleWImpl(pInputBuffer,
                                                pwsOutputBuffer,
                                                cchOutputBuffer,
                                                &cchOutputWritten,
                                                &pWaiter,
                                                pwsInitialData.get(),
                                                cchInitialData,
                                                pwsExeName.get(),
                                                cchExeName,
                                                pInputReadHandleData,
                                                hConsoleClient,
                                                a->CtrlWakeupMask,
                                                &a->ControlKeyState);

        // We must set the reply length in bytes. Convert back from characters.
        LOG_IF_FAILED(SizeTMult(cchOutputWritten, sizeof(wchar_t), &cbWritten));
    }
    else
    {
        char* const psOutputBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchOutputBuffer = cbBufferSize;
        size_t cchOutputWritten;

        char* const psInitialData = reinterpret_cast<char*>(pwsInitialData.get());


        hr = m->_pApiRoutines->ReadConsoleAImpl(pInputBuffer,
                                                psOutputBuffer,
                                                cchOutputBuffer,
                                                &cchOutputWritten,
                                                &pWaiter,
                                                psInitialData,
                                                cbInitialData,
                                                pwsExeName.get(),
                                                cchExeName,
                                                pInputReadHandleData,
                                                hConsoleClient,
                                                a->CtrlWakeupMask,
                                                &a->ControlKeyState);

        cbWritten = cchOutputWritten;
    }

    LOG_IF_FAILED(SizeTToULong(cbWritten, &a->NumBytes));

    if (nullptr != pWaiter) 
    {
        // If we received a waiter, we need to queue the wait and not reply.
        hr = ConsoleWaitQueue::s_CreateWait(m, pWaiter);
        
        if (FAILED(hr))
        {
            delete pWaiter;
            pWaiter = nullptr;
        }
        else
        {
            *pbReplyPending = TRUE;
        }
    }
    else 
    {
        // - This routine is called when a ReadConsole or ReadFile request is about to be completed.
        // - It sets the number of bytes written as the information to be written with the completion status and,
        //   if CTRL+Z processing is enabled and a CTRL+Z is detected, switches the number of bytes read to zero.
        if (a->ProcessControlZ != FALSE &&
            a->NumBytes > 0 &&
            m->State.OutputBuffer != nullptr &&
            *(PUCHAR)m->State.OutputBuffer == 0x1a)
        {
            a->NumBytes = 0;
        }

        m->SetReplyInformation(a->NumBytes);
    }

    return hr;
}

HRESULT ApiDispatchers::ServerWriteConsole(_Inout_ CONSOLE_API_MSG * const m, _Inout_ BOOL* const pbReplyPending)
{
    *pbReplyPending = FALSE;

    CONSOLE_WRITECONSOLE_MSG* const a = &m->u.consoleMsgL1.WriteConsole;
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsole, a->Unicode);

    // Make sure we have a valid screen buffer.
    ConsoleHandleData* HandleData = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, HandleData);
    SCREEN_INFORMATION* pScreenInfo;
    RETURN_IF_FAILED(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));

    // Get input parameter buffer
    PVOID pvBuffer;
    ULONG cbBufferSize;
    auto tracing = wil::ScopeExit([&]()
    {
        Tracing::s_TraceApi(pvBuffer, a);
    });
    RETURN_IF_FAILED(m->GetInputBuffer(&pvBuffer, &cbBufferSize));

    IWaitRoutine* pWaiter = nullptr;
    size_t cbRead;

    // We have to hold onto the HR from the call and return it.
    // We can't return some other error after the actual API call.
    // This is because the write console function is allowed to write part of the string and then return an error.
    // It then must report back how far it got before it failed.
    HRESULT hr;
    if (a->Unicode)
    {
        const wchar_t* const pwsInputBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchInputBuffer = cbBufferSize / sizeof(wchar_t);
        size_t cchInputRead;

        hr = m->_pApiRoutines->WriteConsoleWImpl(pScreenInfo,
                                                 pwsInputBuffer,
                                                 cchInputBuffer,
                                                 &cchInputRead,
                                                 &pWaiter);

        // We must set the reply length in bytes. Convert back from characters.
        LOG_IF_FAILED(SizeTMult(cchInputRead, sizeof(wchar_t), &cbRead));
    }
    else
    {
        const char* const psInputBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchInputBuffer = cbBufferSize;
        size_t cchInputRead;

        hr = m->_pApiRoutines->WriteConsoleAImpl(pScreenInfo,
                                                 psInputBuffer,
                                                 cchInputBuffer,
                                                 &cchInputRead,
                                                 &pWaiter);

        // Reply length is already in bytes (chars), don't need to convert.
        cbRead = cchInputRead;
    }

    // We must return the byte length of the read data in the message.
    LOG_IF_FAILED(SizeTToULong(cbRead, &a->NumBytes));

    if (nullptr != pWaiter)
    {
        // If we received a waiter, we need to queue the wait and not reply.
        hr = ConsoleWaitQueue::s_CreateWait(m, pWaiter);
        if (FAILED(hr))
        {
            delete pWaiter;
            pWaiter = nullptr;
        }
        else
        {
            *pbReplyPending = TRUE;
        }
    }
    else
    {
        // If no waiter, fill the response data and return.
        m->SetReplyInformation(a->NumBytes);
    }

    return hr;
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

    InputBuffer* pObj;
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

    auto tracing = wil::ScopeExit([&]()
    {
        Tracing::s_TraceApi(a);
    });

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
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

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

    auto tracing = wil::ScopeExit([&]()
    {
        Tracing::s_TraceApi(a);
    });

    ConsoleHandleData* const pObjectHandle = m->GetObjectHandle();
    RETURN_HR_IF_NULL(E_HANDLE, pObjectHandle);

    SCREEN_INFORMATION* pObj;
    RETURN_IF_FAILED(pObjectHandle->GetScreenBuffer(GENERIC_WRITE, &pObj));
    RETURN_HR(m->_pApiRoutines->SetConsoleTextAttributeImpl(pObj, a->Attributes));
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

    HRESULT hr = S_OK;
    if (a->Unicode)
    {
        wchar_t* const pwsBuffer = reinterpret_cast<wchar_t*>(pvBuffer);
        size_t const cchBuffer = cbBuffer / sizeof(wchar_t);
        size_t cchWritten;
        size_t cchNeeded;
        if (a->Original)
        {
            hr = m->_pApiRoutines->GetConsoleOriginalTitleWImpl(pwsBuffer,
                                                                cchBuffer,
                                                                &cchWritten,
                                                                &cchNeeded);
        }
        else
        {
            hr = m->_pApiRoutines->GetConsoleTitleWImpl(pwsBuffer,
                                                        cchBuffer,
                                                        &cchWritten,
                                                        &cchNeeded);
        }

        // We must return the needed length of the title string in the TitleLength.
        LOG_IF_FAILED(SizeTToULong(cchNeeded, &a->TitleLength));

        // We must return the actually written length of the title string in the reply.
        m->SetReplyInformation(cchWritten * sizeof(wchar_t));
    }
    else
    {
        char* const psBuffer = reinterpret_cast<char*>(pvBuffer);
        size_t const cchBuffer = cbBuffer;
        size_t cchWritten;
        size_t cchNeeded;
        if (a->Original)
        {
            hr = m->_pApiRoutines->GetConsoleOriginalTitleAImpl(psBuffer,
                                                                cchBuffer,
                                                                &cchWritten,
                                                                &cchNeeded);
        }
        else
        {
            hr = m->_pApiRoutines->GetConsoleTitleAImpl(psBuffer,
                                                        cchBuffer,
                                                        &cchWritten,
                                                        &cchNeeded);
        }

        // We must return the needed length of the title string in the TitleLength.
        LOG_IF_FAILED(SizeTToULong(cchNeeded, &a->TitleLength));

        // We must return the actually written length of the title string in the reply.
        m->SetReplyInformation(cchWritten * sizeof(char));
    }

    return hr;
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
