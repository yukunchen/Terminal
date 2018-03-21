#include "precomp.h"

#include "..\inc\ServiceLocator.hpp"

#include "InteractivityFactory.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Interactivity;

#pragma region Private Static Member Initialization

std::recursive_mutex ServiceLocator::s_interactivityFactoryMutex;
std::unique_ptr<IInteractivityFactory> ServiceLocator::s_interactivityFactory;
std::recursive_mutex ServiceLocator::s_consoleControlMutex;
std::unique_ptr<IConsoleControl> ServiceLocator::s_consoleControl;
std::recursive_mutex ServiceLocator::s_consoleInputThreadMutex;
std::unique_ptr<IConsoleInputThread> ServiceLocator::s_consoleInputThread;
std::recursive_mutex ServiceLocator::s_windowMetricsMutex;
std::unique_ptr<IWindowMetrics> ServiceLocator::s_windowMetrics;
std::recursive_mutex ServiceLocator::s_accessibilityNotifierMutex;
std::unique_ptr<IAccessibilityNotifier> ServiceLocator::s_accessibilityNotifier;
std::recursive_mutex ServiceLocator::s_highDpiApiMutex;
std::unique_ptr<IHighDpiApi> ServiceLocator::s_highDpiApi;
std::recursive_mutex ServiceLocator::s_systemConfigurationProviderMutex;
std::unique_ptr<ISystemConfigurationProvider> ServiceLocator::s_systemConfigurationProvider;
std::recursive_mutex ServiceLocator::s_inputServicesMutex;
std::unique_ptr<IInputServices> ServiceLocator::s_inputServices;

std::recursive_mutex ServiceLocator::s_consoleWindowMutex;
IConsoleWindow* ServiceLocator::s_consoleWindow = nullptr;

Globals                      ServiceLocator::s_globals;

#pragma endregion

#pragma region Public Methods

void ServiceLocator::RundownAndExit(_In_ HRESULT const hr)
{
    // MSFT:15506250
    // In VT I/O Mode, a client application might die before we've rendered
    //      the last bit of text they've emitted. So give the VtRenderer one
    //      last chance to paint before it is killed.
    if (s_globals.pRender)
    {
        s_globals.pRender->TriggerTeardown();
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

    std::lock_guard<std::recursive_mutex> inputLock(s_inputServicesMutex);
    if (s_inputServices.get() != nullptr)
    {
        s_inputServices.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> sysConfigLock(s_systemConfigurationProviderMutex);
    if (s_systemConfigurationProvider.get() != nullptr)
    {
        s_systemConfigurationProvider.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> highDpiLock(s_highDpiApiMutex);
    if (s_highDpiApi.get() != nullptr)
    {
        s_highDpiApi.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> windowMetricsLock(s_windowMetricsMutex);
    if (s_windowMetrics.get() != nullptr)
    {
        s_windowMetrics.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> accessibilityNotifierLock(s_accessibilityNotifierMutex);
    if (s_accessibilityNotifier.get() != nullptr)
    {
        s_accessibilityNotifier.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> consoleControlLock(s_consoleControlMutex);
    if (s_consoleControl.get() != nullptr)
    {
        s_consoleControl.reset(nullptr);
    }

    std::lock_guard<std::recursive_mutex> consoleInputLock(s_consoleInputThreadMutex);
    // Let thread be torn down by terminate process

    std::lock_guard<std::recursive_mutex> consoleWindowLock(s_consoleWindowMutex);
    // Ownership is still inverted here. The individual Win32/OneCore Window classes will own this.
    // We just store a pointer to share the interface with other consumers.

    TerminateProcess(GetCurrentProcess(), hr);
}

#pragma region Creation Methods
[[nodiscard]]
NTSTATUS ServiceLocator::CreateConsoleInputThread(_Outptr_result_nullonfailure_ IConsoleInputThread** thread)
{
    NTSTATUS status = STATUS_SUCCESS;

    std::lock_guard<std::recursive_mutex> lock(s_consoleInputThreadMutex);

    if (s_consoleInputThread)
    {
        status = STATUS_INVALID_HANDLE;
    }
    else
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateConsoleInputThread(s_consoleInputThread);

            if (NT_SUCCESS(status))
            {
                *thread = s_consoleInputThread.get();
            }
        }
    }

    return status;
}

#pragma endregion

#pragma region Set Methods

[[nodiscard]]
NTSTATUS ServiceLocator::SetConsoleWindowInstance(_In_ IConsoleWindow* window)
{
    std::lock_guard<std::recursive_mutex> lock(s_consoleWindowMutex);

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

Locked<IConsoleWindow> ServiceLocator::LocateConsoleWindow()
{
    return Locked<IConsoleWindow>(s_consoleWindow, s_consoleWindowMutex);
}

Locked<IConsoleControl> ServiceLocator::LocateConsoleControl()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_consoleControlMutex);

    if (!s_consoleControl)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateConsoleControl(s_consoleControl);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<IConsoleControl>(s_consoleControl.get(), std::move(lock));
}

Locked<IHighDpiApi> ServiceLocator::LocateHighDpiApi()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_highDpiApiMutex);

    if (!s_highDpiApi)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateHighDpiApi(s_highDpiApi);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<IHighDpiApi>(s_highDpiApi.get(), std::move(lock));
}

Locked<IWindowMetrics> ServiceLocator::LocateWindowMetrics()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_windowMetricsMutex);

    if (!s_windowMetrics)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateWindowMetrics(s_windowMetrics);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<IWindowMetrics>(s_windowMetrics.get(), std::move(lock));
}

Locked<IAccessibilityNotifier> ServiceLocator::LocateAccessibilityNotifier()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_accessibilityNotifierMutex);

    if (!s_accessibilityNotifier)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateAccessibilityNotifier(s_accessibilityNotifier);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<IAccessibilityNotifier>(s_accessibilityNotifier.get(), std::move(lock));
}

Locked<ISystemConfigurationProvider> ServiceLocator::LocateSystemConfigurationProvider()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_systemConfigurationProviderMutex);

    if (!s_systemConfigurationProvider)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateSystemConfigurationProvider(s_systemConfigurationProvider);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<ISystemConfigurationProvider>(s_systemConfigurationProvider.get(), std::move(lock));
}

Locked<IInputServices> ServiceLocator::LocateInputServices()
{
    NTSTATUS status = STATUS_SUCCESS;

    std::unique_lock<std::recursive_mutex> lock(s_inputServicesMutex);

    if (!s_inputServices)
    {
        status = ServiceLocator::_LoadInteractivityFactory();
        if (NT_SUCCESS(status))
        {
            status = s_interactivityFactory->CreateInputServices(s_inputServices);
        }
    }

    LOG_IF_NTSTATUS_FAILED(status);

    return Locked<IInputServices>(s_inputServices.get(), std::move(lock));
}

Globals& ServiceLocator::LocateGlobals()
{
    return s_globals;
}

#pragma endregion

#pragma endregion

#pragma region Private Methods

[[nodiscard]]
NTSTATUS ServiceLocator::_LoadInteractivityFactory()
{
    NTSTATUS status = STATUS_SUCCESS;

    if (s_interactivityFactory.get() == nullptr)
    {
        std::lock_guard<std::recursive_mutex> lock(s_interactivityFactoryMutex);

        if (s_interactivityFactory.get() == nullptr)
        {
            s_interactivityFactory = std::make_unique<InteractivityFactory>();
            status = NT_TESTNULL(s_interactivityFactory.get());
        }
    }
    return status;
}

#pragma endregion
