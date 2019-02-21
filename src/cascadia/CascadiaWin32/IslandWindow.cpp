#include "pch.h"
#include "IslandWindow.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;

#define XAML_HOSTING_WINDOW_CLASS_NAME L"XAML_HOSTING_WINDOW_CLASS"

IslandWindow::IslandWindow() noexcept :
    _currentWidth{ 600 }, // These don't seem to affect the initial window size
    _currentHeight{ 600 }, // These don't seem to affect the initial window size
    _interopWindowHandle{ nullptr },
    _scale{ nullptr },
    _rootGrid{ nullptr }
{
    WNDCLASS wc{};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    wc.lpszClassName = XAML_HOSTING_WINDOW_CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    RegisterClass(&wc);
    WINRT_ASSERT(!_window);

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

void IslandWindow::_InitXamlContent()
{
    // setup a root grid that will be used to apply DPI scaling
    winrt::Windows::UI::Xaml::Media::ScaleTransform dpiScaleTransform;
    winrt::Windows::UI::Xaml::Controls::Grid dpiAdjustmentGrid;
    dpiAdjustmentGrid.RenderTransform(dpiScaleTransform);

    this->_rootGrid = dpiAdjustmentGrid;
    this->_scale = dpiScaleTransform;
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
    // TODO: handle messages here...
    return base_type::MessageHandler(message, wparam, lparam);
}

void IslandWindow::NewScale(UINT dpi) {

    auto scaleFactor = (float)dpi / 100;

    if (_scale != nullptr)
    {
       _scale.ScaleX(scaleFactor);
       _scale.ScaleY(scaleFactor);
    }

    ApplyCorrection(scaleFactor);
}

void IslandWindow::ApplyCorrection(double scaleFactor) {
    double rightCorrection = (_rootGrid.Width() * scaleFactor - _rootGrid.Width()) / scaleFactor;
    double bottomCorrection = (_rootGrid.Height() * scaleFactor - _rootGrid.Height()) / scaleFactor;

    _rootGrid.Padding(winrt::Windows::UI::Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));
}

void IslandWindow::DoResize(UINT width, UINT height) {
    _currentWidth = width;
    _currentHeight = height;
    if (nullptr != _rootGrid)
    {
        OnSize();
        ApplyCorrection(_scale.ScaleX());
    }
}

void IslandWindow::SetRootContent(winrt::Windows::UI::Xaml::UIElement content) {
    _rootGrid.Children().Clear();
    _rootGrid.Children().Append(content);
}
