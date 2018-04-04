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

// Routine Description:
// - Constructs cooked read data class to hold context across key presses while a user is modifying their 'input line'.
// Arguments:
// - pInputBuffer - Buffer that data will be read from.
// - pInputReadHandleData - Context stored across calls from the same input handle to return partial data appropriately.
// - pScreenInfo - Output buffer that will be used for 'echoing' the line back to the user so they can see/manipulate it
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
                                   _In_ SCREEN_INFORMATION* pScreenInfo,
                                   _In_ ULONG BufferSize,
                                   _In_ ULONG BytesRead,
                                   _In_ ULONG CurrentPosition,
                                   _In_ PWCHAR BufPtr,
                                   _In_ PWCHAR BackupLimit,
                                   _In_ ULONG UserBufferSize,
                                   _In_ PWCHAR UserBuffer,
                                   _In_ COORD OriginalCursorPosition,
                                   _In_ DWORD NumberOfVisibleChars,
                                   _In_ ULONG CtrlWakeupMask,
                                   _In_ COMMAND_HISTORY* CommandHistory,
                                   _In_ BOOLEAN Echo,
                                   _In_ BOOLEAN InsertMode,
                                   _In_ BOOLEAN Processed,
                                   _In_ BOOLEAN Line,
                                   _In_ ConsoleHandleData* pTempHandle
    ) :
    ReadData(pInputBuffer, pInputReadHandleData),
    _pScreenInfo{ pScreenInfo },
    _BufferSize{ BufferSize},
    _BytesRead{ BytesRead},
    _CurrentPosition{ CurrentPosition},
    _BufPtr{ BufPtr },
    _BackupLimit{ BackupLimit },
    _UserBufferSize{ UserBufferSize },
    _UserBuffer{ UserBuffer },
    _OriginalCursorPosition(OriginalCursorPosition),
    _NumberOfVisibleChars{ NumberOfVisibleChars },
    _CtrlWakeupMask{ CtrlWakeupMask },
    _CommandHistory{ CommandHistory },
    _Echo{ Echo },
    _InsertMode{ InsertMode },
    _Processed{ Processed },
    _Line{ Line },
    _pTempHandle{ pTempHandle },
    ExeName{ nullptr },
    ExeNameLength{ 0 }

{
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
// - TRUE if the wait is done and result buffer/status code can be sent back to the client.
// - FALSE if we need to continue to wait until more data is available.
bool COOKED_READ_DATA::Notify(_In_ WaitTerminationReason const TerminationReason,
                              _In_ bool const fIsUnicode,
                              _Out_ NTSTATUS* const pReplyStatus,
                              _Out_ DWORD* const pNumBytes,
                              _Out_ DWORD* const pControlKeyState,
                              _Out_ void* const /*pOutputData*/)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    ASSERT(gci.IsConsoleLocked());

    *pNumBytes = 0;
    *pControlKeyState = 0;

    *pReplyStatus = STATUS_SUCCESS;

    ASSERT(IsFlagClear(_pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending));

    // this routine should be called by a thread owning the same lock on the same console as we're reading from.
#ifdef DBG
    _pInputReadHandleData->LockReadCount();
    ASSERT(_pInputReadHandleData->GetReadCount() > 0);
    _pInputReadHandleData->UnlockReadCount();
#endif

    // if ctrl-c or ctrl-break was seen, terminate read.
    if (IsAnyFlagSet(TerminationReason, (WaitTerminationReason::CtrlC | WaitTerminationReason::CtrlBreak)))
    {
        *pReplyStatus = STATUS_ALERTED;
        delete[] _BackupLimit;
        delete[] ExeName;
        gci.lpCookedReadData = nullptr;
        LOG_IF_FAILED(_pTempHandle->CloseHandle());
        return true;
    }

    // See if we were called because the thread that owns this wait block is exiting.
    if (IsFlagSet(TerminationReason, WaitTerminationReason::ThreadDying))
    {
        *pReplyStatus = STATUS_THREAD_IS_TERMINATING;

        // Clean up popup data structures.
        CleanUpPopups(this);
        delete[] _BackupLimit;
        delete[] ExeName;
        gci.lpCookedReadData = nullptr;
        LOG_IF_FAILED(_pTempHandle->CloseHandle());
        return true;
    }

    // We must see if we were woken up because the handle is being closed. If
    // so, we decrement the read count. If it goes to zero, we wake up the
    // close thread. Otherwise, we wake up any other thread waiting for data.

    if (IsFlagSet(_pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::Closing))
    {
        *pReplyStatus = STATUS_ALERTED;

        // Clean up popup data structures.
        CleanUpPopups(this);
        delete[] _BackupLimit;
        delete[] ExeName;
        gci.lpCookedReadData = nullptr;
        LOG_IF_FAILED(_pTempHandle->CloseHandle());
        return true;
    }

    // If we get to here, this routine was called either by the input thread
    // or a write routine. Both of these callers grab the current console
    // lock.

    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.

    ASSERT(gci.IsConsoleLocked());

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
                // if there is more input that is pending from the popup, it will be saved in
                // _pInputReadHandleData by giving it a pointer to _BackupLimit. don't delete _BackupLimit
                // unless all data was read.
                if (IsFlagClear(_pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending))
                {
                    delete[] _BackupLimit;
                }
                delete[] ExeName;
                gci.lpCookedReadData = nullptr;
                LOG_IF_FAILED(_pTempHandle->CloseHandle());

                return true;
            }
            return false;
        }
    }

    *pReplyStatus = CookedRead(this, fIsUnicode, pNumBytes, pControlKeyState);
    if (*pReplyStatus != CONSOLE_STATUS_WAIT)
    {
        gci.lpCookedReadData = nullptr;
        LOG_IF_FAILED(_pTempHandle->CloseHandle());
        return true;
    }
    else
    {
        return false;
    }
}

// Routine Description:
// - Method that actually retrieves a character/input record from the buffer (key press form)
//   and determines the next action based on the various possible cooked read modes.
// - Mode options include the F-keys popup menus, keyboard manipulation of the edit line, etc.
// - This method also does the actual copying of the final manipulated data into the return buffer.
// Arguments:
// - pCookedReadData - Pointer to cooked read data information (edit line, client buffer, etc.)
// - fIsUnicode - Treat as UCS-2 unicode or use Input CP to convert when done.
// - cbNumBytes - On in, the number of bytes available in the client
// buffer. On out, the number of bytes consumed in the client buffer.
// - ulControlKeyState - For some types of reads, this is the modifier key state with the last button press.
[[nodiscard]]
NTSTATUS CookedRead(_In_ COOKED_READ_DATA* const pCookedReadData,
                    _In_ bool const fIsUnicode,
                    _Inout_ ULONG* const cbNumBytes,
                    _Out_ ULONG* const ulControlKeyState)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    NTSTATUS Status = STATUS_SUCCESS;
    *ulControlKeyState = 0;

    WCHAR Char;
    bool commandLineEditingKeys = false;
    DWORD KeyState;
    DWORD NumBytes = 0;
    ULONG NumToWrite;
    BOOL fAddDbcsLead = FALSE;

    INPUT_READ_HANDLE_DATA* const pInputReadHandleData = pCookedReadData->GetInputReadHandleData();
    InputBuffer* const pInputBuffer = pCookedReadData->GetInputBuffer();

    while (pCookedReadData->_BytesRead < pCookedReadData->_BufferSize)
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
                pCookedReadData->_BytesRead = 0;
            }
            break;
        }

        // we should probably set these up in GetChars, but we set them
        // up here because the debugger is multi-threaded and calls
        // read before outputting the prompt.

        if (pCookedReadData->_OriginalCursorPosition.X == -1)
        {
            pCookedReadData->_OriginalCursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();
        }

        if (commandLineEditingKeys)
        {
            // TODO: this is super weird for command line popups only
            pCookedReadData->_fIsUnicode = !!fIsUnicode;
            pCookedReadData->pdwNumBytes = cbNumBytes;

            Status = ProcessCommandLine(pCookedReadData, Char, KeyState);
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
                    pCookedReadData->_BytesRead = 0;
                }
                break;
            }
        }
        else
        {
            if (ProcessCookedReadInput(pCookedReadData, Char, KeyState, &Status))
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

        if (pCookedReadData->_Echo)
        {
            BOOLEAN FoundCR;
            ULONG i, StringLength;
            PWCHAR StringPtr;

            // Figure out where real string ends (at carriage return or end of buffer).
            StringPtr = pCookedReadData->_BackupLimit;
            StringLength = pCookedReadData->_BytesRead;
            FoundCR = FALSE;
            for (i = 0; i < (pCookedReadData->_BytesRead / sizeof(WCHAR)); i++)
            {
                if (*StringPtr++ == UNICODE_CARRIAGERETURN)
                {
                    StringLength = i * sizeof(WCHAR);
                    FoundCR = TRUE;
                    break;
                }
            }

            if (FoundCR)
            {
                // add to command line recall list
                LOG_IF_FAILED(AddCommand(pCookedReadData->_CommandHistory,
                                         pCookedReadData->_BackupLimit,
                                         (USHORT)StringLength,
                                         IsFlagSet(gci.Flags, CONSOLE_HISTORY_NODUP)));

                // check for alias
                i = pCookedReadData->_BufferSize;
                if (NT_SUCCESS(MatchAndCopyAlias(pCookedReadData->_BackupLimit,
                                                 (USHORT)StringLength,
                                                 pCookedReadData->_BackupLimit,
                                                 (PUSHORT)& i,
                                                 pCookedReadData->ExeName,
                                                 pCookedReadData->ExeNameLength,
                                                 &LineCount)))
                {
                    pCookedReadData->_BytesRead = i;
                }
            }
        }

        // at this point, a->NumBytes contains the number of bytes in
        // the UNICODE string read.  UserBufferSize contains the converted
        // size of the app's buffer.
        if (pCookedReadData->_BytesRead > pCookedReadData->_UserBufferSize || LineCount > 1)
        {
            if (LineCount > 1)
            {
                PWSTR Tmp;

                SetFlag(pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::MultiLineInput);
                if (!fIsUnicode)
                {
                    if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                    {
                        fAddDbcsLead = TRUE;
                        std::unique_ptr<IInputEvent> event = pCookedReadData->GetInputBuffer()->FetchReadPartialByteSequence(false);
                        const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                        *pCookedReadData->_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                        pCookedReadData->_UserBuffer++;
                        pCookedReadData->_UserBufferSize -= sizeof(wchar_t);
                    }

                    NumBytes = 0;
                    for (Tmp = pCookedReadData->_BackupLimit;
                         *Tmp != UNICODE_LINEFEED && pCookedReadData->_UserBufferSize / sizeof(WCHAR) > NumBytes;
                         (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++);
                }

#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "LineCount > 1 means there's a UNICODE_LINEFEED")
                for (Tmp = pCookedReadData->_BackupLimit; *Tmp != UNICODE_LINEFEED; Tmp++)
                {
                    ASSERT(Tmp < (pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead));
                }

                *cbNumBytes = (ULONG)(Tmp - pCookedReadData->_BackupLimit + 1) * sizeof(*Tmp);
            }
            else
            {
                if (!fIsUnicode)
                {
                    PWSTR Tmp;

                    if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                    {
                        fAddDbcsLead = TRUE;
                        std::unique_ptr<IInputEvent> event = pCookedReadData->GetInputBuffer()->FetchReadPartialByteSequence(false);
                        const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                        *pCookedReadData->_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                        pCookedReadData->_UserBuffer++;
                        pCookedReadData->_UserBufferSize -= sizeof(wchar_t);
                    }
                    NumBytes = 0;
                    NumToWrite = pCookedReadData->_BytesRead;
                    for (Tmp = pCookedReadData->_BackupLimit;
                         NumToWrite && pCookedReadData->_UserBufferSize / sizeof(WCHAR) > NumBytes;
                         (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++, NumToWrite -= sizeof(WCHAR));
                }
                *cbNumBytes = pCookedReadData->_UserBufferSize;
            }


            SetFlag(pInputReadHandleData->InputHandleFlags, INPUT_READ_HANDLE_DATA::HandleFlags::InputPending);
            pInputReadHandleData->BufPtr = pCookedReadData->_BackupLimit;
            pInputReadHandleData->BytesAvailable = pCookedReadData->_BytesRead - *cbNumBytes;
            pInputReadHandleData->CurrentBufPtr = (PWCHAR)((PBYTE)pCookedReadData->_BackupLimit + *cbNumBytes);
            __analysis_assume(*cbNumBytes <= pCookedReadData->_UserBufferSize);
            memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, *cbNumBytes);
        }
        else
        {
            if (!fIsUnicode)
            {
                PWSTR Tmp;

                if (pInputBuffer->IsReadPartialByteSequenceAvailable())
                {
                    fAddDbcsLead = TRUE;
                    std::unique_ptr<IInputEvent> event = pCookedReadData->GetInputBuffer()->FetchReadPartialByteSequence(false);
                    const KeyEvent* const pKeyEvent = static_cast<const KeyEvent* const>(event.get());
                    *pCookedReadData->_UserBuffer = static_cast<char>(pKeyEvent->GetCharData());
                    pCookedReadData->_UserBuffer++;
                    pCookedReadData->_UserBufferSize -= sizeof(wchar_t);

                    if (pCookedReadData->_UserBufferSize == 0)
                    {
                        *cbNumBytes = 1;
                        delete[] pCookedReadData->_BackupLimit;
                        return STATUS_SUCCESS;
                    }
                }
                NumBytes = 0;
                NumToWrite = pCookedReadData->_BytesRead;
                for (Tmp = pCookedReadData->_BackupLimit;
                     NumToWrite && pCookedReadData->_UserBufferSize / sizeof(WCHAR) > NumBytes;
                     (IsCharFullWidth(*Tmp) ? NumBytes += 2 : NumBytes++), Tmp++, NumToWrite -= sizeof(WCHAR));
            }

            *cbNumBytes = pCookedReadData->_BytesRead;

            if (*cbNumBytes > pCookedReadData->_UserBufferSize)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                ASSERT(false);
                delete[] pCookedReadData->_BackupLimit;
                return Status;
            }

            memmove(pCookedReadData->_UserBuffer, pCookedReadData->_BackupLimit, *cbNumBytes);
            delete[] pCookedReadData->_BackupLimit;
        }
        *ulControlKeyState = pCookedReadData->ControlKeyState;

        if (!fIsUnicode)
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
            *cbNumBytes = TranslateUnicodeToOem(pCookedReadData->_UserBuffer,
                                                *cbNumBytes / sizeof(wchar_t),
                                                tempBuffer.get(),
                                                NumBytes,
                                                partialEvent);

            if (partialEvent.get())
            {
                pCookedReadData->GetInputBuffer()->StoreReadPartialByteSequence(std::move(partialEvent));
            }


            if (*cbNumBytes > pCookedReadData->_UserBufferSize)
            {
                Status = STATUS_BUFFER_OVERFLOW;
                ASSERT(false);
                return Status;
            }

            memmove(pCookedReadData->_UserBuffer, tempBuffer.get(), *cbNumBytes);
            if (fAddDbcsLead)
            {
                (*cbNumBytes)++;
            }
        }

        delete[] pCookedReadData->ExeName;
    }

    return Status;
}

// Routine Description:
// - This method handles the various actions that occur on the edit line like pressing keys left/right/up/down, paging, and
//   the final ENTER key press that will end the wait and finally return the data.
// Arguments:
// - pCookedReadData - Pointer to cooked read data information (edit line, client buffer, etc.)
// - wch - The most recently pressed/retrieved character from the input buffer (keystroke)
// - dwKeyState - Modifier keys/state information with the pressed key/character
// - pStatus - The return code to pass to the client
// Return Value:
// - TRUE if read is completed. FALSE if we need to keep waiting and be called again with the user's next keystroke.
BOOL ProcessCookedReadInput(_In_ COOKED_READ_DATA* pCookedReadData,
                            _In_ WCHAR wch,
                            _In_ const DWORD dwKeyState,
                            _Out_ NTSTATUS* pStatus)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    DWORD NumSpaces = 0;
    SHORT ScrollY = 0;
    ULONG NumToWrite;
    WCHAR wchOrig = wch;
    BOOL fStartFromDelim;

    *pStatus = STATUS_SUCCESS;
    if (pCookedReadData->_BytesRead >= (pCookedReadData->_BufferSize - (2 * sizeof(WCHAR))) && wch != UNICODE_CARRIAGERETURN && wch != UNICODE_BACKSPACE)
    {
        return FALSE;
    }

    if (pCookedReadData->_CtrlWakeupMask != 0 && wch < L' ' && (pCookedReadData->_CtrlWakeupMask & (1 << wch)))
    {
        *pCookedReadData->_BufPtr = wch;
        pCookedReadData->_BytesRead += sizeof(WCHAR);
        pCookedReadData->_BufPtr += 1;
        pCookedReadData->_CurrentPosition += 1;
        pCookedReadData->ControlKeyState = dwKeyState;
        return TRUE;
    }

    if (wch == EXTKEY_ERASE_PREV_WORD)
    {
        wch = UNICODE_BACKSPACE;
    }

    if (AT_EOL(pCookedReadData))
    {
        // If at end of line, processing is relatively simple. Just store the character and write it to the screen.
        if (wch == UNICODE_BACKSPACE2)
        {
            wch = UNICODE_BACKSPACE;
        }

        if (wch != UNICODE_BACKSPACE || pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit)
        {
            fStartFromDelim = IsWordDelim(pCookedReadData->_BufPtr[-1]);

        eol_repeat:
            if (pCookedReadData->_Echo)
            {
                NumToWrite = sizeof(WCHAR);
                *pStatus = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_BufPtr,
                                            &wch,
                                            &NumToWrite,
                                            &NumSpaces,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                            &ScrollY);
                if (NT_SUCCESS(*pStatus))
                {
                    pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                }
                else
                {
                    RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed %x", *pStatus);
                }
            }

            pCookedReadData->_NumberOfVisibleChars += NumSpaces;
            if (wch == UNICODE_BACKSPACE && pCookedReadData->_Processed)
            {
                pCookedReadData->_BytesRead -= sizeof(WCHAR);
#pragma prefast(suppress:__WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "This access is fine")
                *pCookedReadData->_BufPtr = (WCHAR)' ';
                pCookedReadData->_BufPtr -= 1;
                pCookedReadData->_CurrentPosition -= 1;

                // Repeat until it hits the word boundary
                if (wchOrig == EXTKEY_ERASE_PREV_WORD &&
                    pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit &&
                    fStartFromDelim ^ !IsWordDelim(pCookedReadData->_BufPtr[-1]))
                {
                    goto eol_repeat;
                }
            }
            else
            {
                *pCookedReadData->_BufPtr = wch;
                pCookedReadData->_BytesRead += sizeof(WCHAR);
                pCookedReadData->_BufPtr += 1;
                pCookedReadData->_CurrentPosition += 1;
            }
        }
    }
    else
    {
        BOOL CallWrite = TRUE;
        const SHORT sScreenBufferSizeX = pCookedReadData->_pScreenInfo->GetScreenBufferSize().X;

        // processing in the middle of the line is more complex:

        // calculate new cursor position
        // store new char
        // clear the current command line from the screen
        // write the new command line to the screen
        // update the cursor position

        if (wch == UNICODE_BACKSPACE && pCookedReadData->_Processed)
        {
            // for backspace, use writechars to calculate the new cursor position.
            // this call also sets the cursor to the right position for the
            // second call to writechars.

            if (pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit)
            {

                fStartFromDelim = IsWordDelim(pCookedReadData->_BufPtr[-1]);

            bs_repeat:
                // we call writechar here so that cursor position gets updated
                // correctly.  we also call it later if we're not at eol so
                // that the remainder of the string can be updated correctly.

                if (pCookedReadData->_Echo)
                {
                    NumToWrite = sizeof(WCHAR);
                    *pStatus = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                                pCookedReadData->_BackupLimit,
                                                pCookedReadData->_BufPtr,
                                                &wch,
                                                &NumToWrite,
                                                nullptr,
                                                pCookedReadData->_OriginalCursorPosition.X,
                                                WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                nullptr);
                    if (!NT_SUCCESS(*pStatus))
                    {
                        RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed %x", *pStatus);
                    }
                }
                pCookedReadData->_BytesRead -= sizeof(WCHAR);
                pCookedReadData->_BufPtr -= 1;
                pCookedReadData->_CurrentPosition -= 1;
                memmove(pCookedReadData->_BufPtr,
                        pCookedReadData->_BufPtr + 1,
                        pCookedReadData->_BytesRead - (pCookedReadData->_CurrentPosition * sizeof(WCHAR)));
                {
                    PWCHAR buf = (PWCHAR)((PBYTE)pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead);
                    *buf = (WCHAR)' ';
                }
                NumSpaces = 0;

                // Repeat until it hits the word boundary
                if (wchOrig == EXTKEY_ERASE_PREV_WORD &&
                    pCookedReadData->_BufPtr != pCookedReadData->_BackupLimit &&
                    fStartFromDelim ^ !IsWordDelim(pCookedReadData->_BufPtr[-1]))
                {
                    goto bs_repeat;
                }
            }
            else
            {
                CallWrite = FALSE;
            }
        }
        else
        {
            // store the char
            if (wch == UNICODE_CARRIAGERETURN)
            {
                pCookedReadData->_BufPtr = (PWCHAR)((PBYTE)pCookedReadData->_BackupLimit + pCookedReadData->_BytesRead);
                *pCookedReadData->_BufPtr = wch;
                pCookedReadData->_BufPtr += 1;
                pCookedReadData->_BytesRead += sizeof(WCHAR);
                pCookedReadData->_CurrentPosition += 1;
            }
            else
            {
                BOOL fBisect = FALSE;

                if (pCookedReadData->_Echo)
                {
                    if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                            pCookedReadData->_BackupLimit,
                                            pCookedReadData->_CurrentPosition + 1,
                                            sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                            pCookedReadData->_OriginalCursorPosition.X,
                                            TRUE))
                    {
                        fBisect = TRUE;
                    }
                }

                if (pCookedReadData->_InsertMode)
                {
                    memmove(pCookedReadData->_BufPtr + 1,
                            pCookedReadData->_BufPtr,
                            pCookedReadData->_BytesRead - (pCookedReadData->_CurrentPosition * sizeof(WCHAR)));
                    pCookedReadData->_BytesRead += sizeof(WCHAR);
                }
                *pCookedReadData->_BufPtr = wch;
                pCookedReadData->_BufPtr += 1;
                pCookedReadData->_CurrentPosition += 1;

                // calculate new cursor position
                if (pCookedReadData->_Echo)
                {
                    NumSpaces = RetrieveNumberOfSpaces(pCookedReadData->_OriginalCursorPosition.X,
                                                       pCookedReadData->_BackupLimit,
                                                       pCookedReadData->_CurrentPosition - 1);
                    if (NumSpaces > 0 && fBisect)
                        NumSpaces--;
                }
            }
        }

        if (pCookedReadData->_Echo && CallWrite)
        {
            COORD CursorPosition;

            // save cursor position
            CursorPosition = pCookedReadData->_pScreenInfo->TextInfo->GetCursor()->GetPosition();
            CursorPosition.X = (SHORT)(CursorPosition.X + NumSpaces);

            // clear the current command line from the screen
#pragma prefast(suppress:__WARNING_BUFFER_OVERFLOW, "Not sure why prefast doesn't like this call.")
            DeleteCommandLine(pCookedReadData, FALSE);

            // write the new command line to the screen
            NumToWrite = pCookedReadData->_BytesRead;

            DWORD dwFlags = WC_DESTRUCTIVE_BACKSPACE | WC_ECHO;
            if (wch == UNICODE_CARRIAGERETURN)
            {
                dwFlags |= WC_KEEP_CURSOR_VISIBLE;
            }
            *pStatus = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_BackupLimit,
                                        &NumToWrite,
                                        &pCookedReadData->_NumberOfVisibleChars,
                                        pCookedReadData->_OriginalCursorPosition.X,
                                        dwFlags,
                                        &ScrollY);
            if (!NT_SUCCESS(*pStatus))
            {
                RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed 0x%x", *pStatus);
                pCookedReadData->_BytesRead = 0;
                return TRUE;
            }

            // update cursor position
            if (wch != UNICODE_CARRIAGERETURN)
            {
                if (CheckBisectProcessW(pCookedReadData->_pScreenInfo,
                                        pCookedReadData->_BackupLimit,
                                        pCookedReadData->_CurrentPosition + 1,
                                        sScreenBufferSizeX - pCookedReadData->_OriginalCursorPosition.X,
                                        pCookedReadData->_OriginalCursorPosition.X, TRUE))
                {
                    if (CursorPosition.X == (sScreenBufferSizeX - 1))
                    {
                        CursorPosition.X++;
                    }
                }

                // adjust cursor position for WriteChars
                pCookedReadData->_OriginalCursorPosition.Y += ScrollY;
                CursorPosition.Y += ScrollY;
                *pStatus = AdjustCursorPosition(pCookedReadData->_pScreenInfo, CursorPosition, TRUE, nullptr);
                ASSERT(NT_SUCCESS(*pStatus));
                if (!NT_SUCCESS(*pStatus))
                {
                    pCookedReadData->_BytesRead = 0;
                    return TRUE;
                }
            }
        }
    }

    // in cooked mode, enter (carriage return) is converted to
    // carriage return linefeed (0xda).  carriage return is always
    // stored at the end of the buffer.
    if (wch == UNICODE_CARRIAGERETURN)
    {
        if (pCookedReadData->_Processed)
        {
            if (pCookedReadData->_BytesRead < pCookedReadData->_BufferSize)
            {
                *pCookedReadData->_BufPtr = UNICODE_LINEFEED;
                if (pCookedReadData->_Echo)
                {
                    NumToWrite = sizeof(WCHAR);
                    *pStatus = WriteCharsLegacy(pCookedReadData->_pScreenInfo,
                                                pCookedReadData->_BackupLimit,
                                                pCookedReadData->_BufPtr,
                                                pCookedReadData->_BufPtr,
                                                &NumToWrite,
                                                nullptr,
                                                pCookedReadData->_OriginalCursorPosition.X,
                                                WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                                nullptr);
                    if (!NT_SUCCESS(*pStatus))
                    {
                        RIPMSG1(RIP_WARNING, "WriteCharsLegacy failed 0x%x", *pStatus);
                    }
                }
                pCookedReadData->_BytesRead += sizeof(WCHAR);
                pCookedReadData->_BufPtr++;
                pCookedReadData->_CurrentPosition += 1;
            }
        }
        // reset the cursor back to 25% if necessary
        if (pCookedReadData->_Line)
        {
            if (pCookedReadData->_InsertMode != gci.GetInsertMode())
            {
                // Make cursor small.
                LOG_IF_FAILED(ProcessCommandLine(pCookedReadData, VK_INSERT, 0));
            }

            *pStatus = STATUS_SUCCESS;
            return TRUE;
        }
    }

    return FALSE;
}
