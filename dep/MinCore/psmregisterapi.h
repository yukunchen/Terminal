 
/********************************************************************************
*                                                                               *
* psmregisterapi.h -- ApiSet contract for ext-ms-win-com-psmregister-l1         *
*                                                                               *
* Copyright (c) Microsoft Corporation. All rights reserved.                     *
*                                                                               *
********************************************************************************/
#pragma once

#ifndef _APISETPSMREGISTEREXT_H_
#define _APISETPSMREGISTEREXT_H_

#include <apiset.h>
#include <apisetcconv.h>

#ifdef _CONTRACT_GEN        // Header(s) needed for contract generation only.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <apiquery.h>
#include <combaseapi.h>

#define PSM_REGISTER_EXT_HOST
#endif

/* APISET_NAME: ext-ms-win-com-psmregister-l1 */



#undef APICONTRACT
#ifndef PSM_REGISTER_EXT_HOST
#define APICONTRACT         DECLSPEC_IMPORT
#else
#define APICONTRACT
#endif

//
// Activation types.
//
// N.B. The order is important; dedicated background server types must occur
//      later than the PsmActPureHost enumerated value.
//

typedef enum _PSM_ACTIVATE_BACKGROUND_TYPE {
    PsmActNotBackground = 0,    // Activation cannot be treated as background
    PsmActMixedHost,            // Mixed background activation type
    PsmActPureHost,             // Dedicated background host
    PsmActSystemHost,           // Dedicated system background work
    PsmActInvalidType           // Sentinel invalid value; must be the last value
} PSM_ACTIVATE_BACKGROUND_TYPE, *PPSM_ACTIVATE_BACKGROUND_TYPE; 

#ifdef __cplusplus
extern "C" {
#endif

APICONTRACT
NTSTATUS
NTAPI
PsmActivateApplicationByToken(
    __in ULONG SessionId,
    __in HANDLE TokenHandle
    );


#define PSM_ACTIVATION_TOKEN_PACKAGED_APPLICATION               0x00000001UL
#define PSM_ACTIVATION_TOKEN_SHARED_ENTITY                      0x00000002UL
#define PSM_ACTIVATION_TOKEN_FULL_TRUST                         0x00000004UL
#define PSM_ACTIVATION_TOKEN_NATIVE_SERVICE                     0x00000008UL
#define PSM_ACTIVATION_TOKEN_DESKTOP_APP_INHIBIT_BREAKAWAY      0x00000020UL
#define PSM_ACTIVATION_TOKEN_PER_APP_BROKER                     0x00000040UL
#define PSM_ACTIVATION_TOKEN_CLAIMS_NON_INHERITABLE             0x00000080UL
#define PSM_ACTIVATION_TOKEN_CLAIMS_INHERITABLE_ONCE            0x00000100UL
#define PSM_ACTIVATION_TOKEN_GENERATE_DESKTOPAPPX_HOST_ID       0x10000000UL
#define PSM_ACTIVATION_TOKEN_GENERATE_HOST_ID                   0x20000000UL
#define PSM_ACTIVATION_TOKEN_GENERATE_DYNAMIC_ID                0x40000000UL
#define PSM_ACTIVATION_TOKEN_MODIFY_INPUT_TOKEN                 0x80000000UL


#include <AppModel.h>

APICONTRACT
NTSTATUS
NTAPI
PsmAdjustActivationToken(
    __in HANDLE InputToken,
    __in ULONG Flags,
    __in PSM_ACTIVATE_BACKGROUND_TYPE ActivationType,
    __in PCWSTR PackageFullName,
    __in PCWSTR ApplicationId,
    __in PackageOrigin Origin,
    __out_opt PHANDLE OutputToken
    );







#define PSM_CREATE_MATCH_TOKEN_REMOVE_PACKAGED         0x00000001UL
#define PSM_CREATE_MATCH_TOKEN_USE_DEFAULT_SECURITY    0x00000002UL
#define PSM_CREATE_MATCH_TOKEN_MATCH_PER_HOST          0x00000004UL
#define PSM_CREATE_MATCH_TOKEN_MARK_PER_APP_BROKER     0x00000008UL
#define PSM_CREATE_MATCH_TOKEN_CLAIMS_NON_INHERITABLE  0x00000010UL
#define PSM_CREATE_MATCH_TOKEN_CLAIMS_INHERITABLE_ONCE 0x00000020UL

APICONTRACT
NTSTATUS
NTAPI
PsmCreateMatchToken(
    __in HANDLE InputToken,
    __in HANDLE CompareToken,
    __in ULONG Flags,
    __out PHANDLE MatchToken
    );


APICONTRACT
NTSTATUS
NTAPI
PsmQueryBackgroundActivationType(
    __in HANDLE TokenHandle,
    __out PPSM_ACTIVATE_BACKGROUND_TYPE ActivationType
    );


#define PSM_REGISTER_FLAG_ACTIVATE_APPLICATION      0x00000001UL
#define PSM_REGISTER_FLAG_ENABLE_DEBUG              0x00000002UL
#define PSM_REGISTER_FLAG_SYSTEM_TASK               0x00000004UL
#define PSM_REGISTER_FLAG_PURE_BACKGROUND           0x00000008UL
#define PSM_REGISTER_FLAG_AOW_REGISTRATION          0x00000010UL
#define PSM_REGISTER_FLAG_SILENT_BREAKAWAY          0x00000020UL
#define PSM_REGISTER_FLAG_ATTRIBUTION_ONLY          0x00000040UL

APICONTRACT
NTSTATUS
NTAPI
PsmRegisterApplicationProcess(
    __in HANDLE NewProcess,
    __in ULONG ActivateFlags
    );



APICONTRACT
NTSTATUS
NTAPI
PsmRegisterDesktopProcess(
    __in HANDLE NewProcess,
    __in ULONG ActivateFlags,
    __out PULONG_PTR JobHandle
    );


 // !defined(_CONTRACT_GEN) || (_APISET_PSMREGISTEREXT_VER >= 0x0200)


APICONTRACT
NTSTATUS
NTAPI
PsmRegisterDesktopProcessWithAppContainerToken(
    _In_ HANDLE NewProcess,
    _In_ ULONG ActivateFlags,
    _In_ HANDLE AppContainerToken
    );





APICONTRACT
NTSTATUS
NTAPI
PsmAdjustActivationTokenWithDynamicId(
    __in HANDLE InputToken,
    __in ULONG Flags,
    __in PSM_ACTIVATE_BACKGROUND_TYPE ActivationType,
    __in PCWSTR PackageFullName,
    __in PCWSTR ApplicationId,
    __in PackageOrigin Origin,
    __in LPCGUID DynamicId,
    __out_opt PHANDLE OutputToken
    );


 // !defined(_CONTRACT_GEN) || (_APISET_PSMREGISTEREXT_VER >= 0x0201)


APICONTRACT
NTSTATUS
NTAPI
PsmAdjustActivationTokenPkgClaim(
    __in HANDLE InputToken,
    __in ULONG Flags
    );


 // !defined(_CONTRACT_GEN) || (_APISET_PSMREGISTEREXT_VER >= 0x0201)


APICONTRACT
NTSTATUS
NTAPI
PsmRegisterServiceProcess(
    __in HANDLE NewProcess,
    __in PSID UserSid,
    __in ULONG SessionId,
    __in PCWSTR PsmKey,
    __in ULONG64 HostId,
    __in ULONG ActivateFlags
    );


 // !defined(_CONTRACT_GEN) || (_APISET_PSMREGISTEREXT_VER >= 0x0202)

#ifdef __cplusplus
}
#endif

#endif // _APISETPSMREGISTEREXT_H_



#ifndef ext_ms_win_com_psmregister_l1_2_2_query_routines
#define ext_ms_win_com_psmregister_l1_2_2_query_routines



//
//Private Extension API Query Routines
//

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
__stdcall
IsPsmActivateApplicationByTokenPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmAdjustActivationTokenPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmCreateMatchTokenPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmQueryBackgroundActivationTypePresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmRegisterApplicationProcessPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmRegisterDesktopProcessPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmRegisterDesktopProcessWithAppContainerTokenPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmAdjustActivationTokenWithDynamicIdPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmAdjustActivationTokenPkgClaimPresent(
    VOID
    );

BOOLEAN
__stdcall
IsPsmRegisterServiceProcessPresent(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif // endof guard

