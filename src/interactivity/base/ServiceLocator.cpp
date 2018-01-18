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

void ServiceLocator::RundownAndExit(_In_ HRESULT const hr)
{
    // MSFT:15506250
    // In VT I/O Mode, a client application might die before we've renderered 
    //      the last bit of text they've emitted. So give the VtRenderer one 
    //      last chance to paint before it is killed.
    if (s_globals->getConsoleInformation()->IsInVtIoMode())
    {
        s_globals->pRender->TriggerCircling();
    }

    // A History Lesson from MSFT: 13576341:
    // We introduced RundownAndExit to give services that hold onto important handles
    // an opportunity to let those go when we decide to exit from the console for various reasons.
    // This was because Console IO Services (ConIoSvcComm) on OneCore editions was holding onto
    // pipe and ALPC handles to talk to CSRSS ConIoSrv.dll to broker which console got display/keyboard control.
    // If we simply run straight into TerminateProcess, those handles aren't necessarily released right away.
    // The terminate operation can have a rundown period of time where APCs are serviced (such as from 
    // a DirectX kernel callback/flush/cleanup) that can take substantially longer than we expect (several whole seconds).
    // This rundown happens before the final destruction of any outstanding handles or resources.
    // If someone is waiting on one of those handles or resources outside our process, they're stuck waiting
    // for our terminate rundown and can't continue execution until we're done.
    // We don't want to have other execution in the system get stuck , so this is a great
    // place to clean up and notify any objects or threads in the system that have to cleanup safely before
    // we head into TerminateProcess and tear everything else down less gracefully.

    // TODO: MSFT: 14397093 - Expand graceful rundown beyond just the Hot Bug input services case.


    if (s_inputServices != nullptr)
    {
        delete s_inputServices;
        s_inputServices = nullptr;
    }

    TerminateProcess(GetCurrentProcess(), hr);
}

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
