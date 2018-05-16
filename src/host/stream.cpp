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
[[nodiscard]]
NTSTATUS GetChar(_Inout_ InputBuffer* const pInputBuffer,
                 _Out_ wchar_t* const pwchOut,
                 const bool Wait,
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
        else if (inputEvent.get() == nullptr)
        {
            FAIL_FAST_IF(Wait);
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

            if (pdwKeyState)
            {
                *pdwKeyState = keyEvent->GetActiveModifierKeys();
            }

            if (keyEvent->GetCharData() != 0 && !commandLineEditKey)
            {
                // chars that are generated using alt + numpad
                if (!keyEvent->IsKeyDown() && keyEvent->GetVirtualKeyCode() == VK_MENU)
                {
                    if (keyEvent->IsAltNumpadSet())
                    {
                        if (HIBYTE(keyEvent->GetCharData()))
                        {
                            char chT[2] = {
                                static_cast<char>(HIBYTE(keyEvent->GetCharData())),
                                static_cast<char>(LOBYTE(keyEvent->GetCharData())),
                            };
                            *pwchOut = CharToWchar(chT, 2);
                        }
                        else
                        {
                            // Because USER doesn't know our codepage,
                            // it gives us the raw OEM char and we
                            // convert it to a Unicode character.
                            char chT = LOBYTE(keyEvent->GetCharData());
                            *pwchOut = CharToWchar(&chT, 1);
                        }
                    }
                    else
                    {
                        *pwchOut = keyEvent->GetCharData();
                    }
                    return STATUS_SUCCESS;
                }
                // Ignore Escape and Newline chars
                else if (keyEvent->IsKeyDown() &&
                    (IsFlagSet(pInputBuffer->InputMode, ENABLE_VIRTUAL_TERMINAL_INPUT) ||
                         (keyEvent->GetVirtualKeyCode() != VK_ESCAPE &&
                          keyEvent->GetCharData() != UNICODE_LINEFEED)))
                {
                    *pwchOut = keyEvent->GetCharData();
                    return STATUS_SUCCESS;
                }
            }

            if (keyEvent->IsKeyDown())
            {
                if (pCommandLineEditingKeys && commandLineEditKey)
                {
                    *pCommandLineEditingKeys = true;
                    *pwchOut = static_cast<wchar_t>(keyEvent->GetVirtualKeyCode());
                    return STATUS_SUCCESS;
                }
                else if (pCommandLinePopupKeys && commandLineEditKey)
                {
                    *pCommandLinePopupKeys = true;
                    *pwchOut = static_cast<char>(keyEvent->GetVirtualKeyCode());
                    return STATUS_SUCCESS;
                }
                else
                {
                    const short zeroVkeyData = ServiceLocator::LocateInputServices()->VkKeyScanW(0);
                    const byte zeroVKey = LOBYTE(zeroVkeyData);
                    const byte zeroControlKeyState = HIBYTE(zeroVkeyData);

                    try
                    {
                        // Convert real Windows NT modifier bit into bizarre Console bits
                        std::unordered_set<ModifierKeyState> consoleModKeyState = FromVkKeyScan(zeroControlKeyState);

                        if (zeroVKey == keyEvent->GetVirtualKeyCode() &&
                            keyEvent->DoActiveModifierKeysMatch(consoleModKeyState))
                        {
                            // This really is the character 0x0000
                            *pwchOut = keyEvent->GetCharData();
                            return STATUS_SUCCESS;
                        }
                    }
                    catch (...)
                    {
                        LOG_HR(wil::ResultFromCaughtException());
                    }
                }
            }
        }
    }
}

// Routine Description:
// - This routine returns the total number of screen spaces the characters up to the specified character take up.
size_t RetrieveTotalNumberOfSpaces(const SHORT sOriginalCursorPositionX,
                                  _In_reads_(ulCurrentPosition) const WCHAR * const pwchBuffer,
                                  _In_ size_t ulCurrentPosition)
{
    SHORT XPosition = sOriginalCursorPositionX;
    size_t NumSpaces = 0;

    for (size_t i = 0; i < ulCurrentPosition; i++)
    {
        WCHAR const Char = pwchBuffer[i];

        size_t NumSpacesForChar;
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
size_t RetrieveNumberOfSpaces(_In_ SHORT sOriginalCursorPositionX,
                             _In_reads_(ulCurrentPosition + 1) const WCHAR * const pwchBuffer,
                             _In_ size_t ulCurrentPosition)
{
    WCHAR Char = pwchBuffer[ulCurrentPosition];
    if (Char == UNICODE_TAB)
    {
        size_t NumSpaces = 0;
        SHORT XPosition = sOriginalCursorPositionX;

        for (size_t i = 0; i <= ulCurrentPosition; i++)
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
[[nodiscard]]
NTSTATUS ReadPendingInput(_Inout_ InputBuffer* const pInputBuffer,
                          _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                          _Out_ size_t* const pReadByteCount,
                          _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                          const bool Unicode,
                          const size_t OutputBufferSize)
{

    BOOL fAddDbcsLead = FALSE;
    size_t NumToWrite = 0;
    size_t NumToBytes = 0;
    wchar_t* pBuffer = pwchBuffer;
    size_t bufferRemaining = OutputBufferSize;
    *pReadByteCount = 0;

    if (OutputBufferSize < sizeof(wchar_t))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    const auto pending = pHandleData->GetPendingInput();
    size_t pendingBytes = pending.size();
    const std::wstring_view pendingAsWstr{ reinterpret_cast<const wchar_t*>(pending.data()), pendingBytes / sizeof(wchar_t) };
    auto Tmp = pendingAsWstr.cbegin();

    if (pHandleData->IsMultilineInput())
    {
        if (!Unicode)
        {
            if (pInputBuffer->IsReadPartialByteSequenceAvailable())
            {
                std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
                const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                *pBuffer = static_cast<char>(pKeyEvent->GetCharData());
                ++pBuffer;
                bufferRemaining -= sizeof(wchar_t);
                pendingBytes -= sizeof(wchar_t);
                fAddDbcsLead = TRUE;
            }

            if (pendingBytes == 0 || bufferRemaining == 0)
            {
                *pReadByteCount = 1;
                return STATUS_SUCCESS;
            }
            else
            {
                for (NumToWrite = 0, Tmp = pendingAsWstr.cbegin(), NumToBytes = 0;
                     NumToBytes < pendingBytes &&
                     NumToBytes < bufferRemaining / sizeof(wchar_t) &&
                     *Tmp != UNICODE_LINEFEED;
                     (IsCharFullWidth(*Tmp) ? NumToBytes += 2 : NumToBytes++), Tmp++, NumToWrite += sizeof(wchar_t));
            }
        }


        NumToWrite = 0;
        Tmp = pendingAsWstr.cbegin();
        while (NumToWrite < pendingBytes &&
               *Tmp != UNICODE_LINEFEED)
        {
            ++Tmp;
            NumToWrite += sizeof(wchar_t);
        }

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
            if (pInputBuffer->IsReadPartialByteSequenceAvailable())
            {
                std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
                const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                *pBuffer = static_cast<char>(pKeyEvent->GetCharData());
                ++pBuffer;
                bufferRemaining -= sizeof(wchar_t);
                pendingBytes -= sizeof(wchar_t);
                fAddDbcsLead = TRUE;
            }

            if (pendingBytes == 0)
            {
                *pReadByteCount = 1;
                return STATUS_SUCCESS;
            }
            else
            {
                for (NumToWrite = 0, Tmp = pendingAsWstr.cbegin(), NumToBytes = 0;
                     NumToBytes < pendingBytes && NumToBytes < bufferRemaining / sizeof(wchar_t);
                     (IsCharFullWidth(*Tmp) ? NumToBytes += 2 : NumToBytes++), Tmp++, NumToWrite += sizeof(wchar_t));
            }
        }

        NumToWrite = (bufferRemaining < pendingBytes) ? bufferRemaining : pendingBytes;
    }

    memmove(pBuffer, pendingAsWstr.data(), NumToWrite);
    pendingBytes -= NumToWrite;
    if (pendingBytes != 0)
    {
        std::string_view remainingPending{ pending.data() + NumToWrite, pendingBytes };
        pHandleData->UpdatePending(remainingPending);
    }

    if (!Unicode)
    {
        // if ansi, translate string.  we allocated the capture buffer
        // large enough to handle the translated string.
        std::unique_ptr<char[]> tempBuffer = std::make_unique<char[]>(NumToBytes);
        std::unique_ptr<IInputEvent> partialEvent;

        NumToWrite = TranslateUnicodeToOem(pBuffer,
                                           gsl::narrow<ULONG>(NumToWrite / sizeof(wchar_t)),
                                           tempBuffer.get(),
                                           gsl::narrow<ULONG>(NumToBytes),
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
[[nodiscard]]
HRESULT ReadLineInput(_Inout_ InputBuffer* const pInputBuffer,
                      _In_ HANDLE ProcessData,
                      _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                      _Out_ size_t* const pReadByteCount,
                      _Inout_ ULONG* const pControlKeyState,
                      _In_reads_bytes_opt_(cbInitialData) const wchar_t* const pwsInitialData,
                      const size_t cbInitialData,
                      const DWORD dwCtrlWakeupMask,
                      _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                      const std::wstring& exeName,
                      const bool Unicode,
                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                      const size_t OutputBufferSize)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    RETURN_HR_IF(E_FAIL, !gci.HasActiveOutputBuffer());

    SCREEN_INFORMATION& screenInfo = gci.GetActiveOutputBuffer();
    PCOMMAND_HISTORY const pCommandHistory = FindCommandHistory(ProcessData);

    try
    {
        COOKED_READ_DATA CookedReadData(pInputBuffer, // pInputBuffer
                                        pHandleData, // pInputReadHandleData
                                        screenInfo, // pScreenInfo
                                        OutputBufferSize, // UserBufferSize
                                        pwchBuffer, // UserBuffer
                                        dwCtrlWakeupMask, // CtrlWakeupMask
                                        pCommandHistory, // CommandHistory
                                        exeName); // pTempHandle

        if (cbInitialData > 0)
        {
            memcpy_s(CookedReadData._BufPtr, CookedReadData._BufferSize, pwsInitialData, cbInitialData);

            CookedReadData._BytesRead += cbInitialData;

            size_t const cchInitialData = cbInitialData / sizeof(wchar_t);
            CookedReadData._NumberOfVisibleChars = cchInitialData;
            CookedReadData._BufPtr += cchInitialData;
            CookedReadData._CurrentPosition = cchInitialData;

            CookedReadData._OriginalCursorPosition = screenInfo.GetTextBuffer().GetCursor().GetPosition();
            CookedReadData._OriginalCursorPosition.X -= (SHORT)CookedReadData._CurrentPosition;

            const SHORT sScreenBufferSizeX = screenInfo.GetScreenBufferSize().X;
            while (CookedReadData._OriginalCursorPosition.X < 0)
            {
                CookedReadData._OriginalCursorPosition.X += sScreenBufferSizeX;
                CookedReadData._OriginalCursorPosition.Y -= 1;
            }
        }

        gci.lpCookedReadData = &CookedReadData;

        if (CONSOLE_STATUS_WAIT == CookedReadData.Read(Unicode, *pReadByteCount, *pControlKeyState))
        {
            COOKED_READ_DATA* pCookedReadWaiter = new(std::nothrow) COOKED_READ_DATA(std::move(CookedReadData));
            RETURN_IF_NULL_ALLOC(pCookedReadWaiter);

            gci.lpCookedReadData = pCookedReadWaiter;
            *ppWaiter = pCookedReadWaiter;
        }
        else
        {
            gci.lpCookedReadData = nullptr;
        }
    }
    CATCH_RETURN();

    return S_OK;
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
[[nodiscard]]
NTSTATUS ReadCharacterInput(_Inout_ InputBuffer* const pInputBuffer,
                            _Out_writes_bytes_to_(OutputBufferSize, *pReadByteCount) wchar_t* const pwchBuffer,
                            _Out_ size_t* const pReadByteCount,
                            _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                            const bool Unicode,
                            _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                            const size_t OutputBufferSize)
{

    size_t NumToWrite = 0;
    bool addDbcsLead = false;
    NTSTATUS Status = STATUS_SUCCESS;
    wchar_t* pBuffer = pwchBuffer;
    size_t bufferRemaining = OutputBufferSize;
    *pReadByteCount = 0;

    if (OutputBufferSize < sizeof(wchar_t))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (*pReadByteCount < bufferRemaining)
    {
        wchar_t* pwchBufferTmp = pBuffer;

        NumToWrite = 0;

        if (!Unicode && pInputBuffer->IsReadPartialByteSequenceAvailable())
        {
            std::unique_ptr<IInputEvent> event = pInputBuffer->FetchReadPartialByteSequence(false);
            const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
            *pBuffer = static_cast<char>(pKeyEvent->GetCharData());
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
            try
            {
                *ppWaiter = new RAW_READ_DATA(pInputBuffer,
                                              pHandleData,
                                              static_cast<ULONG>(OutputBufferSize),
                                              pwchBuffer);
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }
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
            std::unique_ptr<char[]> tempBuffer;
            try
            {
                tempBuffer = std::make_unique<char[]>(*pReadByteCount);
            }
            catch (...)
            {
                return STATUS_NO_MEMORY;
            }

            pBuffer = pwchBufferTmp;
            std::unique_ptr<IInputEvent> partialEvent;

            *pReadByteCount = TranslateUnicodeToOem(pBuffer,
                                                    gsl::narrow<ULONG>(NumToWrite / sizeof(wchar_t)),
                                                    tempBuffer.get(),
                                                    gsl::narrow<ULONG>(*pReadByteCount),
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
[[nodiscard]]
NTSTATUS DoReadConsole(_Inout_ InputBuffer* const pInputBuffer,
                       _In_ HANDLE ProcessData,
                       _Out_writes_bytes_(*pcbBuffer) wchar_t* const pwchBuffer,
                       _Inout_ size_t* const pcbBuffer,
                       _Inout_ ULONG* const pControlKeyState,
                       _In_reads_bytes_opt_(cbInitialData) const wchar_t* const pwsInitialData,
                       const ULONG cbInitialData,
                       const DWORD dwCtrlWakeupMask,
                       _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                       const std::wstring& exeName,
                       const bool Unicode,
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

    if (pHandleData->IsInputPending())
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
        return NTSTATUS_FROM_HRESULT(ReadLineInput(pInputBuffer,
                                                   ProcessData,
                                                   pwchBuffer,
                                                   pcbBuffer,
                                                   pControlKeyState,
                                                   pwsInitialData,
                                                   cbInitialData,
                                                   dwCtrlWakeupMask,
                                                   pHandleData,
                                                   exeName,
                                                   Unicode,
                                                   ppWaiter,
                                                   OutputBufferSize));
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
                                      const size_t cchTextBuffer,
                                      _Out_ size_t* const pcchTextBufferWritten,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                      _In_reads_opt_(cchInitialData) const char* const psInitialData,
                                      const size_t cchInitialData,
                                      _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                      const size_t cchExeName,
                                      _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                      const HANDLE hConsoleClient,
                                      const DWORD dwControlWakeupMask,
                                      _Out_ DWORD* const pdwControlKeyState)
{
    const std::wstring exeName{ pwsExeName, cchExeName };

    ULONG ulInitialData;
    RETURN_IF_FAILED(SizeTToULong(cchInitialData, &ulInitialData));

    // DoReadConsole performs the same check, but having this here
    // before the function call makes the static analyzer happy.
    if (cchTextBuffer < sizeof(wchar_t))
    {
        return HRESULT_FROM_NT(STATUS_BUFFER_TOO_SMALL);
    }

    size_t cbTextBuffer = cchTextBuffer;
    NTSTATUS const Status = DoReadConsole(pInContext,
                                          hConsoleClient,
                                          (wchar_t*)psTextBuffer,
                                          &cbTextBuffer,
                                          pdwControlKeyState,
                                          (wchar_t*)psInitialData,
                                          ulInitialData,
                                          dwControlWakeupMask,
                                          pHandleData,
                                          exeName,
                                          false,
                                          ppWaiter);

    *pcchTextBufferWritten = cbTextBuffer;

    return HRESULT_FROM_NT(Status);
}

[[nodiscard]]
HRESULT ApiRoutines::ReadConsoleWImpl(_Inout_ IConsoleInputObject* const pInContext,
                                      _Out_writes_to_(cchTextBuffer, *pcchTextBufferWritten) wchar_t* const pwsTextBuffer,
                                      const size_t cchTextBuffer,
                                      _Out_ size_t* const pcchTextBufferWritten,
                                      _Outptr_result_maybenull_ IWaitRoutine** const ppWaiter,
                                      _In_reads_opt_(cchInitialData) const wchar_t* const pwsInitialData,
                                      const size_t cchInitialData,
                                      _In_reads_opt_(cchExeName) const wchar_t* const pwsExeName,
                                      const size_t cchExeName,
                                      _In_ INPUT_READ_HANDLE_DATA* const pHandleData,
                                      const HANDLE hConsoleClient,
                                      const DWORD dwControlWakeupMask,
                                      _Out_ DWORD* const pdwControlKeyState)
{
    size_t cbTextBuffer;
    RETURN_IF_FAILED(SizeTMult(cchTextBuffer, sizeof(wchar_t), &cbTextBuffer));

    size_t cbInitialData;
    RETURN_IF_FAILED(SizeTMult(cchInitialData, sizeof(wchar_t), &cbInitialData));

    ULONG ulInitialData;
    RETURN_IF_FAILED(SizeTToULong(cbInitialData, &ulInitialData));

    const std::wstring exeName{ pwsExeName, cchExeName };

    NTSTATUS const Status = DoReadConsole(pInContext,
                                          hConsoleClient,
                                          pwsTextBuffer,
                                          &cbTextBuffer,
                                          pdwControlKeyState,
                                          pwsInitialData,
                                          ulInitialData,
                                          dwControlWakeupMask,
                                          pHandleData,
                                          exeName,
                                          true,
                                          ppWaiter);

    FAIL_FAST_IF_FALSE(cbTextBuffer % sizeof(wchar_t) == 0);
    *pcchTextBufferWritten = cbTextBuffer/ sizeof(wchar_t);

    return HRESULT_FROM_NT(Status);
}

VOID UnblockWriteConsole(const DWORD dwReason)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.Flags &= ~dwReason;

    if (AreAllFlagsClear(gci.Flags, (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING)))
    {
        // There is no longer any reason to suspend output, so unblock it.
        gci.OutputQueue.NotifyWaiters(true);
    }
}
