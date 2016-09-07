/*++
Copyright (c) Microsoft Corporation

Module Name:
- RegistrySerialization.hpp

Abstract:
- This module is used for reading/writing registry operations

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

class Settings;

class RegistrySerialization
{
public:

    // The following registry methods remain public for DBCS and EUDC lookups.
    _Check_return_
    static NTSTATUS s_OpenKey(_In_opt_ HKEY const hKey, _In_ PCWSTR const pwszSubKey, _Out_ HKEY* const phResult);

    _Check_return_
    static NTSTATUS s_QueryValue(_In_ HKEY const hKey,
                                 _In_ PCWSTR const pwszValueName,
                                 _In_ DWORD const cbValueLength,
                                 _Out_writes_bytes_(cbValueLength) BYTE* const pbData,
                                 _Out_opt_ _Out_range_(0, cbValueLength) DWORD* const pcbDataLength);

    _Check_return_
    static NTSTATUS s_EnumValue(_In_ HKEY const hKey,
                                _In_ DWORD const dwIndex,
                                _In_ DWORD const cbValueLength,
                                _Out_writes_bytes_(cbValueLength) PWSTR const pwszValueName,
                                _In_ DWORD const cbDataLength,
                                _Out_writes_bytes_(cbDataLength) BYTE* const pbData);

    _Check_return_
    static NTSTATUS s_OpenConsoleKey(_Out_ HKEY* phCurrentUserKey, _Out_ HKEY* phConsoleKey);

    _Check_return_
    static NTSTATUS s_CreateKey(_In_ HKEY const hKey, _In_ PCWSTR const pwszSubKey, _Out_ HKEY* const phResult);

    _Check_return_
    static NTSTATUS s_DeleteKey(_In_ HKEY const hKey, _In_ PCWSTR const pwszSubKey);

    _Check_return_
    static NTSTATUS s_SetValue(_In_ HKEY const hKey,
                               _In_ PCWSTR const pwszValueName,
                               _In_ DWORD const dwType,
                               _In_reads_bytes_(cbDataLength) BYTE* const pbData,
                               _In_ DWORD const cbDataLength);

    _Check_return_
    static NTSTATUS s_UpdateValue(_In_ HKEY const hConsoleKey,
                                  _In_ HKEY const hKey,
                                  _In_ PCWSTR const pwszValueName,
                                  _In_ DWORD const dwType,
                                  _In_reads_bytes_(dwDataLength) BYTE* pbData,
                                  _In_ DWORD const dwDataLength);

    _Check_return_
    static NTSTATUS s_OpenCurrentUserConsoleTitleKey(_In_ PCWSTR const title,
                                        _Out_ HKEY* phCurrentUserKey,
                                        _Out_ HKEY* phConsoleKey,
                                        _Out_ HKEY* phTitleKey );

    enum _RegPropertyType
    {
        Boolean,
        Dword,
        Word,
        Byte,
        Coordinate,
        String,
    };

    typedef struct _RegPropertyMap
    {
        _RegPropertyType const propertyType;
        PCWSTR pwszValueName;
        DWORD const dwFieldOffset;
        size_t const cbFieldSize;
        _RegPropertyMap(
                        _RegPropertyType const propertyType,
                        PCWSTR pwszValueName,
                        DWORD const dwFieldOffset,
                        size_t const cbFieldSize
                      ) :
            propertyType(propertyType),
            pwszValueName(pwszValueName),
            dwFieldOffset(dwFieldOffset),
            cbFieldSize(cbFieldSize){};

        _RegPropertyMap & operator=( const _RegPropertyMap & ) { return *this; }
    } RegPropertyMap;

    static const RegPropertyMap s_PropertyMappings[];
    static const size_t RegistrySerialization::s_PropertyMappingsSize;

    static const RegPropertyMap s_GlobalPropMappings[];
    static const size_t RegistrySerialization::s_GlobalPropMappingsSize;

    static NTSTATUS s_LoadRegDword(_In_ HKEY const hKey, _In_ const _RegPropertyMap* const pPropMap, _In_ Settings* const pSettings);
    static NTSTATUS s_LoadRegString(_In_ HKEY const hKey, _In_ const _RegPropertyMap* const pPropMap, _In_ Settings* const pSettings);

};

#else // not __cplusplus

    // The following registry methods remain public for DBCS and EUDC lookups.
    _Check_return_
    NTSTATUS RegistrySerializationOpenKey(_In_opt_ HKEY const hKey, _In_ PCWSTR const pwszSubKey, _Out_ HKEY* const phResult);

    _Check_return_
    NTSTATUS RegistrySerializationQueryValue(_In_ HKEY const hKey,
                                             _In_ PCWSTR const pwszValueName,
                                             _In_ DWORD const cbValueLength,
                                             _Out_writes_bytes_(cbValueLength) BYTE* const pbData,
                                             _Out_opt_ _Out_range_(0, cbValueLength) DWORD* const pcbDataLength);

    _Check_return_
    NTSTATUS RegistrySerializationEnumValue(_In_ HKEY const hKey,
                                            _In_ DWORD const dwIndex,
                                            _In_ DWORD const cbValueLength,
                                            _Out_writes_bytes_(cbValueLength) PWSTR const pwszValueName,
                                            _In_ DWORD const cbDataLength,
                                            _Out_writes_bytes_(cbDataLength) BYTE* const pbData);

    _Check_return_
    NTSTATUS RegistrySerializationOpenConsoleKey(_Out_ HKEY* phCurrentUserKey, _Out_ HKEY* phConsoleKey);

    _Check_return_
    NTSTATUS RegistrySerializationCreateKey(_In_ HKEY const hKey, _In_ PCWSTR const pwszSubKey, _Out_ HKEY* const phResult);

    _Check_return_
    NTSTATUS RegistrySerializationDeleteKey(_In_ HKEY const hKey, _In_ PCWSTR const pwszSubKey);

    _Check_return_
    NTSTATUS RegistrySerializationSetValue(_In_ HKEY const hKey,
                                           _In_ PCWSTR const pwszValueName,
                                           _In_ DWORD const dwType,
                                           _In_reads_bytes_(cbDataLength) BYTE* const pbData,
                                           _In_ DWORD const cbDataLength);

    _Check_return_
    NTSTATUS RegistrySerializationUpdateValue(_In_ HKEY const hConsoleKey,
                                              _In_ HKEY const hKey,
                                              _In_ PCWSTR const pwszValueName,
                                              _In_ DWORD const dwType,
                                              _In_reads_bytes_(dwDataLength) BYTE* pbData,
                                              _In_ DWORD const dwDataLength);

    NTSTATUS RegistrySerializationOpenCurrentUserConsoleTitleKey(_In_ PCWSTR const title,
                                                                 _Out_ HKEY* phCurrentUserKey,
                                                                 _Out_ HKEY* phConsoleKey,
                                                                 _Out_ HKEY* phTitleKey );
#endif
