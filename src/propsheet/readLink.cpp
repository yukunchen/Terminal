/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <intsafe.h>
#include <propvarutil.h>
#include <wrl\client.h>

using namespace Microsoft::WRL;

#pragma hdrstop

#define LINK_FULLINFO    2

HRESULT GetPropertyBoolValue(_In_ IPropertyStore *pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BOOL *pfValue)
{
    PROPVARIANT propvar;
    HRESULT hr = pPropStore->GetValue(refPropKey, &propvar);
    if (SUCCEEDED(hr))
    {
        // If we didn't find the property, return an error. Errors returned here prevent us from changing the default
        // values (from the registry) that we would otherwise use.
        if (propvar.vt == VT_EMPTY)
        {
            hr = E_NOT_SET;
        }

        if (SUCCEEDED(hr))
        {
            hr = PropVariantToBoolean(propvar, pfValue);
        }
    }

    return hr;
}

HRESULT GetPropertyByteValue(_In_ IPropertyStore *pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BYTE *pbValue)
{
    PROPVARIANT propvar;
    HRESULT hr = pPropStore->GetValue(refPropKey, &propvar);
    if (SUCCEEDED(hr))
    {
        // If we didn't find the property, return an error. Errors returned here prevent us from changing the default
        // values (from the registry) that we would otherwise use.
        if (propvar.vt == VT_EMPTY)
        {
            hr = E_NOT_SET;
        }

        if (SUCCEEDED(hr))
        {
            SHORT sValue;
            hr = PropVariantToInt16(propvar, &sValue);
            if (SUCCEEDED(hr))
            {
                hr = (sValue >= 0 && sValue <= BYTE_MAX) ? S_OK : E_INVALIDARG;
                if (SUCCEEDED(hr))
                {
                    *pbValue = (BYTE)sValue;
                }
            }
        }
    }

    return hr;
}

// Given a link file path (pszLinkFile), read v2 console properties and fill settings appropriately
HRESULT LoadConsoleV2LinkProperties(_In_ IShellLink *pslConsole, _Inout_ CONSOLE_STATE_INFO * const pSettings)
{
    ComPtr<IPropertyStore> propStoreLnk;
    HRESULT hr = pslConsole->QueryInterface(IID_PPV_ARGS(&propStoreLnk));
    if (SUCCEEDED(hr))
    {
        // for each supported value, attempt to get the property from the link. if the value exists,
        // apply it to the appropriate location.
        BOOL fWrapText;
        hr = GetPropertyBoolValue(propStoreLnk.Get(), PKEY_Console_WrapText, &fWrapText);
        if (SUCCEEDED(hr))
        {
            pSettings->fWrapText = !!fWrapText;
        }

        BOOL fFilterOnPaste;
        hr = GetPropertyBoolValue(propStoreLnk.Get(), PKEY_Console_FilterOnPaste, &fFilterOnPaste);
        if (SUCCEEDED(hr))
        {
            pSettings->fFilterOnPaste = !!fFilterOnPaste;
        }

        BOOL fCtrlKeyShortcutsDisabled;
        hr = GetPropertyBoolValue(propStoreLnk.Get(), PKEY_Console_CtrlKeyShortcutsDisabled, &fCtrlKeyShortcutsDisabled);
        if (SUCCEEDED(hr))
        {
            pSettings->fCtrlKeyShortcutsDisabled = !!fCtrlKeyShortcutsDisabled;
        }

        BOOL fLineSelection;
        hr = GetPropertyBoolValue(propStoreLnk.Get(), PKEY_Console_LineSelection, &fLineSelection);
        if (SUCCEEDED(hr))
        {
            pSettings->fLineSelection = fLineSelection;
        }

        BYTE bWindowTransparency;
        hr = GetPropertyByteValue(propStoreLnk.Get(), PKEY_Console_WindowTransparency, &bWindowTransparency);
        if (SUCCEEDED(hr))
        {
            pSettings->bWindowTransparency = bWindowTransparency;
        }
    }
    return hr;
}

HRESULT LoadConsoleV1LinkProperties(_In_ IShellLinkDataList *psdlLink, _Out_ CONSOLE_STATE_INFO * const pConsoleStateInfo)
{
    // The supplied shortcut may not have any console properties set on it. If we don't, CopyDataBlock will fail.
    // There's no point in attempting to load the other pieces of data if the most basic piece isn't available.
    LPNT_CONSOLE_PROPS pConsoleProps;
    HRESULT hr = psdlLink->CopyDataBlock(NT_CONSOLE_PROPS_SIG, (void**)&pConsoleProps);
    if (SUCCEEDED(hr))
    {
        // Perform the same validation as conhost.exe
        hr = ((pConsoleProps->uHistoryBufferSize <= MAXSHORT) &&
              (pConsoleProps->uNumberOfHistoryBuffers <= MAXSHORT) &&
              (pConsoleProps->dwScreenBufferSize.X >= 0) &&
              (pConsoleProps->dwScreenBufferSize.Y >= 0)) ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            pConsoleStateInfo->ScreenAttributes = pConsoleProps->wFillAttribute;
            pConsoleStateInfo->PopupAttributes = pConsoleProps->wPopupFillAttribute;
            pConsoleStateInfo->ScreenBufferSize = pConsoleProps->dwScreenBufferSize;
            pConsoleStateInfo->WindowSize = pConsoleProps->dwWindowSize;
            pConsoleStateInfo->WindowPosX = pConsoleProps->dwWindowOrigin.X;
            pConsoleStateInfo->WindowPosY = pConsoleProps->dwWindowOrigin.Y;
            pConsoleStateInfo->FontSize = pConsoleProps->dwFontSize;
            pConsoleStateInfo->FontFamily = pConsoleProps->uFontFamily;
            pConsoleStateInfo->FontWeight = pConsoleProps->uFontWeight;
            CopyMemory(pConsoleStateInfo->FaceName, pConsoleProps->FaceName, sizeof(pConsoleStateInfo->FaceName));
            pConsoleStateInfo->CursorSize = pConsoleProps->uCursorSize;
            pConsoleStateInfo->FullScreen = pConsoleProps->bFullScreen;
            pConsoleStateInfo->QuickEdit = pConsoleProps->bQuickEdit;
            pConsoleStateInfo->InsertMode = pConsoleProps->bInsertMode;
            pConsoleStateInfo->AutoPosition = pConsoleProps->bAutoPosition;
            pConsoleStateInfo->HistoryBufferSize = pConsoleProps->uHistoryBufferSize;
            pConsoleStateInfo->NumberOfHistoryBuffers = pConsoleProps->uNumberOfHistoryBuffers;
            pConsoleStateInfo->HistoryNoDup = pConsoleProps->bHistoryNoDup;
            CopyMemory(pConsoleStateInfo->ColorTable, pConsoleProps->ColorTable, sizeof(pConsoleStateInfo->ColorTable));
        }

        LocalFree(pConsoleProps);
    }

    if (SUCCEEDED(hr))
    {
        // NT_FE_CONSOLE_PROPS are strictly optional, so don't bother bubbling errors if the props aren't available.
        LPNT_FE_CONSOLE_PROPS pEAConsoleProps;
        if (SUCCEEDED(psdlLink->CopyDataBlock(NT_FE_CONSOLE_PROPS_SIG, (void**)&pEAConsoleProps)))
        {
            pConsoleStateInfo->CodePage = pEAConsoleProps->uCodePage;
            LocalFree(pEAConsoleProps);
        }
    }

    return hr;
}

void GetLinkProperties(_In_ PCWSTR pszLinkName, _Inout_ CONSOLE_STATE_INFO * const pConsoleStateInfo)
{
    ComPtr<IShellLink> shellLinkConsole;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&shellLinkConsole));
    if (SUCCEEDED(hr))
    {
        ComPtr<IPersistFile> persistFileConsole;
        hr = shellLinkConsole.As(&persistFileConsole);
        if (SUCCEEDED(hr))
        {
            hr = persistFileConsole->Load(pszLinkName, 0 /*grfMode*/);
            if (SUCCEEDED(hr))
            {
                ComPtr<IShellLinkDataList> shellLinkDataList;
                hr = shellLinkConsole.As(&shellLinkDataList);
                if (SUCCEEDED(hr))
                {
                    hr = LoadConsoleV1LinkProperties(shellLinkDataList.Get(), pConsoleStateInfo);
                    if (SUCCEEDED(hr))
                    {
                        // there's no point in loading v2 properties if v1 properties don't exist.
                        hr = LoadConsoleV2LinkProperties(shellLinkConsole.Get(), pConsoleStateInfo);
                    }
                }
            }
        }
    }
}
