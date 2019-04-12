#include "pch.h"
// #include "BaseWindow.h"

template <typename T>
class IslandHost : public BaseWindow<T>
{
public:
    IslandHost() noexcept;
    virtual ~IslandHost();

    virtual void OnSize();

    virtual LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
    void ApplyCorrection(double scaleFactor);
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);

    void Initialize(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);

protected:

    unsigned int _currentWidth;
    unsigned int _currentHeight;

    HWND _interopWindowHandle;

    winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;

    void _InitXamlContent();

    void _NewScale(UINT dpi);
    void _DoResize(UINT width, UINT height);
};
