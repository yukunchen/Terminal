#include "pch.h"
#include "BaseWindow.h"

class SimpleIslandWindow : public BaseWindow<SimpleIslandWindow>
{
public:
    SimpleIslandWindow() noexcept;
    virtual ~SimpleIslandWindow() override;

    void InitXamlContent();
    // winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager InitXaml(HWND wind,
    //     winrt::Windows::UI::Xaml::Controls::Grid & root,
    //     winrt::Windows::UI::Xaml::Media::ScaleTransform & dpiScale,
    //     winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource & source);
    void OnSize();

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
    void ApplyCorrection(double scaleFactor);
    void NewScale(UINT dpi) override;
    void DoResize(UINT width, UINT height) override;
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);
// private:
    UINT m_currentWidth = 600;
    UINT m_currentHeight = 600;
    HWND m_interopWindowHandle = nullptr;
    winrt::Windows::UI::Xaml::Media::ScaleTransform m_scale{ nullptr };
    winrt::Windows::UI::Xaml::Controls::Grid m_rootGrid{ nullptr };
    // winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };
    // winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_manager{ nullptr };
};
