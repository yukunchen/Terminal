/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "SystemConfigurationProvider.hpp"

#include "icon.hpp"
#include "..\inc\ServiceLocator.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

UINT SystemConfigurationProvider::GetCaretBlinkTime()
{
    return ::GetCaretBlinkTime();
}

bool SystemConfigurationProvider::IsCaretBlinkingEnabled()
{
    return GetSystemMetrics(SM_CARETBLINKINGENABLED) ? true : false;
}

int SystemConfigurationProvider::GetNumberOfMouseButtons()
{
    return GetSystemMetrics(SM_CMOUSEBUTTONS);
}

ULONG SystemConfigurationProvider::GetNumberOfWheelScrollLines()
{
    ULONG lines;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, FALSE);

    return lines;
}

ULONG SystemConfigurationProvider::GetNumberOfWheelScrollCharacters()
{
    ULONG characters;
    SystemParametersInfoW(SPI_GETWHEELSCROLLCHARS, 0, &characters, FALSE);

    return characters;
}

void SystemConfigurationProvider::GetSettingsFromLink(
    _Inout_ Settings* pLinkSettings,
    _Inout_updates_bytes_(*pdwTitleLength) LPWSTR pwszTitle,
    _Inout_ PDWORD pdwTitleLength,
    _In_ PCWSTR pwszCurrDir,
    _In_ PCWSTR pwszAppName)
{
    WCHAR wszIconLocation[MAX_PATH] = { 0 };
    int iIconIndex = 0;

    pLinkSettings->SetCodePage(ServiceLocator::LocateGlobals()->uiOEMCP);

    // Did we get started from a link?
    if (pLinkSettings->GetStartupFlags() & STARTF_TITLEISLINKNAME)
    {
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        {
            const size_t cbTitle = (*pdwTitleLength + 1) * sizeof(WCHAR);
            ServiceLocator::LocateGlobals()->getConsoleInformation()->LinkTitle = (PWSTR) new BYTE[cbTitle];

            NTSTATUS Status = NT_TESTNULL(ServiceLocator::LocateGlobals()->getConsoleInformation()->LinkTitle);
            if (NT_SUCCESS(Status))
            {
                if (FAILED(StringCbCopyNW(ServiceLocator::LocateGlobals()->getConsoleInformation()->LinkTitle, cbTitle, pwszTitle, *pdwTitleLength)))
                {
                    Status = STATUS_UNSUCCESSFUL;
                }

                if (NT_SUCCESS(Status))
                {
                    CONSOLE_STATE_INFO csi = { 0 };
                    csi.LinkTitle = ServiceLocator::LocateGlobals()->getConsoleInformation()->LinkTitle;
                    WCHAR wszShortcutTitle[MAX_PATH];
                    BOOL fReadConsoleProperties;
                    WORD wShowWindow = pLinkSettings->GetShowWindow();
                    DWORD dwHotKey = pLinkSettings->GetHotKey();

                    int iShowWindow;
                    WORD wHotKey;
                    Status = ShortcutSerialization::s_GetLinkValues(&csi,
                        &fReadConsoleProperties,
                        wszShortcutTitle,
                        ARRAYSIZE(wszShortcutTitle),
                        wszIconLocation,
                        ARRAYSIZE(wszIconLocation),
                        &iIconIndex,
                        &iShowWindow,
                        &wHotKey);

                    // Convert results back to appropriate types and set.
                    if (SUCCEEDED(IntToWord(iShowWindow, &wShowWindow)))
                    {
                        pLinkSettings->SetShowWindow(wShowWindow);
                    }

                    dwHotKey = wHotKey;
                    pLinkSettings->SetHotKey(dwHotKey);

                    // if we got a title, use it. even on overall link value load failure, the title will be correct if
                    // filled out.
                    if (wszShortcutTitle[0] != L'\0')
                    {
                        // guarantee null termination to make OACR happy.
                        wszShortcutTitle[ARRAYSIZE(wszShortcutTitle) - 1] = L'\0';
                        StringCbCopyW(pwszTitle, *pdwTitleLength, wszShortcutTitle);

                        // OACR complains about the use of a DWORD here, so roundtrip through a size_t
                        size_t cbTitleLength;
                        if (SUCCEEDED(StringCbLengthW(pwszTitle, *pdwTitleLength, &cbTitleLength)))
                        {
                            // don't care about return result -- the buffer is guaranteed null terminated to at least
                            // the length of Title
                            (void)SizeTToDWord(cbTitleLength, pdwTitleLength);
                        }
                    }

                    if (NT_SUCCESS(Status) && fReadConsoleProperties)
                    {
                        // copy settings
                        pLinkSettings->InitFromStateInfo(&csi);

                        // since we were launched via shortcut, make sure we don't let the invoker's STARTUPINFO pollute the
                        // shortcut's settings
                        pLinkSettings->UnsetStartupFlag(STARTF_USESIZE | STARTF_USECOUNTCHARS);
                    }
                    else
                    {
                        // if we didn't find any console properties, or otherwise failed to load link properties, pretend
                        // like we weren't launched from a shortcut -- this allows us to at least try to find registry
                        // settings based on title.
                        pLinkSettings->UnsetStartupFlag(STARTF_TITLEISLINKNAME);
                    }
                }
            }
            CoUninitialize();
        }
    }

    // Go get the icon
    if (wszIconLocation[0] == L'\0')
    {
        // search for the application along the path so that we can load its icons (if we didn't find one explicitly in
        // the shortcut)
        const DWORD dwLinkLen = SearchPathW(pwszCurrDir, pwszAppName, nullptr, ARRAYSIZE(wszIconLocation), wszIconLocation, nullptr);

        // If we cannot find the application in the path, then try to fall back and see if the window title is a valid path and use that.
        if (dwLinkLen <= 0 || dwLinkLen > sizeof(wszIconLocation))
        {
            if (PathFileExistsW(pwszTitle) && (wcslen(pwszTitle) < sizeof(wszIconLocation)))
            {
                StringCchCopyW(wszIconLocation, ARRAYSIZE(wszIconLocation), pwszTitle);
            }
            else
            {
                // If all else fails, just stick the app name into the path and try to resolve just the app name.
                StringCchCopyW(wszIconLocation, ARRAYSIZE(wszIconLocation), pwszAppName);
            }
        }
    }

    if (wszIconLocation[0] != L'\0')
    {
        Icon::Instance().LoadIconsFromPath(wszIconLocation, iIconIndex);
    }

    if (!IsValidCodePage(pLinkSettings->GetCodePage()))
    {
        // make sure we don't leave this function with an invalid codepage
        pLinkSettings->SetCodePage(ServiceLocator::LocateGlobals()->uiOEMCP);
    }
}
