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

#include "..\inc\IConsoleWindow.hpp"


namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            namespace Win32
            {
                class WindowUiaProvider;

                class Window final : public IConsoleWindow
                {
                public:
                    static NTSTATUS CreateInstance(_In_ Settings* const pSettings,
                                                   _In_ SCREEN_INFORMATION* const pScreen);

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

                    NTSTATUS SetViewportOrigin(_In_ SMALL_RECT NewWindow);

                    void VerticalScroll(_In_ const WORD wScrollCommand,
                                        _In_ const WORD wAbsoluteChange) const;
                    void HorizontalScroll(_In_ const WORD wScrollCommand,
                                          _In_ const WORD wAbsoluteChange) const;

                    BOOL EnableBothScrollBars();
                    int UpdateScrollBar(bool isVertical,
                                        bool isAltBuffer,
                                        UINT pageSize,
                                        int maxSize,
                                        int viewportPosition);

                    void UpdateWindowSize(_In_ COORD const coordSizeInChars) const;
                    void UpdateWindowPosition(_In_ POINT const ptNewPos) const;
                    void UpdateWindowText();

                    void CaptureMouse();
                    BOOL ReleaseMouse();

                    // Dispatchers (requests from other parts of the
                    // console get dispatched onto the window message
                    // queue/thread)
                    BOOL SendNotifyBeep() const;
                    BOOL PostUpdateTitle(_In_ const PCWSTR pwszNewTitle) const;
                    // makes a copy of the original string before
                    // sending the message. The windowproc is
                    // responsible for the copy's lifetime.
                    BOOL PostUpdateTitleWithCopy(_In_ const PCWSTR pwszNewTitle) const;
                    BOOL PostUpdateScrollBars() const;
                    BOOL PostUpdateWindowSize() const;
                    BOOL PostUpdateExtendedEditKeys() const;

                    // Dynamic Settings helpers
                    static void s_PersistWindowPosition(_In_ PCWSTR pwszLinkTitle,
                                                        _In_ PCWSTR pwszOriginalTitle,
                                                        _In_ const DWORD dwFlags,
                                                        _In_ const Window* const pWindow);
                    static void s_PersistWindowOpacity(_In_ PCWSTR pwszLinkTitle,
                                                       _In_ PCWSTR pwszOriginalTitle,
                                                       _In_ const Window* const pWindow);

                    void SetWindowHasMoved(_In_ BOOL const fHasMoved);

                    HRESULT SignalUia(_In_ EVENTID id);

                    void SetOwner();
                    BOOL GetCursorPosition(_Out_ LPPOINT lpPoint);
                    BOOL GetClientRectangle(_Out_ LPRECT lpRect);
                    int MapPoints(_Inout_updates_(cPoints) LPPOINT lpPoints,
                                  _In_ UINT cPoints);
                    BOOL ConvertScreenToClient(_Inout_ LPPOINT lpPoint);

                    HRESULT UiaSetTextAreaFocus();

                protected:
                    // prevent accidental generation of copies
                    Window(Window const&) = delete;
                    void operator=(Window const&) = delete;

                private:
                    Window();

                    // Registration/init
                    static NTSTATUS s_RegisterWindowClass();
                    NTSTATUS _MakeWindow(_In_ Settings* const pSettings,
                                         _In_ SCREEN_INFORMATION* const pScreen);
                    void _CloseWindow() const;

                    static ATOM s_atomWindowClass;
                    Settings* _pSettings;

                    HWND _hWnd;
                    static Window* s_Instance;

                    NTSTATUS _InternalSetWindowSize() const;
                    void _UpdateWindowSize(_In_ SIZE const sizeNew) const;

                    void _UpdateSystemMetrics() const;

                    // Wndproc
                    static LRESULT CALLBACK s_ConsoleWindowProc(_In_ HWND hwnd,
                                                                _In_ UINT uMsg,
                                                                _In_ WPARAM wParam,
                                                                _In_ LPARAM lParam);
                    LRESULT CALLBACK ConsoleWindowProc(_In_ HWND,
                                                       _In_ UINT uMsg,
                                                       _In_ WPARAM wParam,
                                                       _In_ LPARAM lParam);

                    // Wndproc helpers
                    void _HandleDrop(_In_ const WPARAM wParam) const;
                    HRESULT _HandlePaint() const;
                    void _HandleWindowPosChanged(_In_ const LPARAM lParam);

                    // Accessibility/UI Automation
                    LRESULT _HandleGetObject(_In_ HWND const hwnd,
                                             _In_ WPARAM const wParam,
                                             _In_ LPARAM const lParam);
                    IRawElementProviderSimple* _GetUiaProvider();
                    WindowUiaProvider* _pUiaProvider = nullptr;

                    // Dynamic Settings helpers
                    static LRESULT s_RegPersistWindowPos(_In_ PCWSTR const pwszTitle,
                                                         _In_ const BOOL fAutoPos,
                                                         _In_ const Window* const pWindow);
                    static LRESULT s_RegPersistWindowOpacity(_In_ PCWSTR const pwszTitle,
                                                             _In_ const Window* const pWindow);

                    // The size/position of the window on the most recent update.
                    // This is remembered so we can figure out which
                    // size the client was resized from.
                    RECT _rcClientLast;

                    // Full screen
                    void _BackupWindowSizes();
                    void _ApplyWindowSize() const;

                    bool _fIsInFullscreen;
                    RECT _rcFullscreenWindowSize;
                    RECT _rcNonFullscreenWindowSize;

                    // math helpers
                    void _CalculateWindowRect(_In_ COORD const coordWindowInChars,
                                              _Inout_ RECT* const prectWindow) const;
                    static void s_CalculateWindowRect(_In_ COORD const coordWindowInChars,
                                                      _In_ int const iDpi,
                                                      _In_ COORD const coordFontSize,
                                                      _In_ COORD const coordBufferSize,
                                                      _In_opt_ HWND const hWnd,
                                                      _Inout_ RECT* const prectWindow);

                    static void s_ReinitializeFontsForDPIChange();
                    RECT _rcWhenDpiChanges = { 0 };

                    int _iSuggestedDpi = 0;
                    SIZE _sizeSuggested = { 0 };


                    static void s_ConvertWindowPosToWindowRect(_In_ LPWINDOWPOS const lpWindowPos,
                                                               _Out_ RECT* const prc);

                    BOOL _fHasMoved;
                };
            };
        };
    };
};