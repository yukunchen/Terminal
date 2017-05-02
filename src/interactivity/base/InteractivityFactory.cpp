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

NTSTATUS InteractivityFactory::CreateConsoleControl(_Outptr_result_nullonfailure_ IConsoleControl** control)
{
    if (!control)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IConsoleControl *pNewControl = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewControl = new Microsoft::Console::Interactivity::Win32::ConsoleControl();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewControl = new Microsoft::Console::Interactivity::OneCore::ConsoleControl();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *control = pNewControl;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread)
{
    if (!thread)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IConsoleInputThread *pNewThread = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewThread = new Microsoft::Console::Interactivity::Win32::ConsoleInputThread();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewThread = new Microsoft::Console::Interactivity::OneCore::ConsoleInputThread();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *thread = pNewThread;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateHighDpiApi(_Outptr_result_nullonfailure_ IHighDpiApi** api)
{
    if (!api)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IHighDpiApi *pNewApi = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewApi = new Microsoft::Console::Interactivity::Win32::WindowDpiApi();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewApi = nullptr;
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *api = pNewApi;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateWindowMetrics(_Outptr_result_nullonfailure_ IWindowMetrics** metrics)
{
    if (!metrics)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IWindowMetrics *pNewMetrics = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewMetrics = new Microsoft::Console::Interactivity::Win32::WindowMetrics();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewMetrics = new Microsoft::Console::Interactivity::OneCore::WindowMetrics();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *metrics = pNewMetrics;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateAccessibilityNotifier(_Outptr_result_nullonfailure_ IAccessibilityNotifier** notifier)
{
    if (!notifier)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IAccessibilityNotifier *pNewNotifier = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewNotifier = new Microsoft::Console::Interactivity::Win32::AccessibilityNotifier();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewNotifier = new Microsoft::Console::Interactivity::OneCore::AccessibilityNotifier();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *notifier = pNewNotifier;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateSystemConfigurationProvider(_Outptr_result_nullonfailure_ ISystemConfigurationProvider** provider)
{
    if (!provider)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        ISystemConfigurationProvider *pNewProvider = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewProvider = new Microsoft::Console::Interactivity::Win32::SystemConfigurationProvider();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewProvider = new Microsoft::Console::Interactivity::OneCore::SystemConfigurationProvider();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *provider = pNewProvider;
        }
    }

    return status;
}

NTSTATUS InteractivityFactory::CreateInputServices(_Outptr_result_nullonfailure_ IInputServices** services)
{
    if (!services)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;

    ApiLevel level;
    status = ApiDetector::DetectNtUserWindow(&level);

    if (NT_SUCCESS(status))
    {
        IInputServices *pNewServices = nullptr;
        switch (level)
        {
        case ApiLevel::Win32:
            pNewServices = new Microsoft::Console::Interactivity::Win32::InputServices();
            break;

#ifdef BUILD_ONECORE_INTERACTIVITY
        case ApiLevel::OneCore:
            pNewServices = new Microsoft::Console::Interactivity::OneCore::ConIoSrvComm();
            break;
#endif
        }

        if (NT_SUCCESS(status))
        {
            *services = pNewServices;
        }
    }

    return status;
}

#pragma endregion
