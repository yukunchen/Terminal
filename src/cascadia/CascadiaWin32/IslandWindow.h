#include "pch.h"
#include "BaseWindow.h"

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
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);
    void SetNonClientContent(winrt::Windows::UI::Xaml::UIElement content);

    void Initialize(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
    void InitializeNonClient(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
private:

    void _InitXamlContent();

    unsigned int _currentWidth;
    unsigned int _currentHeight;
    HWND _interopWindowHandle;
    HWND _nonClientInteropWindowHandle;
    winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;
    winrt::Windows::UI::Xaml::Controls::Grid _nonClientRootGrid;
};
