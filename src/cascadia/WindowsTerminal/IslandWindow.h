#include "pch.h"
#include "BaseWindow.h"
#include <winrt/Microsoft.Terminal.TerminalControl.h>
#include <winrt/Microsoft.Terminal.TerminalApp.h>

class IslandWindow : public BaseWindow<IslandWindow>
{
public:
    IslandWindow() noexcept;
    virtual ~IslandWindow() override;

    void OnSize();

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
    void ApplyCorrection(double scaleFactor);
    void NewScale(UINT dpi) override;
    void DoResize(UINT width, UINT height) override;
    void SetApp(winrt::Microsoft::Terminal::TerminalApp::TermApp app);
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);

    void Initialize(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
    void AppTitleChanged(winrt::hstring newTitle);

private:

    void _InitXamlContent();

    unsigned int _currentWidth;
    unsigned int _currentHeight;
    HWND _interopWindowHandle;
    winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;

    winrt::Microsoft::Terminal::TerminalApp::TermApp _app;
};
