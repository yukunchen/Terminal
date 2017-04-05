/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"


#include "ConsoleControl.hpp"
#include "icon.hpp"
#include "menu.hpp"
#include "window.hpp"
#include "windowio.hpp"
#include "windowdpiapi.hpp"
#include "windowmetrics.hpp"
#include "windowUiaProvider.hpp"

#include "..\..\host\globals.h"
#include "..\..\host\cursor.h"
#include "..\..\host\dbcs.h"
#include "..\..\host\getset.h"
#include "..\..\host\misc.h"
#include "..\..\host\_output.h"
#include "..\..\host\output.h"
#include "..\..\host\renderData.hpp"
#include "..\..\host\scrolling.hpp"
#include "..\..\host\srvinit.h"
#include "..\..\host\stream.h"
#include "..\..\host\telemetry.hpp"
#include "..\..\host\tracing.hpp"

#include "..\..\renderer\base\renderer.hpp"
#include "..\..\renderer\gdi\gdirenderer.hpp"

#include "..\inc\ServiceLocator.hpp"

// The following default masks are used in creating windows
// Make sure that these flags match when switching to fullscreen and back
#define CONSOLE_WINDOW_FLAGS (WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL)
#define CONSOLE_WINDOW_EX_FLAGS (WS_EX_WINDOWEDGE | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW )

// Window class name
#define CONSOLE_WINDOW_CLASS (L"ConsoleWindowClass")

using namespace Microsoft::Console::Interactivity::Win32;

ATOM Window::s_atomWindowClass = 0;
Window* Window::s_Instance = nullptr;

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
    if (nullptr != _pUiaProvider)
    {
        // This is a COM object, so call Release. It will clean up itself when the last ref is released.
        _pUiaProvider->Release();
    }

    if (ServiceLocator::LocateGlobals()->pRender != nullptr)
    {
        delete ServiceLocator::LocateGlobals()->pRender;
    }

    if (ServiceLocator::LocateGlobals()->pRenderData != nullptr)
    {
        delete ServiceLocator::LocateGlobals()->pRenderData;
    }

    if (ServiceLocator::LocateGlobals()->pRenderEngine != nullptr)
    {
        delete ServiceLocator::LocateGlobals()->pRenderEngine;
    }
}

// Routine Description:
// - This routine allocates and initializes a window for the console
// Arguments:
// - pSettings - All user-configurable settings related to the console host
// - pScreen - The initial screen rendering data to attach to (renders in the client area of this window)
// Return Value:
// - STATUS_SUCCESS or suitable NT error code
NTSTATUS Window::CreateInstance(_In_ Settings* const pSettings,
                                _In_ SCREEN_INFORMATION* const pScreen)
{
    NTSTATUS status = s_RegisterWindowClass();

    if (NT_SUCCESS(status))
    {
        Window* pNewWindow = new Window();

        status = pNewWindow->_MakeWindow(pSettings, pScreen);

        if (NT_SUCCESS(status))
        {
            Window::s_Instance = pNewWindow;
            ServiceLocator::SetConsoleWindowInstance(pNewWindow);
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

    ServiceLocator::LocateGlobals()->sVerticalScrollSize = (SHORT)ServiceLocator::LocateHighDpiApi<WindowDpiApi>()->GetSystemMetricsForDpi(SM_CXVSCROLL, ServiceLocator::LocateGlobals()->dpi);
    ServiceLocator::LocateGlobals()->sHorizontalScrollSize = (SHORT)ServiceLocator::LocateHighDpiApi<WindowDpiApi>()->GetSystemMetricsForDpi(SM_CYHSCROLL, ServiceLocator::LocateGlobals()->dpi);

    ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->TextInfo->GetCursor()->UpdateSystemMetrics();
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

    ServiceLocator::LocateGlobals()->pRenderData = new RenderData();
    status = NT_TESTNULL(ServiceLocator::LocateGlobals()->pRenderData);
    if (NT_SUCCESS(status))
    {
        GdiEngine* pGdiEngine = nullptr;

        try
        {
            pGdiEngine = new GdiEngine();
            status = NT_TESTNULL(pGdiEngine);
        }
        catch (...)
        {
            status = NTSTATUS_FROM_HRESULT(wil::ResultFromCaughtException());
        }

        if (NT_SUCCESS(status))
        {
            ServiceLocator::LocateGlobals()->pRenderEngine = pGdiEngine;

            Renderer* pNewRenderer = nullptr;
            if (SUCCEEDED(Renderer::s_CreateInstance(ServiceLocator::LocateGlobals()->pRenderData, ServiceLocator::LocateGlobals()->pRenderEngine, &pNewRenderer)))
            {
                ServiceLocator::LocateGlobals()->pRender = pNewRenderer;
            }

            status = NT_TESTNULL(ServiceLocator::LocateGlobals()->pRender);
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

            if (!IsFlagSet(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_AUTO_POSITION))
            {
                //if launched from a shortcut, ensure window is visible on screen
                if (pSettings->IsStartupTitleIsLinkNameSet())
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
                ServiceLocator::LocateGlobals()->getConsoleInformation()->Title,
                CONSOLE_WINDOW_FLAGS,
                IsFlagSet(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_AUTO_POSITION) ? CW_USEDEFAULT : rectProposed.left,
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
                this->_hWnd = hWnd;

                status = NTSTATUS_FROM_HRESULT(pGdiEngine->SetHwnd(hWnd));

                if (NT_SUCCESS(status))
                {
                    // Set alpha on window if requested
                    ApplyWindowOpacity();

                    status = Menu::CreateInstance(hWnd);
                    
                    if (NT_SUCCESS(status))
                    {
                        ServiceLocator::LocateGlobals()->getConsoleInformation()->ConsoleIme.RefreshAreaAttributes();

                        // Do WM_GETICON workaround. Must call WM_SETICON once or apps calling WM_GETICON will get null.
                        Icon::Instance().ApplyWindowMessageWorkaround();

                        // Set up the hot key for this window.
                        if (ServiceLocator::LocateGlobals()->getConsoleInformation()->GetHotKey() != 0)
                        {
                            SendMessageW(hWnd, WM_SETHOTKEY, ServiceLocator::LocateGlobals()->getConsoleInformation()->GetHotKey(), 0);
                        }

                        ServiceLocator::LocateHighDpiApi<WindowDpiApi>()->EnableChildWindowDpiMessage(_hWnd, TRUE /*fEnable*/);

                        // Post a window size update so that the new console window will size itself correctly once it's up and
                        // running. This works around chicken & egg cases involving window size calculations having to do with font
                        // sizes, DPI, and non-primary monitors (see MSFT #2367234).
                        psiAttached->PostUpdateWindowSize();
                    }
                }
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
    if (wShowWindow != SW_SHOWNOACTIVATE &&
        wShowWindow != SW_SHOWMINNOACTIVE &&
        wShowWindow != SW_HIDE)
    {
        // Do not check result. On some SKUs, such as WinPE, it's perfectly OK for NULL to be returned.
        SetActiveWindow(hWnd);
    }
    else if (wShowWindow == SW_SHOWMINNOACTIVE)
    {
        // If we're minimized and not the active window, set iconic to stop rendering
        ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags |= CONSOLE_IS_ICONIC;
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
NTSTATUS Window::SetViewportOrigin(_In_ SMALL_RECT NewWindow)
{
    SCREEN_INFORMATION* const ScreenInfo = GetScreenInfo();

    COORD const FontSize = ScreenInfo->GetScreenFontSize();

    if (AreAllFlagsClear(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, (CONSOLE_IS_ICONIC | CONSOLE_NO_WINDOW)))
    {
        Selection* pSelection = &Selection::Instance();
        pSelection->HideSelection();

        // Fire off an event to let accessibility apps know we've scrolled.
        NotifyWinEvent(EVENT_CONSOLE_UPDATE_SCROLL, _hWnd, ScreenInfo->GetBufferViewport().Left - NewWindow.Left, ScreenInfo->GetBufferViewport().Top - NewWindow.Top);

        // The new window is OK. Store it in screeninfo and refresh screen.
        ScreenInfo->SetBufferViewport(NewWindow);
        Tracing::s_TraceWindowViewport(ScreenInfo->GetBufferViewport());

        if (ServiceLocator::LocateGlobals()->pRender != nullptr)
        {
            ServiceLocator::LocateGlobals()->pRender->TriggerScroll();
        }

        pSelection->ShowSelection();
    }
    else
    {
        // we're iconic
        ScreenInfo->SetBufferViewport(NewWindow);
        Tracing::s_TraceWindowViewport(ScreenInfo->GetBufferViewport());
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

// This routine adds or removes the name to or from the beginning of the window title. The possible names are "Scroll", "Mark", and "Select"
void Window::UpdateWindowText()
{
    const bool fInScrollMode = Scrolling::s_IsInScrollMode();

    Selection *pSelection = &Selection::Instance();
    const bool fInKeyboardMarkMode = pSelection->IsInSelectingState() && pSelection->IsKeyboardMarkSelection();
    const bool fInMouseSelectMode = pSelection->IsInSelectingState() && pSelection->IsMouseInitiatedSelection();

    // should have at most one active mode
    ASSERT((fInKeyboardMarkMode && !fInMouseSelectMode && !fInScrollMode) ||
        (!fInKeyboardMarkMode && fInMouseSelectMode && !fInScrollMode) ||
        (!fInKeyboardMarkMode && !fInMouseSelectMode && fInScrollMode) ||
        (!fInKeyboardMarkMode && !fInMouseSelectMode && !fInScrollMode));

    // determine which message, if any, we want to use
    DWORD dwMsgId = 0;
    if (fInKeyboardMarkMode)
    {
        dwMsgId = ID_CONSOLE_MSGMARKMODE;
    }
    else if (fInMouseSelectMode)
    {
        dwMsgId = ID_CONSOLE_MSGSELECTMODE;
    }
    else if (fInScrollMode)
    {
        dwMsgId = ID_CONSOLE_MSGSCROLLMODE;
    }

    // if we have a message, use it
    if (dwMsgId != 0)
    {
        // load format string
        WCHAR szFmt[64];
        if (LoadStringW(ServiceLocator::LocateGlobals()->hInstance, ID_CONSOLE_FMT_WINDOWTITLE, szFmt, ARRAYSIZE(szFmt)) > 0)
        {
            // load mode string
            WCHAR szMsg[64];
            if (LoadStringW(ServiceLocator::LocateGlobals()->hInstance, dwMsgId, szMsg, ARRAYSIZE(szMsg)) > 0)
            {
                // construct new title string
                PWSTR pwszTitle = new WCHAR[MAX_PATH];
                if (pwszTitle != nullptr)
                {
                    if (SUCCEEDED(StringCchPrintfW(pwszTitle,
                        MAX_PATH,
                        szFmt,
                        szMsg,
                        ServiceLocator::LocateGlobals()->getConsoleInformation()->Title)))
                    {
                        ServiceLocator::LocateConsoleWindow<Window>()->PostUpdateTitle(pwszTitle);
                    }
                    else
                    {
                        delete[] pwszTitle;
                    }
                }
            }
        }
    }
    else
    {
        // no mode-based message. set title back to original state.
        //Copy the title into a new buffer, because consuming the update_title HeapFree's the pwsz.
        ServiceLocator::LocateConsoleWindow()->PostUpdateTitleWithCopy(ServiceLocator::LocateGlobals()->getConsoleInformation()->Title);
    }
}

void Window::CaptureMouse()
{
    SetCapture(_hWnd);
}

BOOL Window::ReleaseMouse()
{
    return ReleaseCapture();
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

    if (IsFlagClear(ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags, CONSOLE_IS_ICONIC))
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

    ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags &= ~CONSOLE_SETTING_WINDOW_SIZE;
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
        ServiceLocator::LocateWindowMetrics<WindowMetrics>()->ConvertClientRectToWindowRect(&rectSizeTemp);

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
                WindowSize.cy += ServiceLocator::LocateGlobals()->sHorizontalScrollSize;
            }
            if (!psiAttached->GetMainBuffer()->IsMaximizedY())
            {
                WindowSize.cx += ServiceLocator::LocateGlobals()->sVerticalScrollSize;
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
        RECT const rcWindowCurrent = GetWindowRect();
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

    NewOrigin.X = ScreenInfo->GetBufferViewport().Left;
    NewOrigin.Y = ScreenInfo->GetBufferViewport().Top;

    const SHORT sScreenBufferSizeY = ScreenInfo->GetScreenBufferSize().Y;

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
        ServiceLocator::LocateGlobals()->getConsoleInformation()->Flags |= CONSOLE_SCROLLBAR_TRACKING;
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
        NewOrigin.Y = (WORD)(sScreenBufferSizeY - ScreenInfo->GetScreenWindowSizeY());
        break;
    }

    default:
    {
        return;
    }
    }

    NewOrigin.Y = (WORD)(max(0, min((SHORT)NewOrigin.Y, sScreenBufferSizeY - (SHORT)ScreenInfo->GetScreenWindowSizeY())));

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

    const SHORT sScreenBufferSizeX = ScreenInfo->GetScreenBufferSize().X;

    NewOrigin.X = ScreenInfo->GetBufferViewport().Left;
    NewOrigin.Y = ScreenInfo->GetBufferViewport().Top;
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
        NewOrigin.X = (WORD)(sScreenBufferSizeX - ScreenInfo->GetScreenWindowSizeX());
        break;
    }

    default:
    {
        return;
    }
    }

    NewOrigin.X = (WORD)(max(0, min((SHORT)NewOrigin.X, sScreenBufferSizeX - (SHORT)ScreenInfo->GetScreenWindowSizeX())));
    ScreenInfo->SetViewportOrigin(TRUE, NewOrigin);
}

BOOL Window::EnableBothScrollBars()
{
    return EnableScrollBar(_hWnd, SB_BOTH, ESB_ENABLE_BOTH);
}

int Window::UpdateScrollBar(bool isVertical, bool isAltBuffer, UINT pageSize, int maxSize, int viewportPosition)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = isAltBuffer ? SIF_ALL | SIF_DISABLENOSCROLL : SIF_ALL;
    si.nPage = pageSize;
    si.nMin = 0;
    si.nMax = maxSize;
    si.nPos = viewportPosition;

    return SetScrollInfo(_hWnd, isVertical ? SB_VERT : SB_HORZ, &si, TRUE);
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
    COORD const coordBufferSize = psiAttached->GetScreenBufferSize();
    int const iDpi = ServiceLocator::LocateGlobals()->dpi;

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
    ServiceLocator::LocateWindowMetrics<WindowMetrics>()->AdjustWindowRectEx(&rectProposed, dwStyle, fMenu, dwExStyle, iDpi);

    // Finally compensate for scroll bars

    // If the window is smaller than the buffer in width, add space at the bottom for a horizontal scroll bar
    if (coordWindowInChars.X < coordBufferSize.X)
    {
        rectProposed.bottom += (SHORT)ServiceLocator::LocateHighDpiApi<WindowDpiApi>()->GetSystemMetricsForDpi(SM_CYHSCROLL, iDpi);
    }

    // If the window is smaller than the buffer in height, add space at the right for a vertical scroll bar
    if (coordWindowInChars.Y < coordBufferSize.Y)
    {
        rectProposed.right += (SHORT)ServiceLocator::LocateHighDpiApi<WindowDpiApi>()->GetSystemMetricsForDpi(SM_CXVSCROLL, iDpi);
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
    return this->_hWnd;
}

SCREEN_INFORMATION* Window::GetScreenInfo() const
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
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
        ClearAllFlags(dwWindowStyle, WS_OVERLAPPEDWINDOW);
        SetFlag(dwWindowStyle, WS_POPUP);
    }
    else
    {
        // coming back from fullscreen. undo what we did to get in to fullscreen in the first place.
        ClearFlag(dwWindowStyle, WS_POPUP);
        SetAllFlags(dwWindowStyle, WS_OVERLAPPEDWINDOW);
    }
    SetWindowLongW(hWnd, GWL_STYLE, dwWindowStyle);

    // Now modify extended window styles as appropriate
    LONG dwExWindowStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);
    if (_fIsInFullscreen)
    {
        // moving to fullscreen. remove the window edge style to avoid an ugly border when not focused.
        ClearFlag(dwExWindowStyle, WS_EX_WINDOWEDGE);
    }
    else
    {
        // coming back from fullscreen.
        SetFlag(dwExWindowStyle, WS_EX_WINDOWEDGE);
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
    ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->RefreshFontWithRenderer();
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
        auto windowWidth = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->GetScreenWindowSizeX();
        auto windowHeight = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->GetScreenWindowSizeY();
        DWORD dwValue = MAKELONG(windowWidth, windowHeight);
        Status = RegistrySerialization::s_UpdateValue(hConsoleKey,
                                                      hTitleKey,
                                                      CONSOLE_REGISTRY_WINDOWSIZE,
                                                      REG_DWORD,
                                                      reinterpret_cast<BYTE*>(&dwValue),
                                                      static_cast<DWORD>(sizeof(dwValue)));
        if (NT_SUCCESS(Status))
        {
            const COORD coordScreenBufferSize = ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer->GetScreenBufferSize();
            auto screenBufferWidth = coordScreenBufferSize.X;
            auto screenBufferHeight = coordScreenBufferSize.Y;
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
                    Status = RegistrySerialization::s_DeleteValue(hTitleKey, CONSOLE_REGISTRY_WINDOWPOS);
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
        Menu::s_GetConsoleState(&StateInfo);
        ShortcutSerialization::s_SetLinkValues(&StateInfo, IsEastAsianCP(ServiceLocator::LocateGlobals()->uiOEMCP), true);
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
        Menu::s_GetConsoleState(&StateInfo);
        ShortcutSerialization::s_SetLinkValues(&StateInfo, IsEastAsianCP(ServiceLocator::LocateGlobals()->uiOEMCP), true);
    }
}

void Window::SetWindowHasMoved(_In_ BOOL const fHasMoved)
{
    this->_fHasMoved = fHasMoved;
}

// Routine Description:
// - Creates/retrieves a handle to the UI Automation provider COM interfaces
// Arguments:
// - <none>
// Return Value:
// - Pointer to UI Automation provider class/interfaces.
IRawElementProviderSimple* Window::_GetUiaProvider()
{
    if (nullptr == _pUiaProvider)
    {
        _pUiaProvider = new WindowUiaProvider(this);
    }

    return _pUiaProvider;
}

void Window::SetOwner()
{
    SetConsoleWindowOwner(_hWnd, nullptr);
}

BOOL Window::GetCursorPosition(_Out_ LPPOINT lpPoint)
{
    return GetCursorPos(lpPoint);
}

BOOL Window::GetClientRectangle(_Out_ LPRECT lpRect)
{
    return GetClientRect(_hWnd, lpRect);
}

int Window::MapPoints(_Inout_updates_(cPoints) LPPOINT lpPoints, _In_ UINT cPoints)
{
    return MapWindowPoints(_hWnd, nullptr, lpPoints, cPoints);
}

BOOL Window::ConvertScreenToClient(_Inout_ LPPOINT lpPoint)
{
    return ScreenToClient(_hWnd, lpPoint);
}
