#include "pch.h"
#pragma once
#include "NonClientIslandWindow.h"
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/TerminalApp.h>

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

    NonClientIslandWindow window;

    // This is constructed before the Xaml manager as it provides an implementation
    //      of Windows.UI.Xaml.Application.
    winrt::TerminalApp::TermApp app;

    // Initialize the Xaml Hosting Manager
    auto manager = Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();

    // IslandWindow::Initialize will create the xaml island hwnd and create the
    //      content that should be in the island.
    window.Initialize();

    // Actually create some xaml content, and place it in the island
    app.Create();
    window.SetRootContent(app.GetRoot());
    window.SetNonClientContent(app.GetTabs());

    MSG message;

    while (GetMessage(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    return 0;
}
