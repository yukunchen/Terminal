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
#include "../../types/inc/Locked.hpp"

#include <memory>

#pragma hdrstop

namespace Microsoft::Console::Interactivity
{
    class ServiceLocator final
    {
    public:

        static void RundownAndExit(_In_ HRESULT const hr);

        // N.B.: Location methods without corresponding creation methods
        //       automatically create the singleton object on demand.
        //       In case the on-demand creation fails, the return value
        //       is nullptr and a message is logged.


        static Locked<IAccessibilityNotifier> LocateAccessibilityNotifier();

        static Locked<IConsoleControl> LocateConsoleControl();

        [[nodiscard]]
        static NTSTATUS CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread);

        [[nodiscard]]
        static NTSTATUS SetConsoleWindowInstance(_In_ IConsoleWindow *window);
        static Locked<IConsoleWindow> LocateConsoleWindow();

        static Locked<IWindowMetrics> LocateWindowMetrics();
        
        static Locked<IHighDpiApi> LocateHighDpiApi();

        static Locked<IInputServices> LocateInputServices();
        
        static Locked<ISystemConfigurationProvider> LocateSystemConfigurationProvider();

        static Globals& LocateGlobals();

    protected:
        ServiceLocator(ServiceLocator const&) = delete;
        ServiceLocator& operator=(ServiceLocator const&) = delete;

    private:
        [[nodiscard]]
        static NTSTATUS _LoadInteractivityFactory();
        
        static std::recursive_mutex s_interactivityFactoryMutex;
        static std::unique_ptr<IInteractivityFactory> s_interactivityFactory;

        static std::recursive_mutex s_accessibilityNotifierMutex;
        static std::unique_ptr<IAccessibilityNotifier> s_accessibilityNotifier;
        static std::recursive_mutex s_consoleControlMutex;
        static std::unique_ptr<IConsoleControl> s_consoleControl;
        static std::recursive_mutex s_consoleInputThreadMutex;
        static std::unique_ptr<IConsoleInputThread> s_consoleInputThread;
        static std::recursive_mutex s_consoleWindowMutex;
        // TODO: MSFT 15344939 - some implementations of IConsoleWindow are currently singleton
        // classes so we can't own a pointer to them here. fix this so s_consoleWindow can follow the
        // pattern of the rest of the service interface pointers.
        static IConsoleWindow* s_consoleWindow;
        static std::recursive_mutex s_windowMetricsMutex;
        static std::unique_ptr<IWindowMetrics> s_windowMetrics;
        static std::recursive_mutex s_highDpiApiMutex;
        static std::unique_ptr<IHighDpiApi> s_highDpiApi;    
        static std::recursive_mutex s_systemConfigurationProviderMutex;
        static std::unique_ptr<ISystemConfigurationProvider> s_systemConfigurationProvider;
        static std::recursive_mutex s_inputServicesMutex;
        static std::unique_ptr<IInputServices> s_inputServices;

        static Globals s_globals;
    };
}
