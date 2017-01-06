/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "window.hpp"
#include "windowdpiapi.hpp"
#include "userprivapi.hpp"

#include "_output.h"
#include "output.h"
#include "clipboard.hpp"
#include "cursor.h"
#include "dbcs.h"
#include "find.h"
#include "handle.h"
#include "input.h"
#include "menu.h"
#include "misc.h"
#include "registry.hpp"
#include "scrolling.hpp"

#include "srvinit.h"

// Custom window messages
#define CM_SET_WINDOW_SIZE       (WM_USER + 2)
#define CM_BEEP                  (WM_USER + 3)
#define CM_UPDATE_SCROLL_BARS    (WM_USER + 4)
#define CM_UPDATE_TITLE          (WM_USER + 5)
// CM_MODE_TRANSITION is hard-coded to WM_USER + 6 in kernel\winmgr.c
// unused (CM_MODE_TRANSITION)   (WM_USER + 6)
// unused (CM_CONSOLE_SHUTDOWN)  (WM_USER + 7)
// unused (CM_HIDE_WINDOW)       (WM_USER + 8)
#define CM_CONIME_CREATE         (WM_USER+9)
#define CM_SET_CONSOLEIME_WINDOW (WM_USER+10)
#define CM_WAIT_CONIME_PROCESS   (WM_USER+11)
// unused CM_SET_IME_CODEPAGE      (WM_USER+12)
// unused CM_SET_NLSMODE           (WM_USER+13)
// unused CM_GET_NLSMODE           (WM_USER+14)
#define CM_CONIME_KL_ACTIVATE    (WM_USER+15)
#define CM_CONSOLE_MSG           (WM_USER+16)
#define CM_UPDATE_EDITKEYS       (WM_USER+17)

// The static and specific window procedures for this class are contained here
#pragma region Window Procedure

LRESULT CALLBACK Window::s_ConsoleWindowProc(_In_ HWND hWnd, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    // Save the pointer here to the specific window instance when one is created
    if (Message == WM_CREATE)
    {
        const CREATESTRUCT* const pCreateStruct = reinterpret_cast<CREATESTRUCT*>(lParam);

        Window* const pWindow = reinterpret_cast<Window*>(pCreateStruct->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    }

    // Dispatch the message to the specific class instance
    Window* const pWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (pWindow != nullptr)
    {
        return pWindow->ConsoleWindowProc(hWnd, Message, wParam, lParam);
    }

    // If we get this far, call the default window proc
    return DefWindowProcW(hWnd, Message, wParam, lParam);
}

LRESULT CALLBACK Window::ConsoleWindowProc(_In_ HWND hWnd, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    LRESULT Status = 0;
    BOOL Unlock = TRUE;

    LockConsole();

    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();
    if (hWnd == nullptr || ScreenInfo == nullptr) // TODO: this might not be possible anymore
    {
        if (Message == WM_CLOSE)
        {
            _CloseWindow();
            Status = 0;
        }
        else
        {
            Status = DefWindowProcW(hWnd, Message, wParam, lParam);
        }

        UnlockConsole();
        return Status;
    }

    switch (Message)
    {
    case WM_CREATE:
    {
        // Load all metrics we'll need.
        _UpdateSystemMetrics();

        // The system is not great and the window rect is wrong the first time for High DPI (WM_DPICHANGED scales strangely.)
        // So here we have to grab the DPI of the current window (now that we have a window).
        // Then we have to re-propose a window size for our window that is scaled to DPI and SetWindowPos.

        // First get the new DPI and update all the scaling factors in the console that are affected.

        // NOTE: GetWindowDpi and/or GetDpiForWindow can be *WRONG* at this point in time depending on monitor configuration.
        //       They won't be correct until the window is actually shown. So instead of using those APIs, figure out the DPI
        //       based on the rectangle that is about to be shown using the nearest monitor.

        // Get proposed window rect from create structure
        CREATESTRUCTW* pcs = (CREATESTRUCTW*)lParam;
        RECT rc;
        rc.left = pcs->x;
        rc.top = pcs->y;
        rc.right = rc.left + pcs->cx;
        rc.bottom = rc.top + pcs->cy;

        // Find nearest montitor.
        HMONITOR hmon = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);

        // This API guarantees that dpix and dpiy will be equal, but neither is an optional parameter so give two UINTs.
        UINT dpix = USER_DEFAULT_SCREEN_DPI;
        UINT dpiy = USER_DEFAULT_SCREEN_DPI;
        GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpix, &dpiy); // If this fails, we'll use the default of 96.

        // Pick one and set it to the global DPI.
        g_dpi = (int)dpix;

        _UpdateSystemMetrics(); // scroll bars and cursors and such.
        s_ReinitializeFontsForDPIChange(); // font sizes.

        // Now re-propose the window size with the same origin.
        RECT rectProposed = { rc.left, rc.top, 0, 0 };
        _CalculateWindowRect(_pSettings->GetWindowSize(), &rectProposed);

        SetWindowPos(hWnd, NULL, rectProposed.left, rectProposed.top, RECT_WIDTH(&rectProposed), RECT_HEIGHT(&rectProposed), SWP_NOACTIVATE | SWP_NOZORDER);

        // Save the proposed window rect dimensions here so we can adjust if the system comes back and changes them on what we asked for.
        s_ConvertWindowRectToClientRect(&rectProposed);

        break;
    }

    case WM_DROPFILES:
    {
        _HandleDrop(wParam);
        break;
    }

    case WM_SIZING:
    {
        // Signal that the user changed the window size, so we can return the value later for telemetry. By only
        // sending the data back if the size has changed, helps reduce the amount of telemetry being sent back.
        // WM_SIZING doesn't fire if they resize the window using Win-UpArrow, so we'll miss that scenario. We could
        // listen to the WM_SIZE message instead, but they can fire when the window is being restored from being
        // minimized, and not only when they resize the window.
        Telemetry::Instance().SetWindowSizeChanged();
        goto CallDefWin;
        break;
    }

    case WM_GETDPISCALEDSIZE:
    {
        // This message will send us the DPI we're about to be changed to.
        // Our goal is to use it to try to figure out the Window Rect that we'll need at that DPI to maintain
        // the same client rendering that we have now.

        // First retrieve the new DPI and the current DPI.
        DWORD const dpiProposed = (WORD)wParam;
        DWORD const dpiCurrent = g_dpi;

        // Now we need to get what the font size *would be* if we had this new DPI. We need to ask the renderer about that.
        FontInfo* pfiCurrent = ScreenInfo->TextInfo->GetCurrentFont();
        FontInfoDesired fiDesired(*pfiCurrent);
        FontInfo fiProposed(nullptr, 0, 0, { 0, 0 }, 0);

        g_pRender->GetProposedFont(dpiProposed, &fiDesired, &fiProposed); // fiProposal will be updated by the renderer for this new font.
        COORD coordFontProposed = fiProposed.GetSize();

        // Then from that font size, we need to calculate the client area.
        // Then from the client area we need to calculate the window area (using the proposed DPI scalar here as well.)

        // Retrieve the additional parameters we need for the math call based on the current window & buffer properties.

        SMALL_RECT srViewport = ScreenInfo->GetBufferViewport();
        COORD coordWindowInChars;
        coordWindowInChars.X = srViewport.Right - srViewport.Left + 1;
        coordWindowInChars.Y = srViewport.Bottom - srViewport.Top + 1;

        COORD coordBufferSize = ScreenInfo->TextInfo->GetCoordBufferSize();

        // Now call the math calculation for our proposed size.
        RECT rectProposed = { 0 };
        s_CalculateWindowRect(coordWindowInChars, dpiProposed, coordFontProposed, coordBufferSize, hWnd, &rectProposed);

        // Prepare where we're going to keep our final suggestion.
        SIZE* const pSuggestionSize = (SIZE*)lParam;

        pSuggestionSize->cx = RECT_WIDTH(&rectProposed);
        pSuggestionSize->cy = RECT_HEIGHT(&rectProposed);

        // Store the fact that we suggested this size to see if it's what WM_DPICHANGED gives us back later.
        _iSuggestedDpi = dpiProposed;
        _sizeSuggested = *pSuggestionSize;

        // Format our final suggestion for consumption.
        UnlockConsole();
        return TRUE;
    }

    case WM_DPICHANGED:
    {
        g_dpi = HIWORD(wParam);
        _UpdateSystemMetrics();
        s_ReinitializeFontsForDPIChange();

        // this is the RECT that the system suggests.
        RECT* const prcNewScale = (RECT*)lParam;
        SIZE szNewScale;
        szNewScale.cx = RECT_WIDTH(prcNewScale);
        szNewScale.cy = RECT_HEIGHT(prcNewScale);

        if (IsInFullscreen())
        {
            // If we're a full screen window, completely ignore what the DPICHANGED says as it will be bigger than the monitor and
            // instead use the value that we were given back in the WM_WINDOWPOSCHANGED (that we had ignored at the time)
            *prcNewScale = _rcWhenDpiChanges;
        }

        SetWindowPos(hWnd, HWND_TOP, prcNewScale->left, prcNewScale->top, RECT_WIDTH(prcNewScale), RECT_HEIGHT(prcNewScale), SWP_NOZORDER | SWP_NOACTIVATE);

        // Reset the WM_GETDPISCALEDSIZE proposals if they exist. They only last for one DPI change attempt.
        _iSuggestedDpi = 0;
        _sizeSuggested = { 0 };

        break;
    }

    case WM_ACTIVATE:
    {
        // if we're activated by a mouse click, remember it so
        // we don't pass the click on to the app.
        if (LOWORD(wParam) == WA_CLICKACTIVE)
        {
            g_ciConsoleInformation.Flags |= CONSOLE_IGNORE_NEXT_MOUSE_INPUT;
        }
        goto CallDefWin;
        break;
    }

    case WM_SETFOCUS:
    {
        g_ciConsoleInformation.ProcessHandleList.ModifyConsoleProcessFocus(TRUE);

        g_ciConsoleInformation.Flags |= CONSOLE_HAS_FOCUS;

        Cursor::s_FocusStart(hWnd);

        HandleFocusEvent(TRUE);

        // ActivateTextServices does nothing if already active so this is OK to be called every focus.
        ActivateTextServices(g_ciConsoleInformation.hWnd, GetImeSuggestionWindowPos);
        ConsoleImeMessagePump(CONIME_SETFOCUS, 0, (LPARAM)0);

        break;
    }

    case WM_KILLFOCUS:
    {
        g_ciConsoleInformation.ProcessHandleList.ModifyConsoleProcessFocus(FALSE);

        g_ciConsoleInformation.Flags &= ~CONSOLE_HAS_FOCUS;

        // turn it off when we lose focus.
        g_ciConsoleInformation.CurrentScreenBuffer->TextInfo->GetCursor()->SetIsOn(FALSE);

        Cursor::s_FocusEnd(hWnd);

        HandleFocusEvent(FALSE);

        // This function simply returns false if there isn't an active IME object, so it's safe to call every time.
        Status = ConsoleImeMessagePump(CONIME_KILLFOCUS, 0, (LPARAM)0);

        break;
    }

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_NOTIFY:
    {
        // Try to notify the IME. If it succeeds, we processed the message.
        // If it fails, pass it to the def window proc.
        if (NotifyTextServices(Message, wParam, lParam, &Status))
        {
            break;
        }
        goto CallDefWin;
    }

    case WM_IME_SETCONTEXT:
    {
        lParam &= ~ISC_SHOWUIALL;
        goto CallDefWin;
    }

    case WM_PAINT:
    {
        // Since we handle our own minimized window state, we need to
        // check if we're minimized (iconic) and set our internal state flags accordingly.
        // http://msdn.microsoft.com/en-us/library/windows/desktop/dd162483(v=vs.85).aspx
        // NOTE: We will not get called to paint ourselves when minimized because we set an icon when registering the window class.
        //       That means this CONSOLE_IS_ICONIC is unnnecessary when/if we can decouple the drawing with D2D.
        if (IsIconic(hWnd))
        {
            SetFlag(g_ciConsoleInformation.Flags, CONSOLE_IS_ICONIC);
        }
        else
        {
            ClearFlag(g_ciConsoleInformation.Flags, CONSOLE_IS_ICONIC);
        }

        _HandlePaint();

        // NOTE: We cannot let the OS handle this message (meaning do NOT pass to DefWindowProc)
        // or it will cause missing painted regions in scenarios without a DWM (like Core Server SKU).
        // Ensure it is re-validated in this handler so we don't receive infinite WM_PAINTs after
        // we have stored the invalid region data for the next trip around the renderer thread.

        break;
    }

    case WM_ERASEBKGND:
    {
        break;
    }

    case WM_CLOSE:
    {
        // Write the final trace log during the WM_CLOSE message while the console process is still fully alive.
        // This gives us time to query the process for information.  We shouldn't really miss any useful
        // telemetry between now and when the process terminates.
        Telemetry::Instance().WriteFinalTraceLog();

        _CloseWindow();
        break;
    }

    case WM_SETTINGCHANGE:
    {
        Cursor::s_SettingsChanged(hWnd);
    }
        __fallthrough;

    case WM_DISPLAYCHANGE:
    {
        _UpdateSystemMetrics();
        break;
    }

    case WM_WINDOWPOSCHANGING:
    {
        // Enforce maximum size here instead of WM_GETMINMAXINFO.
        // If we return it in WM_GETMINMAXINFO, then it will be enforced when snapping across DPI boundaries (bad.)

        // Retrieve the suggested dimensions and make a rect and size.
        LPWINDOWPOS lpwpos = (LPWINDOWPOS)lParam;

        // We only need to apply restrictions if the size is changing.
        if (!IsFlagSet(lpwpos->flags, SWP_NOSIZE))
        {
            // Figure out the suggested dimensions
            RECT rcSuggested;
            rcSuggested.left = lpwpos->x;
            rcSuggested.top = lpwpos->y;
            rcSuggested.right = rcSuggested.left + lpwpos->cx;
            rcSuggested.bottom = rcSuggested.top + lpwpos->cy;
            SIZE szSuggested;
            szSuggested.cx = RECT_WIDTH(&rcSuggested);
            szSuggested.cy = RECT_HEIGHT(&rcSuggested);

            // Figure out the current dimensions for comparison.
            RECT rcCurrent = GetWindowRect();

            // Determine whether we're being resized by someone dragging the edge or completely moved around.
            bool fIsEdgeResize = false;
            {
                // We can only be edge resizing if our existing rectangle wasn't empty. If it was empty, we're doing the initial create.
                if (!IsRectEmpty(&rcCurrent))
                {
                    // If one or two sides are changing, we're being edge resized.
                    unsigned int cSidesChanging = 0;
                    if (rcCurrent.left != rcSuggested.left)
                    {
                        cSidesChanging++;
                    }
                    if (rcCurrent.right != rcSuggested.right)
                    {
                        cSidesChanging++;
                    }
                    if (rcCurrent.top != rcSuggested.top)
                    {
                        cSidesChanging++;
                    }
                    if (rcCurrent.bottom != rcSuggested.bottom)
                    {
                        cSidesChanging++;
                    }

                    if (cSidesChanging == 1 || cSidesChanging == 2)
                    {
                        fIsEdgeResize = true;
                    }
                }
            }

            // If the window is maximized, let it do whatever it wants to do.
            // If not, then restrict it to our maximum possible window.
            if (!IsFlagSet(GetWindowStyle(hWnd), WS_MAXIMIZE))
            {
                // Find the related monitor and get the maximum pixel size for the window
                RECT rcMaximum;

                if (fIsEdgeResize)
                {
                    // If someone's dragging from the edge to resize in one direction, we want to make sure we never grow past the current monitor.
                    rcMaximum = Window::s_GetMaxWindowRectInPixels(&rcCurrent);
                }
                else
                {
                    // In other circumstances, assume we're snapping around or some other jump (TS).
                    // Just do whatever we're told using the new suggestion as the restriction monitor.
                    rcMaximum = Window::s_GetMaxWindowRectInPixels(&rcSuggested);
                }

                // Make a size for comparison purposes.
                SIZE szMaximum;
                szMaximum.cx = RECT_WIDTH(&rcMaximum);
                szMaximum.cy = RECT_HEIGHT(&rcMaximum);

                // If the suggested rect is bigger in size than the maximum size, then prevent a move and restrict size to the appropriate monitor dimensions.
                if (szSuggested.cx > szMaximum.cx || szSuggested.cy > szMaximum.cy)
                {
                    lpwpos->cx = min(szMaximum.cx, szSuggested.cx);
                    lpwpos->cy = min(szMaximum.cy, szSuggested.cy);
                    lpwpos->flags |= SWP_NOMOVE;
                }
            }

            break;
        }
        else
        {
            goto CallDefWin;
        }
    }

    case WM_WINDOWPOSCHANGED:
    {
        LPWINDOWPOS lpwpos = (LPWINDOWPOS)lParam;

        int const dpi = WindowDpiApi::s_GetWindowDPI(hWnd);

        // Only handle this if the DPI is the same as last time.
        // If the DPI is different, assume we're about to get a DPICHANGED notification
        // which will have a better suggested rectangle than this one.
        if (dpi == g_dpi)
        {
            _HandleWindowPosChanged(lParam);
        }
        else
        {
            // Store what the rectangle is when the DPI is about to change
            // because if the exact same one comes back in DPICHANGED, then we want to use it
            // not adjust it like we normally would. (Aero snap case).
            _rcWhenDpiChanges.left = lpwpos->x;
            _rcWhenDpiChanges.top = lpwpos->y;
            _rcWhenDpiChanges.right = _rcWhenDpiChanges.left + lpwpos->cx;
            _rcWhenDpiChanges.bottom = _rcWhenDpiChanges.top + lpwpos->cy;
        }

        break;
    }

    case WM_CONTEXTMENU:
    {
        Telemetry::Instance().SetContextMenuUsed();
        if (DefWindowProcW(hWnd, WM_NCHITTEST, 0, lParam) == HTCLIENT)
        {
            HMENU hHeirMenu = g_ciConsoleInformation.hHeirMenu;

            Unlock = FALSE;
            UnlockConsole();

            TrackPopupMenuEx(hHeirMenu,
                TPM_RIGHTBUTTON | (GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0 ? TPM_LEFTALIGN : TPM_RIGHTALIGN),
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), hWnd, nullptr);
        }
        else
        {
            goto CallDefWin;
        }
        break;
    }

    case WM_NCLBUTTONDOWN:
    {
        // allow user to move window even when bigger than the screen
        switch (wParam & 0x00FF)
        {
        case HTCAPTION:
            UnlockConsole();
            Unlock = FALSE;
            SetActiveWindow(hWnd);
            SendMessageTimeoutW(hWnd, WM_SYSCOMMAND, SC_MOVE | wParam, lParam, SMTO_NORMAL, INFINITE, nullptr);
            break;
        default:
            goto CallDefWin;
        }
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_DEADCHAR:
    {
        HandleKeyEvent(hWnd, Message, wParam, lParam, &Unlock);
        break;
    }

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    {
        if (HandleSysKeyEvent(hWnd, Message, wParam, lParam, &Unlock))
        {
            goto CallDefWin;
        }
        break;
    }

    case WM_COMMAND:
        // If this is an edit command from the context menu, treat it like a sys command.
        if ((wParam < ID_CONSOLE_COPY) || (wParam > ID_CONSOLE_SELECTALL))
        {
            break;
        }

        __fallthrough;

    case WM_SYSCOMMAND:
        if (wParam == ID_CONSOLE_MARK)
        {
            Clipboard::s_DoMark();
        }
        else if (wParam == ID_CONSOLE_COPY)
        {
            Clipboard::s_DoCopy();
        }
        else if (wParam == ID_CONSOLE_PASTE)
        {
            Clipboard::s_DoPaste();
        }
        else if (wParam == ID_CONSOLE_SCROLL)
        {
            Scrolling::s_DoScroll();
        }
        else if (wParam == ID_CONSOLE_FIND)
        {
            DoFind();
            Unlock = FALSE;
        }
        else if (wParam == ID_CONSOLE_SELECTALL)
        {
            Selection::Instance().SelectAll();
        }
        else if (wParam == ID_CONSOLE_CONTROL)
        {
            PropertiesDlgShow(hWnd, FALSE);
        }
        else if (wParam == ID_CONSOLE_DEFAULTS)
        {
            PropertiesDlgShow(hWnd, TRUE);
        }
        else
        {
            goto CallDefWin;
        }
        break;

    case WM_TIMER:
    {
        Status = _HandleTimer(wParam);
        break;
    }

    case WM_HSCROLL:
    {
        HorizontalScroll(LOWORD(wParam), HIWORD(wParam));
        break;
    }

    case WM_VSCROLL:
    {
        VerticalScroll(LOWORD(wParam), HIWORD(wParam));
        break;
    }

    case WM_INITMENU:
    {
        HandleMenuEvent(WM_INITMENU);
        InitializeMenu();
        break;
    }

    case WM_MENUSELECT:
    {
        if (HIWORD(wParam) == 0xffff)
        {
            HandleMenuEvent(WM_MENUSELECT);
        }
        break;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    {
        if (HandleMouseEvent(ScreenInfo, Message, wParam, lParam))
        {
            if (Message != WM_MOUSEWHEEL && Message != WM_MOUSEHWHEEL)
            {
                goto CallDefWin;
            }
        }
        else
        {
            break;
        }

        // Don't handle zoom.
        if (wParam & MK_CONTROL)
        {
            goto CallDefWin;
        }

        Status = 1;
        Scrolling::s_HandleMouseWheel(Message, wParam, ScreenInfo);
        break;
    }

    case CM_SET_WINDOW_SIZE:
    {
        Status = _InternalSetWindowSize();
        break;
    }

    case CM_BEEP:
    {
        UnlockConsole();
        Unlock = FALSE;
        if (!PlaySoundW((LPCWSTR)SND_ALIAS_SYSTEMHAND, nullptr, SND_ALIAS_ID | SND_ASYNC | SND_SENTRY))
        {
            Beep(800, 200);
        }
        break;
    }

    case CM_UPDATE_SCROLL_BARS:
    {
        ScreenInfo->InternalUpdateScrollBars();
        break;
    }

    case CM_UPDATE_TITLE:
    {

        SetWindowTextW(hWnd, (PCWSTR)(lParam));
        delete[] (PCWSTR)(lParam);
        break;
    }

    case CM_UPDATE_EDITKEYS:
    {
        Settings& settings = g_ciConsoleInformation;

        // Re-read the edit key settings from registry.
        Registry reg(&settings);
        reg.GetEditKeys(NULL);
        break;
    }

    case EVENT_CONSOLE_CARET:
    case EVENT_CONSOLE_UPDATE_REGION:
    case EVENT_CONSOLE_UPDATE_SIMPLE:
    case EVENT_CONSOLE_UPDATE_SCROLL:
    case EVENT_CONSOLE_LAYOUT:
    case EVENT_CONSOLE_START_APPLICATION:
    case EVENT_CONSOLE_END_APPLICATION:
    {
        NotifyWinEvent(Message, hWnd, (LONG)wParam, (LONG)lParam);
        break;
    }

    case WM_COPYDATA:
    {
        Status = ImeControl((PCOPYDATASTRUCT)lParam);
        break;
    }

    case WM_ENTERMENULOOP:
    {
        if (g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS)
        {
            g_ciConsoleInformation.pInputBuffer->ImeMode.Unavailable = TRUE;
            Status = ConsoleImeMessagePump(CONIME_KILLFOCUS, 0, 0);
        }
        break;
    }

    case WM_EXITMENULOOP:
    {
        if (g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS)
        {
            Status = ConsoleImeMessagePump(CONIME_SETFOCUS, 0, 0);
            g_ciConsoleInformation.pInputBuffer->ImeMode.Unavailable = FALSE;
        }
        break;
    }

    case WM_ENTERSIZEMOVE:
    {
        if (g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS)
        {
            g_ciConsoleInformation.pInputBuffer->ImeMode.Unavailable = TRUE;
        }
        break;
    }

    case WM_EXITSIZEMOVE:
    {
        if (g_ciConsoleInformation.Flags & CONSOLE_HAS_FOCUS)
        {
            g_ciConsoleInformation.pInputBuffer->ImeMode.Unavailable = FALSE;
        }
        break;
    }

    default:
    CallDefWin :
    {
        if (Unlock)
        {
            UnlockConsole();
            Unlock = FALSE;
        }
        Status = DefWindowProcW(hWnd, Message, wParam, lParam);
        break;
    }
    }

    if (Unlock)
    {
        UnlockConsole();
    }

    return Status;
}

#pragma endregion

// Helper handler methods for specific cases within the large window procedure are in this section
#pragma region Message Handlers

void Window::_HandleWindowPosChanged(_In_ const LPARAM lParam)
{
    HWND hWnd = GetWindowHandle();
    SCREEN_INFORMATION* const pScreenInfo = GetScreenInfo();

    LPWINDOWPOS const lpWindowPos = (LPWINDOWPOS)lParam;
    this->_fHasMoved = true;

    // If the frame changed, update the system metrics.
    if (IsFlagSet(lpWindowPos->flags, SWP_FRAMECHANGED))
    {
        _UpdateSystemMetrics();
    }

    // This message is sent as the result of someone calling SetWindowPos(). We use it here to set/clear the
    // CONSOLE_IS_ICONIC bit appropriately. doing so in the WM_SIZE handler is incorrect because the WM_SIZE
    // comes after the WM_ERASEBKGND during SetWindowPos() processing, and the WM_ERASEBKGND needs to know if
    // the console window is iconic or not.
    if (!pScreenInfo->ResizingWindow && (lpWindowPos->cx || lpWindowPos->cy) && !IsIconic(hWnd))
    {
        // calculate the dimensions for the newly proposed window rectangle
        RECT rcNew;
        s_ConvertWindowPosToWindowRect(lpWindowPos, &rcNew);
        s_ConvertWindowRectToClientRect(&rcNew);

        // If the window is not being resized, don't do anything except update our windowrect
        if (!IsFlagSet(lpWindowPos->flags, SWP_NOSIZE))
        {
            pScreenInfo->ProcessResizeWindow(&rcNew, &_rcClientLast);
        }

        // now that operations are complete, save the new rectangle size as the last seen value
        _rcClientLast = rcNew;
    }
}

void Window::_HandlePaint() const
{
    RECT rcUpdate;
    GetUpdateRect(GetWindowHandle(), &rcUpdate, FALSE);

    if (g_pRender != nullptr)
    {
        g_pRender->TriggerSystemRedraw(&rcUpdate);
        ValidateRect(GetWindowHandle(), &rcUpdate);
    }
}

// Routine Description:
// - This routine is called when ConsoleWindowProc receives a WM_DROPFILES message.
// - It initially calls DragQueryFile() to calculate the number of files dropped and then DragQueryFile() is called to retrieve the filename.
// - DoStringPaste() pastes the filename to the console window
// Arguments:
// - wParam  - Identifies the structure containing the filenames of the dropped files.
// - Console - Pointer to CONSOLE_INFORMATION structure
// Return Value:
// - <none>
void Window::_HandleDrop(_In_ const WPARAM wParam) const
{
    WCHAR szPath[MAX_PATH];
    BOOL fAddQuotes;

    if (DragQueryFile((HDROP)wParam, 0, szPath, ARRAYSIZE(szPath)) != 0)
    {
        // Log a telemetry flag saying the user interacted with the Console
        // Only log when DragQueryFile succeeds, because if we don't when the console starts up, we're seeing
        // _HandleDrop get called multiple times (and DragQueryFile fail),
        // which can incorrectly mark this console session as interactive.
        Telemetry::Instance().SetUserInteractive();

        fAddQuotes = (wcschr(szPath, L' ') != nullptr);
        if (fAddQuotes)
        {
            Clipboard::s_DoStringPaste(L"\"", 1);
        }

        Clipboard::s_DoStringPaste(szPath, wcslen(szPath));

        if (fAddQuotes)
        {
            Clipboard::s_DoStringPaste(L"\"", 1);
        }
    }
}


LRESULT Window::_HandleTimer(_In_ const WPARAM /*wParam*/)
{
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    // MSFT:4875135 - Disabling auto-save of window positioning data due to late LNK problem.
    //if(!this->_fHasMoved)
    //{
    //    Window::s_PersistWindowPosition(
    //        g_ciConsoleInformation.LinkTitle,
    //        g_ciConsoleInformation.OriginalTitle,
    //        g_ciConsoleInformation.Flags,
    //        g_ciConsoleInformation.pWindow
    //    );
    //    Window::s_PersistWindowOpacity(
    //        g_ciConsoleInformation.LinkTitle,
    //        g_ciConsoleInformation.OriginalTitle,
    //        g_ciConsoleInformation.pWindow
    //    );
    //}

    this->_fHasMoved = false;

    ScreenInfo->TextInfo->GetCursor()->TimerRoutine(ScreenInfo);
    Scrolling::s_ScrollIfNecessary(ScreenInfo);

    return 0;
}

#pragma endregion

// Dispatchers are used to post or send a window message into the queue from other portions of the codebase without accessing internal properties directly
#pragma region Dispatchers

BOOL Window::PostUpdateWindowSize() const
{
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    if (ScreenInfo->ConvScreenInfo != nullptr)
    {
        return FALSE;
    }

    if (g_ciConsoleInformation.Flags & CONSOLE_SETTING_WINDOW_SIZE)
    {
        return FALSE;
    }

    g_ciConsoleInformation.Flags |= CONSOLE_SETTING_WINDOW_SIZE;
    return PostMessageW(GetWindowHandle(), CM_SET_WINDOW_SIZE, (WPARAM)ScreenInfo, 0);
}

BOOL Window::SendNotifyBeep() const
{
    return SendNotifyMessageW(GetWindowHandle(), CM_BEEP, 0, 0);
}

// NOTE: pszNewTitle must be allocated with new[]. The CM_UPDATE_TITLE
//  handler is responsible for the lifetime of pszNewTitle.
BOOL Window::PostUpdateTitle(_In_ const PCWSTR pwszNewTitle) const
{
    return PostMessageW(GetWindowHandle(), CM_UPDATE_TITLE, 0, (LPARAM)(pwszNewTitle));
}

// The CM_UPDATE_TITLE handler is responsible for the lifetime of the copy generated by this function.
BOOL Window::PostUpdateTitleWithCopy(_In_ const PCWSTR pwszNewTitle) const
{

    size_t cchTitleCharLength;
    if (SUCCEEDED(StringCchLengthW(pwszNewTitle, STRSAFE_MAX_CCH, &cchTitleCharLength))){
        cchTitleCharLength += 1; //"The length does not include the string's terminating null character."
        // - https://msdn.microsoft.com/en-us/library/windows/hardware/ff562856(v=vs.85).aspx
        // this is technically safe because size_t is a ULONG_PTR and STRSAFE_MAX_CCH is INT_MAX.

        PWSTR pwszNewTitleCopy = new WCHAR[cchTitleCharLength];
        if (pwszNewTitleCopy != nullptr)
        {
            if (SUCCEEDED(StringCchCopyW(pwszNewTitleCopy, cchTitleCharLength, pwszNewTitle)))
            {
                return PostMessageW(GetWindowHandle(), CM_UPDATE_TITLE, 0, (LPARAM)(pwszNewTitleCopy));
            }
            else
            {
                delete[] pwszNewTitleCopy;
            }

        }
    }

    return FALSE;
}

BOOL Window::PostUpdateScrollBars() const
{
    return PostMessageW(GetWindowHandle(), CM_UPDATE_SCROLL_BARS, (WPARAM)GetScreenInfo(), 0);
}

BOOL Window::PostUpdateExtendedEditKeys() const
{
    return PostMessageW(GetWindowHandle(), CM_UPDATE_EDITKEYS, 0, 0);
}

#pragma endregion
