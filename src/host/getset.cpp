/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "getset.h"

#include "_output.h"
#include "_stream.h"
#include "output.h"
#include "dbcs.h"
#include "handle.h"
#include "misc.h"
#include "../types/inc/convert.hpp"
#include "../types/inc/viewport.hpp"

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// The following mask is used to test for valid text attributes.
#define VALID_TEXT_ATTRIBUTES (FG_ATTRS | BG_ATTRS | META_ATTRS)

#define INPUT_MODES (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT)
#define OUTPUT_MODES (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_LVB_GRID_WORLDWIDE)
#define PRIVATE_MODES (ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_EXTENDED_FLAGS)

using namespace Microsoft::Console::Types;

void ApiRoutines::GetConsoleInputModeImpl(_In_ InputBuffer* const pContext, _Out_ ULONG* const pMode)
{
    Telemetry::Instance().LogApiCall(Telemetry::ApiCall::GetConsoleMode);
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pMode = pContext->InputMode;

    if (IsFlagSet(gci.Flags, CONSOLE_USE_PRIVATE_FLAGS))
    {
        SetFlag(*pMode, ENABLE_EXTENDED_FLAGS);
        SetFlagIf(*pMode, ENABLE_INSERT_MODE, gci.GetInsertMode());
        SetFlagIf(*pMode, ENABLE_QUICK_EDIT_MODE, IsFlagSet(gci.Flags, CONSOLE_QUICK_EDIT_MODE));
        SetFlagIf(*pMode, ENABLE_AUTO_POSITION, IsFlagSet(gci.Flags, CONSOLE_AUTO_POSITION));
    }
}

void ApiRoutines::GetConsoleOutputModeImpl(_In_ SCREEN_INFORMATION* const pContext, _Out_ ULONG* const pMode)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pMode = pContext->GetActiveBuffer()->OutputMode;
}

[[nodiscard]]
HRESULT ApiRoutines::GetNumberOfConsoleInputEventsImpl(_In_ InputBuffer* const pContext, _Out_ ULONG* const pEvents)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    size_t readyEventCount = pContext->GetNumberOfReadyEvents();
    RETURN_IF_FAILED(SizeTToULong(readyEventCount, pEvents));

    return S_OK;
}

void ApiRoutines::GetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                   _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    DoSrvGetConsoleScreenBufferInfo(pContext->GetActiveBuffer(), pScreenBufferInfoEx);
}

void DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* pInfo)
{
    pInfo->bFullscreenSupported = FALSE; // traditional full screen with the driver support is no longer supported.
    pScreenInfo->GetScreenBufferInformation(&pInfo->dwSize,
                                            &pInfo->dwCursorPosition,
                                            &pInfo->srWindow,
                                            &pInfo->wAttributes,
                                            &pInfo->dwMaximumWindowSize,
                                            &pInfo->wPopupAttributes,
                                            pInfo->ColorTable);
    // Callers of this function expect to recieve an exclusive rect, not an inclusive one.
    pInfo->srWindow.Right += 1;
    pInfo->srWindow.Bottom += 1;
}

void ApiRoutines::GetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                           _Out_ ULONG* const pCursorSize,
                                           _Out_ BOOLEAN* const pIsVisible)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    DoSrvGetConsoleCursorInfo(pContext->GetActiveBuffer(), pCursorSize, pIsVisible);
}

void DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                               _Out_ ULONG* const pCursorSize,
                               _Out_ BOOLEAN* const pIsVisible)
{
    *pCursorSize = pScreenInfo->TextInfo->GetCursor()->GetSize();
    *pIsVisible = (BOOLEAN)pScreenInfo->TextInfo->GetCursor()->IsVisible();
}

void ApiRoutines::GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    const Selection* const pSelection = &Selection::Instance();
    if (pSelection->IsInSelectingState())
    {
        pConsoleSelectionInfo->dwFlags = pSelection->GetPublicSelectionFlags();

        SetFlag(pConsoleSelectionInfo->dwFlags, CONSOLE_SELECTION_IN_PROGRESS);

        pConsoleSelectionInfo->dwSelectionAnchor = pSelection->GetSelectionAnchor();
        pConsoleSelectionInfo->srSelection = pSelection->GetSelectionRectangle();
    }
    else
    {
        ZeroMemory(pConsoleSelectionInfo, sizeof(pConsoleSelectionInfo));
    }
}

void ApiRoutines::GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pButtons = ServiceLocator::LocateSystemConfigurationProvider()->GetNumberOfMouseButtons();
}

[[nodiscard]]
HRESULT ApiRoutines::GetConsoleFontSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                            _In_ DWORD const FontIndex,
                                            _Out_ COORD* const pFontSize)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    if (FontIndex == 0)
    {
        // As of the November 2015 renderer system, we only have a single font at index 0.
        *pFontSize = pContext->GetActiveBuffer()->TextInfo->GetCurrentFont()->GetUnscaledSize();
        return S_OK;
    }
    else
    {
        // Invalid font is 0,0 with STATUS_INVALID_PARAMETER
        *pFontSize = { 0 };
        return E_INVALIDARG;
    }
}

[[nodiscard]]
HRESULT ApiRoutines::GetCurrentConsoleFontExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                 _In_ BOOLEAN const IsForMaximumWindowSize,
                                                 _Out_ CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    const SCREEN_INFORMATION* const psi = pContext->GetActiveBuffer();

    COORD WindowSize;
    if (IsForMaximumWindowSize)
    {
        WindowSize = psi->GetMaxWindowSizeInCharacters();
    }
    else
    {
        WindowSize = psi->TextInfo->GetCurrentFont()->GetUnscaledSize();
    }
    pConsoleFontInfoEx->dwFontSize = WindowSize;

    pConsoleFontInfoEx->nFont = 0;

    const FontInfo* const pfi = psi->TextInfo->GetCurrentFont();
    pConsoleFontInfoEx->FontFamily = pfi->GetFamily();
    pConsoleFontInfoEx->FontWeight = pfi->GetWeight();

    RETURN_IF_FAILED(StringCchCopyW(pConsoleFontInfoEx->FaceName, ARRAYSIZE(pConsoleFontInfoEx->FaceName), pfi->GetFaceName()));

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetCurrentConsoleFontExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                 _In_ BOOLEAN const /*IsForMaximumWindowSize*/,
                                                 _In_ const CONSOLE_FONT_INFOEX* const pConsoleFontInfoEx)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    SCREEN_INFORMATION* const psi = pContext->GetActiveBuffer();

    WCHAR FaceName[ARRAYSIZE(pConsoleFontInfoEx->FaceName)];
    RETURN_IF_FAILED(StringCchCopyW(FaceName, ARRAYSIZE(FaceName), pConsoleFontInfoEx->FaceName));

    FontInfo fi(FaceName,
                static_cast<BYTE>(pConsoleFontInfoEx->FontFamily),
                pConsoleFontInfoEx->FontWeight,
                pConsoleFontInfoEx->dwFontSize,
                gci.OutputCP);

    // TODO: MSFT: 9574827 - should this have a failure case?
    psi->UpdateFont(&fi);

    // If this is the active screen buffer, also cause the window to refresh its viewport size.
    if (psi->IsActiveScreenBuffer())
    {
        IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
        if (nullptr != pWindow)
        {
            pWindow->PostUpdateWindowSize();
        }
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleInputModeImpl(_In_ InputBuffer* const pContext, _In_ ULONG const Mode)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    if (IsAnyFlagSet(Mode, PRIVATE_MODES))
    {
        SetFlag(gci.Flags, CONSOLE_USE_PRIVATE_FLAGS);

        UpdateFlag(gci.Flags, CONSOLE_QUICK_EDIT_MODE, IsFlagSet(Mode, ENABLE_QUICK_EDIT_MODE));
        UpdateFlag(gci.Flags, CONSOLE_AUTO_POSITION, IsFlagSet(Mode, ENABLE_AUTO_POSITION));

        BOOL const PreviousInsertMode = gci.GetInsertMode();
        gci.SetInsertMode(IsFlagSet(Mode, ENABLE_INSERT_MODE));
        if (gci.GetInsertMode() != PreviousInsertMode)
        {
            gci.CurrentScreenBuffer->SetCursorDBMode(false);
            if (gci.lpCookedReadData != nullptr)
            {
                gci.lpCookedReadData->_InsertMode = !!gci.GetInsertMode();
            }
        }
    }
    else
    {
        ClearFlag(gci.Flags, CONSOLE_USE_PRIVATE_FLAGS);
    }

    pContext->InputMode = Mode;
    ClearAllFlags(pContext->InputMode, PRIVATE_MODES);

    // NOTE: For compatibility reasons, we need to set the modes and then return the error codes, not the other way around
    //       as might be expected.
    //       This is a bug from a long time ago and some applications depend on this functionality to operate properly.
    //       ---
    //       A prime example of this is that PSReadline module in Powershell will set the invalid mode 0x1e4
    //       which includes 0x4 for ECHO_INPUT but turns off 0x2 for LINE_INPUT. This is invalid, but PSReadline
    //       relies on it to properly receive the ^C printout and make a new line when the user presses Ctrl+C.
    {
        // Flags we don't understand are invalid.
        RETURN_HR_IF(E_INVALIDARG, IsAnyFlagSet(Mode, ~(INPUT_MODES | PRIVATE_MODES)));

        // ECHO on with LINE off is invalid.
        RETURN_HR_IF(E_INVALIDARG, IsFlagSet(Mode, ENABLE_ECHO_INPUT) && IsFlagClear(Mode, ENABLE_LINE_INPUT));
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleOutputModeImpl(_In_ SCREEN_INFORMATION* const pContext, _In_ ULONG const Mode)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Flags we don't understand are invalid.
    RETURN_HR_IF(E_INVALIDARG, IsAnyFlagSet(Mode, ~OUTPUT_MODES));

    SCREEN_INFORMATION* const pScreenInfo = pContext->GetActiveBuffer();
    const DWORD dwOldMode = pScreenInfo->OutputMode;
    const DWORD dwNewMode = Mode;

    pScreenInfo->OutputMode = dwNewMode;

    // if we're moving from VT on->off
    if (!IsFlagSet(dwNewMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) && IsFlagSet(dwOldMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        // jiggle the handle
        pScreenInfo->GetStateMachine()->ResetState();
    }
    gci.SetVirtTermLevel(IsFlagSet(dwNewMode, ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 1 : 0);
    gci.SetAutomaticReturnOnNewline(IsFlagSet(pScreenInfo->OutputMode, DISABLE_NEWLINE_AUTO_RETURN) ? false : true);
    gci.SetGridRenderingAllowedWorldwide(IsFlagSet(pScreenInfo->OutputMode, ENABLE_LVB_GRID_WORLDWIDE));

    return S_OK;
}

void ApiRoutines::SetConsoleActiveScreenBufferImpl(_In_ SCREEN_INFORMATION* const pNewContext)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    SetActiveScreenBuffer(pNewContext->GetActiveBuffer());
}

void ApiRoutines::FlushConsoleInputBuffer(_In_ InputBuffer* const pContext)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    pContext->Flush();
}

void ApiRoutines::GetLargestConsoleWindowSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                     _Out_ COORD* const pSize)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    SCREEN_INFORMATION* const pScreenInfo = pContext->GetActiveBuffer();

    *pSize = pScreenInfo->GetLargestWindowSizeInCharacters();
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleScreenBufferSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                    _In_ const COORD* const pSize)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    SCREEN_INFORMATION* const pScreenInfo = pContext->GetActiveBuffer();

    COORD const coordMin = pScreenInfo->GetMinWindowSizeInCharacters();

    // Make sure requested screen buffer size isn't smaller than the window.
    RETURN_HR_IF(E_INVALIDARG, (pSize->X < pScreenInfo->GetScreenWindowSizeX() ||
                                pSize->Y < pScreenInfo->GetScreenWindowSizeY() ||
                                pSize->Y < coordMin.Y ||
                                pSize->X < coordMin.X));

    // Ensure the requested size isn't larger than we can handle in our data type.
    RETURN_HR_IF(E_INVALIDARG, (pSize->X == SHORT_MAX || pSize->Y == SHORT_MAX));

    // Only do the resize if we're actually changing one of the dimensions
    COORD const coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (pSize->X != coordScreenBufferSize.X || pSize->Y != coordScreenBufferSize.Y)
    {
        RETURN_NTSTATUS(pScreenInfo->ResizeScreenBuffer(*pSize, TRUE));
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                      _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    RETURN_HR_IF(E_INVALIDARG, (pScreenBufferInfoEx->dwSize.X == 0 ||
                                pScreenBufferInfoEx->dwSize.Y == 0 ||
                                pScreenBufferInfoEx->dwSize.X == SHRT_MAX ||
                                pScreenBufferInfoEx->dwSize.Y == SHRT_MAX));

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    DoSrvSetScreenBufferInfo(pContext->GetActiveBuffer(), pScreenBufferInfoEx);
    return S_OK;
}

void DoSrvSetScreenBufferInfo(_In_ SCREEN_INFORMATION* const pScreenInfo,
                              _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    COORD const coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    COORD const requestedBufferSize = pInfo->dwSize;
    if (requestedBufferSize.X != coordScreenBufferSize.X ||
        requestedBufferSize.Y != coordScreenBufferSize.Y)
    {
        CommandLine* const pCommandLine = &CommandLine::Instance();

        pCommandLine->Hide(FALSE);

        LOG_IF_FAILED(pScreenInfo->ResizeScreenBuffer(pInfo->dwSize, TRUE));

        pCommandLine->Show();
    }

    gci.SetColorTable(pInfo->ColorTable, ARRAYSIZE(pInfo->ColorTable));
    SetScreenColors(pScreenInfo, pInfo->wAttributes, pInfo->wPopupAttributes, TRUE);

    const Viewport requestedViewport = Viewport::FromExclusive(pInfo->srWindow);

    COORD NewSize;
    NewSize.X = std::min(requestedViewport.Width(), pInfo->dwMaximumWindowSize.X);
    NewSize.Y = std::min(requestedViewport.Height(), pInfo->dwMaximumWindowSize.Y);

    // If wrap text is on, then the window width must be the same size as the buffer width
    if (gci.GetWrapText())
    {
        NewSize.X = coordScreenBufferSize.X;
    }

    if (NewSize.X != pScreenInfo->GetScreenWindowSizeX() ||
        NewSize.Y != pScreenInfo->GetScreenWindowSizeY())
    {
        gci.CurrentScreenBuffer->SetViewportSize(&NewSize);

        IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
        if (pWindow != nullptr)
        {
            pWindow->UpdateWindowSize(NewSize);
        }
    }

    // Despite the fact that this API takes in a srWindow for the viewport, it traditionally actually doesn't set
    //  anything using that member - for moving the viewport, you need SetConsoleWindowInfo
    //  (see https://msdn.microsoft.com/en-us/library/windows/desktop/ms686125(v=vs.85).aspx and DoSrvSetConsoleWindowInfo)
    // Note that it also doesn't set cursor position.
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleCursorPositionImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                  _In_ const COORD* const pCursorPosition)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleCursorPosition(pContext->GetActiveBuffer(), pCursorPosition);
}

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo,
                                      _In_ const COORD* const pCursorPosition)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    RETURN_HR_IF(E_INVALIDARG, (pCursorPosition->X >= coordScreenBufferSize.X ||
                                pCursorPosition->Y >= coordScreenBufferSize.Y ||
                                pCursorPosition->X < 0 ||
                                pCursorPosition->Y < 0));

    // MSFT: 15813316 - Try to use this SetCursorPosition call to inherit the cursor position.
    RETURN_IF_FAILED(gci.GetVtIo()->SetCursorPosition(*pCursorPosition));

    RETURN_IF_NTSTATUS_FAILED(pScreenInfo->SetCursorPosition(*pCursorPosition, TRUE));

    LOG_IF_NTSTATUS_FAILED(ConsoleImeResizeCompStrView());

    COORD WindowOrigin;
    WindowOrigin.X = 0;
    WindowOrigin.Y = 0;
    {
        const SMALL_RECT currentViewport = pScreenInfo->GetBufferViewport();
        if (currentViewport.Left > pCursorPosition->X)
        {
            WindowOrigin.X = pCursorPosition->X - currentViewport.Left;
        }
        else if (currentViewport.Right < pCursorPosition->X)
        {
            WindowOrigin.X = pCursorPosition->X - currentViewport.Right;
        }

        if (currentViewport.Top > pCursorPosition->Y)
        {
            WindowOrigin.Y = pCursorPosition->Y - currentViewport.Top;
        }
        else if (currentViewport.Bottom < pCursorPosition->Y)
        {
            WindowOrigin.Y = pCursorPosition->Y - currentViewport.Bottom;
        }
    }

    RETURN_IF_NTSTATUS_FAILED(pScreenInfo->SetViewportOrigin(FALSE, WindowOrigin));

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ ULONG const CursorSize,
                                              _In_ BOOLEAN const IsVisible)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleCursorInfo(pContext->GetActiveBuffer(), CursorSize, IsVisible);
}

[[nodiscard]]
HRESULT DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                                  _In_ ULONG const CursorSize,
                                  _In_ BOOLEAN const IsVisible)
{
    // If more than 100% or less than 0% cursor height, reject it.
    RETURN_HR_IF(E_INVALIDARG, (CursorSize > 100 || CursorSize == 0));

    pScreenInfo->SetCursorInformation(CursorSize,
                                      IsVisible,
                                      pScreenInfo->TextInfo->GetCursor()->GetColor(),
                                      pScreenInfo->TextInfo->GetCursor()->GetType());

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleWindowInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ BOOLEAN const IsAbsoluteRectangle,
                                              _In_ const SMALL_RECT* const pWindowRectangle)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleWindowInfo(pContext->GetActiveBuffer(), IsAbsoluteRectangle, pWindowRectangle);
}

[[nodiscard]]
HRESULT DoSrvSetConsoleWindowInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                                  _In_ BOOLEAN const IsAbsoluteRectangle,
                                  _In_ const SMALL_RECT* const pWindowRectangle)
{
    SMALL_RECT Window = *pWindowRectangle;

    if (!IsAbsoluteRectangle)
    {
        SMALL_RECT currentViewport = pScreenInfo->GetBufferViewport();
        Window.Left += currentViewport.Left;
        Window.Right += currentViewport.Right;
        Window.Top += currentViewport.Top;
        Window.Bottom += currentViewport.Bottom;
    }

    RETURN_HR_IF(E_INVALIDARG, (Window.Right < Window.Left || Window.Bottom < Window.Top));

    COORD NewWindowSize;
    NewWindowSize.X = (SHORT)(CalcWindowSizeX(&Window));
    NewWindowSize.Y = (SHORT)(CalcWindowSizeY(&Window));

    COORD const coordMax = pScreenInfo->GetMaxWindowSizeInCharacters();

    RETURN_HR_IF(E_INVALIDARG, (NewWindowSize.X > coordMax.X || NewWindowSize.Y > coordMax.Y));

    // Even if it's the same size, we need to post an update in case the scroll bars need to go away.
    pScreenInfo->SetViewportRect(Viewport::FromInclusive(Window));
    if (pScreenInfo->IsActiveScreenBuffer())
    {
        // TODO: MSFT: 9574827 - shouldn't we be looking at or at least logging the failure codes here? (Or making them non-void?)
        pScreenInfo->PostUpdateWindowSize();
        WriteToScreen(pScreenInfo, pScreenInfo->GetBufferViewport());
    }
    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::ScrollConsoleScreenBufferAImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ char const chFill,
                                                    _In_ WORD const attrFill)
{
    wchar_t const wchFill = CharToWchar(&chFill, 1);

    return ScrollConsoleScreenBufferWImpl(pContext,
                                          pSourceRectangle,
                                          pTargetOrigin,
                                          pTargetClipRectangle,
                                          wchFill,
                                          attrFill);
}


[[nodiscard]]
HRESULT ApiRoutines::ScrollConsoleScreenBufferWImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                    _In_ const SMALL_RECT* const pSourceRectangle,
                                                    _In_ const COORD* const pTargetOrigin,
                                                    _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                                    _In_ wchar_t const wchFill,
                                                    _In_ WORD const attrFill)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvScrollConsoleScreenBufferW(pContext,
                                           pSourceRectangle,
                                           pTargetOrigin,
                                           pTargetClipRectangle,
                                           wchFill,
                                           attrFill);
}

[[nodiscard]]
HRESULT DoSrvScrollConsoleScreenBufferW(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                        _In_ const SMALL_RECT* const pSourceRectangle,
                                        _In_ const COORD* const pTargetOrigin,
                                        _In_opt_ const SMALL_RECT* const pTargetClipRectangle,
                                        _In_ wchar_t const wchFill,
                                        _In_ WORD const attrFill)
{
    // TODO: MSFT 9574849 - can we make scroll region use these as const so we don't have to screw with them?
    // Scroll region takes non-const parameters but our API has been updated to use maximal consts.
    // As such, we will have to copy some items into local variables to prevent the const-ness from leaking beyond this point
    // until 9574849 is fixed (as above).

    SMALL_RECT* pClipRect = nullptr; // We have to pass nullptr if there wasn't a parameter given to us.
    SMALL_RECT ClipRect = { 0 };
    if (pTargetClipRectangle != nullptr)
    {
        // If there was a valid clip rectangle given, copy its contents to a non-const local and pass that pointer down.
        ClipRect = *pTargetClipRectangle;
        pClipRect = &ClipRect;
    }

    SMALL_RECT ScrollRectangle = *pSourceRectangle;

    CHAR_INFO Fill;
    Fill.Char.UnicodeChar = wchFill;
    Fill.Attributes = attrFill;

    return ScrollRegion(pScreenInfo, &ScrollRectangle, pClipRect, *pTargetOrigin, Fill);
}

// Routine Description:
// - This routine is called when the user changes the screen/popup colors.
// - It goes through the popup structures and changes the saved contents to reflect the new screen/popup colors.
VOID UpdatePopups(IN WORD NewAttributes, IN WORD NewPopupAttributes, IN WORD OldAttributes, IN WORD OldPopupAttributes)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WORD const InvertedOldPopupAttributes = (WORD)(((OldPopupAttributes << 4) & 0xf0) | ((OldPopupAttributes >> 4) & 0x0f));
    WORD const InvertedNewPopupAttributes = (WORD)(((NewPopupAttributes << 4) & 0xf0) | ((NewPopupAttributes >> 4) & 0x0f));
    PLIST_ENTRY const HistoryListHead = &gci.CommandHistoryList;
    PLIST_ENTRY HistoryListNext = HistoryListHead->Blink;
    while (HistoryListNext != HistoryListHead)
    {
        PCOMMAND_HISTORY const History = CONTAINING_RECORD(HistoryListNext, COMMAND_HISTORY, ListLink);
        HistoryListNext = HistoryListNext->Blink;
        if (History->Flags & CLE_ALLOCATED && !CLE_NO_POPUPS(History))
        {
            PLIST_ENTRY const PopupListHead = &History->PopupList;
            PLIST_ENTRY PopupListNext = PopupListHead->Blink;
            while (PopupListNext != PopupListHead)
            {
                PCLE_POPUP const Popup = CONTAINING_RECORD(PopupListNext, CLE_POPUP, ListLink);
                PopupListNext = PopupListNext->Blink;
                PCHAR_INFO OldContents = Popup->OldContents;
                for (SHORT i = Popup->Region.Left; i <= Popup->Region.Right; i++)
                {
                    for (SHORT j = Popup->Region.Top; j <= Popup->Region.Bottom; j++)
                    {
                        if (OldContents->Attributes == OldAttributes)
                        {
                            OldContents->Attributes = NewAttributes;
                        }
                        else if (OldContents->Attributes == OldPopupAttributes)
                        {
                            OldContents->Attributes = NewPopupAttributes;
                        }
                        else if (OldContents->Attributes == InvertedOldPopupAttributes)
                        {
                            OldContents->Attributes = InvertedNewPopupAttributes;
                        }

                        OldContents++;
                    }
                }
            }
        }
    }
}

void SetScreenColors(_In_ SCREEN_INFORMATION* ScreenInfo,
                     _In_ WORD Attributes,
                     _In_ WORD PopupAttributes,
                     _In_ BOOL UpdateWholeScreen)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    const TextAttribute oldPrimaryAttributes = ScreenInfo->GetAttributes();
    const TextAttribute oldPopupAttributes = *ScreenInfo->GetPopupAttributes();
    const WORD DefaultAttributes = oldPrimaryAttributes.GetLegacyAttributes();
    const WORD DefaultPopupAttributes = oldPopupAttributes.GetLegacyAttributes();

    const TextAttribute NewPrimaryAttributes = TextAttribute(Attributes);
    const TextAttribute NewPopupAttributes = TextAttribute(PopupAttributes);

    ScreenInfo->SetDefaultAttributes(NewPrimaryAttributes, NewPopupAttributes);
    gci.ConsoleIme.RefreshAreaAttributes();

    if (UpdateWholeScreen)
    {
        ScreenInfo->ReplaceDefaultAttributes(oldPrimaryAttributes,
                                             oldPopupAttributes,
                                             NewPrimaryAttributes,
                                             NewPopupAttributes);

        if (gci.PopupCount)
        {
            UpdatePopups(Attributes, PopupAttributes, DefaultAttributes, DefaultPopupAttributes);
        }

        // force repaint of entire line
        WriteToScreen(ScreenInfo, ScreenInfo->GetBufferViewport());
    }
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleTextAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                 _In_ WORD const Attribute)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleTextAttribute(pContext, Attribute);
}

[[nodiscard]]
HRESULT DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute)
{
    RETURN_HR_IF(E_INVALIDARG, IsAnyFlagSet(Attribute, ~VALID_TEXT_ATTRIBUTES));

    SetScreenColors(pScreenInfo, Attribute, pScreenInfo->GetPopupAttributes()->GetLegacyAttributes(), FALSE);
    return S_OK;
}

void DoSrvPrivateSetLegacyAttributes(_In_ SCREEN_INFORMATION* pScreenInfo,
                                     _In_ WORD const Attribute,
                                     _In_ const bool fForeground,
                                     _In_ const bool fBackground,
                                     _In_ const bool fMeta)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    const TextAttribute OldAttributes = pScreenInfo->GetAttributes();
    TextAttribute NewAttributes;

    NewAttributes.SetFrom(OldAttributes);

    // Always update the legacy component. This prevents the 1m in "^[[32m^[[1m"
    //  from resetting the colors set by the 32m. (for example)
    WORD wNewLegacy = NewAttributes.GetLegacyAttributes();
    if (fForeground)
    {
        UpdateFlagsInMask(wNewLegacy, FG_ATTRS, Attribute);
    }
    if (fBackground)
    {
        UpdateFlagsInMask(wNewLegacy, BG_ATTRS, Attribute);
    }
    if (fMeta)
    {
        UpdateFlagsInMask(wNewLegacy, META_ATTRS, Attribute);
    }
    NewAttributes.SetFromLegacy(wNewLegacy);

    if (!OldAttributes.IsLegacy())
    {
        // The previous call to SetFromLegacy is going to trash our RGB.
        // Restore it.
        // If the old attributes were "reversed", and the new ones aren't,
        //  then flip the colors back.
        bool resetReverse = fMeta &&
            (IsFlagClear(Attribute, COMMON_LVB_REVERSE_VIDEO)) &&
            (IsFlagSet(OldAttributes.GetLegacyAttributes(), COMMON_LVB_REVERSE_VIDEO));
        if (resetReverse)
        {
            NewAttributes.SetForeground(OldAttributes.CalculateRgbBackground());
            NewAttributes.SetBackground(OldAttributes.CalculateRgbForeground());
        }
        else
        {
            NewAttributes.SetForeground(OldAttributes.CalculateRgbForeground());
            NewAttributes.SetBackground(OldAttributes.CalculateRgbBackground());
        }

        if (fForeground)
        {
            COLORREF rgbColor = gci.GetColorTableEntry(Attribute & FG_ATTRS);
            NewAttributes.SetForeground(rgbColor);
        }
        if (fBackground)
        {
            COLORREF rgbColor = gci.GetColorTableEntry((Attribute >> 4) & FG_ATTRS);
            NewAttributes.SetBackground(rgbColor);
        }
        if (fMeta)
        {
            NewAttributes.SetMetaAttributes(Attribute);
        }
    }

    pScreenInfo->SetAttributes(NewAttributes);
}

void DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                              _In_ const int iXtermTableEntry,
                                              _In_ const bool fIsForeground)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    TextAttribute NewAttributes;
    NewAttributes.SetFrom(pScreenInfo->GetAttributes());

    COLORREF rgbColor;
    if (iXtermTableEntry < COLOR_TABLE_SIZE)
    {
        //Convert the xterm index to the win index
        WORD iWinEntry = ::XtermToWindowsIndex(iXtermTableEntry);

        rgbColor = gci.GetColorTableEntry(iWinEntry);
    }
    else
    {
        rgbColor = gci.GetColorTableEntry(iXtermTableEntry);
    }

    NewAttributes.SetColor(rgbColor, fIsForeground);

    pScreenInfo->SetAttributes(NewAttributes);
}

void DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* const pScreenInfo,
                                            _In_ const COLORREF rgbColor,
                                            _In_ const bool fIsForeground)
{
    TextAttribute NewAttributes;
    NewAttributes.SetFrom(pScreenInfo->GetAttributes());
    NewAttributes.SetColor(rgbColor, fIsForeground);
    pScreenInfo->SetAttributes(NewAttributes);
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleOutputCodePageImpl(_In_ ULONG const CodePage)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Return if it's not known as a valid codepage ID.
    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidCodePage(CodePage));

    // Do nothing if no change.
    if (gci.OutputCP != CodePage)
    {
        // Set new code page
        gci.OutputCP = CodePage;

        SetConsoleCPInfo(TRUE);
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleInputCodePageImpl(_In_ ULONG const CodePage)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Return if it's not known as a valid codepage ID.
    RETURN_HR_IF_FALSE(E_INVALIDARG, IsValidCodePage(CodePage));

    // Do nothing if no change.
    if (gci.CP != CodePage)
    {
        // Set new code page
        gci.CP = CodePage;

        SetConsoleCPInfo(FALSE);
    }

    return S_OK;
}

void ApiRoutines::GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pCodePage = gci.CP;
}

void DoSrvGetConsoleOutputCodePage(_Out_ unsigned int* const pCodePage)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    *pCodePage = gci.OutputCP;
}

void ApiRoutines::GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });
    unsigned int uiCodepage;
    DoSrvGetConsoleOutputCodePage(&uiCodepage);
    *pCodePage = uiCodepage;
}

void ApiRoutines::GetConsoleWindowImpl(_Out_ HWND* const pHwnd)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });
    IConsoleWindow* pWindow = ServiceLocator::LocateConsoleWindow();
    if (pWindow != nullptr)
    {
        *pHwnd = pWindow->GetWindowHandle();
    }
    else
    {
        *pHwnd = NULL;
    }
}

void ApiRoutines::GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    pConsoleHistoryInfo->HistoryBufferSize = gci.GetHistoryBufferSize();
    pConsoleHistoryInfo->NumberOfHistoryBuffers = gci.GetNumberOfHistoryBuffers();
    SetFlagIf(pConsoleHistoryInfo->dwFlags, HISTORY_NO_DUP_FLAG, IsFlagSet(gci.Flags, CONSOLE_HISTORY_NODUP));
}

HRESULT ApiRoutines::SetConsoleHistoryInfoImpl(_In_ const CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    RETURN_HR_IF(E_INVALIDARG, pConsoleHistoryInfo->HistoryBufferSize > SHORT_MAX);
    RETURN_HR_IF(E_INVALIDARG, pConsoleHistoryInfo->NumberOfHistoryBuffers > SHORT_MAX);
    RETURN_HR_IF(E_INVALIDARG, IsAnyFlagSet(pConsoleHistoryInfo->dwFlags, ~CHI_VALID_FLAGS));

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    ResizeCommandHistoryBuffers(pConsoleHistoryInfo->HistoryBufferSize);
    gci.SetNumberOfHistoryBuffers(pConsoleHistoryInfo->NumberOfHistoryBuffers);

    UpdateFlag(gci.Flags, CONSOLE_HISTORY_NODUP, IsFlagSet(pConsoleHistoryInfo->dwFlags, HISTORY_NO_DUP_FLAG));

    return S_OK;
}

// NOTE: This was in private.c, but turns out to be a public API: http://msdn.microsoft.com/en-us/library/windows/desktop/ms683164(v=vs.85).aspx
void ApiRoutines::GetConsoleDisplayModeImpl(_Out_ ULONG* const pFlags)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // Initialize flags portion of structure
    *pFlags = 0;

    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    if (pWindow != nullptr && pWindow->IsInFullscreen())
    {
        SetFlag(*pFlags, CONSOLE_FULLSCREEN_MODE);
    }
}

// Routine Description:
// - This routine sets the console display mode for an output buffer.
// - This API is only supported on x86 machines.
// Parameters:
// - hConsoleOutput - Supplies a console output handle.
// - dwFlags - Specifies the display mode. Options are:
//      CONSOLE_FULLSCREEN_MODE - data is displayed fullscreen
//      CONSOLE_WINDOWED_MODE - data is displayed in a window
// - lpNewScreenBufferDimensions - On output, contains the new dimensions of the screen buffer.  The dimensions are in rows and columns for textmode screen buffers.
// Return value:
// - TRUE - The operation was successful.
// - FALSE/nullptr - The operation failed. Extended error status is available using GetLastError.
// NOTE:
// - This was in private.c, but turns out to be a public API:
// - See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686028(v=vs.85).aspx
[[nodiscard]]
HRESULT ApiRoutines::SetConsoleDisplayModeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                               _In_ ULONG const Flags,
                                               _Out_ COORD* const pNewScreenBufferSize)
{
    // SetIsFullscreen() below ultimately calls SetwindowLong, which ultimately calls SendMessage(). If we retain
    // the console lock, we'll deadlock since ConsoleWindowProc takes the lock before processing messages. Instead,
    // we'll release early.
    LockConsole();
    {
        auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

        SCREEN_INFORMATION* const pScreenInfo = pContext->GetActiveBuffer();

        *pNewScreenBufferSize = pScreenInfo->GetScreenBufferSize();
        RETURN_HR_IF_FALSE(E_INVALIDARG, pScreenInfo->IsActiveScreenBuffer());
    }

    IConsoleWindow* const pWindow = ServiceLocator::LocateConsoleWindow();
    if (IsFlagSet(Flags, CONSOLE_FULLSCREEN_MODE))
    {
        if (pWindow != nullptr)
        {
            pWindow->SetIsFullscreen(true);
        }
    }
    else if (IsFlagSet(Flags, CONSOLE_WINDOWED_MODE))
    {
        if (pWindow != nullptr)
        {
            pWindow->SetIsFullscreen(false);
        }
    }
    else
    {
        RETURN_HR(E_INVALIDARG);
    }

    return S_OK;
}

// Routine Description:
// - A private API call for changing the cursor keys input mode between normal and application mode.
//     The cursor keys are the arrows, plus Home and End.
// Parameters:
// - fApplicationMode - set to true to enable Application Mode Input, false for Numeric Mode Input.
// Return value:
// - True if handled successfully. False otherwise.
[[nodiscard]]
NTSTATUS DoSrvPrivateSetCursorKeysMode(_In_ bool fApplicationMode)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (gci.pInputBuffer == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    gci.pInputBuffer->GetTerminalInput().ChangeCursorKeysMode(fApplicationMode);
    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for changing the keypad input mode between numeric and application mode.
//     This controls what the keys on the numpad translate to.
// Parameters:
// - fApplicationMode - set to true to enable Application Mode Input, false for Numeric Mode Input.
// Return value:
// - True if handled successfully. False otherwise.
[[nodiscard]]
NTSTATUS DoSrvPrivateSetKeypadMode(_In_ bool fApplicationMode)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    if (gci.pInputBuffer == nullptr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    gci.pInputBuffer->GetTerminalInput().ChangeKeypadMode(fApplicationMode);
    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling or disabling the cursor blinking.
// Parameters:
// - fEnable - set to true to enable blinking, false to disable
// Return value:
// - True if handled successfully. False otherwise.
void DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* const pScreenInfo, _In_ bool const fEnable)
{
    pScreenInfo->TextInfo->GetCursor()->SetBlinkingAllowed(fEnable);
    pScreenInfo->TextInfo->GetCursor()->SetIsOn(!fEnable);
}

// Routine Description:
// - A private API call for setting the top and bottom scrolling margins for
//     the current page. This creates a subsection of the screen that scrolls
//     when input reaches the end of the region, leaving the rest of the screen
//     untouched.
//  Currently only accessible through the use of ANSI sequence DECSTBM
// Parameters:
// - psrScrollMargins - A rect who's Top and Bottom members will be used to set
//     the new values of the top and bottom margins. If (0,0), then the margins
//     will be disabled. NOTE: This is a rect in the case that we'll need the
//     left and right margins in the future.
// Return value:
// - True if handled successfully. False otherwise.
[[nodiscard]]
NTSTATUS DoSrvPrivateSetScrollingRegion(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const SMALL_RECT* const psrScrollMargins)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (psrScrollMargins->Top > psrScrollMargins->Bottom)
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    if (NT_SUCCESS(Status))
    {
        SMALL_RECT srViewport = pScreenInfo->GetBufferViewport();
        SMALL_RECT srScrollMargins = pScreenInfo->GetScrollMargins();
        srScrollMargins.Top = psrScrollMargins->Top;
        srScrollMargins.Bottom = psrScrollMargins->Bottom;
        pScreenInfo->SetScrollMargins(&srScrollMargins);

    }

    return Status;
}

// Routine Description:
// - A private API call for performing a "Reverse line feed", essentially, the opposite of '\n'.
//    Moves the cursor up one line, and tries to keep its position in the line
// Parameters:
// - pScreenInfo - a pointer to the screen buffer that should perform the reverse line feed
// Return value:
// - True if handled successfully. False otherwise.
[[nodiscard]]
NTSTATUS DoSrvPrivateReverseLineFeed(_In_ SCREEN_INFORMATION* pScreenInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;

    const SMALL_RECT viewport = pScreenInfo->GetBufferViewport();
    COORD newCursorPosition = pScreenInfo->TextInfo->GetCursor()->GetPosition();

    // If the cursor is at the top of the viewport, we don't want to shift the viewport up.
    // We want it to stay exactly where it is.
    // In that case, shift the buffer contents down, to emulate inserting a line
    //      at the top of the buffer.
    if (newCursorPosition.Y > viewport.Top)
    {
        // Cursor is below the top line of the viewport
        newCursorPosition.Y -= 1;
        Status = AdjustCursorPosition(pScreenInfo, newCursorPosition, TRUE, nullptr);
    }
    else
    {
        // Cursor is at the top of the viewport
        const COORD bufferSize = pScreenInfo->GetScreenBufferSize();
        // Rectangle to cut out of the existing buffer
        SMALL_RECT srScroll;
        srScroll.Left = 0;
        srScroll.Right = bufferSize.X;
        srScroll.Top = viewport.Top;
        srScroll.Bottom = viewport.Bottom - 1;
        // Paste coordinate for cut text above
        COORD coordDestination;
        coordDestination.X = 0;
        coordDestination.Y = viewport.Top + 1;

        SMALL_RECT srClip = viewport;

        Status = DoSrvScrollConsoleScreenBufferW(pScreenInfo, &srScroll, &coordDestination, &srClip, L' ', pScreenInfo->GetAttributes().GetLegacyAttributes());
    }
    return Status;
}

// Routine Description:
// - A private API call for swaping to the alternate screen buffer. In virtual terminals, there exists both a "main"
//     screen buffer and an alternate. ASBSET creates a new alternate, and switches to it. If there is an already
//     existing alternate, it is discarded.
// Parameters:
// - psiCurr - a pointer to the screen buffer that should use an alternate buffer
// Return value:
// - True if handled successfully. False otherwise.
[[nodiscard]]
NTSTATUS DoSrvPrivateUseAlternateScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr)
{
    return psiCurr->UseAlternateScreenBuffer();
}

// Routine Description:
// - A private API call for swaping to the main screen buffer. From the
//     alternate buffer, returns to the main screen buffer. From the main
//     screen buffer, does nothing. The alternate is discarded.
// Parameters:
// - psiCurr - a pointer to the screen buffer that should use the main buffer
// Return value:
// - True if handled successfully. False otherwise.
void DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr)
{
    psiCurr->UseMainScreenBuffer();
}

// Routine Description:
// - A private API call for setting a VT tab stop in the cursor's current column.
// Parameters:
// <none>
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS DoSrvPrivateHorizontalTabSet()
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* const pScreenBuffer = gci.CurrentScreenBuffer;

    const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
    return pScreenBuffer->AddTabStop(cursorPos.X);
}

// Routine Description:
// - A private helper for excecuting a number of tabs.
// Parameters:
// sNumTabs - The number of tabs to execute
// fForward - whether to tab forward or backwards
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS DoPrivateTabHelper(_In_ SHORT const sNumTabs, _In_ bool fForward)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* const pScreenBuffer = gci.CurrentScreenBuffer;

    NTSTATUS Status = STATUS_SUCCESS;
    ASSERT(sNumTabs >= 0);
    for (SHORT sTabsExecuted = 0; sTabsExecuted < sNumTabs && NT_SUCCESS(Status); sTabsExecuted++)
    {
        const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
        COORD cNewPos = (fForward) ? pScreenBuffer->GetForwardTab(cursorPos) : pScreenBuffer->GetReverseTab(cursorPos);
        // GetForwardTab is smart enough to move the cursor to the next line if
        // it's at the end of the current one already. AdjustCursorPos shouldn't
        // to be doing anything funny, just moving the cursor to the location GetForwardTab returns
        Status = AdjustCursorPosition(pScreenBuffer, cNewPos, TRUE, nullptr);
    }
    return Status;
}

// Routine Description:
// - A private API call for performing a forwards tab. This will take the
//     cursor to the tab stop following its current location. If there are no
//     more tabs in this row, it will take it to the right side of the window.
//     If it's already in the last column of the row, it will move it to the next line.
// Parameters:
// - sNumTabs - The number of tabs to perform.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS DoSrvPrivateForwardTab(_In_ SHORT const sNumTabs)
{
    return DoPrivateTabHelper(sNumTabs, true);
}

// Routine Description:
// - A private API call for performing a backwards tab. This will take the
//     cursor to the tab stop previous to its current location. It will not reverse line feed.
// Parameters:
// - sNumTabs - The number of tabs to perform.
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
[[nodiscard]]
NTSTATUS DoSrvPrivateBackwardsTab(_In_ SHORT const sNumTabs)
{
    return DoPrivateTabHelper(sNumTabs, false);
}

// Routine Description:
// - A private API call for clearing the VT tabs that have been set.
// Parameters:
// - fClearAll - If false, only clears the tab in the current column (if it exists)
//      otherwise clears all set tabs. (and reverts to lecacy 8-char tabs behavior.)
// Return value:
// - None
void DoSrvPrivateTabClear(_In_ bool const fClearAll)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    SCREEN_INFORMATION* const pScreenBuffer = gci.CurrentScreenBuffer;
    if (fClearAll)
    {
        pScreenBuffer->ClearTabStops();
    }
    else
    {
        const COORD cursorPos = pScreenBuffer->TextInfo->GetCursor()->GetPosition();
        pScreenBuffer->ClearTabStop(cursorPos.X);
    }
}

// Routine Description:
// - A private API call for enabling VT200 style mouse mode.
// Parameters:
// - fEnable - true to enable default tracking mode, false to disable mouse mode.
// Return value:
// - None
void DoSrvPrivateEnableVT200MouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableDefaultTracking(fEnable);
}

// Routine Description:
// - A private API call for enabling utf8 style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - None
void DoSrvPrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.SetUtf8ExtendedMode(fEnable);
}

// Routine Description:
// - A private API call for enabling SGR style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - None
void DoSrvPrivateEnableSGRExtendedMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.SetSGRExtendedMode(fEnable);
}

// Routine Description:
// - A private API call for enabling button-event mouse mode.
// Parameters:
// - fEnable - true to enable button-event mode, false to disable mouse mode.
// Return value:
// - None
void DoSrvPrivateEnableButtonEventMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableButtonEventTracking(fEnable);
}

// Routine Description:
// - A private API call for enabling any-event mouse mode.
// Parameters:
// - fEnable - true to enable any-event mode, false to disable mouse mode.
// Return value:
// - None
void DoSrvPrivateEnableAnyEventMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableAnyEventTracking(fEnable);
}

// Routine Description:
// - A private API call for enabling alternate scroll mode
// Parameters:
// - fEnable - true to enable alternate scroll mode, false to disable.
// Return value:
// None
void DoSrvPrivateEnableAlternateScroll(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableAlternateScroll(fEnable);
}

// Routine Description:
// - A private API call for performing a VT-style erase all operation on the buffer.
//      See SCREEN_INFORMATION::VtEraseAll's description for details.
// Parameters:
//  The ScreenBuffer to perform the erase on.
// Return value:
// - STATUS_SUCCESS if we succeeded, otherwise the NTSTATUS version of the failure.
[[nodiscard]]
NTSTATUS DoSrvPrivateEraseAll(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    return NTSTATUS_FROM_HRESULT(pScreenInfo->GetActiveBuffer()->VtEraseAll());
}

void DoSrvSetCursorStyle(_In_ const SCREEN_INFORMATION* const pScreenInfo,
                         _In_ CursorType const cursorType)
{
    pScreenInfo->TextInfo->GetCursor()->SetType(cursorType);
}

void DoSrvSetCursorColor(_In_ const SCREEN_INFORMATION* const pScreenInfo,
                         _In_ COLORREF const cursorColor)
{
    pScreenInfo->TextInfo->GetCursor()->SetColor(cursorColor);
}

// Routine Description:
// - A private API call to get only the default color attributes of the screen buffer.
// - This is used as a performance optimization by the VT adapter in SGR (Set Graphics Rendition) instead
//   of calling for this information through the public API GetConsoleScreenBufferInfoEx which returns a lot
//   of extra unnecessary data and takes a lot of extra processing time.
// Parameters
// - pScreenInfo - The screen buffer to retrieve default color attributes information from
// - pwAttributes - Pointer to space that will receive color attributes data
// Return Value:
// - STATUS_SUCCESS if we succeeded or STATUS_INVALID_PARAMETER for bad params (nullptr).
[[nodiscard]]
NTSTATUS DoSrvPrivateGetConsoleScreenBufferAttributes(_In_ SCREEN_INFORMATION* const pScreenInfo, _Out_ WORD* const pwAttributes)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pScreenInfo == nullptr || pwAttributes == nullptr)
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(Status))
    {
        *pwAttributes = pScreenInfo->GetActiveBuffer()->GetAttributes().GetLegacyAttributes();
    }

    return Status;
}

// Routine Description:
// - A private API call for forcing the renderer to repaint the screen. If the
//      input screen buffer is not the active one, then just do nothing. We only
//      want to redraw the screen buffer that requested the repaint, and
//      switching screen buffers will already force a repaint.
// Parameters:
//  The ScreenBuffer to perform the repaint for.
// Return value:
// - None
void DoSrvPrivateRefreshWindow(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    Globals& g = ServiceLocator::LocateGlobals();
    if (pScreenInfo == g.getConsoleInformation().CurrentScreenBuffer)
    {
        g.pRender->TriggerRedrawAll();
    }
}

void GetConsoleTitleWImplHelper(_Out_writes_to_opt_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTitleBuffer,
                                   _In_ size_t const cchTitleBufferSize,
                                   _Out_ size_t* const pcchTitleBufferWritten,
                                   _Out_ size_t* const pcchTitleBufferNeeded,
                                   _In_ bool const fIsOriginal)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Ensure output variables are initialized.
    *pcchTitleBufferWritten = 0;
    *pcchTitleBufferNeeded = 0;

    if (nullptr != pwsTitleBuffer)
    {
        *pwsTitleBuffer = L'\0';
    }

    // Get the appropriate title and length depending on the mode.
    const wchar_t* pwszTitle;
    size_t cchTitleLength;

    if (fIsOriginal)
    {
        pwszTitle = gci.GetOriginalTitle().c_str();
        cchTitleLength = gci.GetOriginalTitle().length();
    }
    else
    {
        pwszTitle = gci.GetTitle().c_str();
        cchTitleLength = gci.GetTitle().length();
    }

    // Always report how much space we would need.
    *pcchTitleBufferNeeded = cchTitleLength;

    // If we have a pointer to receive the data, then copy it out.
    if (nullptr != pwsTitleBuffer)
    {
        HRESULT const hr = StringCchCopyNW(pwsTitleBuffer, cchTitleBufferSize, pwszTitle, cchTitleLength);

        // Insufficient buffer is allowed. If we return a partial string, that's still OK by historical/compat standards.
        // Just say how much we managed to return.
        if (SUCCEEDED(hr) || STRSAFE_E_INSUFFICIENT_BUFFER == hr)
        {
            *pcchTitleBufferWritten = std::min(cchTitleBufferSize, cchTitleLength);
        }
    }
}

[[nodiscard]]
HRESULT GetConsoleTitleAImplHelper(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                   _In_ size_t const cchTitleBufferSize,
                                   _Out_ size_t* const pcchTitleBufferWritten,
                                   _Out_ size_t* const pcchTitleBufferNeeded,
                                   _In_ bool const fIsOriginal)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // Ensure output variables are initialized.
    *pcchTitleBufferWritten = 0;
    *pcchTitleBufferNeeded = 0;

    if (nullptr != psTitleBuffer)
    {
        *psTitleBuffer = '\0';
    }

    // Figure out how big our temporary Unicode buffer must be to get the title.
    size_t cchUnicodeTitleBufferNeeded;
    size_t cchUnicodeTitleBufferWritten;
    GetConsoleTitleWImplHelper(nullptr, 0, &cchUnicodeTitleBufferWritten, &cchUnicodeTitleBufferNeeded, fIsOriginal);

    // If there's nothing to get, then simply return.
    RETURN_HR_IF(S_OK, 0 == cchUnicodeTitleBufferNeeded);

    // Allocate a unicode buffer of the right size.
    size_t const cchUnicodeTitleBufferSize = cchUnicodeTitleBufferNeeded + 1; // add one for null terminator space
    wistd::unique_ptr<wchar_t[]> pwsUnicodeTitleBuffer = wil::make_unique_nothrow<wchar_t[]>(cchUnicodeTitleBufferSize);
    RETURN_IF_NULL_ALLOC(pwsUnicodeTitleBuffer);

    // Retrieve the title in Unicode.
    GetConsoleTitleWImplHelper(pwsUnicodeTitleBuffer.get(),
                               cchUnicodeTitleBufferSize,
                               &cchUnicodeTitleBufferWritten,
                               &cchUnicodeTitleBufferNeeded,
                               fIsOriginal);

    // Convert result to A
    wistd::unique_ptr<char[]> psConverted;
    size_t cchConverted;
    RETURN_IF_FAILED(ConvertToA(gci.CP,
                                pwsUnicodeTitleBuffer.get(),
                                cchUnicodeTitleBufferWritten,
                                psConverted,
                                cchConverted));

    // The legacy A behavior is a bit strange. If the buffer given doesn't have enough space to hold
    // the string without null termination (e.g. the title is 9 long, 10 with null. The buffer given isn't >= 9).
    // then do not copy anything back and do not report how much space we need.
    if (cchTitleBufferSize >= cchConverted)
    {
        // Say how many characters of buffer we would need to hold the entire result.
        *pcchTitleBufferNeeded = cchConverted;

        // Copy safely to output buffer
        HRESULT const hr = StringCchCopyNA(psTitleBuffer,
                                           cchTitleBufferSize,
                                           psConverted.get(),
                                           cchConverted);


        // Insufficient buffer is allowed. If we return a partial string, that's still OK by historical/compat standards.
        // Just say how much we managed to return.
        if (SUCCEEDED(hr) || STRSAFE_E_INSUFFICIENT_BUFFER == hr)
        {
            // And return the size copied (either the size of the buffer or the null terminated length of the string we filled it with.)
            *pcchTitleBufferWritten = std::min(cchTitleBufferSize, cchConverted + 1);

            // Another compatibility fix... If we had exactly the number of bytes needed for an unterminated string,
            // then replace the terminator left behind by StringCchCopyNA with the final character of the title string.
            if (cchTitleBufferSize == cchConverted)
            {
                psTitleBuffer[cchTitleBufferSize - 1] = psConverted.get()[cchConverted - 1];
            }
        }
    }
    else
    {
        // If we didn't copy anything back and there is space, null terminate the given buffer and return.
        if (cchTitleBufferSize > 0)
        {
            psTitleBuffer[0] = '\0';
            *pcchTitleBufferWritten = 1;
        }
    }

    return S_OK;
}

[[nodiscard]]
HRESULT ApiRoutines::GetConsoleTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                          _In_ size_t const cchTitleBufferSize,
                                          _Out_ size_t* const pcchTitleBufferWritten,
                                          _Out_ size_t* const pcchTitleBufferNeeded)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleTitleAImplHelper(psTitleBuffer,
                                      cchTitleBufferSize,
                                      pcchTitleBufferWritten,
                                      pcchTitleBufferNeeded,
                                      false);
}

void ApiRoutines::GetConsoleTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTitleBuffer,
                                          _In_ size_t const cchTitleBufferSize,
                                          _Out_ size_t* const pcchTitleBufferWritten,
                                          _Out_ size_t* const pcchTitleBufferNeeded)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    GetConsoleTitleWImplHelper(pwsTitleBuffer,
                               cchTitleBufferSize,
                               pcchTitleBufferWritten,
                               pcchTitleBufferNeeded,
                               false);
}

[[nodiscard]]
HRESULT ApiRoutines::GetConsoleOriginalTitleAImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) char* const psTitleBuffer,
                                                  _In_ size_t const cchTitleBufferSize,
                                                  _Out_ size_t* const pcchTitleBufferWritten,
                                                  _Out_ size_t* const pcchTitleBufferNeeded)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return GetConsoleTitleAImplHelper(psTitleBuffer,
                                      cchTitleBufferSize,
                                      pcchTitleBufferWritten,
                                      pcchTitleBufferNeeded,
                                      true);
}

void ApiRoutines::GetConsoleOriginalTitleWImpl(_Out_writes_to_(cchTitleBufferSize, *pcchTitleBufferWritten) _Always_(_Post_z_) wchar_t* const pwsTitleBuffer,
                                                  _In_ size_t const cchTitleBufferSize,
                                                  _Out_ size_t* const pcchTitleBufferWritten,
                                                  _Out_ size_t* const pcchTitleBufferNeeded)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    GetConsoleTitleWImplHelper(pwsTitleBuffer,
                               cchTitleBufferSize,
                               pcchTitleBufferWritten,
                               pcchTitleBufferNeeded,
                               true);
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleTitleAImpl(_In_reads_or_z_(cchTitleBufferSize) const char* const psTitleBuffer,
                                          _In_ size_t const cchTitleBufferSize)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    wistd::unique_ptr<wchar_t[]> pwsUnicodeTitleBuffer;
    size_t cchUnicodeTitleBuffer;
    RETURN_IF_FAILED(ConvertToW(gci.CP,
                                psTitleBuffer,
                                cchTitleBufferSize,
                                pwsUnicodeTitleBuffer,
                                cchUnicodeTitleBuffer));

    return SetConsoleTitleWImpl(pwsUnicodeTitleBuffer.get(), cchUnicodeTitleBuffer);
}

[[nodiscard]]
HRESULT ApiRoutines::SetConsoleTitleWImpl(_In_reads_or_z_(cchTitleBufferSize) const wchar_t* const pwsTitleBuffer,
                                          _In_ size_t const cchTitleBufferSize)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleTitleW(pwsTitleBuffer,
                                 cchTitleBufferSize);
}

[[nodiscard]]
HRESULT DoSrvSetConsoleTitleW(_In_reads_or_z_(cchBuffer) const wchar_t* const pwsBuffer,
                              _In_ size_t const cchBuffer)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    // Sanitize the input if we're in pty mode. No control chars - this string
    //      will get emitted back to the TTY in a VT sequence, and we don't want
    //      to embed control characters in that string.
    if (gci.IsInVtIoMode())
    {
        std::wstringstream ss;
        for(size_t i = 0; i < cchBuffer; i++)
        {
            if (pwsBuffer[i] >= L'\x20')
            {
                ss << pwsBuffer[i];
            }
        }

        gci.SetTitle(std::wstring(ss.str()));
    }
    else
    {
        // SetTitle will trigger the renderer to update the titlebar for us.
        gci.SetTitle(std::wstring(pwsBuffer, cchBuffer));

    }

    return S_OK;
}

// Routine Description:
// - A private API call for forcing the VT Renderer to NOT paint the next resize
//      event. This is used by InteractDispatch, to prevent resizes from echoing
//      between terminal and host.
// Parameters:
//  The ScreenBuffer to perform the repaint for.
// Return value:
// - STATUS_SUCCESS if we succeeded, otherwise the NTSTATUS version of the failure.
[[nodiscard]]
NTSTATUS DoSrvPrivateSuppressResizeRepaint()
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    assert(gci.IsInVtIoMode());
    return NTSTATUS_FROM_HRESULT(gci.GetVtIo()->SuppressResizeRepaint());
}
