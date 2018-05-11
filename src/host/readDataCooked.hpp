/*++
Copyright (c) Microsoft Corporation

Module Name:
- readDataCooked.hpp

Abstract:
- This file defines the read data structure for reading the command line.
- Cooked reads specifically refer to when the console host acts as a command line on behalf
  of another console application (e.g. aliases, command history, completion, line manipulation, etc.)
- The data struct will help store context across multiple calls or in the case of a wait condition.
- Wait conditions happen frequently for cooked reads because they're virtually always waiting for
  the user to finish "manipulating" the edit line before hitting enter and submitting the final
  result to the client application.
- A cooked read is also limited specifically to string/textual information. Only keyboard-type input applies.
- This can be triggered via ReadConsole A/W and ReadFile A/W calls.

Author:
- Austin Diviness (AustDi) 1-Mar-2017
- Michael Niksa (MiNiksa) 1-Mar-2017

Revision History:
- Pulled from original authoring by Therese Stowell (ThereseS, 1990)
- Separated from cmdline.h/cmdline.cpp (AustDi, 2017)
--*/

#pragma once

#include "readData.hpp"

class COOKED_READ_DATA final : public ReadData
{
public:
    COOKED_READ_DATA(_In_ InputBuffer* const pInputBuffer,
                     _In_ INPUT_READ_HANDLE_DATA* const pInputReadHandleData,
                     SCREEN_INFORMATION& screenInfo,
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
                     _In_ bool Echo,
                     _In_ bool InsertMode,
                     _In_ bool Processed,
                     _In_ bool Line,
                     _In_ ConsoleHandleData* pTempHandle,
                     const std::wstring& exeName
        );
    ~COOKED_READ_DATA() override;
    COOKED_READ_DATA(COOKED_READ_DATA&&) = default;

    bool AtEol() const;

    bool Notify(const WaitTerminationReason TerminationReason,
                const bool fIsUnicode,
                _Out_ NTSTATUS* const pReplyStatus,
                _Out_ DWORD* const pNumBytes,
                _Out_ DWORD* const pControlKeyState,
                _Out_ void* const pOutputData) override;

// TODO MSFT:11285829 member variable should be made private where possible.
    SCREEN_INFORMATION& _screenInfo;
    ULONG _BufferSize;
    ULONG _BytesRead;
    ULONG _CurrentPosition;  // char position, not byte position
    PWCHAR _BufPtr;
    // should be const. the first char of the buffer
    PWCHAR /*const*/ _BackupLimit;
    ULONG _UserBufferSize;   // doubled size in ansi case
    PWCHAR _UserBuffer;
    COORD _OriginalCursorPosition;
    DWORD _NumberOfVisibleChars;
    ULONG _CtrlWakeupMask;
    PCOMMAND_HISTORY _CommandHistory;
    bool _Echo;
    bool _InsertMode;
    bool _Processed;
    bool _Line;
    ConsoleHandleData* _pTempHandle;

// TODO MSFT:11285829 these variables need to be added to the
// constructor or otherwise handled during object construction.
    ULONG ControlKeyState;
    COORD BeforeDialogCursorPosition; // Currently only used for F9 (ProcessCommandNumberInput) since it's the only pop-up to move the cursor when it starts.
    bool _fIsUnicode;
    DWORD* pdwNumBytes;

    void ProcessAliases(DWORD& lineCount);

    [[nodiscard]]
    HRESULT Read(const bool isUnicode,
                 ULONG& numBytes,
                 ULONG& controlKeyState);

    bool ProcessInput(const wchar_t wch,
                      const DWORD keyState,
                      NTSTATUS& status);

// TODO MSFT:11285829 this is a temporary kludge until the constructors are ironed
// out, so that we can still run the tests in the meantime.
#if UNIT_TESTING
    COOKED_READ_DATA(SCREEN_INFORMATION& screenInfo) :
        _screenInfo{ screenInfo }
    {
    }
#endif

private:
    std::wstring _exeName;
};
