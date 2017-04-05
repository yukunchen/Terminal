#include "precomp.h"

#include "ApiDetector.hpp"

#include <VersionHelpers.h>

#pragma hdrstop

using namespace Microsoft::Console::Interactivity;

// API Sets
#define API_SET_NTUSER_WINEVENT_L1 L"api-ms-win-rtcore-ntuser-winevent-l1-1-0"
#define API_SET_NTUSER_WINDOW_L1   L"api-ms-win-rtcore-ntuser-window-l1-1-0"

// This may not be defined depending on the SDK version being targetted.
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER
#define LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER 0x00004000
#endif

#pragma region Public Methods

// Routine Description
// - This routine detects whether the system supports creating windows.
// - Asserts the presence of CreateWindowExW on the system.
// Arguments:
// - level - pointer to an APILevel enum stating the level of support the system offers for the
//           given functionality.
NTSTATUS ApiDetector::DetectConsoleWindowSupport(_Out_ ApiLevel* level)
{
    return DetectApiSupport(API_SET_NTUSER_WINDOW_L1, nullptr, level);
}

// Routine Description
// - This routine detects whether the system has high DPI support.
// - Asserts the presence of SetProcessDpiAwarenessContext on the system.
// Arguments:
// - level - pointer to an APILevel enum stating the level of support the system offers for the
//           given functionality.
NTSTATUS ApiDetector::DetectHighDpiSupport(_Out_ ApiLevel* level)
{
    return DetectApiSupport(API_SET_NTUSER_WINDOW_L1, nullptr, level);
}

// Routine Description
// - This routine detects whether the system supports accessibility notifications via NotifyWinEvent.
// - Attempts to load the API set that contains NotifyWinEvent.
// Arguments:
// - level - pointer to an APILevel enum stating the level of support the system offers for the
//           given functionality.
NTSTATUS ApiDetector::DetectAccessibilityNotificationSupport(_Out_ ApiLevel* level)
{
    // N.B.: Testing for the API set implies the function is present.
    return DetectApiSupport(API_SET_NTUSER_WINEVENT_L1, nullptr, level);
}

#pragma endregion

#pragma region Private Methods

NTSTATUS ApiDetector::DetectApiSupport(_In_ LPCWSTR lpApiHost, _In_ LPCSTR lpProcedure, _Out_ ApiLevel* level)
{
    if (!level)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_SUCCESS;
    HMODULE  hModule = nullptr;

    status = TryLoadWellKnownLibrary(lpApiHost, &hModule);
    if (NT_SUCCESS(status) && lpProcedure)
    {
        status = TryLocateProcedure(hModule, lpProcedure);
    }

    SetLevelAndFreeIfNecessary(status, hModule, level);

    return STATUS_SUCCESS;
}

NTSTATUS ApiDetector::TryLoadWellKnownLibrary(_In_ LPCWSTR lpLibrary, _Outptr_result_nullonfailure_ HMODULE *phModule)
{
    NTSTATUS status = STATUS_SUCCESS;

    // N.B.: Suppose we attempt to load user32.dll and locate CreateWindowExW
    //       on a Nano Server system with reverse forwarders enabled. Since the
    //       reverse forwarder modules have the same name as their regular
    //       counterparts, the loader will claim to have found the module. In
    //       addition, since reverse forwarders contain all the functions of
    //       their regular counterparts, just stubbed to return or set the last
    //       error to STATUS_NOT_IMPLEMENTED, GetProcAddress will indeed
    //       indicate that the procedure exists. Hence, we need to search for
    //       modules skipping over the reverse forwarders.
    //
    //       This however has the side-effect of not working on downlevel.
    //       LoadLibraryEx asserts that the flags passed in are valid. If any
    //       invalid flags are passed, it sets the last error to
    //       STATUS_INVALID_PARAMETER and returns. Since
    //       LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER does not exist on
    //       downlevel Windows, the call will fail there.
    //
    //       To counteract that, we try to load the library skipping forwarders
    //       first under the assumption that we are running on a sufficiently
    //       new system. If the call fails with STATUS_INVALID_PARAMETER, we
    //       know there is a problem with the flags, so we try again without
    //       the NO_FORWARDER part. Because reverse forwarders do not exist on
    //       downlevel (i.e. < Windows 10), we do not run the risk of failing
    //       to accurately detect system functionality there.
    //
    // N.B.: We cannot use IsWindowsVersionOrGreater or associated helper API's
    //       because those are subject to manifesting and may tell us we are
    //       running on Windows 8 even if we are running on Windows 10.
    //
    // TODO: MSFT 10916452 Determine what manifest magic is required to make
    //       versioning API's behave sanely.

    status = TryLoadWellKnownLibrary(lpLibrary, LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER, phModule);
    if (!NT_SUCCESS(status) && GetLastError() == STATUS_INVALID_PARAMETER)
    {
        status = TryLoadWellKnownLibrary(lpLibrary, LOAD_LIBRARY_SEARCH_SYSTEM32, phModule);
    }

    return status;
}

NTSTATUS ApiDetector::TryLoadWellKnownLibrary(_In_ LPCWSTR lpLibrary, _In_ DWORD dwLoaderFlags, _Outptr_result_nullonfailure_ HMODULE *phModule)
{
    HMODULE  hModule = nullptr;

    hModule = LoadLibraryExW(lpLibrary,
                             nullptr,
                             dwLoaderFlags);
    if (hModule)
    {
        *phModule = hModule;

        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS ApiDetector::TryLocateProcedure(_In_ HMODULE hModule, _In_ LPCSTR lpProcedure)
{
    FARPROC proc = GetProcAddress(hModule, lpProcedure);

    if (proc)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}

void ApiDetector::SetLevelAndFreeIfNecessary(_In_ NTSTATUS status, _In_ HMODULE hModule, _Out_ ApiLevel* level)
{
    if (NT_SUCCESS(status))
    {
        *level = ApiLevel::Win32;
    }
    else
    {
        FreeLibrary(hModule);

        *level = ApiLevel::OneCore;
    }
}

#pragma endregion
