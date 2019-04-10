#include "pch.h"
#include "BaseWindow.h"

class IslandWindow : public BaseWindow<IslandWindow>
{
public:
    IslandWindow(const COORD initialSize) noexcept;
    virtual ~IslandWindow() override;

    void OnSize();

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
    void ApplyCorrection(double scaleFactor);
    void NewScale(UINT dpi) override;
    void DoResize(UINT width, UINT height) override;
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);

    void Initialize(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
private:

    void _InitXamlContent();

    unsigned int _currentWidth;
    unsigned int _currentHeight;
    HWND _interopWindowHandle;
    winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;
};
