#include "pch.h"
#include "IslandWindow.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;

IslandWindow::IslandWindow() noexcept
{
    WNDCLASS wc{};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    wc.lpszClassName = L"XAML island in Win32";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    RegisterClass(&wc);
    WINRT_ASSERT(!m_window);

    WINRT_VERIFY(CreateWindow(wc.lpszClassName,
        L"XAML island in Win32",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, wc.hInstance, this));

    WINRT_ASSERT(m_window);

    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);
}

IslandWindow::~IslandWindow()
{
}

void IslandWindow::InitXamlContent()
{
    // setup a root grid that will be used to apply DPI scaling
    winrt::Windows::UI::Xaml::Media::ScaleTransform dpiScaleTransform;
    winrt::Windows::UI::Xaml::Controls::Grid dpiAdjustmentGrid;
    dpiAdjustmentGrid.RenderTransform(dpiScaleTransform);

    this->m_rootGrid = dpiAdjustmentGrid;
    this->m_scale = dpiScaleTransform;
}


void IslandWindow::OnSize()
{
    // update the interop window size
    SetWindowPos(m_interopWindowHandle, 0, 0, 0, m_currentWidth, m_currentHeight, SWP_SHOWWINDOW);
    m_rootGrid.Width(m_currentWidth);
    m_rootGrid.Height(m_currentHeight);
}

LRESULT IslandWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
{
    // TODO: handle messages here...
    return base_type::MessageHandler(message, wparam, lparam);
}

void IslandWindow::NewScale(UINT dpi) {

    auto scaleFactor = (float)dpi / 100;

    if (m_scale != nullptr)
    {
       m_scale.ScaleX(scaleFactor);
       m_scale.ScaleY(scaleFactor);
    }

    ApplyCorrection(scaleFactor);
}

void IslandWindow::ApplyCorrection(double scaleFactor) {
    double rightCorrection = (m_rootGrid.Width() * scaleFactor - m_rootGrid.Width()) / scaleFactor;
    double bottomCorrection = (m_rootGrid.Height() * scaleFactor - m_rootGrid.Height()) / scaleFactor;

    m_rootGrid.Padding(winrt::Windows::UI::Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));
}

void IslandWindow::DoResize(UINT width, UINT height) {
    m_currentWidth = width;
    m_currentHeight = height;
    if (nullptr != m_rootGrid)
    {
        OnSize();
        ApplyCorrection(m_scale.ScaleX());
    }
}

void IslandWindow::SetRootContent(winrt::Windows::UI::Xaml::UIElement content) {
    m_rootGrid.Children().Clear();
    m_rootGrid.Children().Append(content);
}
