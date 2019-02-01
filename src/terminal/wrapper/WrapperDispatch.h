// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "../adapter/ITermDispatch.hpp"
#include "Parser.g.h"
#pragma once

namespace Microsoft::Console::VirtualTerminal
{
    class WrapperDispatch;
};

class Microsoft::Console::VirtualTerminal::WrapperDispatch : public Microsoft::Console::VirtualTerminal::ITermDispatch
{
public:
    WrapperDispatch(const winrt::VtStateMachine::ITerminalDispatch& t) : _t(t) { }
    void Execute(const wchar_t wchControl) override;
    void Print(const wchar_t wchPrintable) override;
    void PrintString(const wchar_t* const rgwch, const size_t cch) override;
    bool CursorUp(const unsigned int uiDistance) override;
    bool CursorDown(const unsigned int uiDistance) override;
    bool CursorForward(const unsigned int uiDistance) override;
    bool CursorBackward(const unsigned int uiDistance) override;
    bool CursorNextLine(const unsigned int uiDistance) override;
    bool CursorPrevLine(const unsigned int uiDistance) override;
    bool CursorHorizontalPositionAbsolute(const unsigned int uiColumn) override;
    bool VerticalLinePositionAbsolute(const unsigned int uiLine) override;
    bool CursorPosition(const unsigned int row, const unsigned int col) override;
    bool CursorSavePosition() override;
    bool CursorRestorePosition() override;
    bool CursorVisibility(const bool on) override;
    bool InsertCharacter(const unsigned int count) override;
    bool DeleteCharacter(const unsigned int count) override;
    bool ScrollUp(const unsigned int count) override;
    bool ScrollDown(const unsigned int count) override;
    bool InsertLine(const unsigned int lines) override;
    bool DeleteLine(const unsigned int lines) override;
    bool SetColumns(const unsigned int uiColumns) override;
    bool SetCursorKeysMode(const bool fApplicationMode) override;
    bool SetKeypadMode(const bool fApplicationMode) override;
    bool EnableCursorBlinking(const bool isBlinking);
    bool SetTopBottomScrollingMargins(const short sTopMargin, const short sBottomMargin) override;
    bool ReverseLineFeed() override;
    bool SetWindowTitle(std::wstring_view title) override;
    bool EraseInDisplay(const DispatchTypes::EraseType eraseType) override;
    bool EraseInLine(const DispatchTypes::EraseType eraseType) override;
    bool EraseCharacters(const unsigned int uiNumChars) override;
    bool SetGraphicsRendition(const DispatchTypes::GraphicsOptions* const rgOptions, const size_t cOptions) override;
    bool SetPrivateModes(const DispatchTypes::PrivateModeParams* const rgParams, const size_t cParams) override;
    bool ResetPrivateModes(const DispatchTypes::PrivateModeParams* const rgParams, const size_t cParams) override;
    bool DeviceStatusReport(const DispatchTypes::AnsiStatusType statusType) override;
    bool DeviceAttributes() override;
    bool UseAlternateScreenBuffer() override;
    bool UseMainScreenBuffer() override;
    bool HorizontalTabSet() override;
    bool ForwardTab(const SHORT sNumTabs) override;
    bool BackwardsTab(const SHORT sNumTabs) override;
    bool TabClear(const SHORT sClearType) override;
    bool DesignateCharset(const wchar_t wchCharset) override;
    bool SoftReset() override;
    bool HardReset() override;
    bool EnableVT200MouseMode(const bool fEnabled) override;
    bool EnableUTF8ExtendedMouseMode(const bool fEnabled) override;
    bool EnableSGRExtendedMouseMode(const bool fEnabled) override;
    bool EnableButtonEventMouseMode(const bool fEnabled) override;
    bool EnableAnyEventMouseMode(const bool fEnabled) override;
    bool EnableAlternateScroll(const bool fEnabled) override;
    bool SetCursorStyle(const DispatchTypes::CursorStyle cursorStyle) override;
    bool SetCursorColor(const COLORREF cursorColor) override;
    bool SetColorTableEntry(const size_t tableIndex, const DWORD dwColor) override;
    bool WindowManipulation(const DispatchTypes::WindowManipulationType uiFunction, const unsigned short* const rgusParams, const size_t cParams) override;
private:
    winrt::VtStateMachine::ITerminalDispatch _t;
};
