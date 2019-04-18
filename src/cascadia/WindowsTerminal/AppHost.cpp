#include "pch.h"
#include "AppHost.h"
#include "../types/inc/Viewport.hpp"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;
using namespace ::Microsoft::Console::Types;

AppHost::AppHost() noexcept :
    _app{},
    _window{}
{
    auto pfn = std::bind(&AppHost::_HandleCreateWindow, this, std::placeholders::_1,std::placeholders::_2);

    _window.SetCreateCallback(pfn);

    _window.MakeWindow();
}

AppHost::~AppHost()
{
}

// Method Description:
// - Initializes the XAML island, creates the terminal app, and sets the
//   island's content to that of the terminal app's content. Also registers some
//   callbacks with TermApp.
// !!! IMPORTANT!!!
// This must be called *AFTER* WindowsXamlManager::InitializeForCurrentThread.
// If it isn't, then we won't be able to create the XAML island.
// Arguments:
// - <none>
// Return Value:
// - <none>
void AppHost::Initialize()
{
    _window.Initialize();
    _app.Create();

    _app.TitleChanged({ this, &AppHost::AppTitleChanged });

    AppTitleChanged(_app.GetTitle());

    _window.SetRootContent(_app.GetRoot());
}

// Method Description:
// - Called when the app's title changes. Fires off a window message so we can
//   update the window's title on the main thread.
// Arguments:
// - newTitle: the string to use as the new window title
// Return Value:
// - <none>
void AppHost::AppTitleChanged(winrt::hstring newTitle)
{
    _window.UpdateTitle(newTitle.c_str());
}

void AppHost::_HandleCreateWindow(const HWND hwnd, const RECT proposedRect)
{
    // TODO: Get monitor from proposed rect

    // TODO: Get DPI of proposed mon

    // TODO: Pass DPI to GEtLaunchDimensions
    auto initialSize = _app.GetLaunchDimensions();

    auto _currentWidth = gsl::narrow<unsigned int>(ceil(initialSize.X));
    auto _currentHeight = gsl::narrow<unsigned int>(ceil(initialSize.Y));

    // Create a RECT from our requested client size
    auto nonClient = Viewport::FromDimensions({ gsl::narrow<short>(_currentWidth), gsl::narrow<short>(_currentHeight) }).ToRect();

    // Get the size of a window we'd need to host that client rect. This will
    // add the titlebar space.
    AdjustWindowRectExForDpi(&nonClient, WS_OVERLAPPEDWINDOW, false, 0, GetDpiForSystem());
    const auto adjustedHeight = nonClient.bottom - nonClient.top;
    // Don't use the adjusted width - that'll include fat window borders, which
    // we don't have
    const auto adjustedWidth = _currentWidth;


    const COORD origin{ proposedRect.left, proposedRect.top };
    const COORD dimensions{ gsl::narrow<short>(adjustedWidth), gsl::narrow<short>(adjustedHeight) };
    auto newPos = Viewport::FromDimensions(origin, dimensions);

    SetWindowPos(hwnd, NULL,
                 newPos.Left(),
                 newPos.Top(),
                 newPos.Width(),
                 newPos.Height(),
                 SWP_NOACTIVATE | SWP_NOZORDER);


}
