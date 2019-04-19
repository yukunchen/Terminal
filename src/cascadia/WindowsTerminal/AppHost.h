#include "pch.h"

#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>

#include "NonClientIslandWindow.h"

class AppHost
{
public:
    AppHost() noexcept;
    virtual ~AppHost();

    void AppTitleChanged(winrt::hstring newTitle);

    void Initialize();

private:
    NonClientIslandWindow _window;
    winrt::Microsoft::Terminal::TerminalApp::TermApp _app;

    void _HandleCreateWindow(const HWND hwnd, const RECT proposedRect);
};
