/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

// Uncomment to build publically targeted scenarios.
//#define CON_USERPRIVAPI_INDIRECT

#ifndef CON_USERPRIVAPI_INDIRECT
#include <user32p.h>
#endif

#include "UserPrivApi.hpp"

NTSTATUS UserPrivApi::s_ConsoleControl(_In_ CONSOLECONTROL ConsoleCommand,
                                       _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
                                       _In_ DWORD ConsoleInformationLength)
{
#ifdef CON_USERPRIVAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef NTSTATUS(*PfnConsoleControl)(CONSOLECONTROL Command, PVOID Information, DWORD Length);

        PfnConsoleControl pfn = (PfnConsoleControl)GetProcAddress(hUser32, "ConsoleControl");

        if (pfn != nullptr)
        {
            return pfn(ConsoleCommand, ConsoleInformation, ConsoleInformationLength);
        }
    }

    return STATUS_UNSUCCESSFUL;
#else
    return ConsoleControl(ConsoleCommand, ConsoleInformation, ConsoleInformationLength);
#endif
}

BOOL UserPrivApi::s_EnterReaderModeHelper(_In_ HWND hwnd)
{
#ifdef CON_USERPRIVAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef BOOL(*PfnEnterReaderModeHelper)(HWND hwnd);

        PfnEnterReaderModeHelper pfn = (PfnEnterReaderModeHelper)GetProcAddress(hUser32, "EnterReaderModeHelper");

        if (pfn != nullptr)
        {
            return pfn(hwnd);
        }
    }

    return STATUS_UNSUCCESSFUL;
#else
    return EnterReaderModeHelper(hwnd);
#endif
}

BOOL UserPrivApi::s_TranslateMessageEx(_In_ const MSG *pmsg,
                                       _In_ UINT flags)
{
#ifdef CON_USERPRIVAPI_INDIRECT
    HMODULE hUser32 = _Instance()._hUser32;

    if (hUser32 != nullptr)
    {
        typedef BOOL(*PfnTranslateMessageEx)(const MSG *pmsg, UINT flags);

        PfnTranslateMessageEx pfn = (PfnTranslateMessageEx)GetProcAddress(hUser32, "TranslateMessageEx");

        if (pfn != nullptr)
        {
            return pfn(pmsg, flags);
        }
    }

    return STATUS_UNSUCCESSFUL;
#else
    return TranslateMessageEx(pmsg, flags);
#endif
}

#ifdef CON_USERPRIVAPI_INDIRECT
UserPrivApi::UserPrivApi()
{
    _hUser32 = LoadLibraryW(L"user32.dll");
}

UserPrivApi::~UserPrivApi()
{
    if (_hUser32 != nullptr)
    {
        FreeLibrary(_hUser32);
        _hUser32 = nullptr;
    }
}

UserPrivApi& UserPrivApi::_Instance()
{
    static UserPrivApi dpiapi;
    return dpiapi;
}
#endif
