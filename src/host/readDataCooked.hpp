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
                     _In_ size_t UserBufferSize,
                     _In_ PWCHAR UserBuffer,
                     _In_ ULONG CtrlWakeupMask,
                     _In_ CommandHistory* CommandHistory,
                     const std::wstring& exeName
        );
    ~COOKED_READ_DATA() override;
    COOKED_READ_DATA(COOKED_READ_DATA&&) = default;

    bool AtEol() const noexcept;

    bool Notify(const WaitTerminationReason TerminationReason,
                const bool fIsUnicode,
                _Out_ NTSTATUS* const pReplyStatus,
                _Out_ size_t* const pNumBytes,
                _Out_ DWORD* const pControlKeyState,
                _Out_ void* const pOutputData) override;

    gsl::span<wchar_t> SpanAtPointer();
    gsl::span<wchar_t> SpanWholeBuffer();

// TODO MSFT:11285829 member variable should be made private where possible.
    size_t _BufferSize;
    size_t _BytesRead;
    size_t _CurrentPosition;  // char position, not byte position
    PWCHAR _BufPtr; // current position to insert chars at
    // should be const. the first char of the buffer
    PWCHAR  _BackupLimit;
    size_t _UserBufferSize;   // doubled size in ansi case
    PWCHAR _UserBuffer;

    size_t* pdwNumBytes;

    void ProcessAliases(DWORD& lineCount);

    [[nodiscard]]
    HRESULT Read(const bool isUnicode,
                 size_t& numBytes,
                 ULONG& controlKeyState);

    bool ProcessInput(const wchar_t wch,
                      const DWORD keyState,
                      NTSTATUS& status);

    void EndCurrentPopup();
    void CleanUpAllPopups();

    CommandHistory& History() noexcept;
    bool HasHistory() const noexcept;

    const size_t& VisibleCharCount() const noexcept;
    size_t& VisibleCharCount() noexcept;

    SCREEN_INFORMATION& ScreenInfo() noexcept;

    const COORD& OriginalCursorPosition() const noexcept;
    COORD& OriginalCursorPosition() noexcept;

    COORD& BeforeDialogCursorPosition() noexcept;

    bool IsEchoInput() const noexcept;
    bool IsInsertMode() const noexcept;
    void SetInsertMode(const bool mode) noexcept;
    bool IsUnicode() const noexcept;

// TODO MSFT:11285829 this is a temporary kludge until the constructors are ironed
// out, so that we can still run the tests in the meantime.
#if UNIT_TESTING
    COOKED_READ_DATA(SCREEN_INFORMATION& screenInfo);
    friend class CommandLineTests;
#endif

private:
    std::unique_ptr<byte[]> _buffer;
    std::wstring _exeName;
    std::unique_ptr<ConsoleHandleData> _tempHandle;

    // TODO MSFT:11285829 make this something other than a deletable pointer
    // non-ownership pointer
    CommandHistory* _commandHistory;

    ULONG _controlKeyState;
    ULONG _ctrlWakeupMask;
    size_t _visibleCharCount; // TODO MSFT:11285829 is this cells or glyphs? ie. is a wide char counted as 1 or 2?
    SCREEN_INFORMATION& _screenInfo;
    COORD _originalCursorPosition; // TODO MSFT:11285829 original to what? the beginning of the prompt?
    COORD _beforeDialogCursorPosition; // Currently only used for F9 (ProcessCommandNumberInput) since it's the only pop-up to move the cursor when it starts.

    const bool _echoInput;
    const bool _lineInput;
    const bool _processedInput;
    bool _insertMode;
    bool _unicode;
};
