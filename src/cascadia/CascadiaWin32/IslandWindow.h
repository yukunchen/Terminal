#include "pch.h"
#include "BaseWindow.h"

class IslandWindow : public BaseWindow<IslandWindow>
{
public:
    IslandWindow() noexcept;
    virtual ~IslandWindow() override;

    virtual void OnSize();

    virtual LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept override;
    void ApplyCorrection(double scaleFactor);
    void NewScale(UINT dpi) override;
    void DoResize(UINT width, UINT height) override;
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);

    void Initialize();

protected:
    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _source;

    IslandWindow(const bool createWindow) noexcept;

    unsigned int _currentWidth;
    unsigned int _currentHeight;

    HWND _interopWindowHandle;

    winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;

    void _InitXamlContent();
    void _CreateWindow();
};
