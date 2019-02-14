#include "precomp.h"
#include "IslandWindow.h"
#include <winrt/RuntimeComponent1.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;


// LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

// HWND _hWnd;
// HWND _childhWnd;
// HINSTANCE _hInstance;


Windows::UI::Xaml::UIElement CreateDefaultContent() {


    Windows::UI::Xaml::Media::AcrylicBrush acrylicBrush;
    acrylicBrush.BackgroundSource(Windows::UI::Xaml::Media::AcrylicBackgroundSource::HostBackdrop);
    acrylicBrush.TintOpacity(0.5);
    acrylicBrush.TintColor(Windows::UI::Colors::Red());
    acrylicBrush.FallbackColor(Windows::UI::Colors::Magenta());

    Windows::UI::Xaml::Controls::Grid container;
    container.Margin(Windows::UI::Xaml::ThicknessHelper::FromLengths(100, 100, 100, 100));
    /*container.Background(Windows::UI::Xaml::Media::SolidColorBrush{ Windows::UI::Colors::LightSlateGray() });*/
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

    winrt::RuntimeComponent1::Class term;
    int a = term.MyProperty();
    a;

    //container.Children().Append(term);

    return container;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment(apartment_type::single_threaded);

    IslandWindow window;

    auto defaultContent = CreateDefaultContent();
    window.SetRootContent(defaultContent);

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        DispatchMessage(&message);
    }
}
