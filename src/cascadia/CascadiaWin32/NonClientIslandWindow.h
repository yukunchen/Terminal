#include "pch.h"
#include "IslandWindow.h"
#include "../../types/inc/Viewport.hpp"
#include <dwmapi.h>
#include <windowsx.h>

class NonClientIslandWindow : public IslandHost<NonClientIslandWindow>
{
public:
    NonClientIslandWindow() noexcept;
    virtual ~NonClientIslandWindow() override;

    virtual void OnSize() override;

    virtual LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept override;
    // void ApplyCorrection(double scaleFactor);
    virtual void NewScale(UINT dpi) override;
    virtual void DoResize(UINT width, UINT height) override;
    // void SetRootContent(winrt::Windows::UI::Xaml::UIElement content);
    void SetNonClientContent(winrt::Windows::UI::Xaml::UIElement content);

    // void Initialize(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
    void InitializeNonClient(winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source);
private:

    // void _InitXamlContent();

    // unsigned int _currentWidth;
    // unsigned int _currentHeight;
    // HWND _interopWindowHandle;
    HWND _nonClientInteropWindowHandle;
    // winrt::Windows::UI::Xaml::Media::ScaleTransform _scale;
    // winrt::Windows::UI::Xaml::Controls::Grid _rootGrid;
    winrt::Windows::UI::Xaml::Controls::Grid _nonClientRootGrid;

    int _windowMarginBottom = 2;
    int _windowMarginSides = 2;
    // int _titlebarMarginLeft = 2;
    // int _titlebarMarginRight = 0;
    int _titlebarUnscaledMarginRight = 0;
    int _titlebarMarginTop = 2;
    int _titlebarMarginBottom = 0;

    int _titlebarUnscaledContentHeight = 0;

    ::Microsoft::Console::Types::Viewport GetTitlebarContentArea();
    ::Microsoft::Console::Types::Viewport GetClientContentArea();
    MARGINS GetFrameMargins();
    LRESULT HitTestNCA(POINT ptMouse);
    HRESULT _UpdateFrameMargins();

    void _CreateWindow();
    void _ActivateWindow();
};
