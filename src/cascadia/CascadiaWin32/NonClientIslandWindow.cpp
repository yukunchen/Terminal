#include "pch.h"
#include "NonClientIslandWindow.h"

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
    }
    case WM_ACTIVATE:
    {
        _HandleActivateWindow();
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
