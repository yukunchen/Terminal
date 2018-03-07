#pragma once

#include "precomp.h"

#include "ApiDetector.hpp"

#include "..\inc\IInteractivityFactory.hpp"

#include <map>

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            class InteractivityFactory final : public IInteractivityFactory
            {
            public:
                NTSTATUS CreateConsoleControl(_Inout_ std::unique_ptr<IConsoleControl>& control);
                NTSTATUS CreateConsoleInputThread(_Inout_ std::unique_ptr<IConsoleInputThread>& thread);

                NTSTATUS CreateHighDpiApi(_Inout_ std::unique_ptr<IHighDpiApi>& api);
                NTSTATUS CreateWindowMetrics(_Inout_ std::unique_ptr<IWindowMetrics>& metrics);
                NTSTATUS CreateAccessibilityNotifier(_Inout_ std::unique_ptr<IAccessibilityNotifier>& notifier);
                NTSTATUS CreateSystemConfigurationProvider(_Inout_ std::unique_ptr<ISystemConfigurationProvider>& provider);
                NTSTATUS CreateInputServices(_Inout_ std::unique_ptr<IInputServices>& services);
            };
        };
    };
};
