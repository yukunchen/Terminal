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

// The WriteBuffer class provides helpers for writing text into the TextBuffer that is backing a particular console screen buffer.
class WriteBuffer : public Microsoft::Console::VirtualTerminal::AdaptDefaults
{
public:
    WriteBuffer(_In_ Microsoft::Console::IIoProvider& io);

    // Implement Adapter callbacks for default cases (non-escape sequences)
    void Print(const wchar_t wch);
    void PrintString(_In_reads_(cch) wchar_t* const rgwch, const size_t cch);
    void Execute(const wchar_t wch);

    [[nodiscard]]
    NTSTATUS GetResult() { return _ntstatus; };

private:
    void _DefaultCase(const wchar_t wch);
    void _DefaultStringCase(_In_reads_(cch) wchar_t* const rgwch, const size_t cch);

    Microsoft::Console::IIoProvider& _io;
    NTSTATUS _ntstatus;
};

#include "..\terminal\adapter\conGetSet.hpp"


// The ConhostInternalGetSet is for the Conhost process to call the entrypoints for its own Get/Set APIs.
// Normally, these APIs are accessible from the outside of the conhost process (like by the process being "hosted") through
// the kernelbase/32 exposed public APIs and routed by the console driver (condrv) to this console host.
// But since we're trying to call them from *inside* the console host itself, we need to get in the way and route them straight to the
// v-table inside this process instance.
class ConhostInternalGetSet : public Microsoft::Console::VirtualTerminal::ConGetSet
{
public:
    ConhostInternalGetSet(_In_ Microsoft::Console::IIoProvider& io);

    virtual BOOL GetConsoleScreenBufferInfoEx(_Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) const override;
    virtual BOOL SetConsoleScreenBufferInfoEx(const CONSOLE_SCREEN_BUFFER_INFOEX* const pConsoleScreenBufferInfoEx) override;

    virtual BOOL SetConsoleCursorPosition(const COORD coordCursorPosition) override;

    virtual BOOL GetConsoleCursorInfo(_In_ CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) const override;
    virtual BOOL SetConsoleCursorInfo(const CONSOLE_CURSOR_INFO* const pConsoleCursorInfo) override;

    virtual BOOL FillConsoleOutputCharacterW(const WCHAR wch,
                                             const DWORD nLength,
                                             const COORD dwWriteCoord,
                                             _Out_ DWORD* const pNumberOfCharsWritten) override;
    virtual BOOL FillConsoleOutputAttribute(const WORD wAttribute,
                                            const DWORD nLength,
                                            const COORD dwWriteCoord,
                                            _Out_ DWORD* const pNumberOfAttrsWritten) override;

    virtual BOOL SetConsoleTextAttribute(const WORD wAttr) override;
    virtual BOOL PrivateSetLegacyAttributes(const WORD wAttr,
                                            const bool fForeground,
                                            const bool fBackground,
                                            const bool fMeta) override;
    virtual BOOL SetConsoleXtermTextAttribute(const int iXtermTableEntry,
                                              const bool fIsForeground) override;
    virtual BOOL SetConsoleRGBTextAttribute(const COLORREF rgbColor,
                                            const bool fIsForeground) override;

    virtual BOOL WriteConsoleInputW(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                    _Out_ size_t& eventsWritten) override;

    virtual BOOL ScrollConsoleScreenBufferW(const SMALL_RECT* pScrollRectangle,
                                            _In_opt_ const SMALL_RECT* pClipRectangle,
                                            _In_ COORD coordDestinationOrigin,
                                            const CHAR_INFO* pFill) override;

    virtual BOOL SetConsoleWindowInfo(const BOOL bAbsolute,
                                      const SMALL_RECT* const lpConsoleWindow) override;

    virtual BOOL PrivateSetCursorKeysMode(const bool fApplicationMode) override;
    virtual BOOL PrivateSetKeypadMode(const bool fApplicationMode) override;

    virtual BOOL PrivateAllowCursorBlinking(const bool fEnable) override;

    virtual BOOL PrivateSetScrollingRegion(const SMALL_RECT* const srScrollMargins) override;

    virtual BOOL PrivateReverseLineFeed() override;

    virtual BOOL SetConsoleTitleW(_In_reads_(sCchTitleLength) const wchar_t* const pwchWindowTitle,
                                  _In_ unsigned short sCchTitleLength) override;

    virtual BOOL PrivateUseAlternateScreenBuffer() override;

    virtual BOOL PrivateUseMainScreenBuffer() override;

    virtual BOOL PrivateHorizontalTabSet();
    virtual BOOL PrivateForwardTab(const SHORT sNumTabs) override;
    virtual BOOL PrivateBackwardsTab(const SHORT sNumTabs) override;
    virtual BOOL PrivateTabClear(const bool fClearAll) override;

    virtual BOOL PrivateEnableVT200MouseMode(const bool fEnabled) override;
    virtual BOOL PrivateEnableUTF8ExtendedMouseMode(const bool fEnabled) override;
    virtual BOOL PrivateEnableSGRExtendedMouseMode(const bool fEnabled) override;
    virtual BOOL PrivateEnableButtonEventMouseMode(const bool fEnabled) override;
    virtual BOOL PrivateEnableAnyEventMouseMode(const bool fEnabled) override;
    virtual BOOL PrivateEnableAlternateScroll(const bool fEnabled) override;
    virtual BOOL PrivateEraseAll() override;

    virtual BOOL PrivateGetConsoleScreenBufferAttributes(_Out_ WORD* const pwAttributes) override;

    virtual BOOL PrivatePrependConsoleInput(_Inout_ std::deque<std::unique_ptr<IInputEvent>>& events,
                                            _Out_ size_t& eventsWritten) override;

    virtual BOOL SetCursorStyle(const CursorType cursorType) override;
    virtual BOOL SetCursorColor(const COLORREF cursorColor) override;

    virtual BOOL PrivateRefreshWindow() override;

    virtual BOOL PrivateSuppressResizeRepaint() override;

    virtual BOOL PrivateWriteConsoleControlInput(_In_ KeyEvent key) override;

    virtual BOOL GetConsoleOutputCP(_Out_ unsigned int* const puiOutputCP) override;

private:
    Microsoft::Console::IIoProvider& _io;

    BOOL _FillConsoleOutput(const USHORT usElement,
                            const ULONG ulElementType,
                            const DWORD nLength,
                            const COORD dwWriteCoord,
                            _Out_ DWORD* const pNumberWritten);
};
