#include "pch.h"
#include "IslandWindow.h"
#include <winrt/RuntimeComponent1.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

Windows::UI::Xaml::UIElement CreateDefaultContent()
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

    // [=] is important, don't use [&]. [&] will capture b's address on the stack, which will be garbage
    b.Click([=](auto, auto) {
        winrt::RuntimeComponent1::Class term;
        int a = term.MyProperty();
        auto s = winrt::to_hstring(a);
        Windows::UI::Xaml::Controls::TextBlock myTb{};
        myTb.Text(s);
        b.Content(myTb);
    });
    return container;
}



int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment(apartment_type::single_threaded);

    IslandWindow window;

    // IMPORTANT:
    // the manager and interop should NOT be in the window object.
    // If they are, it's destructor seems to close them incorrectly, and the
    //      app will crash on close.

    // Initialize the Xaml Hosting Manager
    auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    // Create the desktop source
    DesktopWindowXamlSource desktopSource;

    // IslandWindow::Initialize will get the xaml island hwnd and create the
    //      content that should be in the island.
    window.Initialize(desktopSource);

    // Actually create some xaml content, and place it in the island
    auto container = CreateDefaultContent();
    window.SetRootContent(container);

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}
