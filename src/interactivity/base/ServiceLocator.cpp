#include "precomp.h"

#include "..\inc\ServiceLocator.hpp"

#include "InteractivityFactory.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Interactivity;

#pragma region Private Static Member Initialization

IInteractivityFactory        *ServiceLocator::s_interactivityFactory        = nullptr;

IConsoleWindow               *ServiceLocator::s_consoleWindow               = nullptr;

IConsoleControl              *ServiceLocator::s_consoleControl              = nullptr;
IConsoleInputThread          *ServiceLocator::s_consoleInputThread          = nullptr;

IHighDpiApi                  *ServiceLocator::s_highDpiApi                  = nullptr;
IWindowMetrics               *ServiceLocator::s_windowMetrics               = nullptr;
IAccessibilityNotifier       *ServiceLocator::s_accessibilityNotifier       = nullptr;
ISystemConfigurationProvider *ServiceLocator::s_systemConfigurationProvider = nullptr;
IInputServices               *ServiceLocator::s_inputServices               = nullptr;

Globals                      *ServiceLocator::s_globals                     = new Globals();

#pragma endregion

#pragma region Public Methods

#pragma region Creation Methods

NTSTATUS ServiceLocator::CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (s_consoleInputThread)
    {
        status = STATUS_INVALID_HANDLE;
    }
    else
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateConsoleInputThread(&s_consoleInputThread);

            if (NT_SUCCESS(status))
            {
                *thread = s_consoleInputThread;
            }
        }
    }

    return status;
}

#pragma endregion

#pragma region Set Methods

NTSTATUS ServiceLocator::SetConsoleWindowInstance(_In_ IConsoleWindow* window)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (s_consoleWindow)
    {
        status = STATUS_INVALID_HANDLE;
    }
    else if (!window)
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        s_consoleWindow = window;
    }

    return status;
}

#pragma endregion

#pragma region Location Methods

IConsoleWindow *ServiceLocator::LocateConsoleWindow()
{
    return s_consoleWindow;
}

IConsoleControl *ServiceLocator::LocateConsoleControl()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_consoleControl)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateConsoleControl(&s_consoleControl);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_consoleControl;
}

IConsoleInputThread* ServiceLocator::LocateConsoleInputThread()
{
    return s_consoleInputThread;
}

IHighDpiApi* ServiceLocator::LocateHighDpiApi()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_highDpiApi)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateHighDpiApi(&s_highDpiApi);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_highDpiApi;
}

IWindowMetrics* ServiceLocator::LocateWindowMetrics()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_windowMetrics)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateWindowMetrics(&s_windowMetrics);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_windowMetrics;
}

IAccessibilityNotifier* ServiceLocator::LocateAccessibilityNotifier()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_accessibilityNotifier)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateAccessibilityNotifier(&s_accessibilityNotifier);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_accessibilityNotifier;
}

ISystemConfigurationProvider* ServiceLocator::LocateSystemConfigurationProvider()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_systemConfigurationProvider)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateSystemConfigurationProvider(&s_systemConfigurationProvider);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_systemConfigurationProvider;
}

IInputServices* ServiceLocator::LocateInputServices()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!s_inputServices)
    {
        IInteractivityFactory *factory;
        status = ServiceLocator::LocateInteractivityFactory(&factory);

        if (NT_SUCCESS(status))
        {
            status = factory->CreateInputServices(&s_inputServices);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return s_inputServices;
}

Globals* ServiceLocator::LocateGlobals()
{
    return s_globals;
}

#pragma endregion

#pragma endregion

#pragma region Private Methods

NTSTATUS ServiceLocator::LocateInteractivityFactory(_Outptr_result_nullonfailure_ IInteractivityFactory** factory)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (s_interactivityFactory)
    {
        *factory = s_interactivityFactory;
    }
    else
    {
        s_interactivityFactory = new InteractivityFactory();
        status = NT_TESTNULL(s_interactivityFactory);

        if (NT_SUCCESS(status))
        {
            *factory = s_interactivityFactory;
        }
    }

    return status;
}

#pragma endregion
