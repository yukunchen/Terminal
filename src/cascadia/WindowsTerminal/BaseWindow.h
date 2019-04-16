#pragma once

template <typename T>
class BaseWindow
{
public:
    virtual ~BaseWindow() = 0;
    static T* GetThisFromHandle(HWND const window) noexcept
    {
        return reinterpret_cast<T *>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        WINRT_ASSERT(window);

        if (WM_NCCREATE == message)
        {
            auto cs = reinterpret_cast<CREATESTRUCT *>(lparam);
            T* that = static_cast<T*>(cs->lpCreateParams);
            WINRT_ASSERT(that);
            WINRT_ASSERT(!that->_window);
            that->_window = window;
            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));

            EnableNonClientDpiScaling(window);
            _currentDpi = GetDpiForWindow(window);
        }
        else if (T* that = GetThisFromHandle(window))
        {
            return that->MessageHandler(message, wparam, lparam);
        }

        return DefWindowProc(window, message, wparam, lparam);
    }

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        switch (message) {
        case WM_DPICHANGED:
        {
            return HandleDpiChange(_window, wparam, lparam);
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_SIZE:
        {
            UINT width = LOWORD(lparam);
            UINT height = HIWORD(lparam);

            if (T* that = GetThisFromHandle(_window))
            {
                that->DoResize(width, height);
            }
        }
        }

        return DefWindowProc(_window, message, wparam, lparam);
    }

    // DPI Change handler. on WM_DPICHANGE resize the window
    LRESULT HandleDpiChange(HWND hWnd, WPARAM wParam, LPARAM lParam)
    {
        HWND hWndStatic = GetWindow(hWnd, GW_CHILD);
        if (hWndStatic != nullptr)
        {
            UINT uDpi = HIWORD(wParam);

            // Resize the window
            auto lprcNewScale = reinterpret_cast<RECT*>(lParam);

            SetWindowPos(hWnd, nullptr, lprcNewScale->left, lprcNewScale->top,
                lprcNewScale->right - lprcNewScale->left, lprcNewScale->bottom - lprcNewScale->top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            if (T* that = GetThisFromHandle(hWnd))
            {
                that->NewScale(uDpi);
            }
        }
        return 0;
    }

    virtual void NewScale(UINT dpi) = 0;
    virtual void DoResize(UINT width, UINT height) = 0;

    HWND GetHandle() noexcept
    {
        return _window;
    };

protected:
    using base_type = BaseWindow<T>;
    HWND _window = nullptr;
    inline static unsigned int _currentDpi = 0;
};

template <typename T>
inline BaseWindow<T>::~BaseWindow() { }
