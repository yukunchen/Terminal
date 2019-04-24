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

protected:
    virtual DWORD _GetWindowStyle() const noexcept override;

private:

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource _nonClientSource;

    HWND _nonClientInteropWindowHandle;
    winrt::Windows::UI::Xaml::Controls::Grid _nonClientRootGrid;

    int _windowMarginBottom = 2;
    int _windowMarginSides = 2;
    int _titlebarUnscaledMarginRight = 0;
    int _titlebarMarginTop = 2;
    int _titlebarMarginBottom = 0;

    int _titlebarUnscaledContentHeight = 0;

    ::Microsoft::Console::Types::Viewport GetTitlebarContentArea();
    ::Microsoft::Console::Types::Viewport GetClientContentArea();
    MARGINS GetFrameMargins();

    MARGINS _maximizedMargins;

    LRESULT HitTestNCA(POINT ptMouse);
    HRESULT _UpdateFrameMargins();

    void _HandleActivateWindow();
    void _HandleGetMinMaxInfo(MINMAXINFO* const minMax);
    bool _HandleWindowPosChanging(WINDOWPOS* const windowPos);

    RECT GetMaxWindowRectInPixels(const RECT * const prcSuggested, _Out_opt_ UINT * pDpiSuggested);
};
