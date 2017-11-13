/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "newdelete.hpp"
#include "ConsoleArguments.hpp"
#include "..\server\Entrypoints.h"
#include "..\interactivity\inc\ServiceLocator.hpp"

// Define TraceLogging provider
TRACELOGGING_DEFINE_PROVIDER(
    g_ConhostLauncherProvider,
    "Microsoft.Windows.Console.Launcher",
    // {770aa552-671a-5e97-579b-151709ec0dbd}
    (0x770aa552, 0x671a, 0x5e97, 0x57, 0x9b, 0x15, 0x17, 0x09, 0xec, 0x0d, 0xbd),
    TraceLoggingOptionMicrosoftTelemetry());
static bool g_fConsoleLegacy = false;
static bool g_fSendTelemetry = false;
static const int c_iTelemetrySampleRate = 1000;
PCSTR g_pszErrorDescription = nullptr;

static bool ShouldUseConhostV2()
{
    // If the registry value doesn't exist, or exists and is non-zero, we should default to using the v2 console.
    // Otherwise, in the case of an explicit value of 0, we should use the legacy console.
    bool fShouldUseConhostV2 = true;
    PCSTR pszErrorDescription = NULL;
    bool fIgnoreError = false;

    // open HKCU\Console
    wil::unique_hkey hConsoleSubKey;
    LONG lStatus = NTSTATUS_FROM_WIN32(RegOpenKeyExW(HKEY_CURRENT_USER, L"Console", 0, KEY_READ, &hConsoleSubKey));
    if (ERROR_SUCCESS == lStatus)
    {
        // now get the value of the ForceV2 reg value, if it exists
        DWORD dwValue;
        DWORD dwType;
        DWORD cbValue = sizeof(dwValue);
        lStatus = RegQueryValueExW(hConsoleSubKey.get(),
                                   L"ForceV2",
                                   nullptr,
                                   &dwType,
                                   (PBYTE)&dwValue,
                                   &cbValue);


        if (ERROR_SUCCESS == lStatus &&
            dwType == REG_DWORD &&                     // response is a DWORD
            cbValue == sizeof(dwValue))                // response data exists
        {
            // Value exists. If non-zero use v2 console.
            fShouldUseConhostV2 = dwValue != 0;
        }
        else
        {
            pszErrorDescription = "RegQueryValueKey Failed";
            fIgnoreError = lStatus == ERROR_FILE_NOT_FOUND;
        }
    }
    else
    {
        pszErrorDescription = "RegOpenKey Failed";
        // ignore error caused by RegOpenKey if it's a simple case of the key not being found
        fIgnoreError = lStatus == ERROR_FILE_NOT_FOUND;
    }

    if (ERROR_SUCCESS != lStatus && !fIgnoreError && g_fSendTelemetry)
    {
        TraceLoggingWrite(g_ConhostLauncherProvider, "ConhostLauncherError",
                          TraceLoggingInt32(c_iTelemetrySampleRate, "SampleRatePer"),
                          TraceLoggingString(__FUNCTION__, "Function"),
                          TraceLoggingInt32(NTSTATUS_FROM_WIN32(lStatus), "NT_STATUS"),
                          TraceLoggingBool(g_fConsoleLegacy, "ConsoleLegacy"),
                          TraceLoggingString(pszErrorDescription, "ErrorDescription"),
                          TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    }

    return fShouldUseConhostV2;
}

// For the console host, we limit the amount of telemetry sent back by only sending it for an "interactive" session
// where the user has clicked, typed, etc.  Because it relies on a live person, this drastically limits the amount of 
// telemetry, which is good.
//
// The problem with the console launcher, is we can't tell if a user is launching the console
// or if the system launched it programmatically.  We're seeing cases where consoles are being launched hundreds
// of times per minute on the same computer, sending back a stream of telemetry, which can affect the responsiveness of the computer.
// To help limit the amount of telemetry being sent back, only send a small % back.  This should still be enough telemetry to help
// determine the issues our users run into.
//
// Only call this method once, since we want to capture all or none of the telemetry while this process runs.
static void DetermineIfSendTelemetry()
{
    LARGE_INTEGER time;
    // QueryPerformanceCounter will succeed on Window XP+ computers, so ignore the return value.
    QueryPerformanceCounter(&time);
    srand(static_cast<unsigned int> (time.LowPart));
    g_fSendTelemetry = !(rand() % c_iTelemetrySampleRate);
}

static HRESULT ValidateServerHandle(_In_ const HANDLE handle)
{
    // Make sure this is a console file.
    FILE_FS_DEVICE_INFORMATION DeviceInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS const Status = NtQueryVolumeInformationFile(handle, &IoStatusBlock, &DeviceInformation, sizeof(DeviceInformation), FileFsDeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        g_pszErrorDescription = "NtQueryVolumeInformationFile Failure";
        RETURN_NTSTATUS(Status);
    }
    else if (DeviceInformation.DeviceType != FILE_DEVICE_CONSOLE && g_fSendTelemetry)
    {
        g_pszErrorDescription = "Unexpected Device Type";
        TraceLoggingWrite(g_ConhostLauncherProvider, "ConhostLauncherError",
                          TraceLoggingInt32(c_iTelemetrySampleRate, "SampleRatePer"),
                          TraceLoggingInt64(DeviceInformation.DeviceType, "DeviceType"),
                          TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

        return E_INVALIDARG;
    }
    else
    {
        return S_OK;
    }
}

static bool ShouldUseLegacyConhost(_In_ const bool fForceV1)
{
    return fForceV1 || !ShouldUseConhostV2();
}

static HRESULT ActivateLegacyConhost(_In_ const HANDLE handle)
{
    HRESULT hr = S_OK;
    g_fConsoleLegacy = true;

    if (g_fSendTelemetry)
    {
        // Write version of console using TraceLogging
        TraceLoggingWrite(g_ConhostLauncherProvider, "TypeOfConsoleLoaded",
                          TraceLoggingInt32(c_iTelemetrySampleRate, "SampleRatePer"),
                          TraceLoggingBool(g_fConsoleLegacy, "ConsoleLegacy"),
                          TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    }

    PCWSTR pszConhostDllName = L"ConhostV1.dll";

    // Load our implementation, and then Load/Launch the IO thread.
    wil::unique_hmodule hConhostBin(LoadLibraryExW(pszConhostDllName, NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
    if (hConhostBin.get() != nullptr)
    {
        typedef NTSTATUS(*PFNCONSOLECREATEIOTHREAD)(__in HANDLE Server);

        PFNCONSOLECREATEIOTHREAD pfnConsoleCreateIoThread = (PFNCONSOLECREATEIOTHREAD)GetProcAddress(hConhostBin.get(), "ConsoleCreateIoThread");
        if (pfnConsoleCreateIoThread != nullptr)
        {
            hr = HRESULT_FROM_NT(pfnConsoleCreateIoThread(handle));
            if (FAILED(hr))
            {
                g_pszErrorDescription = "pfnConsoleCreateIoThread Failure";
            }
        }
        else
        {
            g_pszErrorDescription = "GetProcAddress failed";
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        g_pszErrorDescription = "LoadLibraryEx NULL Pointer";
        // setup status error
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (SUCCEEDED(hr))
    {
        hConhostBin.release();
    }

    return hr;
}

// from srvinit.cpp, fwd declare.
void ConsoleCheckDebug();

// Routine Description:
// - Main entry point for EXE version of console launching.
//   This can be used as a debugging/diagnostics tool as well as a method of testing the console without
//   replacing the system binary.
// Arguments:
// - hInstance - This module instance pointer is saved for resource lookups.
// - hPrevInstance - Unused pointer to the module instances. See wWinMain definitions @ MSDN for more details.
// - pwszCmdLine - Unused variable. We will look up the command line using GetCommandLineW().
// - nCmdShow - Unused variable specifying window show/hide state for Win32 mode applications.
// Return value:
// - [[noreturn]] - This function will not return. It will kill the thread we were called from and the console server threads will take over.
int CALLBACK wWinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE /*hPrevInstance*/,
    _In_ PWSTR /*pwszCmdLine*/,
    _In_ int /*nCmdShow*/)
{
    EnsureHeap();
    ServiceLocator::LocateGlobals()->hInstance = hInstance;

    ConsoleCheckDebug();

    // Register Trace provider by GUID
    TraceLoggingRegister(g_ConhostLauncherProvider);
    DetermineIfSendTelemetry();

    // Pass command line and standard handles at this point in time as 
    // potential preferences for execution that were passed on process creation.
    ConsoleArguments args(GetCommandLineW(),
                          GetStdHandle(STD_INPUT_HANDLE),
                          GetStdHandle(STD_OUTPUT_HANDLE));

    HRESULT hr = args.ParseCommandline();
    if (SUCCEEDED(hr))
    {
        if (ShouldUseLegacyConhost(args.GetForceV1()))
        {
            if (args.ShouldCreateServerHandle())
            {
                hr = E_INVALIDARG;
                g_pszErrorDescription = "Invalid Launch Mechanism for Legacy Console";
            }
            else
            {
                g_fConsoleLegacy = true;
                hr = ActivateLegacyConhost(args.GetServerHandle());
            }
        }
        else
        {
            if (args.ShouldCreateServerHandle())
            {
                hr = Entrypoints::StartConsoleForCmdLine(args.GetClientCommandline().c_str(), &args);
            }
            else
            {
                hr = Entrypoints::StartConsoleForServerHandle(args.GetServerHandle(), &args);
            }
        }
    }
    else
    {
        g_pszErrorDescription = "ParseCommandLine failure";
    }

    if (FAILED(hr) && g_fSendTelemetry)
    {
        TraceLoggingWrite(g_ConhostLauncherProvider, "ConhostLauncherError",
                          TraceLoggingInt32(c_iTelemetrySampleRate, "SampleRatePer"),
                          TraceLoggingString(__FUNCTION__, "Function"),
                          TraceLoggingInt32(hr, "NT_STATUS"),
                          TraceLoggingBool(g_fConsoleLegacy, "ConsoleLegacy"),
                          TraceLoggingString(g_pszErrorDescription, "ErrorDescription"),
                          TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        TraceLoggingUnregister(g_ConhostLauncherProvider);
        return hr;
    }

    // Unregister Tracelogging
    TraceLoggingUnregister(g_ConhostLauncherProvider);

    // Since the lifetime of conhost.exe is inextricably tied to the lifetime of its client processes we set our process
    // shutdown priority to zero in order to effectively opt out of shutdown process enumeration. Conhost will exit when
    // all of its client processes do.
    SetProcessShutdownParameters(0, 0);

    // Only do this if startup was successful. Otherwise, this will leave conhost.exe running with no hosted application.
    if (SUCCEEDED(hr))
    {
        ExitThread(hr);
    }

    return hr;
}
