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
    // Tell the window to callback to us when it's about to handle a WM_CREATE
    auto pfn = std::bind(&AppHost::_HandleCreateWindow,
                         this,
                         std::placeholders::_1,
                         std::placeholders::_2);
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
    _window.SetNonClientContent(_app.GetTabs());
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

// Method Description:
// - Resize the window we're about to create to the appropriate dimensions, as
//   specified in the settings. This will be called during the handling of
//   WM_CREATE. We'll load the settings for the app, then get the proposed size
//   of the terminal from the app. Using that proposed size, we'll resize the
//   window we're creating, so that it'll match the values in the settings.
// Arguments:
// - hwnd: The HWND of the window we're about to create.
// - proposedRect: The location and size of the window that we're about to
//   create. We'll use this rect to determine which monitor the window is about
//   to appear on.
// Return Value:
// - <none>
void AppHost::_HandleCreateWindow(const HWND hwnd, const RECT proposedRect)
{
    // Find nearest montitor.
    HMONITOR hmon = MonitorFromRect(&proposedRect, MONITOR_DEFAULTTONEAREST);

    // This API guarantees that dpix and dpiy will be equal, but neither is an
    // optional parameter so give two UINTs.
    UINT dpix = USER_DEFAULT_SCREEN_DPI;
    UINT dpiy = USER_DEFAULT_SCREEN_DPI;
    // If this fails, we'll use the default of 96.
    GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpix, &dpiy);

    auto initialSize = _app.GetLaunchDimensions(dpix);

    const short _currentWidth = gsl::narrow<short>(ceil(initialSize.X));
    const short _currentHeight = gsl::narrow<short>(ceil(initialSize.Y));

    // Create a RECT from our requested client size
    auto nonClient = Viewport::FromDimensions({ _currentWidth,
                                                _currentHeight }).ToRect();

    // Get the size of a window we'd need to host that client rect. This will
    // add the titlebar space.
    bool succeeded = AdjustWindowRectExForDpi(&nonClient, WS_OVERLAPPEDWINDOW, false, 0, dpix);
    if (!succeeded)
    {
        // If we failed to get the correct window size for whatever reason, log
        // the error and go on. We'll use whatever the control proposed as the
        // size of our window, which will be at least close.
        LOG_LAST_ERROR();
        nonClient = Viewport::FromDimensions({ _currentWidth,
                                               _currentHeight }).ToRect();
    }

    const auto adjustedHeight = nonClient.bottom - nonClient.top;
    const auto adjustedWidth = nonClient.right - nonClient.left;

    const COORD origin{ gsl::narrow<short>(proposedRect.left),
                        gsl::narrow<short>(proposedRect.top) };
    const COORD dimensions{ gsl::narrow<short>(adjustedWidth),
                            gsl::narrow<short>(adjustedHeight) };

    const auto newPos = Viewport::FromDimensions(origin, dimensions);

    succeeded = SetWindowPos(hwnd, nullptr,
                             newPos.Left(),
                             newPos.Top(),
                             newPos.Width(),
                             newPos.Height(),
                             SWP_NOACTIVATE | SWP_NOZORDER);

    // If we can't resize the window, that's really okay. We can just go on with
    // the originally proposed window size.
    LOG_LAST_ERROR_IF(!succeeded);


}
