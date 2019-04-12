#include "pch.h"
#include "IslandWindow.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;
using namespace ::Microsoft::Console::Types;

#define XAML_HOSTING_WINDOW_CLASS_NAME L"CASCADIA_HOSTING_WINDOW_CLASS"
#define RECTWIDTH(r) (r.right - r.left)
#define RECTHEIGHT(r) (r.bottom - r.top)

// const static int LEFTEXTENDWIDTH = 2;
// const static int RIGHTEXTENDWIDTH = 2;
// const static int BOTTOMEXTENDWIDTH = 2;
// const static int TOPEXTENDWIDTH = 128;

IslandWindow::IslandWindow() noexcept :
    _currentWidth{ 600 }, // These don't seem to affect the initial window size
    _currentHeight{ 600 }, // These don't seem to affect the initial window size
    _interopWindowHandle{ nullptr },
    _nonClientInteropWindowHandle{ nullptr },
    _scale{ nullptr },
    _rootGrid{ nullptr },
    _nonClientRootGrid{ nullptr }
{
    WNDCLASS wc{};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    wc.lpszClassName = XAML_HOSTING_WINDOW_CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    RegisterClass(&wc);
    WINRT_ASSERT(!_window);

    // TODO: MSFT:20817473 - load the settings first to figure out how big the
    //      window should be. Using the font and the initial size settings of
    //      the default profile, adjust how big the created window should be,
    //      or continue using the default values from the system
    WINRT_VERIFY(CreateWindow(wc.lpszClassName,
        L"Project Cascadia",
        WS_OVERLAPPEDWINDOW,// | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, wc.hInstance, this));

    WINRT_ASSERT(_window);

    ShowWindow(_window, SW_HIDE);
    UpdateWindow(_window);
}

IslandWindow::~IslandWindow()
{
}

void IslandWindow::Initialize(DesktopWindowXamlSource source)
{
    const bool initialized = (_interopWindowHandle != nullptr);

    auto interop = source.as<IDesktopWindowXamlSourceNative>();
    winrt::check_hresult(interop->AttachToWindow(_window));

    // stash the child interop handle so we can resize it when the main hwnd is resized
    HWND interopHwnd = nullptr;
    interop->get_WindowHandle(&interopHwnd);

    _interopWindowHandle = interopHwnd;
    if (!initialized)
    {
        _InitXamlContent();
    }

    source.Content(_rootGrid);

    // Do a quick resize to force the island to paint
    OnSize();
}

void IslandWindow::InitializeNonClient(DesktopWindowXamlSource source)
{
    const bool initialized = (_interopWindowHandle != nullptr);

    auto interop = source.as<IDesktopWindowXamlSourceNative>();
    winrt::check_hresult(interop->AttachToWindow(_window));

    // stash the child interop handle so we can resize it when the main hwnd is resized
    HWND interopHwnd = nullptr;
    interop->get_WindowHandle(&interopHwnd);

    _nonClientInteropWindowHandle = interopHwnd;
    // if (!initialized)
    // {
    //     _InitXamlContent();
    // }

    _nonClientRootGrid = winrt::Windows::UI::Xaml::Controls::Grid{};

    // winrt::Windows::UI::Xaml::Controls::Button btn{};
    // auto foo = winrt::box_value({ L"Foo" });
    // btn.Content(foo);

    // _nonClientRootGrid.Children().Append(btn);
    source.Content(_nonClientRootGrid);

    // // Do a quick resize to force the island to paint
    // OnSize();
}

void IslandWindow::_InitXamlContent()
{
    // setup a root grid that will be used to apply DPI scaling
    winrt::Windows::UI::Xaml::Media::ScaleTransform dpiScaleTransform;
    winrt::Windows::UI::Xaml::Controls::Grid dpiAdjustmentGrid;

    const auto dpi = GetDpiForWindow(_window);
    const double scale = double(dpi) / double(USER_DEFAULT_SCREEN_DPI);

    _rootGrid = dpiAdjustmentGrid;
    _scale = dpiScaleTransform;

    _scale.ScaleX(scale);
    _scale.ScaleY(scale);
}

Viewport IslandWindow::GetTitlebarContentArea()
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

Viewport IslandWindow::GetClientContentArea()
{
    MARGINS margins = GetFrameMargins();

    auto clientWidth = _currentWidth - (margins.cxLeftWidth + margins.cxRightWidth);
    auto clientHeight = _currentHeight - (margins.cyTopHeight + margins.cyBottomHeight);

    return Viewport::FromDimensions({ static_cast<short>(margins.cxLeftWidth), static_cast<short>(margins.cyTopHeight) },
                                    { static_cast<short>(clientWidth), static_cast<short>(clientHeight) });
}

void IslandWindow::OnSize()
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
LRESULT IslandWindow::HitTestNCA(POINT ptMouse)
{
    // Get the window rectangle.
    RECT rcWindow;
    GetWindowRect(_window, &rcWindow);

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

MARGINS IslandWindow::GetFrameMargins()
{
    auto titlebarView = GetTitlebarContentArea();

    MARGINS margins{0};
    margins.cxLeftWidth = _windowMarginSides;
    margins.cxRightWidth = _windowMarginSides;
    margins.cyBottomHeight = _windowMarginBottom;
    margins.cyTopHeight = titlebarView.BottomInclusive();

    return margins;
}

HRESULT IslandWindow::_UpdateFrameMargins()
{
    // Extend the frame into the client area.
    MARGINS margins = GetFrameMargins();

    HRESULT hr = DwmExtendFrameIntoClientArea(_window, &margins);

    if (!SUCCEEDED(hr))
    {
        // Handle the error.
    }
    return hr;
}

LRESULT IslandWindow::MessageHandler(UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
{

    LRESULT lRet = 0;
    HRESULT hr = S_OK;
    bool fCallDWP = true; // Pass on to DefWindowProc?

    const bool dwmHandledMessage = DwmDefWindowProc(_window, message, wParam, lParam, &lRet);
    fCallDWP = !dwmHandledMessage;

    switch (message) {
    case WM_CREATE:
    {
        RECT rcClient;
        GetWindowRect(_window, &rcClient);

        // Inform application of the frame change.
        SetWindowPos(_window,
                     NULL,
                     rcClient.left, rcClient.top,
                     RECTWIDTH(rcClient), RECTHEIGHT(rcClient),
                     SWP_FRAMECHANGED);

        fCallDWP = true;
        lRet = 0;
    }
    // Handle the window activation.
    case WM_ACTIVATE:
    {
        // // Set up our dimensions:
        // const int captionButtonWidth = GetSystemMetrics(SM_CXSIZE);
        // // SM_CYSIZE will give you JUST THE BUTTON height. Typically there's a decent amount of padding additionally.
        // // const int captionButtonHeight = GetSystemMetrics(SM_CYSIZE);
        // // SM_CYMENUSIZE gives you seemingly _less_ space...
        // const int captionButtonHeight = GetSystemMetrics(SM_CYMENUSIZE);

        const auto dpi = GetDpiForWindow(_window);
        const int captionButtonWidth = GetSystemMetricsForDpi(SM_CXSIZE, dpi);
        const int captionButtonHeight = GetSystemMetricsForDpi(SM_CYMENUSIZE, dpi);

        // Magic multipliers to give you just about the size you want
        _titlebarUnscaledMarginRight = (3 * captionButtonWidth);

        // 72 just so happens to be (2 * captionButtonHeight - 4)
        // Is magic adjustment better than a magic constant?
        // _titlebarContentHeight = 2 * captionButtonHeight - 4;
        // _titlebarContentHeight = 72;
        _titlebarUnscaledContentHeight = 36;

        _UpdateFrameMargins();
        // fCallDWP = true;
        // lRet = 0;
    }
    case WM_NCCALCSIZE:
    {
        // Handle the non-client size message.
        if (wParam == TRUE && lParam)
        {
            // Calculate new NCCALCSIZE_PARAMS based on custom NCA inset.
            NCCALCSIZE_PARAMS *pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

            pncsp->rgrc[0].left   = pncsp->rgrc[0].left   + 0;
            pncsp->rgrc[0].top    = pncsp->rgrc[0].top    + 0;
            pncsp->rgrc[0].right  = pncsp->rgrc[0].right  - 0;
            pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

            // lRet = 0;
            // // No need to pass the message on to the DefWindowProc.
            // fCallDWP = false;
            return 0;
        }
    }
    case WM_NCHITTEST:
    {
        if (dwmHandledMessage)
            return lRet;

        // Handle hit testing in the NCA if not handled by DwmDefWindowProc.
        if (lRet == 0)
        {
            lRet = HitTestNCA({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});

            if (lRet != HTNOWHERE)
            {
                // fCallDWP = false;
                return lRet;
            }
        }
    }



    case WM_SETFOCUS:
    {
        if (_interopWindowHandle != nullptr)
        {
            // send focus to the child window
            SetFocus(_interopWindowHandle);
            return 0; // eat the message
        }
    }
    }

    // TODO: handle messages here...
    return base_type::MessageHandler(message, wParam, lParam);
}

// Method Description:
// - Called when the DPI of this window changes. Updates the XAML content sizing to match the client area of our window.
// Arguments:
// - dpi: new DPI to use. The default is 96, as defined by USER_DEFAULT_SCREEN_DPI.
// Return Value:
// - <none>
void IslandWindow::NewScale(UINT dpi)
{
    const double scaleFactor = static_cast<double>(dpi) / static_cast<double>(USER_DEFAULT_SCREEN_DPI);

    _UpdateFrameMargins();

    if (_scale != nullptr)
    {
       _scale.ScaleX(scaleFactor);
       _scale.ScaleY(scaleFactor);
    }

    ApplyCorrection(scaleFactor);
}

// Method Description:
// - This method updates the padding that exists off the edge of the window to
//      make sure to keep the XAML content size the same as the actual window size.
// Arguments:
// - scaleFactor: the DPI scaling multiplier to use. for a dpi of 96, this would
//      be 1, for 144, this would be 1.5.
// Return Value:
// - <none>
void IslandWindow::ApplyCorrection(double scaleFactor)
{
    // Get the dimensions of the XAML content grid.
    const auto realWidth = _rootGrid.Width();
    const auto realHeight = _rootGrid.Height();

    // Scale those dimensions by our dpi scaling. This is how big the XAML
    //      content thinks it should be.
    const auto dpiAwareWidth = realWidth * scaleFactor;
    const auto dpiAwareHeight = realHeight * scaleFactor;

    // Get the difference between what xaml thinks and the actual client area
    //      of our window.
    const auto deltaX = dpiAwareWidth - realWidth;
    const auto deltaY = dpiAwareHeight - realHeight;

    // correct for the scaling we applied above
    const auto dividedDeltaX = deltaX / scaleFactor;
    const auto dividedDeltaY = deltaY / scaleFactor;

    const double rightCorrection = dividedDeltaX;
    const double bottomCorrection = dividedDeltaY;

    // Apply padding to the root grid, so that it's content is the same size as
    //      our actual window size.
    // Without this, XAML content will seem to spill off the side/bottom of the window
    _rootGrid.Padding(Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));

}

void IslandWindow::DoResize(UINT width, UINT height)
{
    _currentWidth = width;
    _currentHeight = height;
    if (nullptr != _rootGrid)
    {
        OnSize();
        ApplyCorrection(_scale.ScaleX());
    }
}

void IslandWindow::SetRootContent(winrt::Windows::UI::Xaml::UIElement content)
{
    _rootGrid.Children().Clear();
    ApplyCorrection(_scale.ScaleX());
    _rootGrid.Children().Append(content);

        ShowWindow(_window, SW_SHOW);
}


void IslandWindow::SetNonClientContent(winrt::Windows::UI::Xaml::UIElement content)
{
    _nonClientRootGrid.Children().Clear();
    ApplyCorrection(_scale.ScaleX());
    _nonClientRootGrid.Children().Append(content);
}
