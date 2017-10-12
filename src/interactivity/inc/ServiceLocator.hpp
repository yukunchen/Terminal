/*++
Copyright (c) Microsoft Corporation

Module Name:
- ServiceLocator.hpp

Abstract:
- Locates and holds instances of classes for which multiple implementations
  exist depending on API's available on the host OS.

Author(s):
- Hernan Gatta (HeGatta) 29-Mar-2017
--*/

#pragma once

#include "IInteractivityFactory.hpp"
#include "IConsoleWindow.hpp"
#include "../../host/globals.h"

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            class ServiceLocator
            {
            public:
                // N.B.: Location methods without corresponding creation methods
                //       automatically create the singleton object on demand.
                //       In case the on-demand creation fails, the return value
                //       is nullptr and a message is logged.


                static IAccessibilityNotifier *LocateAccessibilityNotifier();

                static IConsoleControl *LocateConsoleControl();
                template <typename T> static T *LocateConsoleControl()
                {
                    return static_cast<T*>(LocateConsoleControl());
                }

                static NTSTATUS CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread);
                static IConsoleInputThread *LocateConsoleInputThread();
                template <typename T> static T *LocateConsoleInputThread()
                {
                    return static_cast<T*>(LocateConsoleInputThread());
                }

                static NTSTATUS SetConsoleWindowInstance(_In_ IConsoleWindow *window);
                static IConsoleWindow *LocateConsoleWindow();
                template <typename T> static T *LocateConsoleWindow()
                {
                    return static_cast<T*>(s_consoleWindow);
                }

                static IWindowMetrics *LocateWindowMetrics();
                template <typename T> static T *LocateWindowMetrics()
                {
                    return static_cast<T*>(LocateWindowMetrics());
                }

                static IHighDpiApi *LocateHighDpiApi();
                template <typename T> static T *LocateHighDpiApi()
                {
                    return static_cast<T*>(LocateHighDpiApi());
                }

                static IInputServices *LocateInputServices();
                template <typename T> static T *LocateInputServices()
                {
                    return static_cast<T*>(LocateInputServices());
                }

                static ISystemConfigurationProvider *LocateSystemConfigurationProvider();

                static Globals *LocateGlobals();

            protected:
                ServiceLocator(ServiceLocator const&) = delete;
                ServiceLocator& operator=(ServiceLocator const&) = delete;

            private:
                static NTSTATUS LocateInteractivityFactory(_Outptr_result_nullonfailure_ IInteractivityFactory** factory);

                static IInteractivityFactory *s_interactivityFactory;

                static IAccessibilityNotifier *s_accessibilityNotifier;
                static IConsoleControl *s_consoleControl;
                static IConsoleInputThread *s_consoleInputThread;
                static IConsoleWindow *s_consoleWindow;
                static IWindowMetrics *s_windowMetrics;
                static IHighDpiApi *s_highDpiApi;
                static IInputServices *s_inputServices;
                static ISystemConfigurationProvider *s_systemConfigurationProvider;

                static Globals *s_globals;
            };
        };
    };
};
