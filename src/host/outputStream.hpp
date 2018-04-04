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
#include "..\types\inc\IInputEvent.hpp"
#include "..\inc\conattrs.hpp"
#include "IIoProvider.hpp"

class SCREEN_INFORMATION;

// The WriteBuffer class provides helpers for writing text into the TEXT_BUFFER_INFO that is backing a particular console screen buffer.
class WriteBuffer : public Microsoft::Console::VirtualTerminal::AdaptDefaults
{
public:
    WriteBuffer(_In_ Microsoft::Console::IIoProvider* const pIo);

    // Implement Adapter callbacks for default cases (non-escape sequences)
    void Print(_In_ wchar_t const wch);
    void PrintString(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);
    void Execute(_In_ wchar_t const wch);

    [[nodiscard]]
    NTSTATUS GetResult() { return _ntstatus; };

private:
    void _DefaultCase(_In_ wchar_t const wch);
    void _DefaultStringCase(_In_reads_(cch) wchar_t* const rgwch, _In_ size_t const cch);

    const Microsoft::Console::IIoProvider* const _pIo;
    NTSTATUS _ntstatus;
};

#include "..\terminal\adapter\conGetSet.hpp"


// The ConhostInternalGetSet is for the Conhost process to call the entrypoints for its own Get/Set APIs.
// Normally, these APIs are accessible from the outside of the conhost process (like by the process being "hosted") through
// the kernelbase/32 exposed public APIs and routed by the console driver (condrv) to this console host.
// But since we're trying to call them from *inside* the console host itself, we need to get in the way and route them straight to the
// v-table inside this process instance.
class ConhostInternalGetSet final : public Microsoft::Console::VirtualTerminal::ConGetSet
{
public:
    ConhostInternalGetSet(_In_ Microsoft::Console::IIoProvider* const pIo);

    BOOL GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const override;
    BOOL SetConsoleScreenBufferInfoEx(_In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const override;

    BOOL SetConsoleCursorPosition(_In_ COORD const coordCursorPosition) override;

    BOOL GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const override;
    BOOL SetConsoleCursorInfo(_In_ const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) override;

    BOOL FillConsoleOutputCharacterW(_In_ WCHAR const wch,
                                     _In_ DWORD const nLength,
                                     _In_ COORD const dwWriteCoord,
                                     _Out_ DWORD* const pNumberOfCharsWritten) override;
    BOOL FillConsoleOutputAttribute(_In_ WORD const wAttribute,
                                    _In_ DWORD const nLength,
                                    _In_ COORD const dwWriteCoord,
                                    _Out_ DWORD* const pNumberOfAttrsWritten) override;

    BOOL SetConsoleTextAttribute(_In_ WORD const wAttr) override;
    BOOL PrivateSetLegacyAttributes(_In_ WORD const wAttr,
                                    _In_ const bool fForeground,
                                    _In_ const bool fBackground,
                                    _In_ const bool fMeta) override;
    BOOL SetConsoleXtermTextAttribute(_In_ int const iXtermTableEntry,
                                              _In_ const bool fIsForeground) override;
    BOOL SetConsoleRGBTextAttribute(_In_ COLORREF const rgbColor,
                                    _In_ const bool fIsForeground) override;

    BOOL WriteConsoleInputW(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                            _Out_ size_t& eventsWritten) override;

    BOOL ScrollConsoleScreenBufferW(_In_ const SMALL_RECT* pScrollRectangle,
                                    _In_opt_ const SMALL_RECT* pClipRectangle,
                                    _In_ COORD coordDestinationOrigin,
                                    _In_ const CHAR_INFO* pFill) override;

    BOOL SetConsoleWindowInfo(_In_ BOOL const bAbsolute,
                              _In_ const SMALL_RECT* const lpConsoleWindow) override;

    BOOL PrivateSetCursorKeysMode(_In_ bool const fApplicationMode) override;
    BOOL PrivateSetKeypadMode(_In_ bool const fApplicationMode) override;

    BOOL PrivateAllowCursorBlinking(_In_ bool const fEnable) override;

    BOOL PrivateSetScrollingRegion(_In_ const SMALL_RECT* const srScrollMargins) override;

    BOOL PrivateReverseLineFeed() override;

    BOOL SetConsoleTitleW(_In_reads_(sCchTitleLength) const wchar_t* const pwchWindowTitle,
                          _In_ unsigned short sCchTitleLength) override;

    BOOL PrivateUseAlternateScreenBuffer() override;

    BOOL PrivateUseMainScreenBuffer() override;

    BOOL PrivateHorizontalTabSet();
    BOOL PrivateForwardTab(_In_ SHORT const sNumTabs) override;
    BOOL PrivateBackwardsTab(_In_ SHORT const sNumTabs) override;
    BOOL PrivateTabClear(_In_ bool const fClearAll) override;

    BOOL PrivateEnableVT200MouseMode(_In_ bool const fEnabled) override;
    BOOL PrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnabled) override;
    BOOL PrivateEnableSGRExtendedMouseMode(_In_ bool const fEnabled) override;
    BOOL PrivateEnableButtonEventMouseMode(_In_ bool const fEnabled) override;
    BOOL PrivateEnableAnyEventMouseMode(_In_ bool const fEnabled) override;
    BOOL PrivateEnableAlternateScroll(_In_ bool const fEnabled) override;
    BOOL PrivateEraseAll() override;

    BOOL PrivateGetConsoleScreenBufferAttributes(_Out_ WORD* const pwAttributes) override;

    BOOL PrivatePrependConsoleInput(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                    _Out_ size_t& eventsWritten) override;

    BOOL SetCursorStyle(_In_ CursorType const cursorType) override;
    BOOL SetCursorColor(_In_ COLORREF const cursorColor) override;

    BOOL PrivateRefreshWindow() override;

    BOOL PrivateSuppressResizeRepaint() override;

    BOOL PrivateWriteConsoleControlInput(_In_ KeyEvent key) override;

    BOOL GetConsoleOutputCP(_Out_ unsigned int* const puiOutputCP) override;

    BOOL IsConsolePty(_Out_ bool* const pIsPty) const override;

private:
    const Microsoft::Console::IIoProvider* const _pIo;

    BOOL _FillConsoleOutput(_In_ USHORT const usElement,
                            _In_ ULONG const ulElementType,
                            _In_ DWORD const nLength,
                            _In_ COORD const dwWriteCoord,
                            _Out_ DWORD* const pNumberWritten);
};
