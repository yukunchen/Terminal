/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "_stream.h"
#include "stream.h"

#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "readDataRaw.hpp"

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

#define LINE_INPUT_BUFFER_SIZE (256 * sizeof(WCHAR))

#define IS_JPN_1BYTE_KATAKANA(c)   ((c) >= 0xa1 && (c) <= 0xdf)

// Routine Description:
// - This routine is used in stream input.  It gets input and filters it for unicode characters.
// Arguments:
// - pInputBuffer - The InputBuffer to read from
// - pwchOut - On a successful read, the char data read
// - Wait - true if a waited read should be performed
// - pCommandLineEditingKeys - if present, arrow keys will be
// returned. on output, if true, pwchOut contains virtual key code for
// arrow key.
// - pCommandLinePopupKeys - if present, arrow keys will be
// returned. on output, if true, pwchOut contains virtual key code for
// arrow key.
// Return Value:
// - STATUS_SUCCESS on success or a relevant error code on failure.
NTSTATUS GetChar(_Inout_ InputBuffer* const pInputBuffer,
                 _Out_ wchar_t* const pwchOut,
                 _In_ bool const Wait,
                 _Out_opt_ bool* const pCommandLineEditingKeys,
                 _Out_opt_ bool* const pCommandLinePopupKeys,
                 _Out_opt_ DWORD* const pdwKeyState)
{
    if (nullptr != pCommandLineEditingKeys)
    {
        *pCommandLineEditingKeys = false;
    }

    if (nullptr != pCommandLinePopupKeys)
    {
        *pCommandLinePopupKeys = false;
    }

    if (nullptr != pdwKeyState)
    {
        *pdwKeyState = 0;
    }

    NTSTATUS Status;
    for (;;)
    {
        std::unique_ptr<IInputEvent> inputEvent;
        Status = pInputBuffer->Read(inputEvent,
                                    false, // peek
                                    Wait,
                                    true); // unicode

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if (inputEvent.get() == nullptr)
        {
            assert(!Wait);
            return STATUS_UNSUCCESSFUL;
        }

        if (inputEvent->EventType() == InputEventType::KeyEvent)
        {
            std::unique_ptr<KeyEvent> keyEvent = std::unique_ptr<KeyEvent>(static_cast<KeyEvent*>(inputEvent.release()));

            bool commandLineEditKey = false;
            if (pCommandLineEditingKeys)
            {
                commandLineEditKey = keyEvent->IsCommandLineEditingKey();
            }
            else if (pCommandLinePopupKeys)
            {
                commandLineEditKey = keyEvent->IsCommandLinePopupKey();
            }

            if ((pCommandLineEditingKeys || pCommandLinePopupKeys) &&
                ServiceLocator::LocateGlobals()->getConsoleInformation()->GetExtendedEditKey() &&
                keyEvent->GetKeySubst())
            {
                keyEvent->ParseEditKeyInfo();
            }

            if (pdwKeyState)
            {
                *pdwKeyState = keyEvent->_activeModifierKeys;
            }

            if (keyEvent->_charData != 0 && !commandLineEditKey)
            {
                // chars that are generated using alt + numpad
                if (!keyEvent->_keyDown && keyEvent->_virtualKeyCode == VK_MENU)
                {
                    if (IsFlagSet(keyEvent->_activeModifierKeys, ALTNUMPAD_BIT))
                    {
                        if (HIBYTE(keyEvent->_charData))
                        {
                            char chT[2] = {
                                static_cast<char>(HIBYTE(keyEvent->_charData)),
                                static_cast<char>(LOBYTE(keyEvent->_charData)),
                            };
                            *pwchOut = CharToWchar(chT, 2);
                        }
                        else
                        {
                            // Because USER doesn't know our codepage,
                            // it gives us the raw OEM char and we
                            // convert it to a Unicode character.
                            char chT = LOBYTE(keyEvent->_charData);
                            *pwchOut = CharToWchar(&chT, 1);
                        }
                    }
                    else
                    {
                        *pwchOut = keyEvent->_charData;
                    }
                    return STATUS_SUCCESS;
                }
                // Ignore Escape and Newline chars
                else if (keyEvent->_keyDown &&
                         (IsFlagSet(pInputBuffer->InputMode, ENABLE_VIRTUAL_TERMINAL_INPUT) ||
                          (keyEvent->_virtualKeyCode != VK_ESCAPE &&
                           keyEvent->_charData != UNICODE_LINEFEED)))
                {
                    *pwchOut = keyEvent->_charData;
                    return STATUS_SUCCESS;
                }
            }

            if (keyEvent->_keyDown)
            {
                if (pCommandLineEditingKeys && commandLineEditKey)
                {
                    *pCommandLineEditingKeys = true;
                    *pwchOut = static_cast<wchar_t>(keyEvent->_virtualKeyCode);
                    return STATUS_SUCCESS;
                }
                else if (pCommandLinePopupKeys && commandLineEditKey)
                {
                    *pCommandLinePopupKeys = true;
                    *pwchOut = static_cast<char>(keyEvent->_virtualKeyCode);
                    return STATUS_SUCCESS;
                }
                else
                {
                    const short zeroVkeyData = ServiceLocator::LocateInputServices()->VkKeyScanW(0);
                    const byte zeroVKey = LOBYTE(zeroVkeyData);
                    const byte zeroControlKeyState = HIBYTE(zeroVkeyData);
                    // Convert real Windows NT modifier bit into bizarre Console bits
                    const DWORD consoleModifierTranslator[] =
                    {
                        0,
                        SHIFT_PRESSED,
                        CTRL_PRESSED,
                        SHIFT_PRESSED | CTRL_PRESSED,
                        ALT_PRESSED,
                        SHIFT_PRESSED | ALT_PRESSED,
                        CTRL_PRESSED | ALT_PRESSED,
                        MOD_PRESSED
                    };
                    if (static_cast<unsigned int>(zeroControlKeyState) < ARRAYSIZE(consoleModifierTranslator))
                    {
                        const DWORD winmod = consoleModifierTranslator[zeroControlKeyState];

                        if (zeroVKey == keyEvent->_virtualKeyCode &&
                            AreAllFlagsSet(keyEvent->_activeModifierKeys, winmod) &&
                            AreAllFlagsClear(keyEvent->_activeModifierKeys, ~winmod))
                        {
                            // This really is the character 0x0000
                            *pwchOut = keyEvent->_charData;
                            return STATUS_SUCCESS;
                        }
                    }
                }
            }
        }
    }
}

// Routine Description:
// - This routine returns the total number of screen spaces the characters up to the specified character take up.
ULONG RetrieveTotalNumberOfSpaces(_In_ const SHORT sOriginalCursorPositionX,
                                  _In_reads_(ulCurrentPosition) const WCHAR * const pwchBuffer,
                                  _In_ ULONG ulCurrentPosition)
{
    SHORT XPosition = sOriginalCursorPositionX;
    ULONG NumSpaces = 0;

    for (ULONG i = 0; i < ulCurrentPosition; i++)
    {
        WCHAR const Char = pwchBuffer[i];

        ULONG NumSpacesForChar;
        if (Char == UNICODE_TAB)
        {
            NumSpacesForChar = NUMBER_OF_SPACES_IN_TAB(XPosition);
        }
        else if (IS_CONTROL_CHAR(Char))
        {
            NumSpacesForChar = 2;
        }
        else if (IsCharFullWidth(Char))
        {
            NumSpacesForChar = 2;
        }
        else
        {
            NumSpacesForChar = 1;
        }
        XPosition = (SHORT)(XPosition + NumSpacesForChar);
        NumSpaces += NumSpacesForChar;
    }

    return NumSpaces;
}

// Routine Description:
// - This routine returns the number of screen spaces the specified character takes up.
ULONG RetrieveNumberOfSpaces(_In_ SHORT sOriginalCursorPositionX,
                             _In_reads_(ulCurrentPosition + 1) const WCHAR * const pwchBuffer,
                             _In_ ULONG ulCurrentPosition)
{
    WCHAR Char = pwchBuffer[ulCurrentPosition];
    if (Char == UNICODE_TAB)
    {
        ULONG NumSpaces = 0;
        SHORT XPosition = sOriginalCursorPositionX;

        for (ULONG i = 0; i <= ulCurrentPosition; i++)
        {
            Char = pwchBuffer[i];
            if (Char == UNICODE_TAB)
            {
                NumSpaces = NUMBER_OF_SPACES_IN_TAB(XPosition);
            }
            else if (IS_CONTROL_CHAR(Char))
            {
                NumSpaces = 2;
            }
            else if (IsCharFullWidth(Char))
            {
                NumSpaces = 2;
            }
            else
            {
                NumSpaces = 1;
            }
            XPosition = (SHORT)(XPosition + NumSpaces);
        }

        return NumSpaces;
    }
    else if (IS_CONTROL_CHAR(Char))
    {
        return 2;
    }
    else if (IsCharFullWidth(Char))
    {
        return 2;
    }
    else
    {
        return 1;
    }
}


// Routine Description:
// - if we have leftover input, copy as much fits into the user's
// buffer and return.  we may have multi line input, if a macro
// has been defined that contains the $T character.
// Arguments:
// - pInputBuffer - Pointer to input buffer to read from.
// - pwchBuffer - buffer to place read char data into
// - pReadByteCount - buffer to place read char data
// - pHandleData - input read handle data associated with this read operation
// - Unicode - true if read should be unicode, false otherwise
// - ppWaiter - If a wait is necessary this will contain the wait
// object on output
// - OutputBufferSize - size of pwchBuffer, in bytes
// Return Value:
// - STATUS_NO_MEMORY in low memory situation
// - other relevant NTSTATUS codes
NTSTATUS ReadPendingInput(_Inout_ InputBuffer* const pInputBuffer,
                          _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                          _Out_ ULONG* const pReadByteCount,
                          _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                          _In_ const bool Unicode,
                          _In_ const size_t OutputBufferSize)
{

    BOOL fAddDbcsLead = FALSE;
    ULONG NumToWrite = 0;
    ULONG NumToBytes = 0;
    wchar_t* pBuffer = pwchBuffer;
    DWORD bufferRemaining = static_cast<DWORD>(OutputBufferSize);

    if (IsFlagSet(pHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput))
    {
        PWSTR Tmp;

        if (!Unicode)
        {
            if (pInputBuffer->IsReadPartialByteSequenceAvailable())
            {
                std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
                const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                *pBuffer = static_cast<char>(pKeyEvent->_charData);
                ++pBuffer;
                bufferRemaining -= sizeof(wchar_t);
                pHandleData->BytesAvailable -= sizeof(wchar_t);
                fAddDbcsLead = TRUE;
            }

            if (pHandleData->BytesAvailable == 0 || bufferRemaining == 0)
            {
                ClearAllFlags(pHandleData->InputHandleFlags,
                                (INPUT_READ_HANDLE_DATA::HandleFlags::InputPending |
                                INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput));
                delete[] pHandleData->BufPtr;
                *pReadByteCount = 1;
                return STATUS_SUCCESS;
            }
            else
            {
                for (NumToWrite = 0, Tmp = pHandleData->CurrentBufPtr, NumToBytes = 0;
                        NumToBytes < pHandleData->BytesAvailable &&
                        NumToBytes < bufferRemaining / sizeof(wchar_t) &&
                        *Tmp != UNICODE_LINEFEED;
                        (IsCharFullWidth(*Tmp) ? NumToBytes += 2 : NumToBytes++), Tmp++, NumToWrite += sizeof(wchar_t));
            }
        }

        for (NumToWrite = 0, Tmp = pHandleData->CurrentBufPtr;
                NumToWrite < pHandleData->BytesAvailable && *Tmp != UNICODE_LINEFEED; Tmp++, NumToWrite += sizeof(wchar_t));
        NumToWrite += sizeof(wchar_t);
        if (NumToWrite > bufferRemaining)
        {
            NumToWrite = bufferRemaining;
        }
    }
    else
    {
        if (!Unicode)
        {
            PWSTR Tmp;

            if (pInputBuffer->IsReadPartialByteSequenceAvailable())
            {
                std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
                const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                *pBuffer = static_cast<char>(pKeyEvent->_charData);
                ++pBuffer;
                bufferRemaining -= sizeof(wchar_t);
                pHandleData->BytesAvailable -= sizeof(wchar_t);
                fAddDbcsLead = TRUE;
            }

            if (pHandleData->BytesAvailable == 0)
            {
                ClearAllFlags(pHandleData->InputHandleFlags,
                              (INPUT_READ_HANDLE_DATA::HandleFlags::InputPending |
                               INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput));
                delete[] pHandleData->BufPtr;
                *pReadByteCount = 1;
                return STATUS_SUCCESS;
            }
            else
            {
                for (NumToWrite = 0, Tmp = pHandleData->CurrentBufPtr, NumToBytes = 0;
                        NumToBytes < pHandleData->BytesAvailable && NumToBytes < bufferRemaining / sizeof(wchar_t);
                        (IsCharFullWidth(*Tmp) ? NumToBytes += 2 : NumToBytes++), Tmp++, NumToWrite += sizeof(wchar_t));
            }
        }

        NumToWrite = (bufferRemaining < pHandleData->BytesAvailable) ? bufferRemaining : pHandleData->BytesAvailable;
    }

    memmove(pBuffer, pHandleData->CurrentBufPtr, NumToWrite);
    pHandleData->BytesAvailable -= NumToWrite;
    if (pHandleData->BytesAvailable == 0)
    {
        ClearAllFlags(pHandleData->InputHandleFlags,
                      (INPUT_READ_HANDLE_DATA::HandleFlags::InputPending |
                       INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput));
        delete[] pHandleData->BufPtr;
    }
    else
    {
        pHandleData->CurrentBufPtr = reinterpret_cast<wchar_t*>(reinterpret_cast<byte*>(pHandleData->CurrentBufPtr) + NumToWrite);
    }

    if (!Unicode)
    {
        // if ansi, translate string.  we allocated the capture buffer
        // large enough to handle the translated string.
        std::unique_ptr<char[]> tempBuffer;
        try
        {
            tempBuffer = std::make_unique<char[]>(NumToBytes);
        }
        catch (...)
        {
            return STATUS_NO_MEMORY;
        }

        std::unique_ptr<IInputEvent> partialEvent;

        NumToWrite = TranslateUnicodeToOem(pBuffer,
                                           NumToWrite / sizeof(wchar_t),
                                           tempBuffer.get(),
                                           NumToBytes,
                                           partialEvent);
        if (partialEvent.get())
        {
            pInputBuffer->StoreReadPartialByteSequence(std::move(partialEvent));
        }

#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "This access is fine but prefast can't follow it, evidently")
        memmove(pBuffer, tempBuffer.get(), NumToWrite);

        if (fAddDbcsLead)
        {
            NumToWrite++;
        }
    }

    *pReadByteCount = NumToWrite;
    return STATUS_SUCCESS;
}

// Routine Description:
// - read in characters until the buffer is full or return is read.
// since we may wait inside this loop, store all important variables
// in the read data structure.  if we do wait, a read data structure
// will be allocated from the heap and its pointer will be stored
// in the wait block.  the CookedReadData will be copied into the
// structure.  the data is freed when the read is completed.
// Arguments:
// - pInputBuffer - input buffer to read data from
// - ProcessData - process handle of process making read request
// - pwchBuffer - buffer to place read char data
// - pReadbyteCount - on output, the number of bytes read into pwchBuffer
// - pControlKeyState - set by a cooked read
// - pwsInitialData - text of initial data found in the read message
// - cbInitialData - length of pwsInitialData, in bytes
// - dwCtrlWakeupMask - used by COOKED_READ_DATA
// - pHandleData - input read handle data associated with this read operation
// - pwsExeName - name of the exe requesting the read
// - cbExeName - length of pwsExeName, in bytes
// - Unicode - true if read should be unicode, false otherwise
// - ppWaiter - If a wait is necessary this will contain the wait
// object on output
// - OutputBufferSize - size of pwchBuffer, in bytes
// Return Value:
// - STATUS_UNSUCCESSFUL if not able to access current screen buffer
// - STATUS_NO_MEMORY in low memory situation
// - other relevant NTSTATUS codes
NTSTATUS ReadLineInput(_Inout_ InputBuffer* const pInputBuffer,
                       _In_ HANDLE ProcessData,
                       _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                       _Out_ ULONG* const pReadByteCount,
                       _Inout_ ULONG* const pControlKeyState,
                       _In_reads_bytes_opt_(cbInitialData) const wchar_t* const pwsInitialData,
                       _In_ const ULONG cbInitialData,
                       _In_ const DWORD dwCtrlWakeupMask,
                       _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                       _In_reads_bytes_opt_(cbExeName) const wchar_t* const pwsExeName,
                       _In_ const ULONG cbExeName,
                       _In_ const bool Unicode,
                       _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                       _In_ const size_t OutputBufferSize)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    SCREEN_INFORMATION* const pScreenInfo = gci->CurrentScreenBuffer;
    if (nullptr == pScreenInfo)
    {
        return STATUS_UNSUCCESSFUL;
    }

    PCOMMAND_HISTORY const pCommandHistory = FindCommandHistory(ProcessData);

    // We need to create a temporary handle to the current screen buffer.
    ConsoleHandleData* pTempHandleData;
    NTSTATUS Status = NTSTATUS_FROM_HRESULT(pScreenInfo->Header.AllocateIoHandle(ConsoleHandleData::HandleType::Output,
                                                                                 GENERIC_WRITE,
                                                                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                                                 &pTempHandleData));

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    bool Echo = IsFlagSet(pInputBuffer->InputMode, ENABLE_ECHO_INPUT);


    // to emulate OS/2 KbdStringIn, we read into our own big buffer
    // (256 bytes) until the user types enter.  then return as many
    // chars as will fit in the user's buffer.

    ULONG const TempBufferSize = static_cast<ULONG>(max(OutputBufferSize, LINE_INPUT_BUFFER_SIZE));
    wchar_t* const TempBuffer = (wchar_t*) new BYTE[TempBufferSize];
    if (TempBuffer == nullptr)
    {
        if (Echo)
        {
            pTempHandleData->CloseHandle();
        }

        return STATUS_NO_MEMORY;
    }

    // Initialize the user's buffer to spaces. This is done so that
    // moving in the buffer via cursor doesn't do strange things.
    for (ULONG i = 0; i < TempBufferSize / sizeof(wchar_t); i++)
    {
        TempBuffer[i] = UNICODE_SPACE;
    }

    COORD invalidCoord;
    invalidCoord.X = -1;
    invalidCoord.Y = -1;
    COOKED_READ_DATA CookedReadData(pInputBuffer,
                                    pHandleData,
                                    pScreenInfo,
                                    TempBufferSize,
                                    0,
                                    0,
                                    TempBuffer,
                                    TempBuffer,
                                    static_cast<ULONG>(OutputBufferSize),
                                    pwchBuffer,
                                    invalidCoord,
                                    0,
                                    dwCtrlWakeupMask,
                                    pCommandHistory,
                                    !!Echo,
                                    !!gci->GetInsertMode(),
                                    IsFlagSet(pInputBuffer->InputMode, ENABLE_PROCESSED_INPUT),
                                    IsFlagSet(pInputBuffer->InputMode, ENABLE_LINE_INPUT),
                                    pTempHandleData);

    if (cbInitialData > 0)
    {
        memcpy_s(CookedReadData._BufPtr, CookedReadData._BufferSize, pwsInitialData, cbInitialData);

        CookedReadData._BytesRead += cbInitialData;

        ULONG const cchInitialData = cbInitialData / sizeof(wchar_t);
        CookedReadData._NumberOfVisibleChars = cchInitialData;
        CookedReadData._BufPtr += cchInitialData;
        CookedReadData._CurrentPosition = cchInitialData;

        CookedReadData._OriginalCursorPosition = pScreenInfo->TextInfo->GetCursor()->GetPosition();
        CookedReadData._OriginalCursorPosition.X -= (SHORT)CookedReadData._CurrentPosition;

        const SHORT sScreenBufferSizeX = pScreenInfo->GetScreenBufferSize().X;
        while (CookedReadData._OriginalCursorPosition.X < 0)
        {
            CookedReadData._OriginalCursorPosition.X += sScreenBufferSizeX;
            CookedReadData._OriginalCursorPosition.Y -= 1;
        }
    }

    if (cbExeName > 0)
    {
        CookedReadData.ExeNameLength = (USHORT)cbExeName;
        CookedReadData.ExeName = (wchar_t*) new BYTE[CookedReadData.ExeNameLength];
        memcpy_s(CookedReadData.ExeName, CookedReadData.ExeNameLength, pwsExeName, cbExeName);
    }

    gci->lpCookedReadData = &CookedReadData;

    Status = CookedRead(&CookedReadData, Unicode, pReadByteCount, pControlKeyState);
    if (CONSOLE_STATUS_WAIT == Status)
    {
        COOKED_READ_DATA* pCookedReadWaiter = new COOKED_READ_DATA(std::move(CookedReadData));
        if (nullptr == pCookedReadWaiter)
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            gci->lpCookedReadData = pCookedReadWaiter;
            *ppWaiter = pCookedReadWaiter;
        }
    }

    if (CONSOLE_STATUS_WAIT != Status)
    {
        gci->lpCookedReadData = nullptr;
    }

    return Status;
}

// Routine Description:
// - Character (raw) mode. Read at least one character in. After one
// character has been read, get any more available characters and
// return. The first call to GetChar may block. If we do wait, a read
// data structure will be allocated from the heap and its pointer will
// be stored in the wait block. The RawReadData will be copied into
// the structure. The data is freed when the read is completed.
// Arguments:
// - pInputBuffer - input buffer to read data from
// - pReadByteCount - buffer to place read char data
// - pcbBuffer - on output, the amount of data read, in bytes
// - pHandleData - input read handle data associated with this read operation
// - Unicode - true if read should be unicode, false otherwise
// - ppWaiter - if a wait is necessary, on output this will contain
// the associated wait object
// - OutputBufferSize - The size of pwchBuffer, in bytes
// Return Value:
// - CONSOLE_STATUS_WAIT if a wait is necessary. ppWaiter will be
// populated.
// - STATUS_SUCCESS on success
// - Other NSTATUS codes as necessary
NTSTATUS ReadCharacterInput(_Inout_ InputBuffer* const pInputBuffer,
                            _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                            _Out_ ULONG* const pReadByteCount,
                            _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                            _In_ const bool Unicode,
                            _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                            _In_ const size_t OutputBufferSize)
{

    ULONG NumToWrite = 0;
    bool addDbcsLead = false;
    NTSTATUS Status = STATUS_SUCCESS;
    wchar_t* pBuffer = pwchBuffer;
    size_t bufferRemaining = OutputBufferSize;

    if (*pReadByteCount < static_cast<ULONG>(bufferRemaining))
    {
        wchar_t* pwchBufferTmp = pBuffer;

        NumToWrite = 0;

        if (!Unicode && pInputBuffer->IsReadPartialByteSequenceAvailable())
        {
            std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
            const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
            *pBuffer = static_cast<char>(pKeyEvent->_charData);
            ++pBuffer;
            bufferRemaining -= sizeof(wchar_t);
            addDbcsLead = true;

            if (bufferRemaining == 0)
            {
                *pReadByteCount = 1;
                return STATUS_SUCCESS;
            }
        }
        else
        {
            Status = GetChar(pInputBuffer,
                                pBuffer,
                                true,
                                nullptr,
                                nullptr,
                                nullptr);
        }

        if (Status == CONSOLE_STATUS_WAIT)
        {
            *ppWaiter = new RAW_READ_DATA(pInputBuffer,
                                          pHandleData,
                                          static_cast<ULONG>(OutputBufferSize),
                                          pwchBuffer);
        }

        if (!NT_SUCCESS(Status))
        {
            *pReadByteCount = 0;
            return Status;
        }

        if (!addDbcsLead)
        {
            if (IsCharFullWidth(*pBuffer))
            {
                *pReadByteCount += 2;
            }
            else
            {
                *pReadByteCount += 1;
            }
            NumToWrite += sizeof(wchar_t);
            pBuffer++;
        }

        while (NumToWrite < static_cast<ULONG>(bufferRemaining))
        {
            Status = GetChar(pInputBuffer,
                             pBuffer,
                             false,
                             nullptr,
                             nullptr,
                             nullptr);
            if (!NT_SUCCESS(Status))
            {
                break;
            }
            if (IsCharFullWidth(*pBuffer))
            {
                *pReadByteCount += 2;
            }
            else
            {
                *pReadByteCount += 1;
            }
            NumToWrite += sizeof(wchar_t);
            pBuffer++;
        }

        // if ansi, translate string.  we allocated the capture buffer large enough to handle the translated string.
        if (!Unicode)
        {
            std::unique_ptr<char[]> tempBuffer = std::make_unique<char[]>(*pReadByteCount);
            if (!tempBuffer.get())
            {
                return STATUS_NO_MEMORY;
            }

            pBuffer = pwchBufferTmp;
            std::unique_ptr<IInputEvent> partialEvent;

            *pReadByteCount = TranslateUnicodeToOem(pBuffer,
                                                    NumToWrite / sizeof(wchar_t),
                                                    tempBuffer.get(),
                                                    *pReadByteCount,
                                                    partialEvent);

            if (partialEvent.get())
            {
                pInputBuffer->StoreReadPartialByteSequence(std::move(partialEvent));
            }

#pragma prefast(suppress:26053 26015, "PREfast claims read overflow. *pReadByteCount is the exact size of tempBuffer as allocated above.")
            memmove(pBuffer, tempBuffer.get(), *pReadByteCount);

            if (addDbcsLead)
            {
                ++*pReadByteCount;
            }
        }
        else
        {
            // We always return the byte count for A & W modes, so in
            // the Unicode case where we didn't translate back, set
            // the return to the byte count that we assembled while
            // pulling characters from the internal buffers.
            *pReadByteCount = NumToWrite;
        }
    }
    return STATUS_SUCCESS;
}

// Routine Description:
// - This routine reads in characters for stream input and does the
// required processing based on the input mode (line, char, echo).
// - This routine returns UNICODE characters.
// Arguments:
// - pInputBuffer - Pointer to input buffer to read from.
// - ProcessData - process handle of process making read request
// - pwchBuffer - buffer to place read char data into
// - pcbBuffer - on input, length of pwchBuffer. on output, the length
// of data place in pwchBuffer. Both are measured in bytes.
// - pControlKeyState - set by a cooked read
// - pwsInitialData - text of initial data found in the read message
// - cbInitialData - length of pwsInitialData, in bytes
// - dwCtrlWakeupMask - used by COOKED_READ_DATA
// - pHandleData - read handle data associated with this read
// - pwsExeName - name of the exe requesting the read
// - cbExeName - length of pwsExeName, in bytes
// - Unicode - true for a unicode read, false for ascii
// - ppWaiter - If a wait is necessary this will contain the wait
// object on output
// Return Value:
// - STATUS_BUFFER_TOO_SMALL if pdwNumBytes is too small to store char
// data.
// - CONSOLE_STATUS_WAIT if a wait is necessary. ppWaiter will be
// populated.
// - STATUS_SUCCESS on success
// - Other NSTATUS codes as necessary
NTSTATUS DoReadConsole(_Inout_ InputBuffer* const pInputBuffer,
                       _In_ HANDLE ProcessData,
                       _Out_writes_bytes_(*pcbBuffer) wchar_t* const pwchBuffer,
                       _Inout_ ULONG* const pcbBuffer,
                       _Inout_ ULONG* const pControlKeyState,
                       _In_reads_bytes_opt_(cbInitialData) const wchar_t* const pwsInitialData,
                       _In_ const ULONG cbInitialData,
                       _In_ const DWORD dwCtrlWakeupMask,
                       _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                       _In_reads_bytes_opt_(cbExeName) const wchar_t* const pwsExeName,
                       _In_ const ULONG cbExeName,
                       _In_ const bool Unicode,
                       _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *ppWaiter = nullptr;

    if (*pcbBuffer < sizeof(wchar_t))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    const size_t OutputBufferSize = *pcbBuffer;
    *pcbBuffer = 0;

    if (IsFlagSet(pHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending))
    {
        return ReadPendingInput(pInputBuffer,
                                pwchBuffer,
                                pcbBuffer,
                                pHandleData,
                                Unicode,
                                OutputBufferSize);
    }
    else if (IsFlagSet(pInputBuffer->InputMode, ENABLE_LINE_INPUT))
    {
        return ReadLineInput(pInputBuffer,
                            ProcessData,
                            pwchBuffer,
                            pcbBuffer,
                            pControlKeyState,
                            pwsInitialData,
                            cbInitialData,
                            dwCtrlWakeupMask,
                            pHandleData,
                            pwsExeName,
                            cbExeName,
                            Unicode,
                            ppWaiter,
                            OutputBufferSize);
    }
    else
    {
        return ReadCharacterInput(pInputBuffer,
                                  pwchBuffer,
                                  pcbBuffer,
                                  pHandleData,
                                  Unicode,
                                  ppWaiter,
                                  OutputBufferSize);
    }
}

HRESULT ApiRoutines::ReadConsoleAImpl(_Inout_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) char* const psTextBuffer,
                                      _In_ size_t const cchTextBuffer,
                                      _Out_ size_t* const pcchTextBufferWritten,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                      _In_reads_opt_(cchInitialData) const char* const psInitialData,
                                      _In_ size_t const cchInitialData,
                                      _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                      _In_ size_t const cchExeName,
                                      _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                      _In_ HANDLE const hConsoleClient,
                                      _In_ DWORD const dwControlWakeupMask,
                                      _Out_ DWORD* const pdwControlKeyState)
{
    ULONG ulTextBuffer;
    RETURN_IF_FAILED(SizeTToULong(cchTextBuffer, &ulTextBuffer));

    size_t cbExeName;
    RETURN_IF_FAILED(SizeTMult(cchExeName, sizeof(wchar_t), &cbExeName));

    ULONG ulExeName;
    RETURN_IF_FAILED(SizeTToULong(cbExeName, &ulExeName));

    ULONG ulInitialData;
    RETURN_IF_FAILED(SizeTToULong(cchInitialData, &ulInitialData));

    // DoReadConsole performs the same check, but having this here
    // before the function call makes the static analyzer happy.
    if (ulTextBuffer < sizeof(wchar_t))
    {
        return HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL);
    }

    NTSTATUS const Status = DoReadConsole(pInContext,
                                          hConsoleClient,
                                          (wchar_t*)psTextBuffer,
                                          &ulTextBuffer,
                                          pdwControlKeyState,
                                          (wchar_t*)psInitialData,
                                          ulInitialData,
                                          dwControlWakeupMask,
                                          pHandleData,
                                          pwsExeName,
                                          ulExeName,
                                          false,
                                          ppWaiter);

    *pcchTextBufferWritten = ulTextBuffer;

    return HRESULT_FROM_NT(Status);
}

HRESULT ApiRoutines::ReadConsoleWImpl(_Inout_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) wchar_t* const pwsTextBuffer,
                                      _In_ size_t const cchTextBuffer,
                                      _Out_ size_t* const pcchTextBufferWritten,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                      _In_reads_opt_(cchInitialData) const wchar_t* const pwsInitialData,
                                      _In_ size_t const cchInitialData,
                                      _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                      _In_ size_t const cchExeName,
                                      _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                      _In_ HANDLE const hConsoleClient,
                                      _In_ DWORD const dwControlWakeupMask,
                                      _Out_ DWORD* const pdwControlKeyState)
{
    size_t cbTextBuffer;
    RETURN_IF_FAILED(SizeTMult(cchTextBuffer, sizeof(wchar_t), &cbTextBuffer));

    ULONG ulTextBuffer;
    RETURN_IF_FAILED(SizeTToULong(cbTextBuffer, &ulTextBuffer));

    size_t cbInitialData;
    RETURN_IF_FAILED(SizeTMult(cchInitialData, sizeof(wchar_t), &cbInitialData));

    ULONG ulInitialData;
    RETURN_IF_FAILED(SizeTToULong(cbInitialData, &ulInitialData));

    size_t cbExeName;
    RETURN_IF_FAILED(SizeTMult(cchExeName, sizeof(wchar_t), &cbExeName));

    ULONG ulExeName;
    RETURN_IF_FAILED(SizeTToULong(cbExeName, &ulExeName));

    NTSTATUS const Status = DoReadConsole(pInContext,
                                          hConsoleClient,
                                          pwsTextBuffer,
                                          &ulTextBuffer,
                                          pdwControlKeyState,
                                          pwsInitialData,
                                          ulInitialData,
                                          dwControlWakeupMask,
                                          pHandleData,
                                          pwsExeName,
                                          ulExeName,
                                          true,
                                          ppWaiter);

    assert(ulTextBuffer % sizeof(wchar_t) == 0);
    *pcchTextBufferWritten = ulTextBuffer / sizeof(wchar_t);

    return HRESULT_FROM_NT(Status);
}

VOID UnblockWriteConsole(_In_ const DWORD dwReason)
{
    CONSOLE_INFORMATION* const gci = ServiceLocator::LocateGlobals()->getConsoleInformation();
    gci->Flags &= ~dwReason;

    if (AreAllFlagsClear(gci->Flags, (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING)))
    {
        // There is no longer any reason to suspend output, so unblock it.
        gci->OutputQueue.NotifyWaiters(true);
    }
}
