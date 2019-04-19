#include "pch.h"
#include "IslandWindow.h"
#include "../../types/inc/Viewport.hpp"
#include <dwmapi.h>
#include <windowsx.h>

class NonClientIslandWindow : public IslandWindow
{
public:
    NonClientIslandWindow() noexcept;
    virtual ~NonClientIslandWindow() override;

    virtual void OnSize() override;

    virtual LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept override;

    void SetNonClientContent(winrt::Windows::UI::Xaml::UIElement content);

    virtual void Initialize() override;

private:

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _nonClientSource;

    HWND _nonClientInteropWindowHandle;
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

    void _HandleActivateWindow();
    RECT GetMaxWindowRectInPixels(const RECT * const prcSuggested, _Out_opt_ UINT * pDpiSuggested);
};
