#include "pch.h"
#include "AppHost.h"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Hosting;
using namespace winrt::Windows::Foundation::Numerics;

AppHost::AppHost() noexcept :
    _window{},
    _app{ nullptr }
{

    _app = winrt::Microsoft::Terminal::TerminalApp::TermApp{};
}

AppHost::~AppHost()
{
}




void AppHost::Initialize()
{
    _window.Initialize();
    _app.Create();

    _app.TitleChanged({ this, &AppHost::AppTitleChanged });

    _window.SetRootContent(_app.GetRoot());
}

// // Method Description:
// // - Sets our reference to the Terminal App we're hosting, and wires up event
// //   handlers. Also sets out root content to the root content of the terminal
// //   app.
// // Arguments:
// // - app: the new Terminal app to use as our app content.
// // Return Value:
// // - <none>
// void AppHost::SetApp(winrt::Microsoft::Terminal::TerminalApp::TermApp app)
// {
//     _app = app;
//     _app.TitleChanged({ this, &AppHost::AppTitleChanged });

//     _window.SetRootContent(_app.GetRoot());
// }

// Method Description:
// - Called when the app's title changes. Fires off a window message so we can
//   update the window's title on the main thread.
// Arguments:
// - newTitle: unused - we'll query the right title value when the message is
//   processed.
// Return Value:
// - <none>
void AppHost::AppTitleChanged(winrt::hstring newTitle)
{
    PostMessageW(_window.GetHandle(), CM_UPDATE_TITLE, 0, (LPARAM)newTitle.c_str());
}
