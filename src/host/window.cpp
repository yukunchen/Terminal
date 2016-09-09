/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "window.hpp"
#include "windowdpiapi.hpp"
#include "userprivapi.hpp"

#include "cursor.h"
#include "dbcs.h"
#include "getset.h"
#include "icon.hpp"
#include "menu.h"
#include "misc.h"
#include "_output.h"
#include "output.h"
#include "scrolling.hpp"
#include "stream.h"
#include "telemetry.hpp"
#include "tracing.hpp"

#include "srvinit.h"

#include "..\renderer\base\renderer.hpp"
#include "..\renderer\gdi\gdirenderer.hpp"
#include "renderData.hpp"

// The following default masks are used in creating windows
#define CONSOLE_WINDOW_FLAGS (WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL)
#define CONSOLE_WINDOW_EX_FLAGS (WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW )

// Window class name
#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

ATOM Window::s_atomWindowClass = 0;

Window::Window() :
    _fIsInFullscreen(false),
    _fHasMoved(false)
{
    ZeroMemory((void*)&_rcClientLast, sizeof(_rcClientLast));
    ZeroMemory((void*)&_rcNonFullscreenWindowSize, sizeof(_rcNonFullscreenWindowSize));
    ZeroMemory((void*)&_rcFullscreenWindowSize, sizeof(_rcFullscreenWindowSize));
}

Window::~Window()
{
    if (g_pRender != nullptr)
    {
        delete g_pRender;
    }

    if (g_pRenderData != nullptr)
    {
        delete g_pRenderData;
    }

    if (g_pRenderEngine != nullptr)
    {
        delete g_pRenderEngine;
    }
}

// Prevent accidental copies from being made by hiding this from public use.
Window::Window(Window const&)
{
    ASSERT(false);
}

// Prevent accidental copies from being made by hiding this from public use.
void Window::operator=(Window const&)
{
    ASSERT(false);
}

// Routine Description:
// - This routine allocates and initializes a window for the console
// Arguments:
// - pSettings - All user-configurable settings related to the console host
// - pScreen - The initial screen rendering data to attach to (renders in the client area of this window)
// - ppNewWindow - Filled with pointer to the new window instance.
// Return Value:
// - STATUS_SUCCESS or suitable NT error code
NTSTATUS Window::CreateInstance(_In_ Settings* const pSettings,
                                _In_ SCREEN_INFORMATION* const pScreen,
                                _Out_ Window** const ppNewWindow)
{
    *ppNewWindow = nullptr;

    NTSTATUS status = s_RegisterWindowClass();

    if (NT_SUCCESS(status))
    {
        Window* pNewWindow = new Window();

        status = pNewWindow->_MakeWindow(pSettings, pScreen);

        if (NT_SUCCESS(status))
        {
            *ppNewWindow = pNewWindow;
        }
    }

    return status;
}

// Routine Description:
// - Registers the window class information with the system
// - Only should happen once for the entire lifetime of this class.
// Arguments:
// - <none>
// Return Value:
// - STATUS_SUCCESS or failure from loading icons/registering class with the system
NTSTATUS Window::s_RegisterWindowClass()
{
    NTSTATUS status = STATUS_SUCCESS;

    // Today we never call this more than once.
    // In the future, if we need multiple windows (for tabs, etc.) we will need to make this thread-safe.
    // As such, the window class should always be 0 when we are entering this the first and only time.
    ASSERT(s_atomWindowClass == 0);

    // Only register if we haven't already registered
    if (s_atomWindowClass == 0)
    {
        // Prepare window class structure
        WNDCLASSEX wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        wc.lpfnWndProc = s_ConsoleWindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = GWL_CONSOLE_WNDALLOC;
        wc.hInstance = nullptr;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // We don't want the background painted. It will cause flickering.
        wc.lpszMenuName = nullptr;
        wc.lpszClassName = CONSOLE_WINDOW_CLASS;

        // Load icons
        status = Icon::Instance().GetIcons(&wc.hIcon, &wc.hIconSm);

        if (NT_SUCCESS(status))
        {
            s_atomWindowClass = RegisterClassExW(&wc);

            if (s_atomWindowClass == 0)
            {
                status = NTSTATUS_FROM_WIN32(GetLastError());
            }
        }
    }

    return status;
}

// Routine Description:
// - Updates some global system metrics when triggered.
// - Calls subroutines to update metrics for other relevant items
// - Example metrics include window borders, scroll size, timer values, etc.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Window::_UpdateSystemMetrics() const
{
    Scrolling::s_UpdateSystemMetrics();

    g_sVerticalScrollSize = (SHORT)WindowDpiApi::s_GetSystemMetricsForDpi(SM_CXVSCROLL, g_dpi);
    g_sHorizontalScrollSize = (SHORT)WindowDpiApi::s_GetSystemMetricsForDpi(SM_CYHSCROLL, g_dpi);

    Cursor::s_UpdateSystemMetrics();
}

// Routine Description:
// - This will call the system to create a window for the console, set up settings, and prepare for rendering
// Arguments:
// - pSettings - Load user-configurable settings from this structure
// - pScreen - Attach to this screen for rendering the client area of the window
// Return Value:
// - STATUS_SUCCESS, invalid parameters, or various potential errors from calling CreateWindow
NTSTATUS Window::_MakeWindow(_In_ Settings* const pSettings, _In_ SCREEN_INFORMATION* const pScreen)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pSettings == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_1;
    }
    else if (pScreen == nullptr)
    {
        status = STATUS_INVALID_PARAMETER_2;
    }

    // Ensure we have appropriate system metrics before we start constructing the window.
    _UpdateSystemMetrics();

    g_pRenderData = new RenderData();
    status = NT_TESTNULL(g_pRenderData);
    if (NT_SUCCESS(status))
    {
        GdiEngine* pGdiEngine = new GdiEngine();
        status = NT_TESTNULL(pGdiEngine);

        if (NT_SUCCESS(status))
        {
            g_pRenderEngine = pGdiEngine;

            Renderer* pNewRenderer = nullptr;
            if (SUCCEEDED(Renderer::s_CreateInstance(g_pRenderData, g_pRenderEngine, &pNewRenderer)))
            {
                g_pRender = pNewRenderer;
            }

            status = NT_TESTNULL(g_pRender);
        }

        if (NT_SUCCESS(status))
        {
            SCREEN_INFORMATION* const psiAttached = GetScreenInfo();

            psiAttached->RefreshFontWithRenderer();

            // Save reference to settings
            _pSettings = pSettings;

            // Figure out coordinates and how big to make the window from the desired client viewport size
            // Put left, top, right and bottom into rectProposed for checking against monitor screens below
            RECT rectProposed = { pSettings->GetWindowOrigin().X, pSettings->GetWindowOrigin().Y, 0, 0 };
            _CalculateWindowRect(pSettings->GetWindowSize(), &rectProposed); //returns with rectangle filled out

            if (!IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_AUTO_POSITION))
            {
                //if launched from a shortcut, ensure window is visible on screen
                if (pSettings->IsStartupFlagSet(STARTF_TITLEISLINKNAME))
                {
                    //if window would be fully OFFscreen, change position so it is ON screen.
                    //This doesn't change the actual coordinates stored in the link, just the starting position of the window
                    //When the user reconnects the other monitor, the window will be where he left it. Great for take home laptop scenario
                    if (!MonitorFromRect(&rectProposed, MONITOR_DEFAULTTONULL))
                    {
                        //Monitor we'll move to
                        HMONITOR hMon = MonitorFromRect(&rectProposed, MONITOR_DEFAULTTONEAREST);
                        MONITORINFO mi = { 0 };

                        //get origin of monitor's workarea
                        mi.cbSize = sizeof(MONITORINFO);
                        GetMonitorInfo(hMon, &mi);

                        //Adjust right and bottom to new positions, relative to monitor workarea's origin
                        //Need to do this before adjusting left/top so RECT_* calculations are correct
                        rectProposed.right = mi.rcWork.left + RECT_WIDTH(&rectProposed);
                        rectProposed.bottom = mi.rcWork.top + RECT_HEIGHT(&rectProposed);

                        //Move origin to top left of nearest monitor's WORKAREA (accounting for taskbar and any app toolbars)
                        rectProposed.left = mi.rcWork.left;
                        rectProposed.top = mi.rcWork.top;
                    }
                }
            }

            // Attempt to create window
            HWND hWnd = CreateWindowExW(
                CONSOLE_WINDOW_EX_FLAGS,
                CONSOLE_WINDOW_CLASS,
                g_ciConsoleInformation.Title,
                CONSOLE_WINDOW_FLAGS,
                IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_AUTO_POSITION) ? CW_USEDEFAULT : rectProposed.left,
                rectProposed.top, // field is ignored if CW_USEDEFAULT was chosen above
                RECT_WIDTH(&rectProposed),
                RECT_HEIGHT(&rectProposed),
                HWND_DESKTOP,
                nullptr,
                nullptr,
                this // handle to this window class, passed to WM_CREATE to help dispatching to this instance
                );

            if (hWnd == nullptr)
            {
                DWORD const gle = GetLastError();
                RIPMSG1(RIP_WARNING, "CreateWindow failed with gle = 0x%x", gle);
                status = NTSTATUS_FROM_WIN32(gle);
            }

            if (NT_SUCCESS(status))
            {
                g_ciConsoleInformation.hWnd = hWnd; // temporarily save into console info

                pGdiEngine->SetHwnd(hWnd);

                // Set alpha on window if requested
                ApplyWindowOpacity();

                g_ciConsoleInformation.hMenu = GetSystemMenu(hWnd, FALSE);

                // Modify system menu to our liking.
                InitSystemMenu();

                g_ciConsoleInformation.ConsoleIme.RefreshAreaAttributes();

                // Do WM_GETICON workaround. Must call WM_SETICON once or apps calling WM_GETICON will get null.
                Icon::Instance().ApplyWindowMessageWorkaround();

                // Set up the hot key for this window.
                if (g_ciConsoleInformation.GetHotKey() != 0)
                {
                    SendMessageW(hWnd, WM_SETHOTKEY, g_ciConsoleInformation.GetHotKey(), 0);
                }

                WindowDpiApi::s_EnableChildWindowDpiMessage(g_ciConsoleInformation.hWnd, TRUE /*fEnable*/);

                // Post a window size update so that the new console window will size itself correctly once it's up and
                // running. This works around chicken & egg cases involving window size calculations having to do with font
                // sizes, DPI, and non-primary monitors (see MSFT #2367234).
                psiAttached->PostUpdateWindowSize();
            }
        }
    }

    return status;
}

// Routine Description:
// - Called when the window is about to close
// - Right now, it just triggers the process list management to notify that we're closing
// Arguments:
// - <none>
// Return Value:
// - <none>
void Window::_CloseWindow() const
{
    // Pass on the notification to attached processes.
    // Since we only have one window for now, this will be the end of the host process as well.
    CloseConsoleProcessState();
}

// Routine Description:
// - Activates and shows this window based on the flags given.
// Arguments:
// - wShowWindow - See STARTUPINFO wShowWindow member: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686331(v=vs.85).aspx
// Return Value:
// - STATUS_SUCCESS or system errors from activating the window and setting its show states
NTSTATUS Window::ActivateAndShow(_In_ WORD const wShowWindow)
{
    NTSTATUS status = STATUS_SUCCESS;
    HWND const hWnd = GetWindowHandle();

    // Only activate if the wShowWindow we were passed at process create doesn't explicitly tell us to remain inactive/hidden
    if (!IsFlagSet(wShowWindow, SW_SHOWNOACTIVATE) &&
        !IsFlagSet(wShowWindow, SW_SHOWMINNOACTIVE) &&
        !IsFlagSet(wShowWindow, SW_HIDE))
    {
        HWND const hWndPrevious = SetActiveWindow(hWnd);

        // If previous window was null, there's an error
        if (hWndPrevious == nullptr)
        {
            status = NTSTATUS_FROM_WIN32(GetLastError());
        }
    }
    else if (IsFlagSet(wShowWindow, SW_SHOWMINNOACTIVE))
    {
        // If we're minimized and not the active window, set iconic to stop rendering
        g_ciConsoleInformation.Flags |= CONSOLE_IS_ICONIC;
    }

    if (NT_SUCCESS(status))
    {
        ShowWindow(hWnd, wShowWindow);

        SCREEN_INFORMATION* const psiAttached = GetScreenInfo();
        psiAttached->InternalUpdateScrollBars();
    }

    return status;
}

// Routine Description:
// - This routine sets the window origin.
// Arguments:
// - Absolute - if TRUE, WindowOrigin is specified in absolute screen buffer coordinates.
//              if FALSE, WindowOrigin is specified in coordinates relative to the current window origin.
// - WindowOrigin - New window origin.
// Return Value:
NTSTATUS Window::SetViewportOrigin(_In_ const SMALL_RECT* const prcNewOrigin)
{
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    SMALL_RECT NewWindow = *prcNewOrigin;

    COORD const FontSize = ScreenInfo->GetScreenFontSize();

    if (!(g_ciConsoleInformation.Flags & (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW)))
    {
        Selection* pSelection = &Selection::Instance();
        pSelection->HideSelection();

        RECT ClipRect = { 0 };
        RECT ScrollRect;

        ScrollRect.left = 0;
        ScrollRect.right = ScreenInfo->GetScreenWindowSizeX() * FontSize.X;
        ScrollRect.top = 0;
        ScrollRect.bottom = ScreenInfo->GetScreenWindowSizeY() * FontSize.Y;

        // Fire off an event to let accessibility apps know we've scrolled.
        NotifyWinEvent(EVENT_CONSOLE_UPDATE_SCROLL, g_ciConsoleInformation.hWnd, ScreenInfo->BufferViewport.Left - NewWindow.Left, ScreenInfo->BufferViewport.Top - NewWindow.Top);

        // The new window is OK. Store it in screeninfo and refresh screen.
        ScreenInfo->BufferViewport = NewWindow;
        Tracing::s_TraceWindowViewport(&ScreenInfo->BufferViewport);

        if (g_pRender != nullptr)
        {
            g_pRender->TriggerScroll();
        }

        pSelection->ShowSelection();
    }
    else
    {
        // we're iconic
        ScreenInfo->BufferViewport = NewWindow;
        Tracing::s_TraceWindowViewport(&ScreenInfo->BufferViewport);
    }

    ConsoleImeResizeCompStrView();

    ScreenInfo->UpdateScrollBars();
    return STATUS_SUCCESS;
}

// Routine Description:
// - Sends an update to the window size based on the character size requested.
// Arguments:
// - Size of the window in characters (relative to the current font)
// Return Value:
// - <none>
void Window::UpdateWindowSize(_In_ COORD const coordSizeInChars) const
{
    GetScreenInfo()->SetViewportSize(&coordSizeInChars);

    PostUpdateWindowSize();
}

// Routine Description:
// Arguments:
// Return Value:
void Window::UpdateWindowPosition(_In_ POINT const ptNewPos) const
{
    SetWindowPos(GetWindowHandle(),
        nullptr,
        ptNewPos.x,
        ptNewPos.y,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER);
}

// Routine Description:
// - Adjusts the outer window frame size. Does not move the position.
// Arguments:
// - sizeNew - The X and Y dimensions
// Return Value:
// - <none>
void Window::_UpdateWindowSize(_In_ SIZE const sizeNew) const
{
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    if (!IsFlagSet(g_ciConsoleInformation.Flags, CONSOLE_IS_ICONIC))
    {
        ScreenInfo->InternalUpdateScrollBars();

        SetWindowPos(GetWindowHandle(),
            nullptr,
            0,
            0,
            sizeNew.cx,
            sizeNew.cy,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
    }
}

// Routine Description:
// - Triggered when the buffer dimensions/viewport is changed.
// - This function recalculates what size the window should be in order to host the given buffer and viewport
// - Then it will trigger an actual adjustment of the outer window frame
// Arguments:
// - <none> - All state is read from the attached screen buffer
// Return Value:
// - STATUS_SUCCESS or suitable error code
NTSTATUS Window::_InternalSetWindowSize() const
{
    SCREEN_INFORMATION* const psiAttached = GetScreenInfo();

    g_ciConsoleInformation.Flags &= ~CONSOLE_SETTING_WINDOW_SIZE;
    if (!IsInFullscreen())
    {
        // Figure out how big to make the window, given the desired client area size.
        psiAttached->ResizingWindow++;

        // First get the buffer viewport size
        WORD WindowSizeX, WindowSizeY;
        WindowSizeX = psiAttached->GetScreenWindowSizeX();
        WindowSizeY = psiAttached->GetScreenWindowSizeY();

        // We'll use the font to convert characters to pixels
        COORD ScreenFontSize = psiAttached->GetScreenFontSize();

        // Now do the multiplication of characters times pixels per char. This is the client area pixel size.
        SIZE WindowSize;
        WindowSize.cx = WindowSizeX * ScreenFontSize.X;
        WindowSize.cy = WindowSizeY * ScreenFontSize.Y;

        // Fill a rectangle to call the system to adjust the client rect into a window rect
        RECT rectSizeTemp = { 0 };
        rectSizeTemp.right = WindowSize.cx;
        rectSizeTemp.bottom = WindowSize.cy;
        ASSERT(rectSizeTemp.top == 0 && rectSizeTemp.left == 0);
        s_ConvertClientRectToWindowRect(&rectSizeTemp);

        // Measure the adjusted rectangle dimensions and fill up the size variable
        WindowSize.cx = RECT_WIDTH(&rectSizeTemp);
        WindowSize.cy = RECT_HEIGHT(&rectSizeTemp);

        if (WindowSizeY != 0)
        {
            // We want the alt to have scroll bars if the main has scroll bars.
            // The bars are disabled, but they're still there.
            // This keeps the window, viewport, and SB size from changing when swapping.
            if (!psiAttached->GetMainBuffer()->IsMaximizedX())
            {
                WindowSize.cy += g_sHorizontalScrollSize;
            }
            if (!psiAttached->GetMainBuffer()->IsMaximizedY())
            {
                WindowSize.cx += g_sVerticalScrollSize;
            }
        }

        // Only attempt to actually change the window size if the difference between the size we just calculated
        // and the size of the current window is substantial enough to make a rendering difference.
        // This is an issue now in the V2 console because we allow sub-character window sizes
        // such that there isn't leftover space around the window when snapping.

        // To figure out if it's substantial, calculate what the window size would be if it were one character larger than what we just proposed
        SIZE WindowSizeMax;
        WindowSizeMax.cx = WindowSize.cx + ScreenFontSize.X;
        WindowSizeMax.cy = WindowSize.cy + ScreenFontSize.Y;

        // And figure out the current window size as well.
        RECT const rcWindowCurrent = g_ciConsoleInformation.pWindow->GetWindowRect();
        SIZE WindowSizeCurrent;
        WindowSizeCurrent.cx = RECT_WIDTH(&rcWindowCurrent);
        WindowSizeCurrent.cy = RECT_HEIGHT(&rcWindowCurrent);

        // If the current window has a few extra sub-character pixels between the proposed size (WindowSize) and the next size up (WindowSizeMax), then don't change anything.
        bool const fDeltaXSubstantial = !(WindowSizeCurrent.cx >= WindowSize.cx && WindowSizeCurrent.cx < WindowSizeMax.cx);
        bool const fDeltaYSubstantial = !(WindowSizeCurrent.cy >= WindowSize.cy && WindowSizeCurrent.cy < WindowSizeMax.cy);

        // If either change was substantial, update the window accordingly to the newly proposed value.
        if (fDeltaXSubstantial || fDeltaYSubstantial)
        {
            _UpdateWindowSize(WindowSize);
        }
        else
        {
            // If the change wasn't substantial, we may still need to update scrollbar positions. Note that PSReadLine
            // scrolls the window via Console.SetWindowPosition, which ultimately calls down to SetConsoleWindowInfo,
            // which ends up in this function.
            psiAttached->InternalUpdateScrollBars();
        }

        psiAttached->ResizingWindow--;
    }

    ConsoleImeResizeCompStrView();

    return STATUS_SUCCESS;
}

// Routine Description:
// - Adjusts the window contents in response to vertical scrolling
// Arguments:
// - ScrollCommand - The relevant command (one line, one page, etc.)
// - AbsoluteChange - The magnitude of the change (for tracking commands)
// Return Value:
// - <none>
void Window::VerticalScroll(_In_ const WORD wScrollCommand, _In_ const WORD wAbsoluteChange) const
{
    COORD NewOrigin;
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    // Log a telemetry event saying the user interacted with the Console
    Telemetry::Instance().SetUserInteractive();

    NewOrigin.X = ScreenInfo->BufferViewport.Left;
    NewOrigin.Y = ScreenInfo->BufferViewport.Top;
    switch (wScrollCommand)
    {
    case SB_LINEUP:
    {
        NewOrigin.Y--;
        break;
    }

    case SB_LINEDOWN:
    {
        NewOrigin.Y++;
        break;
    }

    case SB_PAGEUP:
    {
        NewOrigin.Y -= ScreenInfo->GetScreenWindowSizeY() - 1;
        break;
    }

    case SB_PAGEDOWN:
    {
        NewOrigin.Y += ScreenInfo->GetScreenWindowSizeY() - 1;
        break;
    }

    case SB_THUMBTRACK:
    {
        g_ciConsoleInformation.Flags |= CONSOLE_SCROLLBAR_TRACKING;
        NewOrigin.Y = wAbsoluteChange;
        break;
    }

    case SB_THUMBPOSITION:
    {
        UnblockWriteConsole(CONSOLE_SCROLLBAR_TRACKING);
        NewOrigin.Y = wAbsoluteChange;
        break;
    }

    case SB_TOP:
    {
        NewOrigin.Y = 0;
        break;
    }

    case SB_BOTTOM:
    {
        NewOrigin.Y = (WORD)(ScreenInfo->ScreenBufferSize.Y - ScreenInfo->GetScreenWindowSizeY());
        break;
    }

    default:
    {
        return;
    }
    }

    NewOrigin.Y = (WORD)(max(0, min((SHORT)NewOrigin.Y, (SHORT)ScreenInfo->ScreenBufferSize.Y - (SHORT)ScreenInfo->GetScreenWindowSizeY())));

    ScreenInfo->SetViewportOrigin(TRUE, NewOrigin);
}

// Routine Description:
// - Adjusts the window contents in response to horizontal scrolling
// Arguments:
// - ScrollCommand - The relevant command (one line, one page, etc.)
// - AbsoluteChange - The magnitude of the change (for tracking commands)
// Return Value:
// - <none>
void Window::HorizontalScroll(_In_ const WORD wScrollCommand, _In_ const WORD wAbsoluteChange) const
{
    COORD NewOrigin;

    // Log a telemetry event saying the user interacted with the Console
    Telemetry::Instance().SetUserInteractive();

    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    NewOrigin.X = ScreenInfo->BufferViewport.Left;
    NewOrigin.Y = ScreenInfo->BufferViewport.Top;
    switch (wScrollCommand)
    {
    case SB_LINEUP:
    {
        NewOrigin.X--;
        break;
    }

    case SB_LINEDOWN:
    {
        NewOrigin.X++;
        break;
    }

    case SB_PAGEUP:
    {
        NewOrigin.X -= ScreenInfo->GetScreenWindowSizeX() - 1;
        break;
    }

    case SB_PAGEDOWN:
    {
        NewOrigin.X += ScreenInfo->GetScreenWindowSizeX() - 1;
        break;
    }

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
        NewOrigin.X = wAbsoluteChange;
        break;
    }

    case SB_TOP:
    {
        NewOrigin.X = 0;
        break;
    }

    case SB_BOTTOM:
    {
        NewOrigin.X = (WORD)(ScreenInfo->ScreenBufferSize.X - ScreenInfo->GetScreenWindowSizeX());
        break;
    }

    default:
    {
        return;
    }
    }

    NewOrigin.X = (WORD)(max(0, min((SHORT)NewOrigin.X, (SHORT)ScreenInfo->ScreenBufferSize.X - (SHORT)ScreenInfo->GetScreenWindowSizeX())));
    ScreenInfo->SetViewportOrigin(TRUE, NewOrigin);
}

// Routine Description:
// - Gets the maximum possible window rectangle in pixels. Based on the monitor the window is on or the primary monitor if no window exists yet.
// Arguments:
// - <none>
// Return Value:
// - RECT containing the left, right, top, and bottom positions from the desktop origin in pixels. Measures the outer edges of the potential window.
RECT Window::s_GetMaxWindowRectInPixels()
{
    RECT rc;
    SetRectEmpty(&rc);
    return s_GetMaxWindowRectInPixels(&rc);
}

// Routine Description:
// - Gets the maximum possible window rectangle in pixels. Based on the monitor the window is on or the primary monitor if no window exists yet.
// Arguments:
// - prcSuggested - If we were given a suggested rectangle for where the window is going, we can pass it in here to find out the max size on that monitor.
//                - If this value is zero and we had a valid window handle, we'll use that instead. Otherwise the value of 0 will make us use the primary monitor.
// Return Value:
// - RECT containing the left, right, top, and bottom positions from the desktop origin in pixels. Measures the outer edges of the potential window.
RECT Window::s_GetMaxWindowRectInPixels(_In_ const RECT* const prcSuggested)
{
    // prepare rectangle
    RECT rc = *prcSuggested;
    
    // use zero rect to compare.
    RECT rcZero;
    SetRectEmpty(&rcZero);

    // First get the monitor pointer from either the active window or the default location (0,0,0,0)
    HMONITOR hMonitor = nullptr;

    // NOTE: We must use the nearest monitor because sometimes the system moves the window around into strange spots while performing snap and Win+D operations.
    // Those operations won't work correctly if we use MONITOR_DEFAULTTOPRIMARY.
    if (g_ciConsoleInformation.hWnd == nullptr || (TRUE != EqualRect(&rc, &rcZero)))
    {
        // For invalid window handles or when we were passed a non-zero suggestion rectangle, get the monitor from the rect.
        hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        // Otherwise, get the monitor from the window handle.
        hMonitor = MonitorFromWindow(g_ciConsoleInformation.hWnd, MONITOR_DEFAULTTONEAREST);
    }

    ASSERT(hMonitor != nullptr); // Since we said default to primary, something is seriously wrong with the system if there is no monitor here.

    // Now obtain the monitor pixel dimensions
    MONITORINFO MonitorInfo = { 0 };
    MonitorInfo.cbSize = sizeof(MONITORINFO);

    GetMonitorInfoW(hMonitor, &MonitorInfo);

    // We have to make a correction to the work area. If we actually consume the entire work area (by maximizing the window)
    // The window manager will render the borders off-screen.
    // We need to pad the work rectangle with the border dimensions to represent the actual max outer edges of the window rect.
    WINDOWINFO wi = { 0 };
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(g_ciConsoleInformation.hWnd, &wi);

    if (g_ciConsoleInformation.pWindow != nullptr && g_ciConsoleInformation.pWindow->IsInFullscreen())
    {
        // In full screen mode, we will consume the whole monitor with no chrome.
        rc = MonitorInfo.rcMonitor;
    }
    else
    {
        // In non-full screen, we want to only use the work area (avoiding the task bar space)
        rc = MonitorInfo.rcWork;
        rc.top -= wi.cyWindowBorders;
        rc.bottom += wi.cyWindowBorders;
        rc.left -= wi.cxWindowBorders;
        rc.right += wi.cxWindowBorders;
    }

    return rc;
}

// Routine Description:
// - Gets the maximum possible client rectangle in pixels.
// - This leaves space for potential scroll bars to be visible within the window (which are non-client area pixels when rendered)
// - This is a measurement of the inner area of the window, not including the non-client frame area and not including scroll bars.
// Arguments:
// - <none>
// Return Value:
// - RECT of client area positions in pixels.
RECT Window::s_GetMaxClientRectInPixels()
{
    // This will retrieve the outer window rect. We need the client area to calculate characters.
    RECT rc = s_GetMaxWindowRectInPixels();

    // convert to client rect
    s_ConvertWindowRectToClientRect(&rc);

    return rc;
}

// Routine Description:
// - Gets the minimum possible client rectangle in pixels.
// - Purely based on system metrics. Doesn't compensate for potential scroll bars.
// Arguments:
// - <none>
// Return Value:
// - RECT of client area positions in pixels.
RECT Window::s_GetMinClientRectInPixels()
{
    // prepare rectangle
    RECT rc;
    SetRectEmpty(&rc);

    // set bottom/right dimensions to represent minimum window width/height
    rc.right = GetSystemMetrics(SM_CXMIN);
    rc.bottom = GetSystemMetrics(SM_CYMIN);

    // convert to client rect
    s_ConvertWindowRectToClientRect(&rc);

    // there is no scroll bar subtraction here as the minimum window dimensions can be expanded wider to hold a scroll bar if necessary

    return rc;
}

// Routine Description:
// - Converts a client rect (inside not including non-client area) into a window rect (the outside edge dimensions)
// - NOTE: This one uses the current global DPI for calculations.
// Arguments:
// - prc - Rectangle to be adjusted from window to client
// - dwStyle - Style flags
// - fMenu - Whether or not a menu is present for this window
// - dwExStyle - Extended Style flags
// Return Value:
// - BOOL if adjustment was successful. (See AdjustWindowRectEx definition for details).
BOOL Window::s_AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle)
{
    return WindowDpiApi::s_AdjustWindowRectExForDpi(prc, dwStyle, fMenu, dwExStyle, g_dpi);
}

// Routine Description:
// - Converts a client rect (inside not including non-client area) into a window rect (the outside edge dimensions)
// Arguments:
// - prc - Rectangle to be adjusted from window to client
// - dwStyle - Style flags
// - fMenu - Whether or not a menu is present for this window
// - dwExStyle - Extended Style flags
// - iDpi - The DPI to use for scaling.
// Return Value:
// - BOOL if adjustment was successful. (See AdjustWindowRectEx definition for details).
BOOL Window::s_AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle, _In_ const int iDpi)
{
    return WindowDpiApi::s_AdjustWindowRectExForDpi(prc, dwStyle, fMenu, dwExStyle, iDpi);
}

// Routine Description:
// - Converts a window rect (the outside edge dimensions) into a client rect (inside not including non-client area)
// - Effectively the opposite math of "AdjustWindowRectEx"
// - See: http://blogs.msdn.com/b/oldnewthing/archive/2013/10/17/10457292.aspx
// Arguments:
// - prc - Rectangle to be adjusted from window to client
// - dwStyle - Style flags
// - fMenu - Whether or not a menu is present for this window
// - dwExStyle - Extended Style flags
// Return Value:
// - BOOL if adjustment was successful. (See AdjustWindowRectEx definition for details).
BOOL Window::s_UnadjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle)
{
    RECT rc;
    SetRectEmpty(&rc);
    BOOL fRc = s_AdjustWindowRectEx(&rc, dwStyle, fMenu, dwExStyle);
    if (fRc)
    {
        prc->left -= rc.left;
        prc->top -= rc.top;
        prc->right -= rc.right;
        prc->bottom -= rc.bottom;
    }
    return fRc;
}

// Routine Description:
// - To reduce code duplication, this will do the style lookup and forwards/backwards calls for client/window rectangle translation.
// - Only really intended for use by the related static methods.
// Arguments:
// - prc - Pointer to rectangle to be adjusted from client to window positions.
// - crDirection - specifies which conversion to perform
// Return Value:
// - <none>
void Window::s_ConvertRect(_Inout_ RECT* const prc, _In_ ConvertRect const crDirection)
{
    // collect up current window style (if available) for adjustment
    DWORD dwStyle = 0;
    DWORD dwExStyle = 0;

    if (g_ciConsoleInformation.hWnd != nullptr)
    {
        dwStyle = GetWindowStyle(g_ciConsoleInformation.hWnd);
        dwExStyle = GetWindowExStyle(g_ciConsoleInformation.hWnd);
    }
    else
    {
        dwStyle = CONSOLE_WINDOW_FLAGS;
        dwExStyle = CONSOLE_WINDOW_EX_FLAGS;
    }

    switch (crDirection)
    {
    case CLIENT_TO_WINDOW:
    {
        // ask system to adjust our client rectangle into a window rectangle using the given style
        s_AdjustWindowRectEx(prc, dwStyle, false, dwExStyle);
        break;
    }
    case WINDOW_TO_CLIENT:
    {
        // ask system to adjust our window rectangle into a client rectangle using the given style
        s_UnadjustWindowRectEx(prc, dwStyle, false, dwExStyle);
        break;
    }
    default:
    {
        ASSERT(false); // not implemented
    }
    }
}

// Routine Description:
// - Converts a window rect (the outside edge dimensions) into a client rect (inside not including non-client area)
// - This is a helper to call UnadjustWindowRectEx.
// - It finds the appropriate window styles for the active window or uses the defaults from our class registration.
// - It does NOT compensate for scrollbars or menus.
// Arguments:
// - prc - Pointer to rectangle to be adjusted from window to client positions.
// Return Value:
// - <none>
void Window::s_ConvertWindowRectToClientRect(_Inout_ RECT* const prc)
{
    s_ConvertRect(prc, ConvertRect::WINDOW_TO_CLIENT);
}

// Routine Description:
// - Converts a client rect (inside not including non-client area) into a window rect (the outside edge dimensions)
// - This is a helper to call AdjustWindowRectEx.
// - It finds the appropriate window styles for the active window or uses the defaults from our class registration.
// - It does NOT compensate for scrollbars or menus.
// Arguments:
// - prc - Pointer to rectangle to be adjusted from client to window positions.
// Return Value:
// - <none>
void Window::s_ConvertClientRectToWindowRect(_Inout_ RECT* const prc)
{
    s_ConvertRect(prc, ConvertRect::CLIENT_TO_WINDOW);
}

// Routine Description:
// - Converts a window position structure (sent to us when the window moves) into a window rectangle (the outside edge dimensions)
// Arguments:
// - lpWindowPos - position structure received via Window message
// - prc - rectangle to fill
// Return Value:
// - <none>
void Window::s_ConvertWindowPosToWindowRect(_In_ LPWINDOWPOS const lpWindowPos, _Out_ RECT* const prc)
{
    prc->left = lpWindowPos->x;
    prc->top = lpWindowPos->y;
    prc->right = lpWindowPos->x + lpWindowPos->cx;
    prc->bottom = lpWindowPos->y + lpWindowPos->cy;
}

// Routine Description:
// - Converts character counts of the viewport (client area, screen buffer) into the outer pixel dimensions of the window using the current window for context
// Arguments:
// - coordWindowInChars - The size of the viewport
// - prectWindow - rectangle to fill with pixel positions of the outer edge rectangle for this window
// Return Value:
// - <none>
void Window::_CalculateWindowRect(_In_ COORD const coordWindowInChars, _Inout_ RECT* const prectWindow) const
{
    SCREEN_INFORMATION* const psiAttached = GetScreenInfo();
    COORD const coordFontSize = psiAttached->GetScreenFontSize();
    HWND const hWnd = GetWindowHandle();
    COORD const coordBufferSize = psiAttached->ScreenBufferSize;
    int const iDpi = g_dpi;

    s_CalculateWindowRect(coordWindowInChars, iDpi, coordFontSize, coordBufferSize, hWnd, prectWindow);
}

// Routine Description:
// - Converts character counts of the viewport (client area, screen buffer) into the outer pixel dimensions of the window
// Arguments:
// - coordWindowInChars - The size of the viewport
// - iDpi - The DPI of the monitor on which this window will reside (used to get DPI-scaled system metrics)
// - coordFontSize - the size in pixels of the font on the monitor (this should be already scaled for DPI)
// - coordBufferSize - the character count of the buffer rectangle in X by Y
// - hWnd - If available, a handle to the window we would change so we can retrieve its class information for border/titlebar/etc metrics.
// - prectWindow - rectangle to fill with pixel positions of the outer edge rectangle for this window
// Return Value:
// - <none>
void Window::s_CalculateWindowRect(_In_ COORD const coordWindowInChars, 
                                   _In_ int const iDpi, 
                                   _In_ COORD const coordFontSize, 
                                   _In_ COORD const coordBufferSize,
                                   _In_opt_ HWND const hWnd, 
                                   _Inout_ RECT* const prectWindow)
{
    SIZE sizeWindow;

    // Initially use the given size in characters * font size to get client area pixel size
    sizeWindow.cx = coordWindowInChars.X * coordFontSize.X;
    sizeWindow.cy = coordWindowInChars.Y * coordFontSize.Y;

    // Create a proposed rectangle
    RECT rectProposed = { prectWindow->left, prectWindow->top, prectWindow->left + sizeWindow.cx, prectWindow->top + sizeWindow.cy };

    // Now adjust the client area into a window size
    // 1. Start with default window style
    DWORD dwStyle = CONSOLE_WINDOW_FLAGS;
    DWORD dwExStyle = CONSOLE_WINDOW_EX_FLAGS;
    BOOL fMenu = FALSE;

    // 2. if we already have a window handle, check if the style has been updated
    if (hWnd != nullptr)
    {
        dwStyle = GetWindowStyle(hWnd);
        dwExStyle = GetWindowExStyle(hWnd);

        HMENU hMenu = GetMenu(hWnd);
        if (hMenu != nullptr)
        {
            fMenu = TRUE;
        }
    }

    // 3. Perform adjustment
    // NOTE: This may adjust the position of the window as well as the size. This is why we use rectProposed in the interim.
    s_AdjustWindowRectEx(&rectProposed, dwStyle, fMenu, dwExStyle, iDpi);

    // Finally compensate for scroll bars

    // If the window is smaller than the buffer in width, add space at the bottom for a horizontal scroll bar
    if (coordWindowInChars.X < coordBufferSize.X)
    {
        rectProposed.bottom += (SHORT)WindowDpiApi::s_GetSystemMetricsForDpi(SM_CYHSCROLL, iDpi);
    }

    // If the window is smaller than the buffer in height, add space at the right for a vertical scroll bar
    if (coordWindowInChars.Y < coordBufferSize.Y)
    {
        rectProposed.right += (SHORT)WindowDpiApi::s_GetSystemMetricsForDpi(SM_CXVSCROLL, iDpi);
    }

    // Apply the calculated sizes to the existing window pointer
    // We do this at the end so we can preserve the positioning of the window and just change the size.
    prectWindow->right = prectWindow->left + RECT_WIDTH(&rectProposed);
    prectWindow->bottom = prectWindow->top + RECT_HEIGHT(&rectProposed);
}

RECT Window::GetWindowRect() const
{
    RECT rc = { 0 };
    ::GetWindowRect(GetWindowHandle(), &rc);
    return rc;
}

HWND Window::GetWindowHandle() const
{
    return g_ciConsoleInformation.hWnd;
}

SCREEN_INFORMATION* Window::GetScreenInfo() const
{
    return g_ciConsoleInformation.CurrentScreenBuffer;
}

// Routine Description:
// - Gets the window opacity (alpha channel)
// Arguments:
// - <none>
// Return Value:
// - The level of opacity. 0xff should represent 100% opaque and 0x00 should be 100% transparent. (Used for Alpha channel in drawing.)
BYTE Window::GetWindowOpacity() const
{
    return _pSettings->GetWindowAlpha();
}

// Routine Description:
// - Sets the window opacity (alpha channel) with the given value
// - Will restrict to within the valid range. Invalid values will use 0% transparency/100% opaque.
// Arguments:
// - bOpacity - 0xff/100% opacity = opaque window. 0xb2/70% opacity = 30% transparent window.
// Return Value:
// - <none>
void Window::SetWindowOpacity(_In_ BYTE const bOpacity)
{
    _pSettings->SetWindowAlpha(bOpacity);
}

// Routine Description:
// - Calls the operating system to apply the current window opacity settings to the active console window.
// Arguments:
// - <none>
// Return Value:
// - <none>
void Window::ApplyWindowOpacity() const
{
    const BYTE bAlpha = GetWindowOpacity();
    HWND const hWnd = GetWindowHandle();

    if (bAlpha != BYTE_MAX)
    {
        // See: http://msdn.microsoft.com/en-us/library/ms997507.aspx
        SetWindowLongW(hWnd, GWL_EXSTYLE, GetWindowLongW(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, 0, bAlpha, LWA_ALPHA);
    }
    else
    {
        // if we're at 100% opaque, just turn off the layered style.
        SetWindowLongW(hWnd, GWL_EXSTYLE, GetWindowLongW(hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    }
}

// Routine Description:
// - Changes the window opacity by a specified delta.
// - This will update the internally stored value by the given delta (within boundaries)
//   and then will have the new value applied to the actual window.
// - Values that would make the opacity greater than 100% will be fixed to 100%.
// - Values that would bring the opacity below the minimum threshold will be fixed to the minimum threshold.
// Arguments:
// - sOpacityDelta - How much to modify the current window opacity. Positive = more opaque. Negative = more transparent.
// Return Value:
// - <none>
void Window::ChangeWindowOpacity(_In_ short const sOpacityDelta)
{
    // Window Opacity is always a BYTE (unsigned char, 1 byte)
    // Delta is a short (signed short, 2 bytes)

    int iAlpha = GetWindowOpacity(); // promote unsigned char to fit into a signed int (4 bytes)

    iAlpha += sOpacityDelta; // performing signed math of 2 byte delta into 4 bytes will not under/overflow.

    // comparisons are against 1 byte values and are ok.
    if (iAlpha > BYTE_MAX)
    {
        iAlpha = BYTE_MAX;
    }
    else if (iAlpha < MIN_WINDOW_OPACITY)
    {
        iAlpha = MIN_WINDOW_OPACITY;
    }

    //Opacity bool is set to true when keyboard or mouse short cut used.
    SetWindowOpacity((BYTE)iAlpha); // cast to fit is guaranteed to be within byte bounds by the checks above.
    ApplyWindowOpacity();
}

bool Window::IsInFullscreen() const
{
    return _fIsInFullscreen;
}

void Window::SetIsFullscreen(_In_ bool const fFullscreenEnabled)
{
    // it shouldn't be possible to enter fullscreen while in fullscreen (and vice versa)
    ASSERT(_fIsInFullscreen != fFullscreenEnabled);
    _fIsInFullscreen = fFullscreenEnabled;

    HWND const hWnd = GetWindowHandle();

    // First, modify regular window styles as appropriate
    LONG dwWindowStyle = GetWindowLongW(hWnd, GWL_STYLE);
    if (_fIsInFullscreen)
    {
        // moving to fullscreen. remove WS_OVERLAPPEDWINDOW, which specifies styles for non-fullscreen windows (e.g.
        // caption bar). add the WS_POPUP style to allow us to size ourselves to the monitor size.
        dwWindowStyle = dwWindowStyle & ~WS_OVERLAPPEDWINDOW;
        dwWindowStyle |= WS_POPUP;
    }
    else
    {
        // coming back from fullscreen. undo what we did to get in to fullscreen in the first place.
        dwWindowStyle = dwWindowStyle & ~WS_POPUP;
        dwWindowStyle |= WS_OVERLAPPEDWINDOW;
    }
    SetWindowLongW(hWnd, GWL_STYLE, dwWindowStyle);

    // Now modify extended window styles as appropriate
    LONG dwExWindowStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
    if (_fIsInFullscreen)
    {
        // moving to fullscreen. remove the client edge style to avoid an ugly border when not focused.
        dwExWindowStyle = dwExWindowStyle & ~WS_EX_CLIENTEDGE;
    }
    else
    {
        // coming back from fullscreen.
        dwExWindowStyle |= WS_EX_CLIENTEDGE;
    }
    SetWindowLongW(hWnd, GWL_EXSTYLE, dwExWindowStyle);

    _BackupWindowSizes();
    _ApplyWindowSize();
}

void Window::_BackupWindowSizes()
{
    if (_fIsInFullscreen)
    {
        // back up current window size
        _rcNonFullscreenWindowSize = GetWindowRect();

        // get and back up the current monitor's size
        HMONITOR const hCurrentMonitor = MonitorFromWindow(GetWindowHandle(), MONITOR_DEFAULTTONEAREST);
        MONITORINFO currMonitorInfo;
        currMonitorInfo.cbSize = sizeof(currMonitorInfo);
        if (GetMonitorInfo(hCurrentMonitor, &currMonitorInfo))
        {
            _rcFullscreenWindowSize = currMonitorInfo.rcMonitor;
        }
    }
}

void Window::_ApplyWindowSize() const
{
    const RECT rcNewSize = _fIsInFullscreen ? _rcFullscreenWindowSize : _rcNonFullscreenWindowSize;

    SetWindowPos(GetWindowHandle(),
        HWND_TOP,
        rcNewSize.left,
        rcNewSize.top,
        RECT_WIDTH(&rcNewSize),
        RECT_HEIGHT(&rcNewSize),
        SWP_FRAMECHANGED);

    SCREEN_INFORMATION* const psiAttached = GetScreenInfo();
    psiAttached->MakeCurrentCursorVisible();
}

void Window::ToggleFullscreen()
{
    SetIsFullscreen(!IsInFullscreen());
}

void Window::s_ReinitializeFontsForDPIChange()
{
    g_ciConsoleInformation.CurrentScreenBuffer->RefreshFontWithRenderer();
}

LRESULT Window::s_RegPersistWindowPos(_In_ PCWSTR const pwszTitle,
                                      _In_ const BOOL fAutoPos,
                                      _In_ const Window* const pWindow)
{

    HKEY hCurrentUserKey, hConsoleKey, hTitleKey;
    // Open the current user registry key.
    NTSTATUS Status = RegistrySerialization::s_OpenCurrentUserConsoleTitleKey(pwszTitle, &hCurrentUserKey, &hConsoleKey, &hTitleKey);
    if (NT_SUCCESS(Status))
    {
        // Save window size
        auto windowRect = pWindow->GetWindowRect();
        auto windowWidth = g_ciConsoleInformation.CurrentScreenBuffer->GetScreenWindowSizeX();
        auto windowHeight = g_ciConsoleInformation.CurrentScreenBuffer->GetScreenWindowSizeY();
        DWORD dwValue = MAKELONG(windowWidth, windowHeight);
        Status = RegistrySerialization::s_UpdateValue(hConsoleKey,
                                                      hTitleKey,
                                                      CONSOLE_REGISTRY_WINDOWSIZE,
                                                      REG_DWORD,
                                                      reinterpret_cast<BYTE*>(&dwValue),
                                                      static_cast<DWORD>(sizeof(dwValue)));
        if (NT_SUCCESS(Status))
        {
            auto screenBufferWidth = g_ciConsoleInformation.CurrentScreenBuffer->ScreenBufferSize.X;
            auto screenBufferHeight = g_ciConsoleInformation.CurrentScreenBuffer->ScreenBufferSize.Y;
            dwValue =  MAKELONG(screenBufferWidth, screenBufferHeight);
            Status = RegistrySerialization::s_UpdateValue(hConsoleKey,
                                                          hTitleKey,
                                                          CONSOLE_REGISTRY_BUFFERSIZE,
                                                          REG_DWORD,
                                                          reinterpret_cast<BYTE*>(&dwValue),
                                                          static_cast<DWORD>(sizeof(dwValue)));
            if (NT_SUCCESS(Status))
            {

                // Save window position
                if (fAutoPos)
                {
                    Status = RegistrySerialization::s_DeleteKey(hTitleKey, CONSOLE_REGISTRY_WINDOWPOS);
                }
                else
                {
                    dwValue = MAKELONG(windowRect.left, windowRect.top);
                    Status = RegistrySerialization::s_UpdateValue(hConsoleKey,
                                                                  hTitleKey,
                                                                  CONSOLE_REGISTRY_WINDOWPOS,
                                                                  REG_DWORD,
                                                                  reinterpret_cast<BYTE*>(&dwValue),
                                                                  static_cast<DWORD>(sizeof(dwValue)));
                }
            }
        }

        if (hTitleKey != hConsoleKey)
        {
            RegCloseKey(hTitleKey);
        }
        
        RegCloseKey(hConsoleKey);
        RegCloseKey(hCurrentUserKey);
    }

    return Status;
}

LRESULT Window::s_RegPersistWindowOpacity(_In_ PCWSTR const pwszTitle, _In_ const Window* const pWindow)
{
    HKEY hCurrentUserKey, hConsoleKey, hTitleKey;

    // Open the current user registry key.
    NTSTATUS Status = RegistrySerialization::s_OpenCurrentUserConsoleTitleKey(pwszTitle, &hCurrentUserKey, &hConsoleKey, &hTitleKey);
    if (NT_SUCCESS(Status))
    {
        // Save window opacity
        DWORD dwValue;
        dwValue = pWindow->GetWindowOpacity();
        Status = RegistrySerialization::s_UpdateValue(hConsoleKey,
                                                      hTitleKey,
                                                      CONSOLE_REGISTRY_WINDOWALPHA,
                                                      REG_DWORD,
                                                      reinterpret_cast<BYTE*>(&dwValue),
                                                      static_cast<DWORD>(sizeof(dwValue)));

        if (hTitleKey != hConsoleKey)
        {
            RegCloseKey(hTitleKey);
        }
        RegCloseKey(hConsoleKey);
        RegCloseKey(hCurrentUserKey);
    }
    return Status;
}

void Window::s_PersistWindowPosition(_In_ PCWSTR pwszLinkTitle,
                                     _In_ PCWSTR pwszOriginalTitle,
                                     _In_ const DWORD dwFlags,
                                     _In_ const Window* const pWindow)
{
    if (pwszLinkTitle == nullptr)
    {
        PWSTR pwszTranslatedTitle = TranslateConsoleTitle(pwszOriginalTitle, FALSE, TRUE);

        if (pwszTranslatedTitle != nullptr)
        {
            Window::s_RegPersistWindowPos(pwszTranslatedTitle, IsFlagSet(dwFlags, CONSOLE_AUTO_POSITION), pWindow);
            delete[] pwszTranslatedTitle;
        }
    }
    else
    {
        CONSOLE_STATE_INFO StateInfo = {0};
        GetConsoleState(&StateInfo);
        ShortcutSerialization::s_SetLinkValues(&StateInfo, IsEastAsianCP(g_uiOEMCP), true);
    }
}

void Window::s_PersistWindowOpacity(_In_ PCWSTR pwszLinkTitle, _In_ PCWSTR pwszOriginalTitle, _In_ const Window* const pWindow)
{
    if (pwszLinkTitle == nullptr)
    {
        PWSTR pwszTranslatedTitle = TranslateConsoleTitle(pwszOriginalTitle, FALSE, TRUE);
        if (pwszTranslatedTitle != nullptr)
        {
            Window::s_RegPersistWindowOpacity(pwszTranslatedTitle, pWindow);
            delete[] pwszTranslatedTitle;
        }
    }
    else
    {
        CONSOLE_STATE_INFO StateInfo = {0};
        GetConsoleState(&StateInfo);
        ShortcutSerialization::s_SetLinkValues(&StateInfo, IsEastAsianCP(g_uiOEMCP), true);
    }
}

void Window::SetWindowHasMoved(_In_ BOOL const fHasMoved)
{
    this->_fHasMoved = fHasMoved;
}
