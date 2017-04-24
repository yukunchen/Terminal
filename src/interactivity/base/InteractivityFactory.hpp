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
            class InteractivityFactory sealed : public IInteractivityFactory
            {
            public:
                NTSTATUS CreateConsoleControl(_Outptr_result_nullonfailure_ IConsoleControl** control);
                NTSTATUS CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread);

                NTSTATUS CreateHighDpiApi(_Outptr_result_nullonfailure_ IHighDpiApi** api);
                NTSTATUS CreateWindowMetrics(_Outptr_result_nullonfailure_ IWindowMetrics** metrics);
                NTSTATUS CreateAccessibilityNotifier(_Outptr_result_nullonfailure_ IAccessibilityNotifier** notifier);
                NTSTATUS CreateSystemConfigurationProvider(_Outptr_result_nullonfailure_ ISystemConfigurationProvider** provider);
                NTSTATUS CreateInputServices(_Outptr_result_nullonfailure_ IInputServices** services);
            };
        };
    };
};
