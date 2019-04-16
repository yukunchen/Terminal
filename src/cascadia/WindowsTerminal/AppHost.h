#include "pch.h"
#include "IslandWindow.h"
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>

class AppHost
{
public:
    AppHost() noexcept;
    virtual ~AppHost();

    void AppTitleChanged(winrt::hstring newTitle);
    // void SetApp(winrt::Microsoft::Terminal::TerminalApp::TermApp app);

    void Initialize();

private:
    IslandWindow _window;
    winrt::Microsoft::Terminal::TerminalApp::TermApp _app;
};
