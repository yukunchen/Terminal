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

#include "ApiRoutines.h"

#include "..\interactivity\inc\ServiceLocator.hpp"

#pragma hdrstop

// The following mask is used to test for valid text attributes.
#define VALID_TEXT_ATTRIBUTES (FG_ATTRS | BG_ATTRS | META_ATTRS)

#define INPUT_MODES (ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT)
#define OUTPUT_MODES (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_LVB_GRID_WORLDWIDE)
#define PRIVATE_MODES (ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_AUTO_POSITION | ENABLE_EXTENDED_FLAGS)

HRESULT ApiRoutines::GetConsoleInputModeImpl(_In_ InputBuffer* const pContext, _Out_ ULONG* const pMode)
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

    return S_OK;
}

HRESULT ApiRoutines::GetConsoleOutputModeImpl(_In_ SCREEN_INFORMATION* const pContext, _Out_ ULONG* const pMode)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pMode = pContext->GetActiveBuffer()->OutputMode;

    return S_OK;
}

HRESULT ApiRoutines::GetNumberOfConsoleInputEventsImpl(_In_ InputBuffer* const pContext, _Out_ ULONG* const pEvents)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    size_t readyEventCount = pContext->GetNumberOfReadyEvents();
    RETURN_IF_FAILED(SizeTToULong(readyEventCount, pEvents));

    return S_OK;
}

HRESULT ApiRoutines::GetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                      _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvGetConsoleScreenBufferInfo(pContext->GetActiveBuffer(), pScreenBufferInfoEx);
}

HRESULT DoSrvGetConsoleScreenBufferInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Out_ CONSOLE_SCREEN_BUFFER_INFOEX* pInfo)
{
    pInfo->bFullscreenSupported = FALSE; // traditional full screen with the driver support is no longer supported.
    NTSTATUS Status = pScreenInfo->GetScreenBufferInformation(&pInfo->dwSize,
                                                              &pInfo->dwCursorPosition,
                                                              &pInfo->srWindow,
                                                              &pInfo->wAttributes,
                                                              &pInfo->dwMaximumWindowSize,
                                                              &pInfo->wPopupAttributes,
                                                              pInfo->ColorTable);
    // Callers of this function expect to recieve an exclusive rect, not an inclusive one.
    pInfo->srWindow.Right += 1;
    pInfo->srWindow.Bottom += 1;

    RETURN_NTSTATUS(Status);
}

HRESULT ApiRoutines::GetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _Out_ ULONG* const pCursorSize,
                                              _Out_ BOOLEAN* const pIsVisible)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvGetConsoleCursorInfo(pContext->GetActiveBuffer(), pCursorSize, pIsVisible);
}

HRESULT DoSrvGetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo, _Out_ ULONG* const pCursorSize, _Out_ BOOLEAN* const pIsVisible)
{
    *pCursorSize = pScreenInfo->TextInfo->GetCursor()->GetSize();
    *pIsVisible = (BOOLEAN)pScreenInfo->TextInfo->GetCursor()->IsVisible();
    return S_OK;
}

HRESULT ApiRoutines::GetConsoleSelectionInfoImpl(_Out_ CONSOLE_SELECTION_INFO* const pConsoleSelectionInfo)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    const Selection* const pSelection = &Selection::Instance();
    if (pSelection->IsInSelectingState())
    {
        pSelection->GetPublicSelectionFlags(&pConsoleSelectionInfo->dwFlags);

        SetFlag(pConsoleSelectionInfo->dwFlags, CONSOLE_SELECTION_IN_PROGRESS);

        pSelection->GetSelectionAnchor(&pConsoleSelectionInfo->dwSelectionAnchor);
        pSelection->GetSelectionRectangle(&pConsoleSelectionInfo->srSelection);
    }
    else
    {
        ZeroMemory(pConsoleSelectionInfo, sizeof(pConsoleSelectionInfo));
    }

    return S_OK;
}

HRESULT ApiRoutines::GetNumberOfConsoleMouseButtonsImpl(_Out_ ULONG* const pButtons)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pButtons = ServiceLocator::LocateSystemConfigurationProvider()->GetNumberOfMouseButtons();

    return S_OK;
}

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
            gci.CurrentScreenBuffer->SetCursorDBMode(FALSE);
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

HRESULT ApiRoutines::SetConsoleActiveScreenBufferImpl(_In_ SCREEN_INFORMATION* const pNewContext)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    RETURN_NTSTATUS(SetActiveScreenBuffer(pNewContext->GetActiveBuffer()));
}

HRESULT ApiRoutines::FlushConsoleInputBuffer(_In_ InputBuffer* const pContext)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    // TODO: MSFT: 9574827 - shouldn't this have a status code?
    pContext->Flush();

    return S_OK;
}

HRESULT ApiRoutines::GetLargestConsoleWindowSizeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                     _Out_ COORD* const pSize)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    SCREEN_INFORMATION* const pScreenInfo = pContext->GetActiveBuffer();

    *pSize = pScreenInfo->GetLargestWindowSizeInCharacters();

    return S_OK;
}

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

HRESULT ApiRoutines::SetConsoleScreenBufferInfoExImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                      _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pScreenBufferInfoEx)
{
    RETURN_HR_IF(E_INVALIDARG, (pScreenBufferInfoEx->dwSize.X == 0 ||
                                pScreenBufferInfoEx->dwSize.Y == 0 ||
                                pScreenBufferInfoEx->dwSize.X == SHRT_MAX ||
                                pScreenBufferInfoEx->dwSize.Y == SHRT_MAX));

    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetScreenBufferInfo(pContext->GetActiveBuffer(), pScreenBufferInfoEx);
}

HRESULT DoSrvSetScreenBufferInfo(_In_ SCREEN_INFORMATION* const pScreenInfo, _In_ const CONSOLE_SCREEN_BUFFER_INFOEX* const pInfo)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();

    COORD const coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    if (pInfo->dwSize.X != coordScreenBufferSize.X || (pInfo->dwSize.Y != coordScreenBufferSize.Y))
    {
        CommandLine* const pCommandLine = &CommandLine::Instance();

        pCommandLine->Hide(FALSE);

        pScreenInfo->ResizeScreenBuffer(pInfo->dwSize, TRUE);

        pCommandLine->Show();
    }

    gci.SetColorTable(pInfo->ColorTable, ARRAYSIZE(pInfo->ColorTable));
    SetScreenColors(pScreenInfo, pInfo->wAttributes, pInfo->wPopupAttributes, TRUE);

    COORD NewSize;
    NewSize.X = min((pInfo->srWindow.Right - pInfo->srWindow.Left), pInfo->dwMaximumWindowSize.X);
    NewSize.Y = min((pInfo->srWindow.Bottom - pInfo->srWindow.Top), pInfo->dwMaximumWindowSize.Y);

    // If wrap text is on, then the window width must be the same size as the buffer width
    if (gci.GetWrapText())
    {
        NewSize.X = coordScreenBufferSize.X;
    }

    if (NewSize.X != pScreenInfo->GetScreenWindowSizeX() || NewSize.Y != pScreenInfo->GetScreenWindowSizeY())
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

    return S_OK;
}

HRESULT ApiRoutines::SetConsoleCursorPositionImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                  _In_ const COORD* const pCursorPosition)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleCursorPosition(pContext->GetActiveBuffer(), pCursorPosition);
}

HRESULT DoSrvSetConsoleCursorPosition(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ const COORD* const pCursorPosition)
{
    const COORD coordScreenBufferSize = pScreenInfo->GetScreenBufferSize();
    RETURN_HR_IF(E_INVALIDARG, (pCursorPosition->X >= coordScreenBufferSize.X ||
                                pCursorPosition->Y >= coordScreenBufferSize.Y ||
                                pCursorPosition->X < 0 ||
                                pCursorPosition->Y < 0));

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

HRESULT ApiRoutines::SetConsoleCursorInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ ULONG const CursorSize,
                                              _In_ BOOLEAN const IsVisible)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleCursorInfo(pContext->GetActiveBuffer(), CursorSize, IsVisible);
}

HRESULT DoSrvSetConsoleCursorInfo(_In_ SCREEN_INFORMATION* pScreenInfo,
                                  _In_ ULONG const CursorSize,
                                  _In_ BOOLEAN const IsVisible)
{
    // If more than 100% or less than 0% cursor height, reject it.
    RETURN_HR_IF(E_INVALIDARG, (CursorSize > 100 || CursorSize == 0));

    RETURN_IF_NTSTATUS_FAILED(pScreenInfo->SetCursorInformation(CursorSize, IsVisible));

    return S_OK;
}

HRESULT ApiRoutines::SetConsoleWindowInfoImpl(_In_ SCREEN_INFORMATION* const pContext,
                                              _In_ BOOLEAN const IsAbsoluteRectangle,
                                              _In_ const SMALL_RECT* const pWindowRectangle)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleWindowInfo(pContext->GetActiveBuffer(), IsAbsoluteRectangle, pWindowRectangle);
}

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
    NTSTATUS Status = pScreenInfo->SetViewportRect(&Window);
    if (pScreenInfo->IsActiveScreenBuffer())
    {
        // TODO: MSFT: 9574827 - shouldn't we be looking at or at least logging the failure codes here? (Or making them non-void?)
        pScreenInfo->PostUpdateWindowSize();
        WriteToScreen(pScreenInfo, pScreenInfo->GetBufferViewport());
    }

    RETURN_NTSTATUS(Status);
}

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

NTSTATUS SetScreenColors(_In_ SCREEN_INFORMATION* ScreenInfo,
                         _In_ WORD Attributes,
                         _In_ WORD PopupAttributes,
                         _In_ BOOL UpdateWholeScreen)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    WORD const DefaultAttributes = ScreenInfo->GetAttributes().GetLegacyAttributes();
    WORD const DefaultPopupAttributes = ScreenInfo->GetPopupAttributes()->GetLegacyAttributes();
    TextAttribute NewPrimaryAttributes = TextAttribute(Attributes);
    TextAttribute NewPopupAttributes = TextAttribute(PopupAttributes);
    ScreenInfo->SetAttributes(NewPrimaryAttributes);
    ScreenInfo->SetPopupAttributes(&NewPopupAttributes);
    gci.ConsoleIme.RefreshAreaAttributes();

    if (UpdateWholeScreen)
    {
        // TODO: MSFT 9354902: Fix this up to be clearer with less magic bit shifting and less magic numbers. http://osgvsowi/9354902
        WORD const InvertedOldPopupAttributes = (WORD)(((DefaultPopupAttributes << 4) & 0xf0) | ((DefaultPopupAttributes >> 4) & 0x0f));
        WORD const InvertedNewPopupAttributes = (WORD)(((PopupAttributes << 4) & 0xf0) | ((PopupAttributes >> 4) & 0x0f));

        // change all chars with default color
        const SHORT sScreenBufferSizeY = ScreenInfo->GetScreenBufferSize().Y;
        for (SHORT i = 0; i < sScreenBufferSizeY; i++)
        {
            try
            {
                ROW& Row = ScreenInfo->TextInfo->GetRowAtIndex(i);
                Row.AttrRow.ReplaceLegacyAttrs(DefaultAttributes, Attributes);
                Row.AttrRow.ReplaceLegacyAttrs(DefaultPopupAttributes, PopupAttributes);
                Row.AttrRow.ReplaceLegacyAttrs(InvertedOldPopupAttributes, InvertedNewPopupAttributes);
            }
            catch (...)
            {
                return NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
            }
        }

        if (gci.PopupCount)
        {
            UpdatePopups(Attributes, PopupAttributes, DefaultAttributes, DefaultPopupAttributes);
        }

        // force repaint of entire line
        WriteToScreen(ScreenInfo, ScreenInfo->GetBufferViewport());
    }

    return STATUS_SUCCESS;
}

HRESULT ApiRoutines::SetConsoleTextAttributeImpl(_In_ SCREEN_INFORMATION* const pContext,
                                                 _In_ WORD const Attribute)
{
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    return DoSrvSetConsoleTextAttribute(pContext, Attribute);
}

HRESULT DoSrvSetConsoleTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute)
{
    RETURN_HR_IF(E_INVALIDARG, IsAnyFlagSet(Attribute, ~VALID_TEXT_ATTRIBUTES));

    RETURN_NTSTATUS(SetScreenColors(pScreenInfo, Attribute, pScreenInfo->GetPopupAttributes()->GetLegacyAttributes(), FALSE));
}

HRESULT DoSrvPrivateSetLegacyAttributes(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ WORD const Attribute, _In_ const bool fForeground, _In_ const bool fBackground, _In_ const bool fMeta)
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
        NewAttributes.SetForeground(OldAttributes.GetRgbForeground());
        NewAttributes.SetBackground(OldAttributes.GetRgbBackground());
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

    return STATUS_SUCCESS;
}

NTSTATUS DoSrvPrivateSetConsoleXtermTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ int const iXtermTableEntry, _In_ bool fIsForeground)
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

    return STATUS_SUCCESS;
}

NTSTATUS DoSrvPrivateSetConsoleRGBTextAttribute(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ COLORREF const rgbColor, _In_ bool fIsForeground)
{
    TextAttribute NewAttributes;
    NewAttributes.SetFrom(pScreenInfo->GetAttributes());
    NewAttributes.SetColor(rgbColor, fIsForeground);
    pScreenInfo->SetAttributes(NewAttributes);

    return STATUS_SUCCESS;
}

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

HRESULT ApiRoutines::GetConsoleInputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pCodePage = gci.CP;

    return S_OK;
}

HRESULT ApiRoutines::GetConsoleOutputCodePageImpl(_Out_ ULONG* const pCodePage)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    *pCodePage = gci.OutputCP;

    return S_OK;
}

HRESULT ApiRoutines::GetConsoleWindowImpl(_Out_ HWND* const pHwnd)
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

    return S_OK;
}

HRESULT ApiRoutines::GetConsoleHistoryInfoImpl(_Out_ CONSOLE_HISTORY_INFO* const pConsoleHistoryInfo)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    LockConsole();
    auto Unlock = wil::ScopeExit([&] { UnlockConsole(); });

    pConsoleHistoryInfo->HistoryBufferSize = gci.GetHistoryBufferSize();
    pConsoleHistoryInfo->NumberOfHistoryBuffers = gci.GetNumberOfHistoryBuffers();
    SetFlagIf(pConsoleHistoryInfo->dwFlags, HISTORY_NO_DUP_FLAG, IsFlagSet(gci.Flags, CONSOLE_HISTORY_NODUP));

    return S_OK;
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
HRESULT ApiRoutines::GetConsoleDisplayModeImpl(_Out_ ULONG* const pFlags)
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

    return S_OK;
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
NTSTATUS DoSrvPrivateAllowCursorBlinking(_In_ SCREEN_INFORMATION* pScreenInfo, _In_ bool fEnable)
{
    pScreenInfo->TextInfo->GetCursor()->SetBlinkingAllowed(fEnable);
    pScreenInfo->TextInfo->GetCursor()->SetIsOn(!fEnable);

    return STATUS_SUCCESS;
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
NTSTATUS DoSrvPrivateUseMainScreenBuffer(_In_ SCREEN_INFORMATION* const psiCurr)
{
    return psiCurr->UseMainScreenBuffer();
}

// Routine Description:
// - A private API call for setting a VT tab stop in the cursor's current column.
// Parameters:
// <none>
// Return value:
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
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
// - STATUS_SUCCESS if handled successfully. Otherwise, an approriate status code indicating the error.
NTSTATUS DoSrvPrivateTabClear(_In_ bool const fClearAll)
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
    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling VT200 style mouse mode.
// Parameters:
// - fEnable - true to enable default tracking mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableVT200MouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableDefaultTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling utf8 style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableUTF8ExtendedMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.SetUtf8ExtendedMode(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling SGR style mouse mode.
// Parameters:
// - fEnable - true to enable, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableSGRExtendedMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.SetSGRExtendedMode(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling button-event mouse mode.
// Parameters:
// - fEnable - true to enable button-event mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableButtonEventMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableButtonEventTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling any-event mouse mode.
// Parameters:
// - fEnable - true to enable any-event mode, false to disable mouse mode.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableAnyEventMouseMode(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableAnyEventTracking(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for enabling alternate scroll mode
// Parameters:
// - fEnable - true to enable alternate scroll mode, false to disable.
// Return value:
// - STATUS_SUCCESS always.
NTSTATUS DoSrvPrivateEnableAlternateScroll(_In_ bool const fEnable)
{
    CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.terminalMouseInput.EnableAlternateScroll(fEnable);

    return STATUS_SUCCESS;
}

// Routine Description:
// - A private API call for performing a VT-style erase all operation on the buffer.
//      See SCREEN_INFORMATION::VtEraseAll's description for details.
// Parameters:
//  The ScreenBuffer to perform the erase on.
// Return value:
// - STATUS_SUCCESS if we succeeded, otherwise the NTSTATUS version of the failure.
NTSTATUS DoSrvPrivateEraseAll(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    return NTSTATUS_FROM_HRESULT(pScreenInfo->GetActiveBuffer()->VtEraseAll());
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
// - STATUS_SUCCESS if we succeeded, otherwise the NTSTATUS version of the failure.
NTSTATUS DoSrvPrivateRefreshWindow(_In_ SCREEN_INFORMATION* const pScreenInfo)
{
    Globals& g = ServiceLocator::LocateGlobals();
    if (pScreenInfo == g.getConsoleInformation().CurrentScreenBuffer)
    {
        g.pRender->TriggerRedrawAll();
    }

    return STATUS_SUCCESS;
}
