#include "pch.h"
#include "IslandWindow.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;

#define XAML_HOSTING_WINDOW_CLASS_NAME L"CASCADIA_HOSTING_WINDOW_CLASS"


IslandWindow::IslandWindow() noexcept :
    _currentWidth{ 600 }, // These don't seem to affect the initial window size
    _currentHeight{ 600 }, // These don't seem to affect the initial window size
    _interopWindowHandle{ nullptr },
    _scale{ nullptr },
    _rootGrid{ nullptr },
    _source{ nullptr }
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
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, wc.hInstance, this));

    WINRT_ASSERT(_window);

    ShowWindow(_window, SW_SHOW);
    UpdateWindow(_window);
}

IslandWindow::~IslandWindow()
{
}

void IslandWindow::Initialize()
{
    const bool initialized = (_interopWindowHandle != nullptr);

    _source = DesktopWindowXamlSource{};

    auto interop = _source.as<IDesktopWindowXamlSourceNative>();
    winrt::check_hresult(interop->AttachToWindow(_window));

    // stash the child interop handle so we can resize it when the main hwnd is resized
    HWND interopHwnd = nullptr;
    interop->get_WindowHandle(&interopHwnd);

    _interopWindowHandle = interopHwnd;
    if (!initialized)
    {
        _InitXamlContent();
    }

    _source.Content(_rootGrid);

    // Do a quick resize to force the island to paint
    OnSize();
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


void IslandWindow::OnSize()
{
    // update the interop window size
    SetWindowPos(_interopWindowHandle, 0, 0, 0, _currentWidth, _currentHeight, SWP_SHOWWINDOW);
    _rootGrid.Width(_currentWidth);
    _rootGrid.Height(_currentHeight);
}

LRESULT IslandWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
{
    switch (message) {
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
    return base_type::MessageHandler(message, wparam, lparam);
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
}
