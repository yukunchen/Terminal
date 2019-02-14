#include "pch.h"
#include "BaseWindow.h"

struct IslandWindow : BaseWindow<IslandWindow>
{
    IslandWindow() noexcept;
    ~IslandWindow();
    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager InitXaml(HWND wind,
        winrt::Windows::UI::Xaml::Controls::Grid & root,
        winrt::Windows::UI::Xaml::Media::ScaleTransform & dpiScale,
        winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource & source);
    void OnSize(HWND interopHandle,
                winrt::Windows::UI::Xaml::Controls::Grid& rootGrid,
                UINT width,
                UINT height);
    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept;
    void NewScale(UINT dpi);
    void ApplyCorrection(double scaleFactor);
    void DoResize(UINT width, UINT height);
    void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);
private:
    UINT m_currentWidth = 600;
    UINT m_currentHeight = 600;
    HWND m_interopWindowHandle = nullptr;
    winrt::Windows::UI::Xaml::Media::ScaleTransform m_scale{ nullptr };
    winrt::Windows::UI::Xaml::Controls::Grid m_rootGrid{ nullptr };
    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };
    winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager m_manager{ nullptr };
};
