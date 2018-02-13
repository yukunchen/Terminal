/*++
Copyright (c) Microsoft Corporation

Module Name:
- ShortcutSerialization.hpp

Abstract:
- This module is used for writing console properties to the link associated
    with a particular console title.

Author(s):
- Michael Niksa (MiNiksa) 23-Jul-2014
- Paul Campbell (PaulCam) 23-Jul-2014
- Mike Griese   (MiGrie)  04-Aug-2015

Revision History:
- From components of srvinit.c
- From miniksa, paulcam's Registry.cpp
--*/

#pragma once

#ifdef __cplusplus

class ShortcutSerialization
{
public:
    static NTSTATUS s_SetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo, _In_ const BOOL fEastAsianSystem, _In_ const BOOL fForceV2);
    static NTSTATUS s_GetLinkConsoleProperties(_Inout_ PCONSOLE_STATE_INFO pStateInfo);
    static NTSTATUS s_GetLinkValues(_Inout_ PCONSOLE_STATE_INFO pStateInfo,
                                    _Out_ BOOL * const pfReadConsoleProperties,
                                    _Out_writes_opt_(cchShortcutTitle) PWSTR pwszShortcutTitle,
                                    _In_ const size_t cchShortcutTitle,
                                    _Out_writes_opt_(cchIconLocation) PWSTR pwszIconLocation,
                                    _In_ const size_t cchIconLocation,
                                    _Out_opt_ int * const piIcon,
                                    _Out_opt_ int * const piShowCmd,
                                    _Out_opt_ WORD * const pwHotKey);

private:


    static void s_InitPropVarFromBool(_In_ BOOL fVal, _Out_ PROPVARIANT *ppropvar);
    static void s_InitPropVarFromByte(_In_ BYTE bVal, _Out_ PROPVARIANT *ppropvar);
    static void s_InitPropVarFromDword(_In_ DWORD dwVal, _Out_ PROPVARIANT *ppropvar);

    static void s_SetLinkPropertyBoolValue(_In_ IPropertyStore *pps, _In_ REFPROPERTYKEY refPropKey,_In_ const BOOL fVal);
    static void s_SetLinkPropertyByteValue(_In_ IPropertyStore *pps, _In_ REFPROPERTYKEY refPropKey,_In_ const BYTE bVal);
    static void s_SetLinkPropertyDwordValue(_In_ IPropertyStore *pps, _In_ REFPROPERTYKEY refPropKey,_In_ const DWORD dwVal);

    static HRESULT s_GetPropertyBoolValue(_In_ IPropertyStore * const pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BOOL * const pfValue);
    static HRESULT s_GetPropertyByteValue(_In_ IPropertyStore * const pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BYTE * const pbValue);
    static HRESULT s_GetPropertyDwordValue(_In_ IPropertyStore * const pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ DWORD * const pdwValue);

    static HRESULT s_PopulateV1Properties(_In_ IShellLink * const pslConsole, _In_ PCONSOLE_STATE_INFO pStateInfo);
    static HRESULT s_PopulateV2Properties(_In_ IShellLink * const pslConsole, _In_ PCONSOLE_STATE_INFO pStateInfo);

    static void s_GetLinkTitle(_In_ PCWSTR pwszShortcutFilename, _Out_writes_(cchShortcutTitle) PWSTR pwszShortcutTitle, _In_ const size_t cchShortcutTitle);
    static HRESULT s_GetLoadedShellLinkForShortcut(_In_ PCWSTR pwszShortcutFileName, _In_ const DWORD dwMode, _COM_Outptr_ IShellLink **ppsl, _COM_Outptr_ IPersistFile **ppPf);
};

#else // not __cplusplus

    // The following registry methods remain public for DBCS and EUDC lookups.

    NTSTATUS ShortcutSerializationSetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo, _In_ const BOOL fEastAsianSystem, _In_ const BOOL fForceV2);
    NTSTATUS ShortcutSerializationGetLinkConsoleProperties(_In_ PCONSOLE_STATE_INFO pStateInfo);
    NTSTATUS ShortcutSerializationGetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo,
                                                _Out_opt_ BOOL * const pfReadConsoleProperties,
                                                _Out_writes_opt_(cchShortcutTitle) PWSTR pwszShortcutTitle,
                                                _In_opt_ const size_t cchShortcutTitle,
                                                _Out_writes_opt_(cchIconLocation) PWSTR pwszIconLocation,
                                                _In_opt_ const size_t cchIconLocation,
                                                _Out_opt_ int * const piIcon,
                                                _Out_opt_ int * const piShowCmd,
                                                _Out_opt_ int * const piHotKey);

#endif
