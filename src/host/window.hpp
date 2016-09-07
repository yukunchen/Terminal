/*++
Copyright (c) Microsoft Corporation

Module Name:
- window.hpp

Abstract:
- This module is used for managing the main console window

Author(s):
- Michael Niksa (MiNiksa) 14-Oct-2014
- Paul Campbell (PaulCam) 14-Oct-2014
--*/
#pragma once


class Window sealed
{
public:
    static RECT s_GetMinClientRectInPixels();
    static RECT s_GetMaxClientRectInPixels();
    static RECT s_GetMaxWindowRectInPixels();
    static RECT s_GetMaxWindowRectInPixels(_In_ const RECT* const prcSuggested);

    static NTSTATUS CreateInstance(_In_ Settings* const pSettings,
                                   _In_ SCREEN_INFORMATION* const pScreen,
                                   _Out_ Window** const ppNewWindow);

    NTSTATUS ActivateAndShow(_In_ WORD const wShowWindow);

    ~Window();

    RECT GetWindowRect() const;
    HWND GetWindowHandle() const;
    SCREEN_INFORMATION* GetScreenInfo() const;

    BYTE GetWindowOpacity() const;
    void SetWindowOpacity(_In_ BYTE const bOpacity);
    void ApplyWindowOpacity() const;
    void ChangeWindowOpacity(_In_ short const sOpacityDelta);

    bool IsInFullscreen() const;
    void SetIsFullscreen(_In_ bool const fFullscreenEnabled);
    void ToggleFullscreen();

    NTSTATUS SetViewportOrigin(_In_ const SMALL_RECT* const prcNewOrigin);

    void VerticalScroll(_In_ const WORD wScrollCommand, _In_ const WORD wAbsoluteChange) const;
    void HorizontalScroll(_In_ const WORD wScrollCommand, _In_ const WORD wAbsoluteChange) const;

    void UpdateWindowSize(_In_ COORD const coordSizeInChars) const;
    void UpdateWindowPosition(_In_ POINT const ptNewPos) const;

    // Dispatchers (requests from other parts of the console get dispatched onto the window message queue/thread)
    BOOL SendNotifyBeep() const;
    BOOL PostUpdateTitle(_In_ const PCWSTR pwszNewTitle) const;
    // makes a copy of the original string before sending the message. The windowproc is responsible for the copy's lifetime.
    BOOL PostUpdateTitleWithCopy(_In_ const PCWSTR pwszNewTitle) const;
    BOOL PostUpdateScrollBars() const;
    BOOL PostUpdateWindowSize() const;
    BOOL PostUpdateExtendedEditKeys() const;


    // Dynamic Settings helpers
    static void s_PersistWindowPosition(_In_ PCWSTR pwszLinkTitle,
                                        _In_ PCWSTR pwszOriginalTitle,
                                        _In_ const DWORD dwFlags,
                                        _In_ const Window* const pWindow);
    static void s_PersistWindowOpacity(_In_ PCWSTR pwszLinkTitle, _In_ PCWSTR pwszOriginalTitle, _In_ const Window* const pWindow);

    void SetWindowHasMoved(_In_ BOOL const fHasMoved);

protected:
    // prevent accidental generation of copies
    Window(Window const&);
    void operator=(Window const&);

private:
    Window();

    // Registration/init
    static NTSTATUS s_RegisterWindowClass();
    NTSTATUS _MakeWindow(_In_ Settings* const pSettings, _In_ SCREEN_INFORMATION* const pScreen);
    void _CloseWindow() const;

    static ATOM s_atomWindowClass;
    Settings* _pSettings;

    NTSTATUS _InternalSetWindowSize() const;
    void _UpdateWindowSize(_In_ SIZE const sizeNew) const;

    void _UpdateSystemMetrics() const;

    // Wndproc
    static LRESULT CALLBACK s_ConsoleWindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
    LRESULT CALLBACK ConsoleWindowProc(_In_ HWND, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    // Wndproc helpers
    LRESULT _HandleTimer(_In_ const WPARAM wParam);
    void _HandleDrop(_In_ const WPARAM wParam) const;
    void _HandlePaint() const;
    void _HandleWindowPosChanged(_In_ const LPARAM lParam);

    // Dynamic Settings helpers
    static LRESULT s_RegPersistWindowPos(_In_ PCWSTR const pwszTitle, _In_ const BOOL fAutoPos, _In_ const Window* const pWindow);
    static LRESULT s_RegPersistWindowOpacity(_In_ PCWSTR const pwszTitle, _In_ const Window* const pWindow);

    // The size/position of the window on the most recent update
    // This is remembered so we can figure out which size the client was resized from
    RECT _rcClientLast;

    // Full screen
    void _BackupWindowSizes();
    void _ApplyWindowSize() const;

    bool _fIsInFullscreen;
    RECT _rcFullscreenWindowSize;
    RECT _rcNonFullscreenWindowSize;

    // math helpers
    void _CalculateWindowRect(_In_ COORD const coordWindowInChars, _Inout_ RECT* const prectWindow) const;
    static void s_CalculateWindowRect(_In_ COORD const coordWindowInChars, 
                                      _In_ int const iDpi, 
                                      _In_ COORD const coordFontSize, 
                                      _In_ COORD const coordBufferSize,
                                      _In_opt_ HWND const hWnd, 
                                      _Inout_ RECT* const prectWindow);

    static BOOL s_AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle);
    static BOOL s_AdjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle, _In_ const int iDpi);
    static BOOL s_UnadjustWindowRectEx(_Inout_ LPRECT prc, _In_ const DWORD dwStyle, _In_ const BOOL fMenu, _In_ const DWORD dwExStyle);

    static void s_ReinitializeFontsForDPIChange();
    RECT _rcWhenDpiChanges = { 0 };

    int _iSuggestedDpi = 0;
    SIZE _sizeSuggested = { 0 };


    enum ConvertRect
    {
        CLIENT_TO_WINDOW,
        WINDOW_TO_CLIENT
    };

    static void s_ConvertRect(_Inout_ RECT* const prc, _In_ ConvertRect const crDirection);
    static void s_ConvertWindowRectToClientRect(_Inout_ RECT* const prc);
    static void s_ConvertClientRectToWindowRect(_Inout_ RECT* const prc);

    static void s_ConvertWindowPosToWindowRect(_In_ LPWINDOWPOS const lpWindowPos, _Out_ RECT* const prc);

    BOOL _fHasMoved;
};
