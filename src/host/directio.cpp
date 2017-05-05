/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "directio.h"

#include "_output.h"
#include "output.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "readDataDirect.hpp"

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

class CONSOLE_INFORMATION;

#define UNICODE_DBCS_PADDING 0xffff

ULONG TranslateInputToUnicode(_Inout_ PINPUT_RECORD InputRecords, _In_ ULONG NumRecords, _Inout_ PINPUT_RECORD DBCSLeadByte)
{
    ULONG i, j;

    DBGCHARS(("TranslateInputToUnicode\n"));

    INPUT_RECORD AsciiDbcs[2];
    CHAR Strings[2];
    if (DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar)
    {
        AsciiDbcs[0] = *DBCSLeadByte;
        Strings[0] = DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar;
    }
    else
    {
        ZeroMemory(AsciiDbcs, sizeof(AsciiDbcs));
    }
    for (i = j = 0; i < NumRecords; i++)
    {
        if (InputRecords[i].EventType == KEY_EVENT)
        {
            if (AsciiDbcs[0].Event.KeyEvent.uChar.AsciiChar)
            {
                AsciiDbcs[1] = InputRecords[i];
                Strings[1] = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;

                WCHAR UnicodeDbcs[2];
                ULONG NumBytes = sizeof(Strings);
                NumBytes = ConvertInputToUnicode(ServiceLocator::LocateGlobals()->getConsoleInformation()->CP,
                                                 &Strings[0],
                                                 NumBytes,
                                                 &UnicodeDbcs[0],
                                                 NumBytes);

                PWCHAR Uni = &UnicodeDbcs[0];
                while (NumBytes--)
                {
                    InputRecords[j] = AsciiDbcs[0];
                    InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = *Uni++;
                    j++;
                }
                ZeroMemory(AsciiDbcs, sizeof(AsciiDbcs));
                if (DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar)
                    ZeroMemory(DBCSLeadByte, sizeof(INPUT_RECORD));
            }
            else if (IsDBCSLeadByteConsole(InputRecords[i].Event.KeyEvent.uChar.AsciiChar, &ServiceLocator::LocateGlobals()->getConsoleInformation()->CPInfo))
            {
                if (i < NumRecords - 1)
                {
                    AsciiDbcs[0] = InputRecords[i];
                    Strings[0] = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;
                }
                else
                {
                    *DBCSLeadByte = InputRecords[i];
                    break;
                }
            }
            else
            {
                InputRecords[j] = InputRecords[i];
                CHAR const c = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;
                ConvertInputToUnicode(ServiceLocator::LocateGlobals()->getConsoleInformation()->CP,
                                      &c,
                                      1,
                                      &InputRecords[j].Event.KeyEvent.uChar.UnicodeChar,
                                      1);
                j++;
            }
        }
        else
        {
            InputRecords[j++] = InputRecords[i];
        }
    }
    return j;
}

// Routine Description:
// - This routine reads or peeks input events.  In both cases, the events
//   are copied to the user's buffer.  In the read case they are removed
//   from the input buffer and in the peek case they are not.
// Arguments:
// - pInputBuffer - The input buffer to take records from to return to the client
// - pRecords - The user buffer given to us to fill with records
// - pcRecords - On in, the size of the buffer (count of INPUT_RECORD).
//             - On out, the number of INPUT_RECORDs used.
// - fIsUnicode - Whether to operate on Unicode characters or convert on the current Input Codepage.
// - fIsPeek - If this is a peek operation (a.k.a. do not remove characters from the input buffer while copying to client buffer.)
// - ppWaiter - If we have to wait (not enough data to fill client buffer), this contains context that will allow the server to restore this call later.
// Return Value:
// - STATUS_SUCCESS - If data was found and ready for return to the client.
// - CONSOLE_STATUS_WAIT - If we didn't have enough data or needed to block, this will be returned along with context in *ppWaiter.
// - Or an out of memory/math/string error message in NTSTATUS format.
NTSTATUS DoGetConsoleInput(_In_ InputBuffer* const pInputBuffer,
                           _Out_writes_(*pcRecords) INPUT_RECORD* const pRecords,
                           _Inout_ ULONG* const pcRecords,
                           _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                           _In_ bool const fIsUnicode,
                           _In_ bool const fIsPeek,
                           _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    *ppWaiter = nullptr;

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    bool fAddDbcsLead = false;
    DIRECT_READ_DATA DirectReadData(pInputBuffer,
                                    pInputReadHandleData,
                                    pRecords,
                                    *pcRecords,
                                    fIsPeek);
    DWORD nBytesUnicode = *pcRecords;
    *pcRecords = 0;

    INPUT_RECORD* Buffer = pRecords;

    if (!fIsUnicode)
    {
        // MSFT: 6429490 removed the usage of the ReadConInpNumBytesUnicode variable and the early return when the value was 1.
        // This resolved an issue with returning DBCS codepages to getc() and ReadConsoleInput.
        // There is another copy of the same pattern above in the Wait routine, but that usage scenario isn't 100% clear and
        // doesn't affect the issue, so it's left alone for now.

        // if there is a saved partial byte for a dbcs char,
        // move it to the buffer.
        if (pInputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
        {
            // Saved DBCS Trailing byte
            *Buffer = pInputBuffer->ReadConInpDbcsLeadByte;
            if (!fIsPeek)
            {
                ZeroMemory(&pInputBuffer->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
            }

            nBytesUnicode--;
            Buffer++;
            fAddDbcsLead = true;
        }
    }

    NTSTATUS Status = pInputBuffer->ReadInputBuffer(Buffer,
                                                    &nBytesUnicode,
                                                    fIsPeek,
                                                    true,
                                                    fIsUnicode);

    if (CONSOLE_STATUS_WAIT == Status)
    {
        // If we're told to wait until later, move all of our context from stack into a heap context and send it back up to the server.
        *ppWaiter = new DIRECT_READ_DATA(std::move(DirectReadData));
        if (*ppWaiter == nullptr)
        {
            Status = STATUS_NO_MEMORY;
        }
    }
    else if (!fIsUnicode)
    {
        *pcRecords = TranslateInputToOem(Buffer,
                                         fAddDbcsLead ? *pcRecords - 1 : *pcRecords,
                                         nBytesUnicode,
                                         fIsPeek ? nullptr : &pInputBuffer->ReadConInpDbcsLeadByte);
        if (fAddDbcsLead)
        {
            (*pcRecords)++;
            Buffer--;
        }
    }

    return Status;
}

// Routine Description:
// - Retrieves input records from the given input object and returns them to the client.
// - The peek version will NOT remove records when it copies them out.
// - The A version will convert to W using the console's current Input codepage (see SetConsoleCP)
// Arguments:
// - pInputBuffer - The input buffer to take records from to return to the client
// - pRecords - The user buffer given to us to fill with records
// - cRecords - The size of the buffer (count of INPUT_RECORD).
// - pcRecordsWritten - The number of INPUT_RECORDs used in the client's buffer.
// - pInputReadHandleData - A structure that will help us maintain some input context across various calls on the same input handle
//                        - Primarily used to restore the "other piece" of partially returned strings (because client buffer wasn't big enough) on the next call.
// - ppWaiter - If we have to wait (not enough data to fill client buffer), this contains context that will allow the server to restore this call later.
HRESULT ApiRoutines::PeekConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(cRecords, *pcRecordsWritten) INPUT_RECORD* const pRecords,
                                           _In_ size_t const cRecords,
                                           _Out_ size_t* const pcRecordsWritten,
                                           _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                           _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    ULONG ulRecords;
    RETURN_IF_FAILED(SizeTToULong(cRecords, &ulRecords));

    HRESULT const hr = DoGetConsoleInput(pInContext, pRecords, &ulRecords, pInputReadHandleData, false, true, ppWaiter);

    *pcRecordsWritten = ulRecords;

    return hr;
}

// Routine Description:
// - Retrieves input records from the given input object and returns them to the client.
// - The peek version will NOT remove records when it copies them out.
// - The W version accepts UCS-2 formatted characters (wide characters)
// Arguments:
// - pInputBuffer - The input buffer to take records from to return to the client
// - pRecords - The user buffer given to us to fill with records
// - cRecords - The size of the buffer (count of INPUT_RECORD).
// - pcRecordsWritten - The number of INPUT_RECORDs used in the client's buffer.
// - pInputReadHandleData - A structure that will help us maintain some input context across various calls on the same input handle
//                        - Primarily used to restore the "other piece" of partially returned strings (because client buffer wasn't big enough) on the next call.
// - ppWaiter - If we have to wait (not enough data to fill client buffer), this contains context that will allow the server to restore this call later.
HRESULT ApiRoutines::PeekConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(cRecords, *pcRecordsWritten) INPUT_RECORD* const pRecords,
                                           _In_ size_t const cRecords,
                                           _Out_ size_t* const pcRecordsWritten,
                                           _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                           _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    ULONG ulRecords;
    RETURN_IF_FAILED(SizeTToULong(cRecords, &ulRecords));

    HRESULT const hr = DoGetConsoleInput(pInContext, pRecords, &ulRecords, pInputReadHandleData, true, true, ppWaiter);

    *pcRecordsWritten = ulRecords;

    return hr;
}

// Routine Description:
// - Retrieves input records from the given input object and returns them to the client.
// - The read version WILL remove records when it copies them out.
// - The A version will convert to W using the console's current Input codepage (see SetConsoleCP)
// Arguments:
// - pInputBuffer - The input buffer to take records from to return to the client
// - pRecords - The user buffer given to us to fill with records
// - cRecords - The size of the buffer (count of INPUT_RECORD).
// - pcRecordsWritten - The number of INPUT_RECORDs used in the client's buffer.
// - pInputReadHandleData - A structure that will help us maintain some input context across various calls on the same input handle
//                        - Primarily used to restore the "other piece" of partially returned strings (because client buffer wasn't big enough) on the next call.
// - ppWaiter - If we have to wait (not enough data to fill client buffer), this contains context that will allow the server to restore this call later.
HRESULT ApiRoutines::ReadConsoleInputAImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(cRecords, *pcRecordsWritten) INPUT_RECORD* const pRecords,
                                           _In_ size_t const cRecords,
                                           _Out_ size_t* const pcRecordsWritten,
                                           _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                           _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    ULONG ulRecords;
    RETURN_IF_FAILED(SizeTToULong(cRecords, &ulRecords));

    HRESULT const hr = DoGetConsoleInput(pInContext, pRecords, &ulRecords, pInputReadHandleData, false, false, ppWaiter);

    *pcRecordsWritten = ulRecords;

    return hr;
}

// Routine Description:
// - Retrieves input records from the given input object and returns them to the client.
// - The read version WILL remove records when it copies them out.
// - The W version accepts UCS-2 formatted characters (wide characters)
// Arguments:
// - pInputBuffer - The input buffer to take records from to return to the client
// - pRecords - The user buffer given to us to fill with records
// - cRecords - The size of the buffer (count of INPUT_RECORD).
// - pcRecordsWritten - The number of INPUT_RECORDs used in the client's buffer.
// - pInputReadHandleData - A structure that will help us maintain some input context across various calls on the same input handle
//                        - Primarily used to restore the "other piece" of partially returned strings (because client buffer wasn't big enough) on the next call.
// - ppWaiter - If we have to wait (not enough data to fill client buffer), this contains context that will allow the server to restore this call later.
HRESULT ApiRoutines::ReadConsoleInputWImpl(_In_ IConsoleInputObject* const pInContext,
                                           _Out_writes_to_(cRecords, *pcRecordsWritten) INPUT_RECORD* const pRecords,
                                           _In_ size_t const cRecords,
                                           _Out_ size_t* const pcRecordsWritten,
                                           _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                           _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    ULONG ulRecords;
    RETURN_IF_FAILED(SizeTToULong(cRecords, &ulRecords));

    HRESULT const hr = DoGetConsoleInput(pInContext, pRecords, &ulRecords, pInputReadHandleData, true, false, ppWaiter);

    *pcRecordsWritten = ulRecords;

    return hr;
}


NTSTATUS SrvWriteConsoleInput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_WRITECONSOLEINPUT_MSG const a = &m->u.consoleMsgL2.WriteConsoleInput;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleInput, a->Unicode);

    PINPUT_RECORD Buffer;
    ULONG Size;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer((PVOID*)&Buffer, &Size));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->NumRecords = Size / sizeof(INPUT_RECORD);

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* const HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        a->NumRecords = 0;
        Status = STATUS_INVALID_HANDLE;
    }

    if (NT_SUCCESS(Status))
    {
        InputBuffer* pInputBuffer;
        if (FAILED(HandleData->GetInputBuffer(GENERIC_WRITE, &pInputBuffer)))
        {
            a->NumRecords = 0;
            Status = STATUS_INVALID_HANDLE;
        }
        else
        {
            Status = DoSrvWriteConsoleInput(pInputBuffer, a, Buffer);
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvWriteConsoleInput(_In_ InputBuffer* pInputBuffer, _Inout_ CONSOLE_WRITECONSOLEINPUT_MSG* pMsg, _In_ INPUT_RECORD* const rgInputRecords)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pMsg->Unicode)
    {
        pMsg->NumRecords = TranslateInputToUnicode(rgInputRecords, pMsg->NumRecords, &pInputBuffer->WriteConInpDbcsLeadByte[0]);
    }

    if (pMsg->Append)
    {
        pMsg->NumRecords = pInputBuffer->WriteInputBuffer(rgInputRecords, pMsg->NumRecords);
    }
    else
    {
        Status = pInputBuffer->PrependInputBuffer(rgInputRecords, &pMsg->NumRecords);
    }

    return Status;
}

// Routine Description:
// - This is used when the app reads oem from the output buffer the app wants
//   real OEM characters. We have real Unicode or UnicodeOem.
NTSTATUS TranslateOutputToOem(_Inout_ PCHAR_INFO OutputBuffer, _In_ COORD Size)
{
    ULONG NumBytes;
    ULONG uX, uY;
    if (FAILED(ShortToULong(Size.X, &uX)) ||
        FAILED(ShortToULong(Size.Y, &uY)) ||
        FAILED(ULongMult(uX, uY, &NumBytes)) ||
        FAILED(ULongMult(NumBytes, 2 * sizeof(CHAR_INFO), &NumBytes)))
    {
        return STATUS_NO_MEMORY;
    }
    PCHAR_INFO TmpBuffer = (PCHAR_INFO) new BYTE[NumBytes];
    PCHAR_INFO const SaveBuffer = TmpBuffer;
    if (TmpBuffer == nullptr)
    {
        return STATUS_NO_MEMORY;
    }

    UINT const Codepage = ServiceLocator::LocateGlobals()->getConsoleInformation()->OutputCP;

    memmove(TmpBuffer, OutputBuffer, Size.X * Size.Y * sizeof(CHAR_INFO));

#pragma prefast(push)
#pragma prefast(disable:26019, "The buffer is the correct size for any given DBCS characters. No additional validation needed.")
    for (int i = 0; i < Size.Y; i++)
    {
        for (int j = 0; j < Size.X; j++)
        {
            if (IsFlagSet(TmpBuffer->Attributes, COMMON_LVB_LEADING_BYTE))
            {
                if (j < Size.X - 1)
                {   // -1 is safe DBCS in buffer
                    j++;
                    CHAR AsciiDbcs[2] = { 0 };
                    NumBytes = sizeof(AsciiDbcs);
                    NumBytes = ConvertToOem(Codepage, &TmpBuffer->Char.UnicodeChar, 1, &AsciiDbcs[0], NumBytes);
                    OutputBuffer->Char.AsciiChar = AsciiDbcs[0];
                    OutputBuffer->Attributes = TmpBuffer->Attributes;
                    OutputBuffer++;
                    TmpBuffer++;
                    OutputBuffer->Char.AsciiChar = AsciiDbcs[1];
                    OutputBuffer->Attributes = TmpBuffer->Attributes;
                    OutputBuffer++;
                    TmpBuffer++;
                }
                else
                {
                    OutputBuffer->Char.AsciiChar = ' ';
                    OutputBuffer->Attributes = TmpBuffer->Attributes;
                    ClearAllFlags(OutputBuffer->Attributes, COMMON_LVB_SBCSDBCS);
                    OutputBuffer++;
                    TmpBuffer++;
                }
            }
            else if (AreAllFlagsClear(TmpBuffer->Attributes, COMMON_LVB_SBCSDBCS))
            {
                ConvertToOem(Codepage, &TmpBuffer->Char.UnicodeChar, 1, &OutputBuffer->Char.AsciiChar, 1);
                OutputBuffer->Attributes = TmpBuffer->Attributes;
                OutputBuffer++;
                TmpBuffer++;
            }
        }
    }
#pragma prefast(pop)

    delete[] SaveBuffer;
    return STATUS_SUCCESS;
}

// Routine Description:
// - This is used when the app writes oem to the output buffer we want
//   UnicodeOem or Unicode in the buffer, depending on font
NTSTATUS TranslateOutputToUnicode(_Inout_ PCHAR_INFO OutputBuffer, _In_ COORD Size)
{
    UINT const Codepage = ServiceLocator::LocateGlobals()->getConsoleInformation()->OutputCP;

    for (int i = 0; i < Size.Y; i++)
    {
        for (int j = 0; j < Size.X; j++)
        {
            ClearAllFlags(OutputBuffer->Attributes, COMMON_LVB_SBCSDBCS);
            if (IsDBCSLeadByteConsole(OutputBuffer->Char.AsciiChar, &ServiceLocator::LocateGlobals()->getConsoleInformation()->OutputCPInfo))
            {
                if (j < Size.X - 1)
                {   // -1 is safe DBCS in buffer
                    j++;
                    CHAR AsciiDbcs[2];
                    AsciiDbcs[0] = OutputBuffer->Char.AsciiChar;
                    AsciiDbcs[1] = (OutputBuffer + 1)->Char.AsciiChar;

                    WCHAR UnicodeDbcs[2];
                    ConvertOutputToUnicode(Codepage, &AsciiDbcs[0], 2, &UnicodeDbcs[0], 2);

                    OutputBuffer->Char.UnicodeChar = UnicodeDbcs[0];
                    SetFlag(OutputBuffer->Attributes, COMMON_LVB_LEADING_BYTE);
                    OutputBuffer++;
                    OutputBuffer->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                    ClearAllFlags(OutputBuffer->Attributes, COMMON_LVB_SBCSDBCS);
                    SetFlag(OutputBuffer->Attributes, COMMON_LVB_TRAILING_BYTE);
                    OutputBuffer++;
                }
                else
                {
                    OutputBuffer->Char.UnicodeChar = UNICODE_SPACE;
                    OutputBuffer++;
                }
            }
            else
            {
                CHAR c = OutputBuffer->Char.AsciiChar;

                ConvertOutputToUnicode(Codepage, &c, 1, &OutputBuffer->Char.UnicodeChar, 1);
                OutputBuffer++;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS TranslateOutputToPaddingUnicode(_Inout_ PCHAR_INFO OutputBuffer, _In_ COORD Size, _Inout_ PCHAR_INFO OutputBufferR)
{
    DBGCHARS(("TranslateOutputToPaddingUnicode\n"));

    for (SHORT i = 0; i < Size.Y; i++)
    {
        for (SHORT j = 0; j < Size.X; j++)
        {
            WCHAR wch = OutputBuffer->Char.UnicodeChar;

            if (OutputBufferR)
            {
                OutputBufferR->Attributes = OutputBuffer->Attributes;
                ClearAllFlags(OutputBufferR->Attributes, COMMON_LVB_SBCSDBCS);
                if (IsCharFullWidth(OutputBuffer->Char.UnicodeChar))
                {
                    if (j == Size.X - 1)
                    {
                        OutputBufferR->Char.UnicodeChar = UNICODE_SPACE;
                    }
                    else
                    {
                        OutputBufferR->Char.UnicodeChar = OutputBuffer->Char.UnicodeChar;
                        SetFlag(OutputBufferR->Attributes, COMMON_LVB_LEADING_BYTE);
                        OutputBufferR++;
                        OutputBufferR->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                        OutputBufferR->Attributes = OutputBuffer->Attributes;
                        ClearAllFlags(OutputBufferR->Attributes, COMMON_LVB_SBCSDBCS);
                        SetFlag(OutputBufferR->Attributes, COMMON_LVB_TRAILING_BYTE);
                    }
                }
                else
                {
                    OutputBufferR->Char.UnicodeChar = OutputBuffer->Char.UnicodeChar;
                }
                OutputBufferR++;
            }

            if (IsCharFullWidth(wch))
            {
                if (j != Size.X - 1)
                {
                    j++;
                }
            }
            OutputBuffer++;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS SrvReadConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_READCONSOLEOUTPUT_MSG const a = &m->u.consoleMsgL2.ReadConsoleOutput;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleOutput, a->Unicode);

    PCHAR_INFO Buffer;
    ULONG Size;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer((PVOID*)&Buffer, &Size));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        Status = STATUS_INVALID_HANDLE;
    }

    SCREEN_INFORMATION* pScreenInfo = nullptr;
    if (NT_SUCCESS(Status))
    {
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    }

    if (!NT_SUCCESS(Status))
    {
        // a region of zero size is indicated by the right and bottom coordinates being less than the left and top.
        a->CharRegion.Right = (USHORT)(a->CharRegion.Left - 1);
        a->CharRegion.Bottom = (USHORT)(a->CharRegion.Top - 1);
    }
    else
    {
        COORD BufferSize;
        BufferSize.X = (SHORT)(a->CharRegion.Right - a->CharRegion.Left + 1);
        BufferSize.Y = (SHORT)(a->CharRegion.Bottom - a->CharRegion.Top + 1);

        if (((BufferSize.X > 0) && (BufferSize.Y > 0)) &&
            ((BufferSize.X * BufferSize.Y > ULONG_MAX / sizeof(CHAR_INFO)) || (Size < BufferSize.X * BufferSize.Y * sizeof(CHAR_INFO))))
        {
            UnlockConsole();
            return STATUS_INVALID_PARAMETER;
        }

        SCREEN_INFORMATION* const psi = pScreenInfo->GetActiveBuffer();

        Status = ReadScreenBuffer(psi, Buffer, &a->CharRegion);
        if (!a->Unicode)
        {
            TranslateOutputToOem(Buffer, BufferSize);
        }
        else if (!psi->TextInfo->GetCurrentFont()->IsTrueTypeFont())
        {
            // For compatibility reasons, we must maintain the behavior that munges the data if we are writing while a raster font is enabled.
            // This can be removed when raster font support is removed.
            RemoveDbcsMarkCell(Buffer, Buffer, BufferSize.X * BufferSize.Y);
        }

        if (NT_SUCCESS(Status))
        {
            m->SetReplyInformation(CalcWindowSizeX(&a->CharRegion) * CalcWindowSizeY(&a->CharRegion) * sizeof(CHAR_INFO));
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvWriteConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_WRITECONSOLEOUTPUT_MSG const a = &m->u.consoleMsgL2.WriteConsoleOutput;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleOutput, a->Unicode);

    PCHAR_INFO Buffer;
    ULONG Size;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer((PVOID*)&Buffer, &Size));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        Status = STATUS_INVALID_HANDLE;
    }

    SCREEN_INFORMATION* pScreenInfo = nullptr;
    if (NT_SUCCESS(Status))
    {
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    }
    if (!NT_SUCCESS(Status))
    {
        // A region of zero size is indicated by the right and bottom coordinates being less than the left and top.
        a->CharRegion.Right = (USHORT)(a->CharRegion.Left - 1);
        a->CharRegion.Bottom = (USHORT)(a->CharRegion.Top - 1);
    }
    else
    {
        COORD BufferSize;
        ULONG NumBytes;

        BufferSize.X = (SHORT)(a->CharRegion.Right - a->CharRegion.Left + 1);
        BufferSize.Y = (SHORT)(a->CharRegion.Bottom - a->CharRegion.Top + 1);
        if (FAILED(ULongMult(BufferSize.X, BufferSize.Y, &NumBytes)) ||
            FAILED(ULongMult(NumBytes, sizeof(*Buffer), &NumBytes)) ||
            (Size < NumBytes))
        {

            UnlockConsole();
            return STATUS_INVALID_PARAMETER;
        }

        PSCREEN_INFORMATION const ScreenBufferInformation = pScreenInfo->GetActiveBuffer();

        if (!a->Unicode)
        {
            TranslateOutputToUnicode(Buffer, BufferSize);
            Status = WriteScreenBuffer(ScreenBufferInformation, Buffer, &a->CharRegion);
        }
        else if (!ScreenBufferInformation->TextInfo->GetCurrentFont()->IsTrueTypeFont())
        {
            // For compatibility reasons, we must maintain the behavior that munges the data if we are writing while a raster font is enabled.
            // This can be removed when raster font support is removed.

            PCHAR_INFO TransBuffer = nullptr;
            int cBufferResult;
            if (SUCCEEDED(Int32Mult(BufferSize.X, BufferSize.Y, &cBufferResult)))
            {
                if (SUCCEEDED(Int32Mult(cBufferResult, 2 * sizeof(CHAR_INFO), &cBufferResult)))
                {
                    TransBuffer = (PCHAR_INFO)new BYTE[cBufferResult];
                }
            }

            if (TransBuffer == nullptr)
            {
                UnlockConsole();
                return STATUS_NO_MEMORY;
            }

            TranslateOutputToPaddingUnicode(Buffer, BufferSize, &TransBuffer[0]);
            Status = WriteScreenBuffer(ScreenBufferInformation, &TransBuffer[0], &a->CharRegion);
            delete[] TransBuffer;
        }
        else
        {
            Status = WriteScreenBuffer(ScreenBufferInformation, Buffer, &a->CharRegion);
        }

        if (NT_SUCCESS(Status))
        {
            // cause screen to be updated
            WriteToScreen(ScreenBufferInformation, a->CharRegion);
        }
    }

    UnlockConsole();
    return Status;
}


NTSTATUS SrvReadConsoleOutputString(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_READCONSOLEOUTPUTSTRING_MSG const a = &m->u.consoleMsgL2.ReadConsoleOutputString;

    switch (a->StringType)
    {
    case CONSOLE_ATTRIBUTE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleOutputAttribute);
        break;
    case CONSOLE_ASCII:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleOutputCharacter, false);
        break;
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleOutputCharacter, true);
        break;
    }

    PVOID Buffer;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetOutputBuffer(&Buffer, &a->NumRecords));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        Status = ERROR_INVALID_HANDLE;
    }

    SCREEN_INFORMATION* pScreenInfo = nullptr;
    if (NT_SUCCESS(Status))
    {
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_READ, &pScreenInfo));
    }
    if (!NT_SUCCESS(Status))
    {
        // a region of zero size is indicated by the right and bottom coordinates being less than the left and top.
        a->NumRecords = 0;
    }
    else
    {
        ULONG nSize;

        if (a->StringType == CONSOLE_ASCII)
        {
            nSize = sizeof(CHAR);
        }
        else
        {
            nSize = sizeof(WORD);
        }

        a->NumRecords /= nSize;

        Status = ReadOutputString(pScreenInfo->GetActiveBuffer(), Buffer, a->ReadCoord, a->StringType, &a->NumRecords);

        if (NT_SUCCESS(Status))
        {
            m->SetReplyInformation(a->NumRecords * nSize);
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvWriteConsoleOutputString(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG const a = &m->u.consoleMsgL2.WriteConsoleOutputString;

    auto tracing = wil::ScopeExit([&]()
    {
        Tracing::s_TraceApi(a);
    });

    switch (a->StringType)
    {
    case CONSOLE_ATTRIBUTE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleOutputAttribute);
        break;
    case CONSOLE_ASCII:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleOutputCharacter, false);
        break;
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleOutputCharacter, true);
        break;
    }

    PVOID Buffer;
    ULONG BufferSize;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(m->GetInputBuffer(&Buffer, &BufferSize));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        Status = ERROR_INVALID_HANDLE;
    }

    SCREEN_INFORMATION* pScreenInfo = nullptr;
    if (NT_SUCCESS(Status))
    {
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    }
    if (!NT_SUCCESS(Status))
    {
        a->NumRecords = 0;
    }
    else
    {
        if (a->WriteCoord.X < 0 || a->WriteCoord.Y < 0)
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            if (a->StringType == CONSOLE_ASCII)
            {
                a->NumRecords = BufferSize;
            }
            else
            {
                a->NumRecords = BufferSize / sizeof(WCHAR);
            }

            Status = WriteOutputString(pScreenInfo->GetActiveBuffer(), Buffer, a->WriteCoord, a->StringType, &a->NumRecords, nullptr);
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvFillConsoleOutput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_FILLCONSOLEOUTPUT_MSG const a = &m->u.consoleMsgL2.FillConsoleOutput;

    switch (a->ElementType)
    {
    case CONSOLE_ATTRIBUTE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FillConsoleOutputAttribute);
        break;
    case CONSOLE_ASCII:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FillConsoleOutputCharacter, false);
        break;
    case CONSOLE_REAL_UNICODE:
    case CONSOLE_FALSE_UNICODE:
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::FillConsoleOutputCharacter, true);
        break;
    }

    CONSOLE_INFORMATION *Console;
    NTSTATUS Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ConsoleHandleData* HandleData = m->GetObjectHandle();
    if (HandleData == nullptr)
    {
        Status = ERROR_INVALID_HANDLE;
    }

    SCREEN_INFORMATION* pScreenInfo = nullptr;
    if (NT_SUCCESS(Status))
    {
        Status = NTSTATUS_FROM_HRESULT(HandleData->GetScreenBuffer(GENERIC_WRITE, &pScreenInfo));
    }
    if (!NT_SUCCESS(Status))
    {
        a->Length = 0;
    }
    else
    {
        Status = DoSrvFillConsoleOutput(pScreenInfo->GetActiveBuffer(), a);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvFillConsoleOutput(_In_ SCREEN_INFORMATION* pScreenInfo, _Inout_ CONSOLE_FILLCONSOLEOUTPUT_MSG* pMsg)
{
    return FillOutput(pScreenInfo, pMsg->Element, pMsg->WriteCoord, pMsg->ElementType, &pMsg->Length);
}

// There used to be a text mode and a graphics mode flag.
// Text mode was used for regular applications like CMD.exe.
// Graphics mode was used for bitmap VDM buffers and is no longer supported.
// OEM console font mode used to represent rewriting the entire buffer into codepage 437 so the renderer could handle it with raster fonts.
//  But now the entire buffer is always kept in Unicode and the renderer asks for translation when/if necessary for raster fonts only.
// We keep these definitions here so the API can enforce that the only one we support any longer is the original text mode.
// See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms682122(v=vs.85).aspx
#define CONSOLE_TEXTMODE_BUFFER 1
//#define CONSOLE_GRAPHICS_BUFFER 2
//#define CONSOLE_OEMFONT_DISPLAY 4

NTSTATUS ConsoleCreateScreenBuffer(_Out_ ConsoleHandleData** ppHandle,
                                   _In_ PCONSOLE_API_MSG /*Message*/,
                                   _In_ PCD_CREATE_OBJECT_INFORMATION Information,
                                   _In_ PCONSOLE_CREATESCREENBUFFER_MSG a)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::CreateConsoleScreenBuffer);

    // If any buffer type except the one we support is set, it's invalid.
    if (IsAnyFlagSet(a->Flags, ~CONSOLE_TEXTMODE_BUFFER))
    {
        ASSERT(false); // We no longer support anything other than a textmode buffer
        return STATUS_INVALID_PARAMETER;
    }

    ConsoleHandleData::HandleType const HandleType = ConsoleHandleData::HandleType::Output;

    const SCREEN_INFORMATION* const psiExisting = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;

    // Create new screen buffer.
    CHAR_INFO Fill;
    Fill.Char.UnicodeChar = UNICODE_SPACE;
    Fill.Attributes = psiExisting->GetAttributes().GetLegacyAttributes();

    COORD WindowSize;
    WindowSize.X = (SHORT)psiExisting->GetScreenWindowSizeX();
    WindowSize.Y = (SHORT)psiExisting->GetScreenWindowSizeY();

    const FontInfo* const pfiExistingFont = psiExisting->TextInfo->GetCurrentFont();

    PSCREEN_INFORMATION ScreenInfo = nullptr;
    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(WindowSize,
                                                         pfiExistingFont,
                                                         WindowSize,
                                                         Fill,
                                                         Fill,
                                                         CURSOR_SMALL_SIZE,
                                                         &ScreenInfo);

    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    Status = NTSTATUS_FROM_HRESULT(ScreenInfo->Header.AllocateIoHandle(HandleType,
                                                                       Information->DesiredAccess,
                                                                       Information->ShareMode,
                                                                       ppHandle));

    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    SCREEN_INFORMATION::s_InsertScreenBuffer(ScreenInfo);

Exit:
    if (!NT_SUCCESS(Status))
    {
        delete ScreenInfo;
    }

    return Status;
}
