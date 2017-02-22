#pragma once
#include <minwindef.h> // FILETIME, HINSTANCE
#include <sysinfoapi.h> // GetSystemTimeAsFileTime
#include <libloaderapi.h> // GetProcAddress

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <PathCch.h>
#include "Result.h"
#include "Resource.h"
#endif

namespace wil
{
#pragma region FILETIME helpers
    // FILETIME duration values. FILETIME is in 100 nanosecond units.
    namespace FileTimeDuration
    {
        long long const OneMillisecond = 10000LL;
        long long const OneSecond      = 10000000LL;
        long long const OneMinute      = 10000000LL * 60;
        long long const OneHour        = 10000000LL * 60 * 60;
        long long const OneDay         = 10000000LL * 60 * 60 * 24;
    };

    namespace FileTime
    {
        inline unsigned long long ToInt64(const FILETIME &ft)
        {
            static_assert(sizeof(unsigned long long) == sizeof(ft), "sizes don't match");
            return *reinterpret_cast<unsigned long long const*>(&ft);
        }

        inline FILETIME FromInt64(unsigned long long i64)
        {
            static_assert(sizeof(i64) == sizeof(FILETIME), "sizes don't match");
            return *reinterpret_cast<FILETIME *>(&i64);
        }

        inline FILETIME Add(_In_ FILETIME const &ft, long long delta)
        {
            return FromInt64(ToInt64(ft) + delta);
        }

        inline bool IsEmpty(const FILETIME &ft)
        {
            return (ft.dwHighDateTime == 0) && (ft.dwLowDateTime == 0);
        }

        inline FILETIME GetSystemTime()
        {
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            return ft;
        }
    }
#pragma endregion

    // Retrieve the HINSTANCE for the current DLL or EXE using this symbol that
    // the linker provides for every module. This avoids the need for a global HINSTANCE variable
    // and provides access to this value for static libraries.
    EXTERN_C IMAGE_DOS_HEADER __ImageBase;
    inline HINSTANCE GetModuleInstanceHandle() { return reinterpret_cast<HINSTANCE>(&__ImageBase); }
}

// Macro for calling GetProcAddress(), with type safety for C++ clients
// using the type information from the specified function.
// The return value is automatically cast to match the function prototype of the input function.
//
// Sample usage:
//
// auto pfnSendMail = GetProcAddressByFunctionDeclaration(hinstMAPI, MAPISendMailW);
// if (pfnSendMail)
// {
//    pfnSendMail(0, 0, pmm, MAPI_USE_DEFAULT, 0);
// }
//  Declaration 
#define GetProcAddressByFunctionDeclaration(hinst, fn) reinterpret_cast<decltype(::fn)*>(GetProcAddress(hinst, #fn))

