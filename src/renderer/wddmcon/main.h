/********************************************************************************
*                                                                               *
* NTUSERCONSOLEEXT.h                                                            *
* -- ApiSet Contract for ext-ms-win-rtcore-ntuser-console-l1-1-0                *
*                                                                               *
* Copyright (c) Microsoft Corporation. All rights reserved.                     *
*                                                                               *
* Hosts the WDDM console needed for when WDDM is brought up at boot             *
********************************************************************************/


#pragma once
HRESULT
WINAPI
WDDMConBeginUpdateDisplayBatch(
    _In_ HANDLE hDisplay
    );

HRESULT
WINAPI
WDDMConCreate(
    _In_ HANDLE* phDisplay
    );

VOID
WINAPI
WDDMConDestroy(
    _In_ HANDLE hDisplay
    );

HRESULT
WINAPI
WDDMConEnableDisplayAccess(
    _In_ HANDLE phDisplay,
    _In_ BOOLEAN fEnabled
    );

HRESULT
WINAPI
WDDMConEndUpdateDisplayBatch(
    _In_ HANDLE hDisplay
    );

HRESULT
WINAPI
WDDMConGetDisplaySize(
    _In_ HANDLE hDisplay,
    _In_ CD_IO_DISPLAY_SIZE* pDisplaySize
    );

HRESULT
WINAPI
WDDMConUpdateDisplay(
    _In_ HANDLE hDisplay,
    _In_ CD_IO_ROW_INFORMATION* pRowInformation,
    _In_ BOOLEAN fInvalidate
    );
