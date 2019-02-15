#include "pch.h"
#include "IslandWindow.h"
#include "SimpleIslandWindow.hpp"
#include <winrt/RuntimeComponent1.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

#define CLARKZONES_SAMPLE 0
#define MIGUELS_SAMPLE 1
#define COMBO_SAMPLE 2

#define SAMPLE_TYPE COMBO_SAMPLE


#if SAMPLE_TYPE == CLARKZONES_SAMPLE
// Not miguel's sample:

Windows::UI::Xaml::UIElement CreateDefaultContent() {
    Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
    acrylicBrush.BackgroundSource(Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
    acrylicBrush.TintOpacity(0.5);
    acrylicBrush.TintColor(Windows::UI::Colors::Red());
    acrylicBrush.FallbackColor(Windows::UI::Colors::Magenta());

    Windows::UI::Xaml::Controls::Grid container;
    container.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(100, 100, 100, 100));
    container.Background(acrylicBrush);

    Windows::UI::Xaml::Controls::Button b;
    b.Width(600);
    b.Height(60);
    b.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(Windows::UI::Xaml::VerticalAlignment::Center));

    b.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(Windows::UI::Xaml::HorizontalAlignment::Center));
    b.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });

    Windows::UI::Xaml::Controls::TextBlock tb;
    tb.Text(L"Hello Win32 love XAML and C++/WinRT xx");
    b.Content(tb);
    tb.FontSize(30.0f);
    container.Children().Append(b);

    Windows::UI::Xaml::Controls::TextBlock dpitb;
    dpitb.Text(L"(p.s. high DPI just got much easier for win32 devs)");
    dpitb.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });
    dpitb.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(10, 10, 10, 10));
    dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(Windows::UI::Xaml::VerticalAlignment::Bottom));

    dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(Windows::UI::Xaml::HorizontalAlignment::Right));
    container.Children().Append(dpitb);

    //winrt::RuntimeComponent1::Class term;
    //int a = term.MyProperty();
    //a;
    // [=] is important, don't use [&]. [&] will capture b's address on the stack, which will be garbage
    b.Click([=](auto, auto) {
        winrt::RuntimeComponent1::Class term;
        int a = term.MyProperty();
        auto s = winrt::to_hstring(a);
        Windows::UI::Xaml::Controls::TextBlock myTb{};
        myTb.Text(s);
        b.Content(myTb);
    });

    //container.Children().Append(term);

    return container;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
// int __stdcall main(int, char*)
{
    init_apartment(apartment_type::single_threaded);

    IslandWindow window;

    {
        auto defaultContent = CreateDefaultContent();
        window.SetRootContent(defaultContent);
    }

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}

#elif SAMPLE_TYPE == MIGUELS_SAMPLE

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

HWND _hWnd;
HWND _childhWnd;
HINSTANCE _hInstance;


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    _hInstance = hInstance;

    // The main window class name.
    const wchar_t szWindowClass[] = L"Win32DesktopApp";
    WNDCLASSEX windowClass = { };

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = szWindowClass;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    windowClass.hIconSm = LoadIcon(windowClass.hInstance, IDI_APPLICATION);

    if (RegisterClassEx(&windowClass) == NULL)
    {
        MessageBox(NULL, L"Windows registration failed!", L"Error", NULL);
        return 0;
    }

    _hWnd = CreateWindow(
        szWindowClass,
        L"Windows c++ Win32 Desktop App",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    if (_hWnd == NULL)
    {
        MessageBox(NULL, L"Call to CreateWindow failed!", L"Error", NULL);
        return 0;
    }


    //XAML Island section

    // The call to winrt::init_apartment initializes COM; by default, in a multithreaded apartment.
    winrt::init_apartment(apartment_type::single_threaded);

    // Initialize the Xaml Framework's corewindow for current thread
    WindowsXamlManager winxamlmanager = WindowsXamlManager::InitializeForCurrentThread();

    // This DesktopWindowXamlSource is the object that enables a non-UWP desktop application
    // to host UWP controls in any UI element that is associated with a window handle (HWND).
    DesktopWindowXamlSource desktopSource;
    // Get handle to corewindow
    auto interop = desktopSource.as<IDesktopWindowXamlSourceNative>();
    // Parent the DesktopWindowXamlSource object to current window
    check_hresult(interop->AttachToWindow(_hWnd));

    // This Hwnd will be the window handler for the Xaml Island: A child window that contains Xaml.
    HWND hWndXamlIsland = nullptr;
    // Get the new child window's hwnd
    interop->get_WindowHandle(&hWndXamlIsland);
    // Update the xaml island window size becuase initially is 0,0
    SetWindowPos(hWndXamlIsland, 0, 200, 100, 800, 200, SWP_SHOWWINDOW);

    //Creating the Xaml content
    Windows::UI::Xaml::Controls::StackPanel xamlContainer;
    xamlContainer.Background(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::LightGray() });

    Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
    acrylicBrush.BackgroundSource(Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
    acrylicBrush.TintOpacity(0.5);
    acrylicBrush.TintColor(Windows::UI::Colors::Red());
    acrylicBrush.FallbackColor(Windows::UI::Colors::Magenta());

    xamlContainer.Background(acrylicBrush);

    Windows::UI::Xaml::Controls::TextBlock tb;
    tb.Text(L"Hello World from Xaml Islands!");
    tb.VerticalAlignment(Windows::UI::Xaml::VerticalAlignment::Center);
    tb.HorizontalAlignment(Windows::UI::Xaml::HorizontalAlignment::Center);
    tb.FontSize(48);

    xamlContainer.Children().Append(tb);
    xamlContainer.UpdateLayout();
    desktopSource.Content(xamlContainer);

    //End XAML Island section

    ShowWindow(_hWnd, nCmdShow);
    UpdateWindow(_hWnd);

    //Message loop:
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT messageCode, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    wchar_t greeting[] = L"Hello World in Win32!";
    RECT rcClient;

    switch (messageCode)
    {
    case WM_PAINT:
        if (hWnd == _hWnd)
        {
            hdc = BeginPaint(hWnd, &ps);
            TextOut(hdc, 300, 5, greeting, wcslen(greeting));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        //creating main window
    case WM_CREATE:
        _childhWnd = CreateWindowEx(0, L"ChildWClass", NULL, WS_CHILD | WS_BORDER, 0, 0, 0, 0, hWnd, NULL, _hInstance, NULL);
        return 0;
        // main window changed size
    case WM_SIZE:
        // Get the dimensions of the main window's client
        // area, and enumerate the child windows. Pass the
        // dimensions to the child windows during enumeration.
        GetClientRect(hWnd, &rcClient);
        MoveWindow(_childhWnd, 200, 200, 400, 500, TRUE);
        ShowWindow(_childhWnd, SW_SHOW);
        return 0;
        // Process other messages.
    default:
        return DefWindowProc(hWnd, messageCode, wParam, lParam);
        break;
    }

    return 0;
}

#else

Windows::UI::Xaml::UIElement CreateDefaultContent() {
    Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
    acrylicBrush.BackgroundSource(Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
    acrylicBrush.TintOpacity(0.5);
    acrylicBrush.TintColor(Windows::UI::Colors::Red());
    acrylicBrush.FallbackColor(Windows::UI::Colors::Magenta());

    Windows::UI::Xaml::Controls::Grid container;
    container.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(100, 100, 100, 100));
    container.Background(acrylicBrush);

    Windows::UI::Xaml::Controls::Button b;
    b.Width(600);
    b.Height(60);
    b.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(Windows::UI::Xaml::VerticalAlignment::Center));

    b.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(Windows::UI::Xaml::HorizontalAlignment::Center));
    b.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });

    Windows::UI::Xaml::Controls::TextBlock tb;
    tb.Text(L"Hello Win32 love XAML and C++/WinRT xx");
    b.Content(tb);
    tb.FontSize(30.0f);
    container.Children().Append(b);

    Windows::UI::Xaml::Controls::TextBlock dpitb;
    dpitb.Text(L"(p.s. high DPI just got much easier for win32 devs)");
    dpitb.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });
    dpitb.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(10, 10, 10, 10));
    dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
        box_value(Windows::UI::Xaml::VerticalAlignment::Bottom));

    dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
        box_value(Windows::UI::Xaml::HorizontalAlignment::Right));
    container.Children().Append(dpitb);

    //winrt::RuntimeComponent1::Class term;
    //int a = term.MyProperty();
    //a;
    // [=] is important, don't use [&]. [&] will capture b's address on the stack, which will be garbage
    b.Click([=](auto, auto) {
        winrt::RuntimeComponent1::Class term;
        int a = term.MyProperty();
        auto s = winrt::to_hstring(a);
        Windows::UI::Xaml::Controls::TextBlock myTb{};
        myTb.Text(s);
        b.Content(myTb);
    });

    //container.Children().Append(term);

    return container;
}
int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
// int __stdcall main(int, char*)
{
    init_apartment(apartment_type::single_threaded);

    SimpleIslandWindow window;

    auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    // Create the desktop source
    DesktopWindowXamlSource desktopSource;
    auto interop = desktopSource.as<IDesktopWindowXamlSourceNative>();
    check_hresult(interop->AttachToWindow(window.m_window));

    // stash the interop handle so we can resize it when the main hwnd is resized
    HWND h = nullptr;
    interop->get_WindowHandle(&h);
    window.m_interopWindowHandle = h;

    // setup a root grid that will be used to apply DPI scaling
    // Windows::UI::Xaml::Media::ScaleTransform dpiScaleTransform;
    Windows::UI::Xaml::Controls::Grid dpiAdjustmentGrid;
    // dpiAdjustmentGrid.RenderTransform(dpiScaleTransform);
    // Windows::UI::Xaml::Media::SolidColorBrush background{ Windows::UI::Colors::White() };

    // Set the content of the rootgrid to the DPI scaling grid
    desktopSource.Content(dpiAdjustmentGrid);

    // // Update the window size, DPI layout correction
    // OnSize(h, dpiAdjustmentGrid, m_currentWidth, m_currentHeight);

    // set out params
    window.m_rootGrid = dpiAdjustmentGrid;
    // dpiScale = dpiScaleTransform;
    // source = desktopSource;
    // return manager;

    {
        Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
        acrylicBrush.BackgroundSource(Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
        acrylicBrush.TintOpacity(0.5);
        acrylicBrush.TintColor(Windows::UI::Colors::Blue());
        acrylicBrush.FallbackColor(Windows::UI::Colors::Cyan());

        Windows::UI::Xaml::Controls::Grid container;
        container.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(100, 100, 100, 100));
        container.Background(acrylicBrush);

        Windows::UI::Xaml::Controls::Button b;
        b.Width(600);
        b.Height(60);
        b.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
            box_value(Windows::UI::Xaml::VerticalAlignment::Center));

        b.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
            box_value(Windows::UI::Xaml::HorizontalAlignment::Center));
        b.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });

        Windows::UI::Xaml::Controls::TextBlock tb;
        tb.Text(L"Hello Win32 love XAML and C++/WinRT xx");
        b.Content(tb);
        tb.FontSize(30.0f);
        container.Children().Append(b);

        Windows::UI::Xaml::Controls::TextBlock dpitb;
        dpitb.Text(L"(p.s. high DPI just got much easier for win32 devs)");
        dpitb.Foreground(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::White() });
        dpitb.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(10, 10, 10, 10));
        dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::VerticalAlignmentProperty(),
            box_value(Windows::UI::Xaml::VerticalAlignment::Bottom));

        dpitb.SetValue(Windows::UI::Xaml::FrameworkElement::HorizontalAlignmentProperty(),
            box_value(Windows::UI::Xaml::HorizontalAlignment::Right));
        container.Children().Append(dpitb);

        //winrt::RuntimeComponent1::Class term;
        //int a = term.MyProperty();
        //a;
        // [=] is important, don't use [&]. [&] will capture b's address on the stack, which will be garbage
        b.Click([=](auto, auto) {
            winrt::RuntimeComponent1::Class term;
            int a = term.MyProperty();
            auto s = winrt::to_hstring(a);
            Windows::UI::Xaml::Controls::TextBlock myTb{};
            myTb.Text(s);
            b.Content(myTb);
        });

        //container.Children().Append(term);

        // return container;
        window.SetRootContent(container);
    }

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}

#endif
