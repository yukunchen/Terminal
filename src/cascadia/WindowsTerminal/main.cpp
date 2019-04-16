#include "pch.h"
#include "AppHost.h"

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;
using namespace Windows::Foundation::Numerics;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    // The Windows.Storage APIs (used by TerminalApp for saving/loading
    //      settings) will fail if you use the single_threaded apartment
    init_apartment(apartment_type::multi_threaded);

    // Create the AppHost objecct, which will create both the window and the
    // Terminal App. This MUST BE constructed before the Xaml manager as TermApp
    // provides an implementation of Windows.UI.Xaml.Application.
    AppHost host;

    // Initialize the Xaml Hosting Manager
    auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    // Initialize the xaml content. This must be called AFTER the
    // WindowsXamlManager is initalized.
    host.Initialize();

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}
