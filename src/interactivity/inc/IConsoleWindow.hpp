/*++
Copyright (c) Microsoft Corporation

Module Name:
- IConsoleWindow.hpp

Abstract:
- Defines the methods and properties of what makes a window into a console
  window.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

// copied typedef from uiautomationcore.h
typedef int EVENTID;

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            class IConsoleWindow
            {
            public:
                virtual BOOL EnableBothScrollBars() = 0;
                virtual int UpdateScrollBar(_In_ bool isVertical,
                                            _In_ bool isAltBuffer,
                                            _In_ UINT pageSize,
                                            _In_ int maxSize,
                                            _In_ int viewportPosition) = 0;

                virtual bool IsInFullscreen() const = 0;

                virtual void SetIsFullscreen(_In_ bool const fFullscreenEnabled) = 0;
                virtual NTSTATUS SetViewportOrigin(_In_ SMALL_RECT NewWindow) = 0;
                virtual void SetWindowHasMoved(_In_ BOOL const fHasMoved) = 0;

                virtual void CaptureMouse() = 0;
                virtual BOOL ReleaseMouse() = 0;

                virtual HWND GetWindowHandle() const = 0;

                // Pass null.
                virtual void SetOwner() = 0;

                virtual BOOL GetCursorPosition(_Out_ LPPOINT lpPoint) = 0;
                virtual BOOL GetClientRectangle(_Out_ LPRECT lpRect) = 0;
                virtual int MapPoints(_Inout_updates_(cPoints) LPPOINT lpPoints, _In_ UINT cPoints) = 0;
                virtual BOOL ConvertScreenToClient(_Inout_ LPPOINT lpPoint) = 0;

                virtual BOOL SendNotifyBeep() const = 0;

                virtual BOOL PostUpdateScrollBars() const = 0;
                virtual BOOL PostUpdateTitleWithCopy(_In_ const PCWSTR pwszNewTitle) const = 0;
                virtual BOOL PostUpdateWindowSize() const = 0;

                virtual void UpdateWindowSize(_In_ COORD const coordSizeInChars) const = 0;
                virtual void UpdateWindowText() = 0;

                virtual void HorizontalScroll(_In_ const WORD wScrollCommand,
                                              _In_ const WORD wAbsoluteChange) const = 0;
                virtual void VerticalScroll(_In_ const WORD wScrollCommand,
                                            _In_ const WORD wAbsoluteChange) const = 0;
                virtual HRESULT SignalUia(_In_ EVENTID id) = 0;
                virtual RECT GetWindowRect() const = 0;
            };
        };
    };
};
