#include "pch.h"
#include "IslandWindow.h"
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment(apartment_type::multi_threaded);

    IslandWindow window;

    // IMPORTANT:
    // the manager and interop should NOT be in the window object.
    // If they are, its destructor seems to close them incorrectly, and the
    //      app will crash on close.
    // TODO MSFT:20638746 - Investigate encapsulating DesktopWindowXamlSource with IslandWindow

    // Initialize the Xaml Hosting Manager
    auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    // Create the desktop source
    DesktopWindowXamlSource desktopSource;

    // IslandWindow::Initialize will get the xaml island hwnd and create the
    //      content that should be in the island.
    window.Initialize(desktopSource);

    // Actually create some xaml content, and place it in the island
    winrt::Microsoft::Terminal::TerminalApp::TermApp app{};
    app.Create();
    window.SetRootContent(app.GetRoot());

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}
