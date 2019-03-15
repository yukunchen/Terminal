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
        //CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
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

void IslandWindow::NewScale(UINT dpi)
{
    const float fDpi = (float)(dpi);
    const float F_DEFAULT_DPI = (float)USER_DEFAULT_SCREEN_DPI;
    // auto scaleFactor = ((float)(dpi)) / ((float)USER_DEFAULT_SCREEN_DPI);
    // auto scaleFactor = dpi / F_DEFAULT_DPI;
    auto scaleFactor = F_DEFAULT_DPI / dpi;

    if (_scale != nullptr)
    {
       _scale.ScaleX(scaleFactor);
       _scale.ScaleY(scaleFactor);
    }

    ApplyCorrection(scaleFactor);
}

void IslandWindow::ApplyCorrection(double scaleFactor) {

    static double lastScale = 0.0;

    const auto realWidth = _rootGrid.Width();
    const auto realHeight = _rootGrid.Height();

    const auto dpiAwareWidth = realWidth / scaleFactor;
    const auto dpiAwareHeight = realHeight / scaleFactor;

    const auto deltaX = dpiAwareWidth - realWidth;
    const auto deltaY = dpiAwareHeight - realHeight;
    //double rightCorrection = (_rootGrid.Width() * scaleFactor - _rootGrid.Width()) / scaleFactor;
    //double bottomCorrection = (_rootGrid.Height() * scaleFactor - _rootGrid.Height()) / scaleFactor;

    const auto dividedDeltaX = deltaX / scaleFactor;
    const auto dividedDeltaY = deltaY / scaleFactor;

    const auto multipliedDeltaX = deltaX * scaleFactor;
    const auto multipliedDeltaY = deltaY * scaleFactor;

    double rightCorrection = dividedDeltaX;
    double bottomCorrection = dividedDeltaY;
    // double rightCorrection = multipliedDeltaX;
    // double bottomCorrection = multipliedDeltaY;

    // double rightCorrection = deltaX;
    // double bottomCorrection = deltaY;

    dividedDeltaX;
    dividedDeltaY;
    multipliedDeltaX;
    multipliedDeltaY;

    //_rootGrid.Padding(winrt::Windows::UI::Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));

    _rootGrid.Padding(Xaml::ThicknessHelper::FromLengths(0, 0, 0, 0));
    // _rootGrid.Padding(Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));
    // _rootGrid.Padding(Xaml::ThicknessHelper::FromLengths(rightCorrection, bottomCorrection, 0, 0));

    // _rootGrid.RenderTransform(_scale);

    lastScale = scaleFactor;
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
