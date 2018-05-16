/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "readDataCooked.hpp"
#include "dbcs.h"
#include "stream.h"
#include "misc.h"
#include "_stream.h"
#include "inputBuffer.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

#define LINE_INPUT_BUFFER_SIZE (256 * sizeof(WCHAR))

// Routine Description:
// - Constructs cooked read data class to hold context across key presses while a user is modifying their 'input line'.
// Arguments:
// - pInputBuffer - Buffer that data will be read from.
// - pInputReadHandleData - Context stored across calls from the same input handle to return partial data appropriately.
// - screenInfo - Output buffer that will be used for 'echoing' the line back to the user so they can see/manipulate it
// - BufferSize -
// - BytesRead -
// - CurrentPosition -
// - BufPtr -
// - BackupLimit -
// - UserBufferSize - The byte count of the buffer presented by the client
// - UserBuffer - The buffer that was presented by the client for filling with input data on read conclusion/return from server/host.
// - OriginalCursorPosition -
// - NumberOfVisibleChars
// - CtrlWakeupMask - Special client parameter to interrupt editing, end the wait, and return control to the client application
// - CommandHistory -
// - Echo -
// - InsertMode -
// - Processed -
// - Line -
// - pTempHandle - A handle to the output buffer to prevent it from being destroyed while we're using it to present 'edit line' text.
// Return Value:
// - THROW: Throws E_INVALIDARG for invalid pointers.
COOKED_READ_DATA::COOKED_READ_DATA(_In_ InputBuffer* const pInputBuffer,
                                   _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                                   SCREEN_INFORMATION& screenInfo,
                                   _In_ size_t UserBufferSize,
                                   _In_ PWCHAR UserBuffer,
                                   _In_ ULONG CtrlWakeupMask,
                                   _In_ COMMAND_HISTORY* CommandHistory,
                                   const std::wstring& exeName
) :
    ReadData(pInputBuffer, pInputReadHandleData),
    _screenInfo{ screenInfo },
    _BytesRead{ 0 },
    _CurrentPosition{ 0 },
    _UserBufferSize{ UserBufferSize },
    _UserBuffer{ UserBuffer },
    _OriginalCursorPosition{ -1, -1 },
    _NumberOfVisibleChars{ 0 },
    _CtrlWakeupMask{ CtrlWakeupMask },
    _CommandHistory{ CommandHistory },
    _Echo{ IsFlagSet(pInputBuffer->InputMode, ENABLE_ECHO_INPUT) },
    _InsertMode{ ServiceLocator::LocateGlobals().getConsoleInformation().GetInsertMode() },
    _Processed{ IsFlagSet(pInputBuffer->InputMode, ENABLE_PROCESSED_INPUT) },
    _Line{ IsFlagSet(pInputBuffer->InputMode, ENABLE_LINE_INPUT) },
    _tempHandle{ nullptr },
    _exeName{ exeName },
    BeforeDialogCursorPosition{ 0, 0 },
    _fIsUnicode{ false },
    ControlKeyState{ 0 },
    pdwNumBytes{ nullptr }
{
    THROW_IF_FAILED(screenInfo.Header.AllocateIoHandle(ConsoleHandleData::HandleType::Output,
                                                       GENERIC_WRITE,
                                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                       _tempHandle));

    // to emulate OS/2 KbdStringIn, we read into our own big buffer
    // (256 bytes) until the user types enter.  then return as many
    // chars as will fit in the user's buffer.
    _BufferSize = std::max(UserBufferSize, LINE_INPUT_BUFFER_SIZE);
    _buffer = std::make_unique<byte[]>(_BufferSize);
    _BackupLimit = reinterpret_cast<wchar_t*>(_buffer.get());
    _BufPtr = reinterpret_cast<wchar_t*>(_buffer.get());

    // Initialize the user's buffer to spaces. This is done so that
    // moving in the buffer via cursor doesn't do strange things.
    std::fill_n(_BufPtr, _BufferSize / sizeof(wchar_t), UNICODE_SPACE);
}

// Routine Description:
// - Destructs a read data class.
// - Decrements count of readers waiting on the given handle.
COOKED_READ_DATA::~COOKED_READ_DATA()
{
}

// Routine Description:
// - This routine is called to complete a cooked read that blocked in ReadInputBuffer.
// - The context of the read was saved in the CookedReadData structure.
// - This routine is called when events have been written to the input buffer.
// - It is called in the context of the writing thread.
// - It may be called more than once.
// Arguments:
// - TerminationReason - if this routine is called because a ctrl-c or ctrl-break was seen, this argument
//                      contains CtrlC or CtrlBreak. If the owning thread is exiting, it will have ThreadDying. Otherwise 0.
// - fIsUnicode - Whether to convert the final data to A (using Console Input CP) at the end or treat everything as Unicode (UCS-2)
// - pReplyStatus - The status code to return to the client application that originally called the API (before it was queued to wait)
// - pNumBytes - The number of bytes of data that the server/driver will need to transmit back to the client process
// - pControlKeyState - For certain types of reads, this specifies which modifier keys were held.
// - pOutputData - not used
// Return Value:
// - true if the wait is done and result buffer/status code can be sent back to the client.
// - false if we need to continue to wait until more data is available.
bool COOKED_READ_DATA::Notify(const WaitTerminationReason TerminationReason,
                              const bool fIsUnicode,
                              _Out_ NTSTATUS* const pReplyStatus,
                              _Out_ size_t* const pNumBytes,
                              _Out_ DWORD* const pControlKeyState,
                              _Out_ void* const /*pOutputData*/)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    FAIL_FAST_IF(!gci.IsConsoleLocked());

    *pNumBytes = 0;
    *pControlKeyState = 0;

    *pReplyStatus = STATUS_SUCCESS;

    FAIL_FAST_IF(_pInputReadHandleData->IsInputPending());

    // this routine should be called by a thread owning the same lock on the same console as we're reading from.
    FAIL_FAST_IF(_pInputReadHandleData->GetReadCount() == 0);

    // if ctrl-c or ctrl-break was seen, terminate read.
    if (IsAnyFlagSet(TerminationReason, (WaitTerminationReason::CtrlC | WaitTerminationReason::CtrlBreak)))
    {
        *pReplyStatus = STATUS_ALERTED;
        gci.lpCookedReadData = nullptr;
        return true;
    }

    // See if we were called because the thread that owns this wait block is exiting.
    if (IsFlagSet(TerminationReason, WaitTerminationReason::ThreadDying))
    {
        *pReplyStatus = STATUS_THREAD_IS_TERMINATING;

        // Clean up popup data structures.
        CleanUpPopups(this);
        gci.lpCookedReadData = nullptr;
        return true;
    }

    // We must see if we were woken up because the handle is being closed. If
    // so, we decrement the read count. If it goes to zero, we wake up the
    // close thread. Otherwise, we wake up any other thread waiting for data.

    if (IsFlagSet(TerminationReason, WaitTerminationReason::HandleClosing))
    {
        *pReplyStatus = STATUS_ALERTED;

        // Clean up popup data structures.
        CleanUpPopups(this);
        gci.lpCookedReadData = nullptr;
        return true;
    }

    // If we get to here, this routine was called either by the input thread
    // or a write routine. Both of these callers grab the current console
    // lock.

    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.

    FAIL_FAST_IF_FALSE(gci.IsConsoleLocked());

    // MSFT:13994975 This is REALLY weird.
    // When we're doing cooked reading for popups, we come through this method
    //   twice. Once when we press F7 to bring up the popup, then again when we
    //   press enter to input the selected command.
    // The first time, there is no popup, and we go to CookedRead. We pass into
    //   CookedRead `pNumBytes`, which is passed to us as the address of the
    //   stack variable dwNumBytes, in ConsoleWaitBlock::Notify.
    // CookedRead sets this->pdwNumBytes to that value, and starts the popup,
    //   which returns all the way up, and pops the ConsoleWaitBlock::Notify
    //   stack frame containing the address we're pointing at.
    // Then on the second time  through this function, we hit this if block,
    //   because there is a popup to get input from.
    // However, pNumBytes is now the address of a different stack frame, and not
    //   necessarily the same as before (presumably not at all). The
    //   PopupInputRoutine would try and write the number of bytes read to the
    //   value in pdwNumBytes, and then we'd return up to ConsoleWaitBlock::Notify,
    //   who's dwNumBytes had nothing in it.
    // To fix this, when we hit this with a popup, we're going to make sure to
    //   refresh the value of pdwNumBytes to the current address we want to put
    //   the out value into.
    // It's still really weird, but limits the potential fallout of changing a
    //   piece of old spaghetti code.
    if (_CommandHistory)
    {
        PCLE_POPUP Popup;
        if (!CLE_NO_POPUPS(_CommandHistory))
        {
            // (see above comment, MSFT:13994975)
            // Make sure that the popup writes the dwNumBytes to the right place
            if (pNumBytes)
            {
                pdwNumBytes = pNumBytes;
            }

            Popup = CONTAINING_RECORD(_CommandHistory->PopupList.Flink, CLE_POPUP, ListLink);
            *pReplyStatus = (Popup->PopupInputRoutine) (this, nullptr, TRUE);
            if (*pReplyStatus == CONSOLE_STATUS_READ_COMPLETE || (*pReplyStatus != CONSOLE_STATUS_WAIT && *pReplyStatus != CONSOLE_STATUS_WAIT_NO_BLOCK))
            {
                *pReplyStatus = S_OK;
                gci.lpCookedReadData = nullptr;

                return true;
            }
            return false;
        }
    }

    *pReplyStatus = Read(fIsUnicode, *pNumBytes, *pControlKeyState);
    if (*pReplyStatus != CONSOLE_STATUS_WAIT)
    {
        gci.lpCookedReadData = nullptr;
        return true;
    }
    else
    {
        return false;
    }
}

bool COOKED_READ_DATA::AtEol() const
{
    return _BytesRead == (_CurrentPosition * 2);
}

// Routine Description:
// - Method that actually retrieves a character/input record from the buffer (key press form)
//   and determines the next action based on the various possible cooked read modes.
// - Mode options include the F-keys popup menus, keyboard manipulation of the edit line, etc.
// - This method also does the actual copying of the final manipulated data into the return buffer.
// Arguments:
// - isUnicode - Treat as UCS-2 unicode or use Input CP to convert when done.
// - numBytes - On in, the number of bytes available in the client
// buffer. On out, the number of bytes consumed in the client buffer.
// - controlKeyState - For some types of reads, this is the modifier key state with the last button press.
[[nodiscard]]
HRESULT COOKED_READ_DATA::Read(const bool isUnicode,
                               size_t& numBytes,
                               ULONG& controlKeyState)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS Status = STATUS_SUCCESS;
    controlKeyState = 0;

    WCHAR Char;
    bool commandLineEditingKeys = false;
    DWORD KeyState;
    size_t NumBytes = 0;
    size_t NumToWrite;
    bool fAddDbcsLead = false;

    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = GetInputReadHandleData();
    InputBuffer* const pInputBuffer = GetInputBuffer();

    while (_BytesRead < _BufferSize)
    {
        // This call to GetChar may block.
        Status = GetChar(pInputBuffer,
                         &Char,
                         true,
                         &commandLineEditingKeys,
                         nullptr,
                         &KeyState);
        if (!NT_SUCCESS(Status))
        {
            if (Status != CONSOLE_STATUS_WAIT)
            {
                _BytesRead = 0;
            }
            break;
        }

        // we should probably set these up in GetChars, but we set them
        // up here because the debugger is multi-threaded and calls
        // read before outputting the prompt.

        if (_OriginalCursorPosition.X == -1)
        {
            _OriginalCursorPosition = _screenInfo.GetTextBuffer().GetCursor().GetPosition();
        }

        if (commandLineEditingKeys)
        {
            // TODO: this is super weird for command line popups only
            _fIsUnicode = isUnicode;

            // TODO: 5-11-2018 this is gross af. Pass it or something jeez.
            pdwNumBytes = &numBytes;

            Status = ProcessCommandLine(this, Char, KeyState);
            if (Status == CONSOLE_STATUS_READ_COMPLETE || Status == CONSOLE_STATUS_WAIT)
            {
                break;
            }
            if (!NT_SUCCESS(Status))
            {
                if (Status == CONSOLE_STATUS_WAIT_NO_BLOCK)
                {
                    Status = CONSOLE_STATUS_WAIT;
                }
                else
                {
                    _BytesRead = 0;
                }
                break;
            }
        }
        else
        {
            if (ProcessInput(Char, KeyState, Status))
            {
                gci.Flags |= CONSOLE_IGNORE_NEXT_KEYUP;
                break;
            }
        }
    }

    // if the read was completed (status != wait), free the cooked read
    // data.  also, close the temporary output handle that was opened to
    // echo the characters read.

    if (Status != CONSOLE_STATUS_WAIT)
    {
        DWORD LineCount = 1;

        if (_Echo)
        {
            // Figure out where real string ends (at carriage return or end of buffer).
            PWCHAR StringPtr = _BackupLimit;
            size_t StringLength = _BytesRead;
            bool FoundCR = false;
            for (size_t i = 0; i < (_BytesRead / sizeof(WCHAR)); i++)
            {
                if (*StringPtr++ == UNICODE_CARRIAGERETURN)
                {
                    StringLength = i * sizeof(WCHAR);
                    FoundCR = true;
                    break;
                }
            }

            if (FoundCR)
            {
                // add to command line recall list
                LOG_IF_FAILED(AddCommand(_CommandHistory,
                                         _BackupLimit,
                                         (USHORT)StringLength,
                                         IsFlagSet(gci.Flags, CONSOLE_HISTORY_NODUP)));

                // check for alias
                ProcessAliases(LineCount);
            }
        }

        // at this point, a->NumBytes contains the number of bytes in
        // the UNICODE string read.  UserBufferSize contains the converted
        // size of the app's buffer.
        if (_BytesRead > _UserBufferSize || LineCount > 1)
        {
            if (LineCount > 1)
            {
                PWSTR Tmp;
                if (!isUnicode)
                {
                    if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                    {
                        fAddDbcsLead = true;
                        std::unique_ptr<IInputEvent> event = GetInputBuffer()->FetchReadPartialByteSequence(false);
                        const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                        *_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                        _UserBuffer++;
                        _UserBufferSize -= sizeof(wchar_t);
                    }

                    NumBytes = 0;
                    for (Tmp = _BackupLimit;
                         *Tmp != UNICODE_LINEFEED && _UserBufferSize / sizeof(WCHAR) > NumBytes;
                         (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++);
                }

#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "LineCount > 1 means there's a UNICODE_LINEFEED")
                for (Tmp = _BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
                {
                    FAIL_FAST_IF_FALSE(Tmp < (_BackupLimit + _BytesRead));
                }

                numBytes = (ULONG)(Tmp - _BackupLimit + 1) * sizeof(*Tmp);
            }
            else
            {
                if (!isUnicode)
                {
                    PWSTR Tmp;

                    if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                    {
                        fAddDbcsLead = true;
                        std::unique_ptr<IInputEvent> event = GetInputBuffer()->FetchReadPartialByteSequence(false);
                        const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                        *_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                        _UserBuffer++;
                        _UserBufferSize -= sizeof(wchar_t);
                    }
                    NumBytes = 0;
                    NumToWrite = _BytesRead;
                    for (Tmp = _BackupLimit;
                         NumToWrite && _UserBufferSize / sizeof(WCHAR) > NumBytes;
                         (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++, NumToWrite -= sizeof(WCHAR));
                }
                numBytes = _UserBufferSize;
            }

            __analysis_assume(numBytes <= _UserBufferSize);
            memmove(_UserBuffer, _BackupLimit, numBytes);

            const std::string_view pending{ ((char*)_BackupLimit + numBytes), _BytesRead - numBytes };
            if (LineCount > 1)
            {
                pInputReadHandleData->SaveMultilinePendingInput(pending);
            }
            else
            {
                pInputReadHandleData->SavePendingInput(pending);
            }
        }
        else
        {
            if (!isUnicode)
            {
                PWSTR Tmp;

                if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                {
                    fAddDbcsLead = true;
                    std::unique_ptr<IInputEvent> event = GetInputBuffer()->FetchReadPartialByteSequence(false);
                    const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                    *_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                    _UserBuffer++;
                    _UserBufferSize -= sizeof(wchar_t);

                    if (_UserBufferSize == 0)
                    {
                        numBytes = 1;
                        return STATUS_SUCCESS;
                    }
                }
                NumBytes = 0;
                NumToWrite = _BytesRead;
                for (Tmp = _BackupLimit;
                     NumToWrite && _UserBufferSize / sizeof(WCHAR) > NumBytes;
                     (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++, NumToWrite -= sizeof(WCHAR));
            }

            numBytes = _BytesRead;

            if (numBytes > _UserBufferSize)
            {
                return STATUS_BUFFER_OVERFLOW;
            }

            memmove(_UserBuffer, _BackupLimit, numBytes);
        }
        controlKeyState = ControlKeyState;

        if (!isUnicode)
        {
            // if ansi, translate string.
            std::unique_ptr<char[]> tempBuffer;
            try
            {
                tempBuffer = std::make_unique<char[]>(NumBytes);
            }
            catch (...)
            {
                return STATUS_NO_MEMORY;
            }

            std::unique_ptr<IInputEvent> partialEvent;
            numBytes = TranslateUnicodeToOem(_UserBuffer,
                                             gsl::narrow<ULONG>(numBytes / sizeof(wchar_t)),
                                             tempBuffer.get(),
                                             gsl::narrow<ULONG>(NumBytes),
                                             partialEvent);

            if (partialEvent.get())
            {
                GetInputBuffer()->StoreReadPartialByteSequence(std::move(partialEvent));
            }


            if (numBytes > _UserBufferSize)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                return Status;
            }

            memmove(_UserBuffer, tempBuffer.get(), numBytes);
            if (fAddDbcsLead)
            {
                numBytes++;
            }
        }
    }

    return Status;
}

void COOKED_READ_DATA::ProcessAliases(DWORD& lineCount)
{
    Alias::s_MatchAndCopyAliasLegacy(_BackupLimit,
                                     _BytesRead,
                                     _BackupLimit,
                                     _BufferSize,
                                     &_BytesRead,
                                     _exeName,
                                     lineCount);
}

// Routine Description:
// - This method handles the various actions that occur on the edit line like pressing keys left/right/up/down, paging, and
//   the final ENTER key press that will end the wait and finally return the data.
// Arguments:
// - pCookedReadData - Pointer to cooked read data information (edit line, client buffer, etc.)
// - wch - The most recently pressed/retrieved character from the input buffer (keystroke)
// - keyState - Modifier keys/state information with the pressed key/character
// - status - The return code to pass to the client
// Return Value:
// - true if read is completed. false if we need to keep waiting and be called again with the user's next keystroke.
bool COOKED_READ_DATA::ProcessInput(const wchar_t wchOrig,
                                    const DWORD keyState,
                                    NTSTATUS& status)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    size_t NumSpaces = 0;
    SHORT ScrollY = 0;
    size_t NumToWrite;
    WCHAR wch = wchOrig;
    bool fStartFromDelim;

    status = STATUS_SUCCESS;
    if (_BytesRead >= (_BufferSize - (2 * sizeof(WCHAR))) && wch != UNICODE_CARRIAGERETURN && wch != UNICODE_BACKSPACE)
    {
        return false;
    }

    if (_CtrlWakeupMask != 0 && wch < L' ' && (_CtrlWakeupMask & (1 << wch)))
    {
        *_BufPtr = wch;
        _BytesRead += sizeof(WCHAR);
        _BufPtr += 1;
        _CurrentPosition += 1;
        ControlKeyState = keyState;
        return true;
    }

    if (wch == EXTKEY_ERASE_PREV_WORD)
    {
        wch = UNICODE_BACKSPACE;
    }

    if (AtEol())
    {
        // If at end of line, processing is relatively simple. Just store the character and write it to the screen.
        if (wch == UNICODE_BACKSPACE2)
        {
            wch = UNICODE_BACKSPACE;
        }

        if (wch != UNICODE_BACKSPACE || _BufPtr != _BackupLimit)
        {
            fStartFromDelim = IsWordDelim(_BufPtr[-1]);

        eol_repeat:
            if (_Echo)
            {
                NumToWrite = sizeof(WCHAR);
                status = WriteCharsLegacy(_screenInfo,
                                          _BackupLimit,
                                          _BufPtr,
                                          &wch,
                                          &NumToWrite,
                                          &NumSpaces,
                                          _OriginalCursorPosition.X,
                                          WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                          &ScrollY);
                if (NT_SUCCESS(status))
                {
                    _OriginalCursorPosition.Y += ScrollY;
                }
                else
                {
                    RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed %x", status);
                }
            }

            _NumberOfVisibleChars += NumSpaces;
            if (wch == UNICODE_BACKSPACE && _Processed)
            {
                _BytesRead -= sizeof(WCHAR);
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "This access is fine")
                *_BufPtr = (WCHAR)' ';
                _BufPtr -= 1;
                _CurrentPosition -= 1;

                // Repeat until it hits the word boundary
                if (wchOrig == EXTKEY_ERASE_PREV_WORD &&
                    _BufPtr != _BackupLimit &&
                    fStartFromDelim ^ !IsWordDelim(_BufPtr[-1]))
                {
                    goto eol_repeat;
                }
            }
            else
            {
                *_BufPtr = wch;
                _BytesRead += sizeof(WCHAR);
                _BufPtr += 1;
                _CurrentPosition += 1;
            }
        }
    }
    else
    {
        bool CallWrite = true;
        const SHORT sScreenBufferSizeX = _screenInfo.GetScreenBufferSize().X;

        // processing in the middle of the line is more complex:

        // calculate new cursor position
        // store new char
        // clear the current command line from the screen
        // write the new command line to the screen
        // update the cursor position

        if (wch == UNICODE_BACKSPACE && _Processed)
        {
            // for backspace, use writechars to calculate the new cursor position.
            // this call also sets the cursor to the right position for the
            // second call to writechars.

            if (_BufPtr != _BackupLimit)
            {

                fStartFromDelim = IsWordDelim(_BufPtr[-1]);

            bs_repeat:
                // we call writechar here so that cursor position gets updated
                // correctly.  we also call it later if we're not at eol so
                // that the remainder of the string can be updated correctly.

                if (_Echo)
                {
                    NumToWrite = sizeof(WCHAR);
                    status = WriteCharsLegacy(_screenInfo,
                                              _BackupLimit,
                                              _BufPtr,
                                              &wch,
                                              &NumToWrite,
                                              nullptr,
                                              _OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    if (!NT_SUCCESS(status))
                    {
                        RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed %x", status);
                    }
                }
                _BytesRead -= sizeof(WCHAR);
                _BufPtr -= 1;
                _CurrentPosition -= 1;
                memmove(_BufPtr,
                        _BufPtr + 1,
                        _BytesRead - (_CurrentPosition * sizeof(WCHAR)));
                {
                    PWCHAR buf = (PWCHAR)((PBYTE)_BackupLimit + _BytesRead);
                    *buf = (WCHAR)' ';
                }
                NumSpaces = 0;

                // Repeat until it hits the word boundary
                if (wchOrig == EXTKEY_ERASE_PREV_WORD &&
                    _BufPtr != _BackupLimit &&
                    fStartFromDelim ^ !IsWordDelim(_BufPtr[-1]))
                {
                    goto bs_repeat;
                }
            }
            else
            {
                CallWrite = false;
            }
        }
        else
        {
            // store the char
            if (wch == UNICODE_CARRIAGERETURN)
            {
                _BufPtr = (PWCHAR)((PBYTE)_BackupLimit + _BytesRead);
                *_BufPtr = wch;
                _BufPtr += 1;
                _BytesRead += sizeof(WCHAR);
                _CurrentPosition += 1;
            }
            else
            {
                bool fBisect = false;

                if (_Echo)
                {
                    if (CheckBisectProcessW(_screenInfo,
                                            _BackupLimit,
                                            _CurrentPosition + 1,
                                            sScreenBufferSizeX - _OriginalCursorPosition.X,
                                            _OriginalCursorPosition.X,
                                            TRUE))
                    {
                        fBisect = true;
                    }
                }

                if (_InsertMode)
                {
                    memmove(_BufPtr + 1,
                            _BufPtr,
                            _BytesRead - (_CurrentPosition * sizeof(WCHAR)));
                    _BytesRead += sizeof(WCHAR);
                }
                *_BufPtr = wch;
                _BufPtr += 1;
                _CurrentPosition += 1;

                // calculate new cursor position
                if (_Echo)
                {
                    NumSpaces = RetrieveNumberOfSpaces(_OriginalCursorPosition.X,
                                                       _BackupLimit,
                                                       _CurrentPosition - 1);
                    if (NumSpaces > 0 && fBisect)
                        NumSpaces--;
                }
            }
        }

        if (_Echo && CallWrite)
        {
            COORD CursorPosition;

            // save cursor position
            CursorPosition = _screenInfo.GetTextBuffer().GetCursor().GetPosition();
            CursorPosition.X = (SHORT)(CursorPosition.X + NumSpaces);

            // clear the current command line from the screen
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast doesn't like this call.")
            DeleteCommandLine(this, FALSE);

            // write the new command line to the screen
            NumToWrite = _BytesRead;

            DWORD dwFlags = WC_DESTRUCTIVE_BACKSPACE | WC_ECHO;
            if (wch == UNICODE_CARRIAGERETURN)
            {
                dwFlags |= WC_KEEP_CURSOR_VISIBLE;
            }
            status = WriteCharsLegacy(_screenInfo,
                                      _BackupLimit,
                                      _BackupLimit,
                                      _BackupLimit,
                                      &NumToWrite,
                                      &_NumberOfVisibleChars,
                                      _OriginalCursorPosition.X,
                                      dwFlags,
                                      &ScrollY);
            if (!NT_SUCCESS(status))
            {
                RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed 0x%x", status);
                _BytesRead = 0;
                return true;
            }

            // update cursor position
            if (wch != UNICODE_CARRIAGERETURN)
            {
                if (CheckBisectProcessW(_screenInfo,
                                        _BackupLimit,
                                        _CurrentPosition + 1,
                                        sScreenBufferSizeX - _OriginalCursorPosition.X,
                                        _OriginalCursorPosition.X, TRUE))
                {
                    if (CursorPosition.X == (sScreenBufferSizeX - 1))
                    {
                        CursorPosition.X++;
                    }
                }

                // adjust cursor position for WriteChars
                _OriginalCursorPosition.Y += ScrollY;
                CursorPosition.Y += ScrollY;
                status = AdjustCursorPosition(_screenInfo, CursorPosition, TRUE, nullptr);
                if (!NT_SUCCESS(status))
                {
                    _BytesRead = 0;
                    return true;
                }
            }
        }
    }

    // in cooked mode, enter (carriage return) is converted to
    // carriage return linefeed (0xda).  carriage return is always
    // stored at the end of the buffer.
    if (wch == UNICODE_CARRIAGERETURN)
    {
        if (_Processed)
        {
            if (_BytesRead < _BufferSize)
            {
                *_BufPtr = UNICODE_LINEFEED;
                if (_Echo)
                {
                    NumToWrite = sizeof(WCHAR);
                    status = WriteCharsLegacy(_screenInfo,
                                              _BackupLimit,
                                              _BufPtr,
                                              _BufPtr,
                                              &NumToWrite,
                                              nullptr,
                                              _OriginalCursorPosition.X,
                                              WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                              nullptr);
                    if (!NT_SUCCESS(status))
                    {
                        RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed 0x%x", status);
                    }
                }
                _BytesRead += sizeof(WCHAR);
                _BufPtr++;
                _CurrentPosition += 1;
            }
        }
        // reset the cursor back to 25% if necessary
        if (_Line)
        {
            if (!!_InsertMode != gci.GetInsertMode())
            {
                // Make cursor small.
                LOG_IF_FAILED(ProcessCommandLine(this, VK_INSERT, 0));
            }

            status = STATUS_SUCCESS;
            return true;
        }
    }

    return false;
}
