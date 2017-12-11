#include "precomp.h"

#include "InteractivityFactory.hpp"

#ifdef BUILD_ONECORE_INTERACTIVITY
#include "..\onecore\AccessibilityNotifier.hpp"
#include "..\onecore\ConsoleControl.hpp"
#include "..\onecore\ConsoleInputThread.hpp"
#include "..\onecore\ConsoleWindow.hpp"
#include "..\onecore\ConIoSrvComm.hpp"
#include "..\onecore\SystemConfigurationProvider.hpp"
#include "..\onecore\WindowMetrics.hpp"
#endif

#include "..\win32\AccessibilityNotifier.hpp"
#include "..\win32\ConsoleControl.hpp"
#include "..\win32\ConsoleInputThread.hpp"
#include "..\win32\InputServices.hpp"
#include "..\win32\WindowDpiApi.hpp"
#include "..\win32\WindowMetrics.hpp"
#include "..\win32\SystemConfigurationProvider.hpp"

#pragma hdrstop

using namespace std;
using namespace Microsoft::Console::Interactivity;

#pragma region Public Methods

NTSTATUS InteractivityFactory::CreateConsoleControl(_Inout_ std::unique_ptr<IConsoleControl>& control)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IConsoleControl> newControl;
        switch (level)
        {
        case ApiLevel::Win32:
            newControl = std::make_unique<Microsoft::Console::Interactivity::Win32::ConsoleControl>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newControl = std::make_unique<Microsoft::Console::Interactivity::OneCore::ConsoleControl>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            control.swap(newControl);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateConsoleInputThread(_Inout_ std::unique_ptr<IConsoleInputThread>& thread)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IConsoleInputThread> newThread;
        switch (level)
        {
        case ApiLevel::Win32:
            newThread = std::make_unique<Microsoft::Console::Interactivity::Win32::ConsoleInputThread>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newThread = std::make_unique<Microsoft::Console::Interactivity::OneCore::ConsoleInputThread>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            thread.swap(newThread);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateHighDpiApi(_Inout_ std::unique_ptr<IHighDpiApi>& api)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IHighDpiApi> newApi;
        switch (level)
        {
        case ApiLevel::Win32:
            newApi = std::make_unique<Microsoft::Console::Interactivity::Win32::WindowDpiApi>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newApi.reset();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            api.swap(newApi);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateWindowMetrics(_Inout_ std::unique_ptr<IWindowMetrics>& metrics)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IWindowMetrics> newMetrics;
        switch (level)
        {
        case ApiLevel::Win32:
            newMetrics = std::make_unique<Microsoft::Console::Interactivity::Win32::WindowMetrics>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newMetrics = std::make_unique<Microsoft::Console::Interactivity::OneCore::WindowMetrics>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            metrics.swap(newMetrics);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateAccessibilityNotifier(_Inout_ std::unique_ptr<IAccessibilityNotifier>& notifier)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IAccessibilityNotifier> newNotifier;
        switch (level)
        {
        case ApiLevel::Win32:
            newNotifier = std::make_unique<Microsoft::Console::Interactivity::Win32::AccessibilityNotifier>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newNotifier = std::make_unique<Microsoft::Console::Interactivity::OneCore::AccessibilityNotifier>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            notifier.swap(newNotifier);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateSystemConfigurationProvider(_Inout_ std::unique_ptr<ISystemConfigurationProvider>& provider)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<ISystemConfigurationProvider> NewProvider;
        switch (level)
        {
        case ApiLevel::Win32:
            NewProvider = std::make_unique<Microsoft::Console::Interactivity::Win32::SystemConfigurationProvider>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            NewProvider = std::make_unique<Microsoft::Console::Interactivity::OneCore::SystemConfigurationProvider>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            provider.swap(NewProvider);
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateInputServices(_Inout_ std::unique_ptr<IInputServices>& services)
{
    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        std::unique_ptr<IInputServices> newServices;
        switch (level)
        {
        case ApiLevel::Win32:
            newServices = std::make_unique<Microsoft::Console::Interactivity::Win32::InputServices>();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            newServices = std::make_unique<Microsoft::Console::Interactivity::OneCore::ConIoSrvComm>();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            services.swap(newServices);
        }
    }

    return status;
}

#pragma endregion
