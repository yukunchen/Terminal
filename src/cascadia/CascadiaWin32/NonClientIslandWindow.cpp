#include "pch.h"
#include "NonClientIslandWindow.h"
#include <shellscalingapi.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;
using namespace ::Microsoft::Console::Types;

#define XAML_HOSTING_WINDOW_CLASS_NAME L"CASCADIA_HOSTING_WINDOW_CLASS"

#define RECT_WIDTH(x) ((x)->right - (x)->left)
#define RECT_HEIGHT(x) ((x)->bottom - (x)->top)

NonClientIslandWindow::NonClientIslandWindow() noexcept :
    IslandWindow{ false },
    _nonClientInteropWindowHandle{ nullptr },
    _nonClientRootGrid{ nullptr },
    _nonClientSource{ nullptr }
{
    _CreateWindow();
}

NonClientIslandWindow::~NonClientIslandWindow()
{
}

void NonClientIslandWindow::Initialize()
{
    _nonClientSource = DesktopWindowXamlSource{};
    auto interop = _nonClientSource.as<IDesktopWindowXamlSourceNative>();
    winrt::check_hresult(interop->AttachToWindow(_window));

    // stash the child interop handle so we can resize it when the main hwnd is resized
    HWND interopHwnd = nullptr;
    interop->get_WindowHandle(&interopHwnd);

    _nonClientInteropWindowHandle = interopHwnd;

    _nonClientRootGrid = winrt::Windows::UI::Xaml::Controls::Grid{};

    _nonClientSource.Content(_nonClientRootGrid);

    // Call the IslandWindow Initialize to set up the client xaml island
    IslandWindow::Initialize();
}

Viewport NonClientIslandWindow::GetTitlebarContentArea()
{
    const auto dpi = GetDpiForWindow(_window);
    const double scale = double(dpi) / double(USER_DEFAULT_SCREEN_DPI);

    const auto titlebarContentHeight = (_titlebarUnscaledContentHeight) * (scale);
    const auto titlebarMarginRight = (_titlebarUnscaledMarginRight) * (scale);

    auto titlebarWidth = _currentWidth - (_windowMarginSides + titlebarMarginRight);
    auto titlebarHeight = titlebarContentHeight - (_titlebarMarginTop + _titlebarMarginBottom);
    return Viewport::FromDimensions({ static_cast<short>(_windowMarginSides), static_cast<short>(_titlebarMarginTop) },
                                    { static_cast<short>(titlebarWidth), static_cast<short>(titlebarHeight) });
}

Viewport NonClientIslandWindow::GetClientContentArea()
{
    MARGINS margins = GetFrameMargins();

    auto clientWidth = _currentWidth - (margins.cxLeftWidth + margins.cxRightWidth);
    auto clientHeight = _currentHeight - (margins.cyTopHeight + margins.cyBottomHeight);

    return Viewport::FromDimensions({ static_cast<short>(margins.cxLeftWidth), static_cast<short>(margins.cyTopHeight) },
                                    { static_cast<short>(clientWidth), static_cast<short>(clientHeight) });
}

void NonClientIslandWindow::OnSize()
{
    auto clientArea = GetClientContentArea();
    auto titlebarArea = GetTitlebarContentArea();

    // update the interop window size
    SetWindowPos(_interopWindowHandle, 0,
                 clientArea.Left(),
                 clientArea.Top(),
                 clientArea.Width(),
                 clientArea.Height(),
                 SWP_SHOWWINDOW);

    _rootGrid.Width(clientArea.Width());
    _rootGrid.Height(clientArea.Height());

    // update the interop window size
    SetWindowPos(_nonClientInteropWindowHandle, 0,
                 titlebarArea.Left(),
                 titlebarArea.Top(),
                 titlebarArea.Width(),
                 titlebarArea.Height(),
                 SWP_SHOWWINDOW);
}

// Hit test the frame for resizing and moving.
LRESULT NonClientIslandWindow::HitTestNCA(POINT ptMouse)
{
    // Get the window rectangle.
    RECT rcWindow = BaseWindow::GetWindowRect();

    MARGINS margins = GetFrameMargins();

    // Get the frame rectangle, adjusted for the style without a caption.
    RECT rcFrame = { 0 };
    AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

    // Determine if the hit test is for resizing. Default middle (1,1).
    USHORT uRow = 1;
    USHORT uCol = 1;
    bool fOnResizeBorder = false;

    // Determine if the point is at the top or bottom of the window.
    if (ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + margins.cyTopHeight)
    {
        fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
        uRow = 0;
    }
    else if (ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - margins.cyBottomHeight)
    {
        uRow = 2;
    }

    // Determine if the point is at the left or right of the window.
    if (ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + margins.cxLeftWidth)
    {
        uCol = 0; // left side
    }
    else if (ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - margins.cxRightWidth)
    {
        uCol = 2; // right side
    }

    // Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
    LRESULT hitTests[3][3] =
    {
        { HTTOPLEFT,    fOnResizeBorder ? HTTOP : HTCAPTION,    HTTOPRIGHT },
        { HTLEFT,       HTNOWHERE,     HTRIGHT },
        { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
    };

    return hitTests[uRow][uCol];
}

MARGINS NonClientIslandWindow::GetFrameMargins()
{
    auto titlebarView = GetTitlebarContentArea();

    MARGINS margins{0};
    margins.cxLeftWidth = _windowMarginSides;
    margins.cxRightWidth = _windowMarginSides;
    margins.cyBottomHeight = _windowMarginBottom;
    margins.cyTopHeight = titlebarView.BottomInclusive();

    return margins;
}

HRESULT NonClientIslandWindow::_UpdateFrameMargins()
{
    // Extend the frame into the client area.
    MARGINS margins = GetFrameMargins();
    return DwmExtendFrameIntoClientArea(_window, &margins);
}

RECT NonClientIslandWindow::GetMaxWindowRectInPixels(const RECT * const prcSuggested, _Out_opt_ UINT * pDpiSuggested)
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
    if (!EqualRect(&rc, &rcZero))
    {
        // For invalid window handles or when we were passed a non-zero suggestion rectangle, get the monitor from the rect.
        hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        // Otherwise, get the monitor from the window handle.
        hMonitor = MonitorFromWindow(_window, MONITOR_DEFAULTTONEAREST);
    }

    // If for whatever reason there is no monitor, we're going to give back whatever we got since we can't figure anything out.
    // We won't adjust the DPI either. That's OK. DPI doesn't make much sense with no display.
    if (nullptr == hMonitor)
    {
        return rc;
    }

    // Now obtain the monitor pixel dimensions
    MONITORINFO MonitorInfo = { 0 };
    MonitorInfo.cbSize = sizeof(MONITORINFO);

    GetMonitorInfoW(hMonitor, &MonitorInfo);

    // We have to make a correction to the work area. If we actually consume the entire work area (by maximizing the window)
    // The window manager will render the borders off-screen.
    // We need to pad the work rectangle with the border dimensions to represent the actual max outer edges of the window rect.
    WINDOWINFO wi = { 0 };
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(_window, &wi);

    // In non-full screen, we want to only use the work area (avoiding the task bar space)
    rc = MonitorInfo.rcWork;
    // rc.top -= wi.cyWindowBorders;
    // rc.bottom += wi.cyWindowBorders;
    // rc.left -= wi.cxWindowBorders;
    // rc.right += wi.cxWindowBorders;

    if (pDpiSuggested != nullptr)
    {
        UINT monitorDpiX;
        UINT monitorDpiY;
        if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &monitorDpiX, &monitorDpiY)))
        {
            *pDpiSuggested = monitorDpiX;
        }
        else
        {
            *pDpiSuggested = GetDpiForWindow(_window);
        }
    }

    return rc;
}


LRESULT NonClientIslandWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
{
    LRESULT lRet = 0;

    // First call DwmDefWindowProc. This might handle things like the
    // min/max/close buttons for us.
    const bool dwmHandledMessage = DwmDefWindowProc(_window, message, wParam, lParam, &lRet);

    switch (message) {
    case WM_CREATE:
    {
        // Wher the window is first created, quick send a resize message to it.
        // This will force it to repaint with our updated margins.
        _HandleCreateWindow();
        break;
    }
    case WM_ACTIVATE:
    {
        _HandleActivateWindow();
        break;
    }
    case WM_NCCALCSIZE:
    {
        if (wParam == false)
        {
            return 0;
        }
        // Handle the non-client size message.
        if (wParam == TRUE && lParam)
        {
            // Calculate new NCCALCSIZE_PARAMS based on custom NCA inset.
            NCCALCSIZE_PARAMS *pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

            pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 0;
            pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
            pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 0;
            pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

            return 0;
        }
        break;
    }
    case WM_NCHITTEST:
    {
        if (dwmHandledMessage)
        {
            return lRet;
        }

        // Handle hit testing in the NCA if not handled by DwmDefWindowProc.
        if (lRet == 0)
        {
            lRet = HitTestNCA({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});

            if (lRet != HTNOWHERE)
            {
                return lRet;
            }
        }
        break;
    }

    // case WM_GETMINMAXINFO:
    // {
    //     MINMAXINFO* minMax = (MINMAXINFO*)lParam;
    //     if (minMax == nullptr) break;
    //     // minMax.ptMaxPosition.cx += 8;

    //     RECT rcSuggested;
    //     rcSuggested.left = minMax->ptMaxPosition.x;
    //     rcSuggested.top = minMax->ptMaxPosition.y;
    //     rcSuggested.right = minMax->ptMaxPosition.x + minMax->ptMaxSize.x;
    //     rcSuggested.bottom = minMax->ptMaxPosition.y + minMax->ptMaxSize.y;

    //     UINT dpiOfMaximum;
    //     RECT rcMaximum = GetMaxWindowRectInPixels(&rcSuggested, &dpiOfMaximum);

    //     const auto overhangX = std::min(0L, minMax->ptMaxPosition.x);
    //     const auto overhangY = std::min(0L, minMax->ptMaxPosition.y);
    //     //const auto overhangY = minMax->ptMaxPosition.y;
    //     const auto maxWidth = RECT_WIDTH(&rcMaximum);
    //     const auto maxHeight = RECT_HEIGHT(&rcMaximum);
    //     // minMax->ptMaxSize.x +=  2 * minMax->ptMaxPosition.x;

    //     minMax->ptMaxPosition.x = 0 - _windowMarginSides;
    //     minMax->ptMaxSize.x = maxWidth + (2 * (overhangX)) +(2*_windowMarginSides);

    //     minMax->ptMaxPosition.y = rcMaximum.top - overhangY - _titlebarMarginTop;
    //      //minMax->ptMaxPosition.y = rcMaximum.top + 32;
    //     // minMax->ptMaxSize.y = maxHeight + (2 * -7);
    //     minMax->ptMaxSize.y = 512;
    //     //minMax->ptMaxSize.y = maxHeight + (2 * (overhangY)) - (_titlebarMarginTop + _windowMarginBottom);
    //     //minMax->ptMaxSize.y = maxHeight + (2 * (overhangY)) - (_titlebarMarginTop);// +_windowMarginBottom);
    //     //minMax->ptMaxSize.y = maxHeight + (2 * (overhangY));// -(_titlebarMarginTop);// +_windowMarginBottom);

    //     return 0;
    // }

    case WM_WINDOWPOSCHANGING:
    {
        if (!lParam) break;
        // Enforce maximum size here instead of WM_GETMINMAXINFO.
        // If we return it in WM_GETMINMAXINFO, then it will be enforced when snapping across DPI boundaries (bad.)

        // Retrieve the suggested dimensions and make a rect and size.
        LPWINDOWPOS lpwpos = (LPWINDOWPOS)lParam;

        // We only need to apply restrictions if the size is changing.
        if (WI_IsFlagSet(lpwpos->flags, SWP_NOSIZE))
        {
            break;
        }

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

        auto windowStyle = GetWindowStyle(_window);
        auto isMaximized = WI_IsFlagSet(windowStyle, WS_MAXIMIZE);
        // If the window is maximized, let it do whatever it wants to do.
        // If not, then restrict it to our maximum possible window.
        // if (!isMaximized)
        // if (true)
        if (isMaximized)
        {
            // Find the related monitor, the maximum pixel size,
            // and the dpi for the suggested rect.
            UINT dpiOfMaximum;
            RECT rcMaximum;

            if (fIsEdgeResize)
            {
                // If someone's dragging from the edge to resize in one direction, we want to make sure we never grow past the current monitor.
                // rcMaximum = ServiceLocator::LocateWindowMetrics<WindowMetrics>()->GetMaxWindowRectInPixels(&rcCurrent, &dpiOfMaximum);
                rcMaximum = GetMaxWindowRectInPixels(&rcCurrent, &dpiOfMaximum);
            }
            else
            {
                // In other circumstances, assume we're snapping around or some other jump (TS).
                // Just do whatever we're told using the new suggestion as the restriction monitor.
                rcMaximum = GetMaxWindowRectInPixels(&rcSuggested, &dpiOfMaximum);
            }

            const auto suggestedWidth = szSuggested.cx;
            const auto suggestedHeight = szSuggested.cy;

            const auto maxWidth = RECT_WIDTH(&rcMaximum);
            const auto maxHeight = RECT_HEIGHT(&rcMaximum);

            // Only apply the maximum size restriction if the current DPI matches the DPI of the
            // maximum rect. This keeps us from applying the wrong restriction if the monitor
            // we're moving to has a different DPI but we've yet to get notified of that DPI
            // change. If we do apply it, then we'll restrict the console window BEFORE its
            // been resized for the DPI change, so we're likely to shrink the window too much
            // or worse yet, keep it from moving entirely. We'll get a WM_DPICHANGED,
            // resize the window, and then process the restriction in a few window messages.
            if ( ((int)dpiOfMaximum == _currentDpi) &&
                 ( (suggestedWidth > maxWidth) ||
                   (szSuggested.cy > maxHeight) ) )
            {
                const auto offsetX = lpwpos->x;
                const auto offsetY = lpwpos->y;

                //lpwpos->cx = std::min(maxWidth, suggestedWidth);
                lpwpos->cx = std::min(maxWidth, suggestedWidth);
                lpwpos->x = rcMaximum.left;

                //lpwpos->cy = 512;
                //lpwpos->y = 64;
                //lpwpos->cy = std::min(maxHeight, suggestedHeight) - (_windowMarginBottom + _titlebarMarginTop);
                //lpwpos->cy = std::min(maxHeight, suggestedHeight) - (_windowMarginBottom);
                //lpwpos->cy = std::min(maxHeight, suggestedHeight);// -(_windowMarginBottom);
                //lpwpos->cy = std::min(maxHeight, suggestedHeight) - (1);

                // If you make this window so much as 1px taller, the call will fail, and default to the ~8px cut off on each side state
                lpwpos->cy = std::min(maxHeight, suggestedHeight) - (1);
                lpwpos->y = rcMaximum.top - _titlebarMarginTop - 2;

                // We usually add SWP_NOMOVE so that if the user is dragging the left or top edge
                // and hits the restriction, then the window just stops growing, it doesn't
                // move with the mouse. However during DPI changes, we need to allow a move
                // because the RECT from WM_DPICHANGED has been specially crafted by win32k
                // to keep the mouse cursor from jumping away from the caption bar.
                if (!_inDpiChange)
                {
                    // lpwpos->flags |= SWP_NOMOVE;
                }
            }
        }

        return 0;
    }

    }

    return IslandWindow::MessageHandler(message, wParam, lParam);
}

void NonClientIslandWindow::SetNonClientContent(winrt::Windows::UI::Xaml::UIElement content)
{
    _nonClientRootGrid.Children().Clear();
    ApplyCorrection(_scale.ScaleX());
    _nonClientRootGrid.Children().Append(content);
}

void NonClientIslandWindow::_HandleCreateWindow()
{
    RECT rcClient;
    ::GetWindowRect(_window, &rcClient);

    // Inform application of the frame change.
    SetWindowPos(_window,
                 NULL,
                 rcClient.left, rcClient.top,
                 RECT_WIDTH(&rcClient), RECT_HEIGHT(&rcClient),
                 SWP_FRAMECHANGED);
}

void NonClientIslandWindow::_HandleActivateWindow()
{
    const auto dpi = GetDpiForWindow(_window);
    const int captionButtonWidth = GetSystemMetricsForDpi(SM_CXSIZE, dpi);
    const int captionButtonHeight = GetSystemMetricsForDpi(SM_CYMENUSIZE, dpi);

    // Magic multipliers to give you just about the size you want
    _titlebarUnscaledMarginRight = (3 * captionButtonWidth);

    // The tabs are just about 36px tall (unscaled). If we change the size of
    // those, we'll need to change the size here, too.
    _titlebarUnscaledContentHeight = 36;

    _UpdateFrameMargins();
}
