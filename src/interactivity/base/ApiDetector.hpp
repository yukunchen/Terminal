#include "precomp.h"

#pragma hdrstop

namespace Microsoft
{
    namespace Console
    {
        namespace Interactivity
        {
            enum class ApiLevel
            {
                Win32,
                OneCore
            };

            class ApiDetector
            {
            public:
                static NTSTATUS DetectConsoleWindowSupport(_Out_ ApiLevel* level);

                static NTSTATUS DetectHighDpiSupport(_Out_ ApiLevel* level);
                static NTSTATUS DetectAccessibilityNotificationSupport(_Out_ ApiLevel* level);

            private:
                static NTSTATUS DetectApiSupport(_In_ LPCWSTR lpLibrary, _In_ LPCSTR lpApi, _Out_ ApiLevel* level);
                static NTSTATUS TryLoadWellKnownLibrary(_In_ LPCWSTR library, _Outptr_result_nullonfailure_ HMODULE* module);
                static NTSTATUS TryLocateProcedure(_In_ HMODULE hModule, _In_ LPCSTR lpProcedure);
                static void SetLevelAndFreeIfNecessary(_In_ NTSTATUS status, _In_ HMODULE hModule, _Out_ ApiLevel* level);
                static NTSTATUS TryLoadWellKnownLibrary(_In_ LPCWSTR lpLibrary, _In_ DWORD dwLoaderFlags, _Outptr_result_nullonfailure_ HMODULE *phModule);
            };
        };
    };
};