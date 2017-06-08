/*++
Copyright (c) Microsoft Corporation

Module Name:
- outputStream.hpp

Abstract:
- Classes to process text written into the console on the attached application's output stream (usually STDOUT).

Author:
- Michael Niksa <miniksa> July 27 2015
--*/

#pragma once

#include "..\terminal\adapter\adaptDefaults.hpp"

class SCREEN_INFORMATION;

// The WriteBuffer class provides helpers for writing text into the TEXT_BUFFER_INFO that is backing a particular console screen buffer.
class WriteBuffer : public Microsoft::Console::VirtualTerminal::AdaptDefaults
{
public:
    WriteBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);

    // Implement Adapter callbacks for default cases (non-escape sequences)
    void Print(_In_ wchar_t const wch);
    void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);
    void Execute(_In_ wchar_t const wch);

    NTSTATUS GetResult() { return _ntstatus; };

    void SetActiveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);
private:
    void _DefaultCase(_In_ wchar_t const wch);
    void _DefaultStringCase(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);

    SCREEN_INFORMATION* _dcsi;
    NTSTATUS _ntstatus;
};

#include "..\terminal\adapter\conGetSet.hpp"

#include "inputBuffer.hpp"

// The ConhostInternalGetSet is for the Conhost process to call the entrypoints for its own Get/Set APIs.
// Normally, these APIs are accessible from the outside of the conhost process (like by the process being "hosted") through
// the kernelbase/32 exposed public APIs and routed by the console driver (condrv) to this console host.
// But since we're trying to call them from *inside* the console host itself, we need to get in the way and route them straight to the
// v-table inside this process instance.
class ConhostInternalGetSet : public Microsoft::Console::VirtualTerminal::ConGetSet
{
public:
    ConhostInternalGetSet(_In_ SCREEN_INFORMATION* const pScreenInfo, _In_ InputBuffer* const pInputBuffer);

    virtual BOOL GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const;
    virtual BOOL SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const;

    virtual BOOL SetConsoleCursorPosition(_In_ COORD const coordCursorPosition);

    virtual BOOL GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const;
    virtual BOOL SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo);

    virtual BOOL FillConsoleOutputCharacterW(_In_ WCHAR const wch,
                                             _In_ DWORD const nLength,
                                             _In_ COORD const dwWriteCoord,
                                             _Out_ DWORD* const pNumberOfCharsWritten);
    virtual BOOL FillConsoleOutputAttribute(_In_ WORD const wAttribute,
                                            _In_ DWORD const nLength,
                                            _In_ COORD const dwWriteCoord,
                                            _Out_ DWORD* const pNumberOfAttrsWritten);

    virtual BOOL SetConsoleTextAttribute(_In_ WORD const wAttr);
    virtual BOOL PrivateSetLegacyAttributes(_In_ WORD const wAttr, _In_ const bool fForeground, _In_ const bool fBackground, _In_ const bool fMeta);
    virtual BOOL SetConsoleXtermTextAttribute(_In_ int const iXtermTableEntry, _In_ const bool fIsForeground);
    virtual BOOL SetConsoleRGBTextAttribute(_In_ COLORREF const rgbColor, _In_ const bool fIsForeground);

    virtual BOOL WriteConsoleInputW(_In_reads_(nLength) INPUT_RECORD* const rgInputRecords,
                                    _In_ DWORD const nLength,
                                    _Out_ DWORD* const pNumberOfEventsWritten);

    virtual BOOL ScrollConsoleScreenBufferW(_In_ const SMALL_RECT* pScrollRectangle, _In_opt_ const SMALL_RECT* pClipRectangle, _In_ COORD coordDestinationOrigin, _In_ const CHAR_INFO* pFill);

    virtual BOOL SetConsoleWindowInfo(_In_ BOOL const bAbsolute, _In_ const SMALL_RECT* const lpConsoleWindow);

    virtual BOOL PrivateSetCursorKeysMode(_In_ bool const fApplicationMode);
    virtual BOOL PrivateSetKeypadMode(_In_ bool const fApplicationMode);

    virtual BOOL PrivateAllowCursorBlinking(_In_ bool const fEnable);

    virtual BOOL PrivateSetScrollingRegion(_In_ const SMALL_RECT* const srScrollMargins);

    virtual BOOL PrivateReverseLineFeed();

    virtual BOOL SetConsoleTitleW(_In_reads_(sCchTitleLength) const wchar_t* const pwchWindowTitle, _In_ unsigned short sCchTitleLength);

    virtual BOOL PrivateUseAlternateScreenBuffer();

    virtual BOOL PrivateUseMainScreenBuffer();

    void SetActiveScreenBuffer(_In_ SCREEN_INFORMATION* const pScreenInfo);

    virtual BOOL PrivateHorizontalTabSet();
    virtual BOOL PrivateForwardTab(_In_ SHORT const sNumTabs);
    virtual BOOL PrivateBackwardsTab(_In_ SHORT const sNumTabs);
    virtual BOOL PrivateTabClear(_In_ bool const fClearAll);

    virtual BOOL PrivateEnableVT200MouseMode(_In_ bool const fEnabled);
    virtual BOOL PrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnabled);
    virtual BOOL PrivateEnableSGRExtendedMouseMode(_In_ bool const fEnabled);
    virtual BOOL PrivateEnableButtonEventMouseMode(_In_ bool const fEnabled);
    virtual BOOL PrivateEnableAnyEventMouseMode(_In_ bool const fEnabled);
    virtual BOOL PrivateEnableAlternateScroll(_In_ bool const fEnabled);

private:
    SCREEN_INFORMATION* _pScreenInfo; // not const because switching to the alternate buffer will change this pointer.
    InputBuffer* const _pInputBuffer;

    BOOL _FillConsoleOutput(_In_ USHORT const usElement,
                            _In_ ULONG const ulElementType,
                            _In_ DWORD const nLength,
                            _In_ COORD const dwWriteCoord,
                            _Out_ DWORD* const pNumberWritten);
};
