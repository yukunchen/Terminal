/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "directio.h"

#include "_output.h"
#include "output.h"
#include "cursor.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"

#pragma hdrstop

class CONSOLE_INFORMATION;

typedef struct _DIRECT_READ_DATA
{
    PINPUT_INFORMATION InputInfo;
    CONSOLE_INFORMATION *Console;
    PCONSOLE_PROCESS_HANDLE ProcessData;
    CONSOLE_HANDLE_DATA* HandleIndex;
} DIRECT_READ_DATA, *PDIRECT_READ_DATA;

#define UNICODE_DBCS_PADDING 0xffff

ULONG TranslateInputToOem(_Inout_ PINPUT_RECORD InputRecords,
                          _In_ ULONG NumRecords,    // in : ASCII byte count
                          _In_ ULONG UnicodeLength, // in : Number of events (char count)
                          _Inout_opt_ PINPUT_RECORD DbcsLeadInpRec)
{
    DBGCHARS(("TranslateInputToOem\n"));

    ASSERT(NumRecords >= UnicodeLength);
    __analysis_assume(NumRecords >= UnicodeLength);

    ULONG NumBytes;
    if (FAILED(DWordMult(NumRecords, sizeof(INPUT_RECORD), &NumBytes)))
    {
        return 0;
    }

    PINPUT_RECORD const TmpInpRec = (PINPUT_RECORD) new BYTE[NumBytes];
    if (TmpInpRec == nullptr)
    {
        return 0;
    }

    memmove(TmpInpRec, InputRecords, NumBytes);
    BYTE AsciiDbcs[2];
    AsciiDbcs[1] = 0;
    ULONG i, j;
    for (i = 0, j = 0; i < UnicodeLength; i++, j++)
    {
        if (TmpInpRec[i].EventType == KEY_EVENT)
        {
            if (IsCharFullWidth(TmpInpRec[i].Event.KeyEvent.uChar.UnicodeChar))
            {
                NumBytes = sizeof(AsciiDbcs);
                ConvertToOem(g_ciConsoleInformation.CP, &TmpInpRec[i].Event.KeyEvent.uChar.UnicodeChar, 1, (LPSTR) & AsciiDbcs[0], NumBytes);
                if (IsDBCSLeadByteConsole(AsciiDbcs[0], &g_ciConsoleInformation.CPInfo))
                {
                    if (j < NumRecords - 1)
                    {   // -1 is safe DBCS in buffer
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                        j++;
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
                        AsciiDbcs[1] = 0;
                    }
                    else if (j == NumRecords - 1)
                    {
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                        j++;
                        break;
                    }
                    else
                    {
                        AsciiDbcs[1] = 0;
                        break;
                    }
                }
                else
                {
                    InputRecords[j] = TmpInpRec[i];
                    InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                    InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                    AsciiDbcs[1] = 0;
                }
            }
            else
            {
                InputRecords[j] = TmpInpRec[i];
                ConvertToOem(g_ciConsoleInformation.CP, &InputRecords[j].Event.KeyEvent.uChar.UnicodeChar, 1, &InputRecords[j].Event.KeyEvent.uChar.AsciiChar, 1);
            }
        }
    }
    if (DbcsLeadInpRec)
    {
        if (AsciiDbcs[1])
        {
            ASSERT(i < UnicodeLength);
            __analysis_assume(i < UnicodeLength);

            *DbcsLeadInpRec = TmpInpRec[i];
            DbcsLeadInpRec->Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
        }
        else
        {
            ZeroMemory(DbcsLeadInpRec, sizeof(INPUT_RECORD));
        }
    }
    delete[] TmpInpRec;
    return j;
}

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
                NumBytes = ConvertInputToUnicode(g_ciConsoleInformation.CP, &Strings[0], NumBytes, &UnicodeDbcs[0], NumBytes);

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
            else if (IsDBCSLeadByteConsole(InputRecords[i].Event.KeyEvent.uChar.AsciiChar, &g_ciConsoleInformation.CPInfo))
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
                ConvertInputToUnicode(g_ciConsoleInformation.CP, &c, 1, &InputRecords[j].Event.KeyEvent.uChar.UnicodeChar, 1);
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
// - This routine is called to complete a direct read that blocked in
//   ReadInputBuffer.  The context of the read was saved in the DirectReadData
//   structure.  This routine is called when events have been written to
//   the input buffer.  It is called in the context of the writing thread.
// Arguments:
// - WaitQueue - Pointer to queue containing wait block.
// - WaitReplyMessage - Pointer to reply message to return to dll when read is completed.
// - DirectReadData - Context of read.
// - SatisfyParameter - Flags.
// - ThreadDying - Indicates if the thread (and process) is exiting.
// Return Value:
BOOL DirectReadWaitRoutine(_In_ PLIST_ENTRY /*WaitQueue*/, _In_ PCONSOLE_API_MSG WaitReplyMessage, _In_ PVOID WaitParameter, _In_ PVOID SatisfyParameter, _In_ BOOL ThreadDying)
{
    PCONSOLE_GETCONSOLEINPUT_MSG const a = &WaitReplyMessage->u.consoleMsgL1.GetConsoleInput;

    BOOLEAN RetVal = TRUE;
    PDIRECT_READ_DATA DirectReadData = (PDIRECT_READ_DATA)WaitParameter;
    PCONSOLE_HANDLE_DATA HandleData = DirectReadData->HandleIndex;
    NTSTATUS Status = STATUS_SUCCESS;

    // If ctrl-c or ctrl-break was seen, ignore it.
    if ((ULONG_PTR) SatisfyParameter & (CONSOLE_CTRL_C_SEEN | CONSOLE_CTRL_BREAK_SEEN))
    {
        return FALSE;
    }

    PINPUT_RECORD Buffer = nullptr;
    if (!a->Unicode)
    {
        if (HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
        {
            if (a->NumRecords == 1)
            {
                Buffer = (PINPUT_RECORD) WaitReplyMessage->State.OutputBuffer;
                *Buffer = HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte;
                if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                    ZeroMemory(&HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));

                SetReplyStatus(WaitReplyMessage, STATUS_SUCCESS);
                SetReplyInformation(WaitReplyMessage, sizeof(INPUT_RECORD));
            }
        }
    }

    BOOLEAN fAddDbcsLead = FALSE;
    // this routine should be called by a thread owning the same lock on the same console as we're reading from.
    __try
    {
#ifdef DBG
        HandleData->GetClientInput()->LockReadCount();
        ASSERT(HandleData->GetClientInput()->GetReadCount() > 0);
        HandleData->GetClientInput()->UnlockReadCount();
#endif
        HandleData->GetClientInput()->DecrementReadCount();

        // See if called by CsrDestroyProcess or CsrDestroyThread
        // via ConsoleNotifyWaitBlock. If so, just decrement the ReadCount and return.
        if (ThreadDying)
        {
            Status = STATUS_THREAD_IS_TERMINATING;
            __leave;
        }

        // We must see if we were woken up because the handle is being
        // closed. If so, we decrement the read count. If it goes to
        // zero, we wake up the close thread. Otherwise, we wake up any
        // other thread waiting for data.
        if (HandleData->GetClientInput()->InputHandleFlags & HANDLE_CLOSING)
        {
            Status = STATUS_ALERTED;
            __leave;
        }

        // if we get to here, this routine was called either by the input
        // thread or a write routine.  both of these callers grab the
        // current console lock.

        // this routine should be called by a thread owning the same
        // lock on the same console as we're reading from.

        ASSERT(g_ciConsoleInformation.IsConsoleLocked());

        Buffer = (PINPUT_RECORD) WaitReplyMessage->State.OutputBuffer;

        PDWORD nLength = nullptr;

        g_ciConsoleInformation.ReadConInpNumBytesUnicode = a->NumRecords;
        if (!a->Unicode)
        {
            // ASCII : a->NumRecords is ASCII byte count
            if (HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
            {
                // Saved DBCS Traling byte
                if (g_ciConsoleInformation.ReadConInpNumBytesUnicode != 1)
                {
                    g_ciConsoleInformation.ReadConInpNumBytesUnicode--;
                    Buffer++;
                    fAddDbcsLead = TRUE;
                    nLength = &g_ciConsoleInformation.ReadConInpNumBytesUnicode;
                }
                else
                {
                    ASSERT(g_ciConsoleInformation.ReadConInpNumBytesUnicode == 1);
                }
            }
            else
            {
                nLength = &g_ciConsoleInformation.ReadConInpNumBytesUnicode;
            }
        }
        else
        {
            nLength = &a->NumRecords;
        }

        Status = ReadInputBuffer(DirectReadData->InputInfo,
                                 Buffer,
                                 nLength,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT),
                                 FALSE,
                                 HandleData,
                                 WaitReplyMessage,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
#pragma prefast(suppress: 28132, "sizeof(DirectReadData) is used as an indicator of the recursion depth and needs to be the size of the pointer in this case. This is clearer as-is than measuring the pointer type.")
                                 sizeof(DirectReadData),
                                 TRUE,
                                 a->Unicode);
        if (Status == CONSOLE_STATUS_WAIT)
        {
            RetVal = FALSE;
        }
    }
    __finally
    {
        // If the read was completed (status != wait), free the direct read data.
        if (Status != CONSOLE_STATUS_WAIT)
        {
            if (Status == STATUS_SUCCESS && !a->Unicode)
            {
                if (fAddDbcsLead && HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
                {
                    a->NumRecords--;
                }

                a->NumRecords = TranslateInputToOem(Buffer,
                                                    a->NumRecords,
                                                    g_ciConsoleInformation.ReadConInpNumBytesUnicode,
                                                    a->Flags & CONSOLE_READ_NOREMOVE ? nullptr : &HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte);
                if (fAddDbcsLead && HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
                {
                    *(Buffer - 1) = HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte;
                    if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                        ZeroMemory(&HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));
                    a->NumRecords++;
                    Buffer--;
                }
            }

            SetReplyStatus(WaitReplyMessage, Status);
            SetReplyInformation(WaitReplyMessage, a->NumRecords * sizeof(INPUT_RECORD));

            delete[] DirectReadData;
        }
    }

    return RetVal;
}

// Routine Description:
// - This routine reads or peeks input events.  In both cases, the events
//   are copied to the user's buffer.  In the read case they are removed
//   from the input buffer and in the peek case they are not.
// Arguments:
// - m - message containing api parameters
// - ReplyStatus - Indicates whether to reply to the dll port.
// Return Value:
NTSTATUS SrvGetConsoleInput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL ReplyPending)
{
    PCONSOLE_GETCONSOLEINPUT_MSG const a = &m->u.consoleMsgL1.GetConsoleInput;

    if (IsFlagSet(a->Flags, CONSOLE_READ_NOREMOVE))
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::PeekConsoleInput, a->Unicode);
    }
    else
    {
        Telemetry::Instance().LogApiCall(Telemetry::ApiCall::ReadConsoleInput, a->Unicode);
    }

    if (a->Flags & ~CONSOLE_READ_VALID)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PINPUT_RECORD Buffer;
    NTSTATUS Status = GetOutputBuffer(m, (PVOID*) &Buffer, &a->NumRecords);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    a->NumRecords /= sizeof(INPUT_RECORD);

    CONSOLE_INFORMATION *Console;
    Status = RevalidateConsole(&Console);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_INPUT_HANDLE, GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        a->NumRecords = 0;
    }
    else
    {
        BOOLEAN fAddDbcsLead = FALSE;
        PDWORD nLength = &a->NumRecords;
        DIRECT_READ_DATA DirectReadData;
        DWORD nBytesUnicode = a->NumRecords;

        // if we're reading, wait for data.  if we're peeking, don't.
        DirectReadData.InputInfo = HandleData->GetInputBuffer();
        DirectReadData.ProcessData = GetMessageProcess(m);
        DirectReadData.HandleIndex = GetMessageObject(m);

        if (!a->Unicode)
        {
            // MSFT: 6429490 removed the usage of the ReadConInpNumBytesUnicode variable and the early return when the value was 1.
            // This resolved an issue with returning DBCS codepages to getc() and ReadConsoleInput.
            // There is another copy of the same pattern above in the Wait routine, but that usage scenario isn't 100% clear and
            // doesn't affect the issue, so it's left alone for now.

            nLength = &nBytesUnicode;
            // ASCII : a->NumRecords is ASCII byte count
            if (HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar)
            {
                // Saved DBCS Traling byte
                *Buffer = HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte;
                if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                    ZeroMemory(&HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte, sizeof(INPUT_RECORD));

                nBytesUnicode--;
                Buffer++;
                fAddDbcsLead = TRUE;
            }
        }

        Status = ReadInputBuffer(HandleData->GetInputBuffer(),
                                 Buffer,
                                 nLength,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT) && !fAddDbcsLead,
                                 FALSE,
                                 HandleData,
                                 m,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
                                 sizeof(DirectReadData),
                                 FALSE,
                                 a->Unicode);
        if (Status == CONSOLE_STATUS_WAIT)
        {
            *ReplyPending = TRUE;
        }
        else if (!a->Unicode)
        {
            a->NumRecords = TranslateInputToOem(Buffer,
                                                fAddDbcsLead ?
                                                a->NumRecords - 1 :
                                                a->NumRecords,
                                                nBytesUnicode, a->Flags & CONSOLE_READ_NOREMOVE ? nullptr : &HandleData->GetInputBuffer()->ReadConInpDbcsLeadByte);
            if (fAddDbcsLead)
            {
                a->NumRecords++;
                Buffer--;
            }
        }
    }

    UnlockConsole();

    if (NT_SUCCESS(Status))
    {
        SetReplyInformation(m, a->NumRecords * sizeof(INPUT_RECORD));
    }

    return Status;
}

NTSTATUS SrvWriteConsoleInput(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_WRITECONSOLEINPUT_MSG const a = &m->u.consoleMsgL2.WriteConsoleInput;

    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::WriteConsoleInput, a->Unicode);

    PINPUT_RECORD Buffer;
    ULONG Size;
    NTSTATUS Status = GetInputBuffer(m, (PVOID*) &Buffer, &Size);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_INPUT_HANDLE, GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        a->NumRecords = 0;
    }
    else
    {
        Status = DoSrvWriteConsoleInput(HandleData->GetInputBuffer(), a, Buffer);
    }

    UnlockConsole();
    return Status;
}

NTSTATUS DoSrvWriteConsoleInput(_In_ INPUT_INFORMATION* pInputInfo, _Inout_ CONSOLE_WRITECONSOLEINPUT_MSG* pMsg, _In_ INPUT_RECORD* const rgInputRecords)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pMsg->Unicode)
    {
        pMsg->NumRecords = TranslateInputToUnicode(rgInputRecords, pMsg->NumRecords, &pInputInfo->WriteConInpDbcsLeadByte[0]);
    }

    if (pMsg->Append)
    {
        pMsg->NumRecords = WriteInputBuffer(pInputInfo, rgInputRecords, pMsg->NumRecords);
    }
    else
    {
        Status = PrependInputBuffer(pInputInfo, rgInputRecords, &pMsg->NumRecords);
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

    UINT const Codepage = g_ciConsoleInformation.OutputCP;

    memmove(TmpBuffer, OutputBuffer, Size.X * Size.Y * sizeof(CHAR_INFO));

#pragma prefast(push)
#pragma prefast(disable:26019, "The buffer is the correct size for any given DBCS characters. No additional validation needed.")
    for (int i = 0; i < Size.Y; i++)
    {
        for (int j = 0; j < Size.X; j++)
        {
            if (TmpBuffer->Attributes & COMMON_LVB_LEADING_BYTE)
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
                    OutputBuffer->Attributes = TmpBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                    OutputBuffer++;
                    TmpBuffer++;
                }
            }
            else if (!(TmpBuffer->Attributes & COMMON_LVB_SBCSDBCS))
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
    UINT const Codepage = g_ciConsoleInformation.OutputCP;

    for (int i = 0; i < Size.Y; i++)
    {
        for (int j = 0; j < Size.X; j++)
        {
            OutputBuffer->Attributes &= ~COMMON_LVB_SBCSDBCS;
            if (IsDBCSLeadByteConsole(OutputBuffer->Char.AsciiChar, &g_ciConsoleInformation.OutputCPInfo))
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
                    OutputBuffer->Attributes |= COMMON_LVB_LEADING_BYTE;
                    OutputBuffer++;
                    OutputBuffer->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                    OutputBuffer->Attributes &= ~COMMON_LVB_SBCSDBCS;
                    OutputBuffer->Attributes |= COMMON_LVB_TRAILING_BYTE;
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
                OutputBufferR->Attributes = OutputBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                if (IsCharFullWidth(OutputBuffer->Char.UnicodeChar))
                {
                    if (j == Size.X - 1)
                    {
                        OutputBufferR->Char.UnicodeChar = UNICODE_SPACE;
                    }
                    else
                    {
                        OutputBufferR->Char.UnicodeChar = OutputBuffer->Char.UnicodeChar;
                        OutputBufferR->Attributes |= COMMON_LVB_LEADING_BYTE;
                        OutputBufferR++;
                        OutputBufferR->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                        OutputBufferR->Attributes = OutputBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                        OutputBufferR->Attributes |= COMMON_LVB_TRAILING_BYTE;
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
    NTSTATUS Status = GetOutputBuffer(m, (PVOID*) &Buffer, &Size);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_OUTPUT_HANDLE, GENERIC_READ);
    if (!NT_SUCCESS(Status))
    {
        // a region of zero size is indicated by the right and bottom coordinates being less than the left and top.
        a->CharRegion.Right = (USHORT) (a->CharRegion.Left - 1);
        a->CharRegion.Bottom = (USHORT) (a->CharRegion.Top - 1);
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

        SCREEN_INFORMATION* const psi = HandleData->GetScreenBuffer()->GetActiveBuffer();

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
            SetReplyInformation(m, CalcWindowSizeX(&a->CharRegion) * CalcWindowSizeY(&a->CharRegion) * sizeof(CHAR_INFO));
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
    NTSTATUS Status = GetInputBuffer(m, (PVOID*) &Buffer, &Size);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_OUTPUT_HANDLE, GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        // A region of zero size is indicated by the right and bottom coordinates being less than the left and top.
        a->CharRegion.Right = (USHORT) (a->CharRegion.Left - 1);
        a->CharRegion.Bottom = (USHORT) (a->CharRegion.Top - 1);
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

        PSCREEN_INFORMATION const ScreenBufferInformation = HandleData->GetScreenBuffer()->GetActiveBuffer();

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
            WriteToScreen(ScreenBufferInformation, &a->CharRegion);
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
    NTSTATUS Status = GetOutputBuffer(m, &Buffer, &a->NumRecords);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_OUTPUT_HANDLE, GENERIC_READ);
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

        Status = ReadOutputString(HandleData->GetScreenBuffer()->GetActiveBuffer(), Buffer, a->ReadCoord, a->StringType, &a->NumRecords);
        if (NT_SUCCESS(Status))
        {
            SetReplyInformation(m, a->NumRecords * nSize);
        }
    }

    UnlockConsole();
    return Status;
}

NTSTATUS SrvWriteConsoleOutputString(_Inout_ PCONSOLE_API_MSG m, _Inout_ PBOOL /*ReplyPending*/)
{
    PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG const a = &m->u.consoleMsgL2.WriteConsoleOutputString;

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
    NTSTATUS Status = GetInputBuffer(m, &Buffer, &BufferSize);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_OUTPUT_HANDLE, GENERIC_WRITE);
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

            Status = WriteOutputString(HandleData->GetScreenBuffer()->GetActiveBuffer(), Buffer, a->WriteCoord, a->StringType, &a->NumRecords, nullptr);
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

    PCONSOLE_HANDLE_DATA HandleData = GetMessageObject(m);
    Status = HandleData->DereferenceIoHandle(CONSOLE_OUTPUT_HANDLE, GENERIC_WRITE);
    if (!NT_SUCCESS(Status))
    {
        a->Length = 0;
    }
    else
    {
        Status = DoSrvFillConsoleOutput(HandleData->GetScreenBuffer()->GetActiveBuffer(), a);
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

NTSTATUS ConsoleCreateScreenBuffer(_Out_ CONSOLE_HANDLE_DATA** ppHandle,
                                   _In_ PCONSOLE_API_MSG /*Message*/,
                                   _In_ PCD_CREATE_OBJECT_INFORMATION Information,
                                   _In_ PCONSOLE_CREATESCREENBUFFER_MSG a)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::CreateConsoleScreenBuffer);

    if (a->Flags != CONSOLE_TEXTMODE_BUFFER)
    {
        ASSERT(false); // We no longer support anything other than a textmode buffer
        return STATUS_INVALID_PARAMETER;
    }

    ULONG const HandleType = CONSOLE_OUTPUT_HANDLE;

    const SCREEN_INFORMATION* const psiExisting = g_ciConsoleInformation.CurrentScreenBuffer;

    // Create new screen buffer.
    CHAR_INFO Fill;
    Fill.Char.UnicodeChar = UNICODE_SPACE;
    Fill.Attributes = psiExisting->GetAttributes()->GetLegacyAttributes();

    COORD WindowSize;
    WindowSize.X = (SHORT)psiExisting->GetScreenWindowSizeX();
    WindowSize.Y = (SHORT)psiExisting->GetScreenWindowSizeY();

    const FontInfo* const pfiExistingFont = psiExisting->TextInfo->GetCurrentFont();

    PSCREEN_INFORMATION ScreenInfo = nullptr;
    NTSTATUS Status = SCREEN_INFORMATION::CreateInstance(WindowSize, pfiExistingFont, WindowSize, Fill, Fill, CURSOR_SMALL_SIZE, &ScreenInfo);

    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    if (FAILED(ScreenInfo->Header.AllocateIoHandle(HandleType,
                                                   Information->DesiredAccess,
                                                   Information->ShareMode,
                                                   ppHandle)))
    {
        // TODO: correct this function to return HRs and pass errors.
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    InsertScreenBuffer(ScreenInfo);

Exit:
    if (!NT_SUCCESS(Status))
    {
        delete ScreenInfo;
    }

    return Status;
}
