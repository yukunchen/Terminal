#include "pch.h"
#include "SimpleIslandWindow.hpp"

extern "C" IMAGE_DOS_HEADER __ImageBase;

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

SimpleIslandWindow::SimpleIslandWindow() noexcept
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

    // m_manager = InitXaml(m_window, m_rootGrid, m_scale, m_xamlSource);
    // NewScale(m_currentDpi);

    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);
}

SimpleIslandWindow::~SimpleIslandWindow()
{
    //m_manager.Close();
}

// Windows::UI::Xaml::Hosting::WindowsXamlManager SimpleIslandWindow::InitXaml(HWND wind,
//                                                                       Windows::UI::Xaml::Controls::Grid & root,
//                                                                       Windows::UI::Xaml::Media::ScaleTransform & dpiScale,
//                                                                       DesktopWindowXamlSource & source)
// {


//     auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

//     // Create the desktop source
//     DesktopWindowXamlSource desktopSource;
//     auto interop = desktopSource.as<IDesktopWindowXamlSourceNative>();
//     check_hresult(interop->AttachToWindow(wind));

//     // stash the interop handle so we can resize it when the main hwnd is resized
//     HWND h = nullptr;
//     interop->get_WindowHandle(&h);
//     m_interopWindowHandle = h;

//     // setup a root grid that will be used to apply DPI scaling
//     Windows::UI::Xaml::Media::ScaleTransform dpiScaleTransform;
//     Windows::UI::Xaml::Controls::Grid dpiAdjustmentGrid;
//     dpiAdjustmentGrid.RenderTransform(dpiScaleTransform);
//     Windows::UI::Xaml::Media::SolidColorBrush background{ Windows::UI::Colors::White() };

//     // Set the content of the rootgrid to the DPI scaling grid
//     desktopSource.Content(dpiAdjustmentGrid);

//     // Update the window size, DPI layout correction
//     OnSize(h, dpiAdjustmentGrid, m_currentWidth, m_currentHeight);

//     // set out params
//     root = dpiAdjustmentGrid;
//     dpiScale = dpiScaleTransform;
//     source = desktopSource;
//     return manager;
// }

void SimpleIslandWindow::OnSize()
{
    // update the interop window size
    SetWindowPos(m_interopWindowHandle, 0, 0, 0, m_currentWidth, m_currentHeight, SWP_SHOWWINDOW);
    m_rootGrid.Width(m_currentWidth);
    m_rootGrid.Height(m_currentHeight);
}

LRESULT SimpleIslandWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
{
    // TODO: handle messages here...
    return base_type::MessageHandler(message, wparam, lparam);
}

void SimpleIslandWindow::NewScale(UINT dpi) {

    auto scaleFactor = (float)dpi / 100;

    // if (m_scale != nullptr) {
    //    m_scale.ScaleX(scaleFactor);
    //    m_scale.ScaleY(scaleFactor);
    // }

    ApplyCorrection(scaleFactor);
}

void SimpleIslandWindow::ApplyCorrection(double scaleFactor) {
    double rightCorrection = (m_rootGrid.Width() * scaleFactor - m_rootGrid.Width()) / scaleFactor;
    double bottomCorrection = (m_rootGrid.Height() * scaleFactor - m_rootGrid.Height()) / scaleFactor;

    m_rootGrid.Padding(Windows::UI::Xaml::ThicknessHelper::FromLengths(0, 0, rightCorrection, bottomCorrection));
}

void SimpleIslandWindow::DoResize(UINT width, UINT height) {
    m_currentWidth = width;
    m_currentHeight = height;
    if (nullptr != m_rootGrid) {
        OnSize();
        //ApplyCorrection(m_scale.ScaleX());
    }
}

void SimpleIslandWindow::SetRootContent(Windows::UI::Xaml::UIElement content) {
    m_rootGrid.Children().Clear();
    m_rootGrid.Children().Append(content);
}
