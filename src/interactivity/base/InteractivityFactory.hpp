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
                NTSTATUS CreateConsoleControl(std::unique_ptr<IConsoleControl>& control);
                NTSTATUS CreateConsoleInputThread(std::unique_ptr<IConsoleInputThread>& thread);

                NTSTATUS CreateHighDpiApi(std::unique_ptr<IHighDpiApi>& api);
                NTSTATUS CreateWindowMetrics(_Inout_ std::unique_ptr<IWindowMetrics>& metrics);
                NTSTATUS CreateAccessibilityNotifier(_Inout_ std::unique_ptr<IAccessibilityNotifier>& notifier);
                NTSTATUS CreateSystemConfigurationProvider(_Outptr_result_nullonfailure_ ISystemConfigurationProvider** provider);
                NTSTATUS CreateInputServices(_Outptr_result_nullonfailure_ IInputServices** services);
            };
        };
    };
};
