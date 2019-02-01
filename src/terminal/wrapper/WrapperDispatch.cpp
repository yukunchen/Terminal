// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "pch.h"
#include "WrapperDispatch.h"
using namespace winrt;
using namespace Microsoft::Console::VirtualTerminal;

void WrapperDispatch::Execute(const wchar_t wchControl)
{
    _t.ExecuteChar(wchControl);
}
void WrapperDispatch::Print(const wchar_t wchPrintable)
{
    _t.PrintString(std::wstring(wchPrintable, 1));
}
void WrapperDispatch::PrintString(const wchar_t* const rgwch, const size_t cch)
{
    if (cch > 0)
    {
        _t.PrintString(std::wstring(rgwch, cch));
    }
}
bool WrapperDispatch::CursorUp(const unsigned int uiDistance)
{
    return _t.CursorUp(uiDistance);
};
bool WrapperDispatch::CursorDown(const unsigned int uiDistance)
{
    return _t.CursorDown(uiDistance);
}
bool WrapperDispatch::CursorForward(const unsigned int uiDistance)
{
    return _t.CursorForward(uiDistance);
}
bool WrapperDispatch::CursorBackward(const unsigned int uiDistance)
{
    return _t.CursorBackward(uiDistance);
}
bool WrapperDispatch::CursorNextLine(const unsigned int uiDistance)
{
    return _t.CursorNextLine(uiDistance);
}
bool WrapperDispatch::CursorPrevLine(const unsigned int uiDistance)
{
    return _t.CursorPrevLine(uiDistance);
}
bool WrapperDispatch::CursorHorizontalPositionAbsolute(const unsigned int uiColumn)
{
    return _t.CursorHorizontalPositionAbsolute(uiColumn);
}
bool WrapperDispatch::VerticalLinePositionAbsolute(const unsigned int uiLine)
{
    return _t.VerticalLinePositionAbsolute(uiLine);
}
bool WrapperDispatch::CursorPosition(const unsigned int row, const unsigned int col)
{
    return _t.CursorPosition(row, col);
}
bool WrapperDispatch::CursorSavePosition()
{
    return _t.CursorSavePosition();
}
bool WrapperDispatch::CursorRestorePosition()
{
    return _t.CursorRestorePosition();
}
bool WrapperDispatch::CursorVisibility(const bool on)
{
    return _t.CursorVisibility(on);
}
bool WrapperDispatch::InsertCharacter(const unsigned int count)
{
    return _t.InsertCharacter(count);
}
bool WrapperDispatch::DeleteCharacter(const unsigned int count)
{
    return _t.DeleteCharacter(count);
}
bool WrapperDispatch::ScrollUp(const unsigned int count)
{
    return _t.ScrollUp(count);
}
bool WrapperDispatch::ScrollDown(const unsigned int count)
{
    return _t.ScrollDown(count);
}
bool WrapperDispatch::InsertLine(const unsigned int lines)
{
    return _t.InsertLine(lines);
}
bool WrapperDispatch::DeleteLine(const unsigned int lines)
{
    return _t.DeleteLine(lines);
}
bool WrapperDispatch::SetColumns(const unsigned int uiColumns)
{
    return _t.SetColumns(uiColumns);
}
bool WrapperDispatch::SetCursorKeysMode(const bool fApplicationMode)
{
    return _t.SetCursorKeysMode(fApplicationMode);
}
bool WrapperDispatch::SetKeypadMode(const bool fApplicationMode)
{
    return _t.SetKeypadMode(fApplicationMode);
}
bool WrapperDispatch::EnableCursorBlinking(const bool isBlinking)
{
    return _t.EnableCursorBlinking(isBlinking);
}
bool WrapperDispatch::SetTopBottomScrollingMargins(const SHORT sTopMargin,
                                                   const SHORT sBottomMargin)
{
    return _t.SetTopBottomScrollingMargins(sTopMargin, sBottomMargin);
}
bool WrapperDispatch::ReverseLineFeed()
{
    return _t.ReverseLineFeed();
}
bool WrapperDispatch::SetWindowTitle(std::wstring_view title)
{
    return _t.SetWindowTitle(title);
}
bool WrapperDispatch::EraseInDisplay(const DispatchTypes::EraseType eraseType)
{
    return _t.EraseInDisplay(static_cast<VtStateMachine::EraseType>(eraseType));
}
bool WrapperDispatch::EraseInLine(const DispatchTypes::EraseType eraseType)
{
    return _t.EraseInLine(static_cast<VtStateMachine::EraseType>(eraseType));
}
bool WrapperDispatch::EraseCharacters(const unsigned int uiNumChars)
{
    return _t.EraseCharacters(uiNumChars);
}
bool WrapperDispatch::SetGraphicsRendition(const DispatchTypes::GraphicsOptions* const rgOptions, const size_t cOptions)
{
    std::vector<VtStateMachine::GraphicsOptions> opts;
    opts.reserve(cOptions);
    for (size_t i = 0; i < cOptions; i++)
    {
        opts.push_back(static_cast<VtStateMachine::GraphicsOptions>(rgOptions[i]));
    }
    return _t.SetGraphicsRendition(winrt::array_view<VtStateMachine::GraphicsOptions const>(opts));
}
bool WrapperDispatch::SetPrivateModes(const DispatchTypes::PrivateModeParams* const rgParams, const size_t cParams)
{
    std::vector<VtStateMachine::PrivateModeParams> opts;
    opts.reserve(cParams);
    for (size_t i = 0; i < cParams; i++)
    {
        opts.push_back(static_cast<VtStateMachine::PrivateModeParams>(rgParams[i]));
    }
    return _t.SetPrivateModes(winrt::array_view<VtStateMachine::PrivateModeParams const>(opts));
}
bool WrapperDispatch::ResetPrivateModes(const DispatchTypes::PrivateModeParams* const rgParams, const size_t cParams)
{
    std::vector<VtStateMachine::PrivateModeParams> opts;
    opts.reserve(cParams);
    for (size_t i = 0; i < cParams; i++)
    {
        opts.push_back(static_cast<VtStateMachine::PrivateModeParams>(rgParams[i]));
    }
    return _t.ResetPrivateModes(winrt::array_view<VtStateMachine::PrivateModeParams const>(opts));
}
bool WrapperDispatch::WindowManipulation(const DispatchTypes::WindowManipulationType uiFunction, const unsigned short* const rgusParams, const size_t cParams)
{
    bool fSuccess = false;
    switch (uiFunction)
    {
        case DispatchTypes::WindowManipulationType::RefreshWindow:
            if (cParams == 0)
            {
                fSuccess = _t.RefreshWindow();
            }
            break;
        case DispatchTypes::WindowManipulationType::ResizeWindowInCharacters:
            if (cParams == 2)
            {
                fSuccess = _t.ResizeWindow(rgusParams[1], rgusParams[0]);
            }
            break;
        default:
            fSuccess = false;
    }
    return fSuccess;
}
bool WrapperDispatch::DeviceStatusReport(const DispatchTypes::AnsiStatusType statusType)
{
    return _t.DeviceStatusReport(static_cast<VtStateMachine::AnsiStatusType>(statusType));
}
bool WrapperDispatch::DeviceAttributes()
{
    return _t.DeviceAttributes();
}
bool WrapperDispatch::UseAlternateScreenBuffer()
{
    return _t.UseAlternateScreenBuffer();
}
bool WrapperDispatch::UseMainScreenBuffer()
{
    return _t.UseMainScreenBuffer();
}
bool WrapperDispatch::HorizontalTabSet()
{
    return _t.HorizontalTabSet();
}
bool WrapperDispatch::ForwardTab(const SHORT sNumTabs)
{
    return _t.ForwardTab(sNumTabs);
}
bool WrapperDispatch::BackwardsTab(const SHORT sNumTabs)
{
    return _t.BackwardsTab(sNumTabs);
}
bool WrapperDispatch::TabClear(const SHORT sClearType)
{
    return _t.TabClear(static_cast<VtStateMachine::TabClearType>(sClearType));
}
bool WrapperDispatch::DesignateCharset(const wchar_t wchCharset)
{
    return _t.DesignateCharset(static_cast<VtStateMachine::CharacterSet>(wchCharset));
}
bool WrapperDispatch::SoftReset()
{
    return _t.SoftReset();
}
bool WrapperDispatch::HardReset()
{
    return _t.HardReset();
}
bool WrapperDispatch::EnableVT200MouseMode(const bool fEnabled)
{
    return _t.EnableVT200MouseMode(fEnabled);
}
bool WrapperDispatch::EnableUTF8ExtendedMouseMode(const bool fEnabled)
{
    return _t.EnableUTF8ExtendedMouseMode(fEnabled);
}
bool WrapperDispatch::EnableSGRExtendedMouseMode(const bool fEnabled)
{
    return _t.EnableSGRExtendedMouseMode(fEnabled);
}
bool WrapperDispatch::EnableButtonEventMouseMode(const bool fEnabled)
{
    return _t.EnableButtonEventMouseMode(fEnabled);
}
bool WrapperDispatch::EnableAnyEventMouseMode(const bool fEnabled)
{
    return _t.EnableAnyEventMouseMode(fEnabled);
}
bool WrapperDispatch::EnableAlternateScroll(const bool fEnabled)
{
    return _t.EnableAlternateScroll(fEnabled);
}
bool WrapperDispatch::SetCursorStyle(const DispatchTypes::CursorStyle cursorStyle)
{
    return _t.SetCursorStyle(static_cast<VtStateMachine::CursorStyle>(cursorStyle));
}
bool WrapperDispatch::SetCursorColor(const COLORREF cursorColor)
{
    return _t.SetCursorColor(cursorColor);
}
bool WrapperDispatch::SetColorTableEntry(const size_t tableIndex,
                                         const DWORD dwColor)
{
    return _t.SetColorTableEntry(static_cast<uint32_t>(tableIndex), dwColor);
}
