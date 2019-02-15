#include "pch.h"
#include "BaseWindow.h"

class IslandWindow : public BaseWindow<IslandWindow>
{
public:
    IslandWindow() noexcept;
    virtual ~IslandWindow() override;

    void InitXamlContent();
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
};
