/*++
Copyright (c) Microsoft Corporation

Module Name:
- userdpiapi.hpp

Abstract:
- This module is used for abstracting calls to ntdll DLL APIs to break DDK dependencies.

Author(s):
- Michael Niksa (MiNiksa) July-2016
--*/
#pragma once

// From winternl.h

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

//++
//
// VOID
// InitializeObjectAttributes(
//     OUT POBJECT_ATTRIBUTES p,
//     IN PUNICODE_STRING n,
//     IN ULONG a,
//     IN HANDLE r,
//     IN PSECURITY_DESCRIPTOR s
//     )
//
//--

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessDebugPort = 7,
    ProcessWow64Information = 26,
    ProcessImageFileName = 27,
    ProcessBreakOnTermination = 29
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION {
    PVOID Reserved1;
    PVOID PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    ULONG_PTR Reserved3;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;
// end From winternl.h

class NtPrivApi sealed
{
public:
    static NTSTATUS s_GetProcessParentId(_Inout_ PULONG ProcessId);

    ~NtPrivApi();

private:
    static NTSTATUS s_NtOpenProcess(_Out_ PHANDLE ProcessHandle,
                                    _In_ ACCESS_MASK DesiredAccess,
                                    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                                    _In_opt_ PCLIENT_ID ClientId);

    static NTSTATUS s_NtQueryInformationProcess(_In_ HANDLE ProcessHandle,
                                                _In_ PROCESSINFOCLASS ProcessInformationClass,
                                                _Out_ PVOID ProcessInformation,
                                                _In_ ULONG ProcessInformationLength,
                                                _Out_opt_ PULONG ReturnLength);

    static NTSTATUS s_NtClose(_In_ HANDLE Handle);

    static NtPrivApi& _Instance();
    HMODULE _hNtDll;

    NtPrivApi();
};