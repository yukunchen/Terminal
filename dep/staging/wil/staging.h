// Windows Internal Libraries (wil)
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)
//
// This WIL staging header supports the ability to create features which are dynamically withheld or used to report health
// or other considerations through the Velocity back-end.
//
// For more information see the wiki:
// https://osgwiki.com/wiki/Coin_Velocity
//
// Velocity discussion Alias (velocitydisc):
// http://idwebelements/GroupManagement.aspx?Group=velocitydisc&Operation=join  (one-click join)

//! @file
/** Patterns to enable mixing features under development with shipping code and relating the concept of a feature to backend
services for the purposes of controlling enabled state and reporting usage or feature health. */

#ifndef __WIL_STAGING_INCLUDED
#define __WIL_STAGING_INCLUDED

#ifndef FEATURE_STAGING_LEGACY_MODE

// WARNING:
// Code within this scope must satisfy both C99 and C++

#ifdef _KERNEL_MODE
// This define is specifically for Kernel components that are ONLY using Staging from WIL.
// There are some Kernel components that build as C++, but have some language features disabled
// and thus can't build templates. This define allows WIL to exclude everything EXCEPT the minimum
// necessary to support staging work in Kernel for those customers. If you're trying to BOTH use WIL
// and use staging from Kernel, just ensure that you include the corresponding WIL header BEFORE you
// include the staging header.
#define __WIL_MIN_KERNEL

// Kernel features use the C API
#ifndef WIL_ENABLE_C_FEATURES
#define WIL_ENABLE_C_FEATURES
#endif

// In Kernelmode, the WNF APIs may only be called from PASSIVE_LEVEL. Annotate
// the functions in this header that call into those APIs to catch callers who
// invoke the staging APIs at elevated IRQLs.
#define WIL_PAGED_CODE() PAGED_CODE()
#define WIL_PAGED_FUNCTION _IRQL_requires_max_(PASSIVE_LEVEL)

#else
#define WIL_PAGED_CODE()
#define WIL_PAGED_FUNCTION
#endif

#include "ResultMacros.h"
#include <minwindef.h> // For WINAPI

#if defined(WIL_SUPPRESS_PRIVATE_API_USE) || defined(WIL_STAGING_DLL)
#define WIL_ENABLE_STAGING_API
#endif

//
// "const" implies internal linkage in C++ by default, whereas it
// implies external linkage in C.
// Following is to avoid multiple symbol errors when this header is
// included in multiple C files.
//

#if defined(__cplusplus)
#define __WIL_CONSTANT
#else
#define __WIL_CONSTANT extern __declspec(selectany)
#endif

//*************************************************************************************************
// Interface - Public API for features (shcore.dll)
//*************************************************************************************************

#pragma warning(push)
#pragma warning(disable:4746)

typedef enum FEATURE_CHANGE_TIME
{
    FEATURE_CHANGE_TIME_READ = 0,
    FEATURE_CHANGE_TIME_MODULE_RELOAD = 1,
    FEATURE_CHANGE_TIME_SESSION = 2,
    FEATURE_CHANGE_TIME_REBOOT = 3,
    FEATURE_CHANGE_TIME_USER_FLAG = 0x80
} FEATURE_CHANGE_TIME;
DEFINE_ENUM_FLAG_OPERATORS(FEATURE_CHANGE_TIME);

typedef enum FEATURE_ENABLED_STATE
{
    FEATURE_ENABLED_STATE_DEFAULT = 0,
    FEATURE_ENABLED_STATE_DISABLED = 1,
    FEATURE_ENABLED_STATE_ENABLED = 2,
    FEATURE_ENABLED_STATE_HAS_NOTIFICATION = 0x80,
    FEATURE_ENABLED_STATE_HAS_VARIANT_CONFIGURATION = 0x40,
} FEATURE_ENABLED_STATE;
DEFINE_ENUM_FLAG_OPERATORS(FEATURE_ENABLED_STATE);

typedef struct FEATURE_ERROR
{
    HRESULT hr;
    UINT16 lineNumber;
    PCSTR file;
    PCSTR process;
    PCSTR modulePath;
    UINT32 callerReturnAddressOffset;
    PCSTR callerModule;
    PCSTR message;
    UINT16 originLineNumber;
    PCSTR originFile;
    PCSTR originModule;
    UINT32 originCallerReturnAddressOffset;
    PCSTR originCallerModule;
    PCSTR originName;
} FEATURE_ERROR;

DECLARE_HANDLE(FEATURE_STATE_CHANGE_SUBSCRIPTION);
typedef void WINAPI FEATURE_STATE_CHANGE_CALLBACK(_In_opt_ void* context);
typedef FEATURE_STATE_CHANGE_CALLBACK *PFEATURE_STATE_CHANGE_CALLBACK;

STDAPI_(FEATURE_ENABLED_STATE) GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime);
STDAPI_(UINT32) GetFeatureVariant(UINT32 featureId, FEATURE_CHANGE_TIME changeTime, _Out_ UINT32* payloadId, _Out_ BOOL* hasNotification);
STDAPI_(void) RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, _In_ PCSTR originName);
STDAPI_(void) RecordFeatureError(UINT32 featureId, _In_ const FEATURE_ERROR* error);
STDAPI_(void) SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context);
STDAPI_(void) UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription);


//*************************************************************************************************
// Interface - RAW C
//*************************************************************************************************

//! Use this macro when utilizing a feature to provide an #ifdef based upon whether code is withheld
//! from the binary.
#define WI_IS_FEATURE_PRESENT(FeatureName) (__INTERNAL_FEATURE_PRESENT_##FeatureName == 1)

typedef enum wil_FeatureStage
{
    wil_FeatureStage_AlwaysDisabled,            //!< The feature is always excluded from the production build and cannot be enabled
    wil_FeatureStage_DisabledByDefault,         //!< The feature is included, but off by default; it can be switched on through configuration
    wil_FeatureStage_EnabledByDefault,          //!< The feature is included and on by default; it can be switched off through configuration
    wil_FeatureStage_AlwaysEnabled,             //!< The feature is always included in the production build and cannot be disabled
} wil_FeatureStage;

typedef enum wil_UsageReportingMode
{
    wil_UsageReportingMode_Default,             //!< Normal mode. Everything is reported
    wil_UsageReportingMode_SuppressPotential,   //!< Suppress potential usage reporting
    wil_UsageReportingMode_SuppressImplicit     //!< Suppress implicit usage reporting. Any reporting not explicitly specified should not occur
} wil_UsageReportingMode;

typedef enum wil_ReportingKind
{
    wil_ReportingKind_None,                     //!< Do not report any usage information from this primitive; use for module initialization and other unrelated actions to usage.
    wil_ReportingKind_UniqueUsage,              //!< Report a unique usage of this feature (when enabled) or a potential usage of the feature (when disabled).
    wil_ReportingKind_UniqueOpportunity,        //!< Report that the user was given an opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
    wil_ReportingKind_DeviceUsage,              //!< [Default for most primitives] Report that this device has used the feature (when enabled) or had the potential to use the feature (when disabled).
    wil_ReportingKind_DeviceOpportunity,        //!< Report that this device has presented the opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
    wil_ReportingKind_TotalDuration,            //!< Report the overall total time (in milliseconds) spent executing the feature (including time spent waiting on user input)
    wil_ReportingKind_PausedDuration,           //!< Report the time (in milliseconds) spent waiting on user input while the feature is being executed
} wil_ReportingKind;

typedef enum wil_VariantReportingKind
{
    wil_VariantReportingKind_None,
    wil_VariantReportingKind_UniqueUsage,
    wil_VariantReportingKind_DeviceUsage,
} wil_VariantReportingKind;

typedef enum wil_FeatureChangeTime
{
    wil_FeatureChangeTime_OnRead        = FEATURE_CHANGE_TIME_READ,
    wil_FeatureChangeTime_OnReload      = FEATURE_CHANGE_TIME_MODULE_RELOAD,
    wil_FeatureChangeTime_OnSession     = FEATURE_CHANGE_TIME_SESSION,
    wil_FeatureChangeTime_OnReboot      = FEATURE_CHANGE_TIME_REBOOT
} wil_FeatureChangeTime;

typedef enum wil_FeatureEnabledState
{
    wil_FeatureEnabledState_Default =   FEATURE_ENABLED_STATE_DEFAULT,
    wil_FeatureEnabledState_Disabled =  FEATURE_ENABLED_STATE_DISABLED,
    wil_FeatureEnabledState_Enabled =   FEATURE_ENABLED_STATE_ENABLED
} wil_FeatureEnabledState;

typedef enum wil_FeatureEnabledStateKind
{
    wil_FeatureEnabledStateKind_All     = 0,    // Used to reset state for all 'kinds' for a feature
    wil_FeatureEnabledStateKind_Service = 1,
    wil_FeatureEnabledStateKind_User    = 2,
    wil_FeatureEnabledStateKind_Test    = 3
} wil_FeatureEnabledStateKind;

typedef enum wil_FeatureEnabledStateOptions
{
    wil_FeatureEnabledStateOptions_None = 0x00,
    wil_FeatureEnabledStateOptions_VariantConfig = 0x01,
} wil_FeatureEnabledStateOptions;
DEFINE_ENUM_FLAG_OPERATORS(wil_FeatureEnabledStateOptions);

typedef enum wil_VariantPayloadType
{
    wil_VariantPayloadType_None = 0,
    wil_VariantPayloadType_Fixed = 1
} wil_VariantPayloadType;

typedef enum wil_FeatureStore
{
    wil_FeatureStore_Machine,
    wil_FeatureStore_User,
    wil_FeatureStore_All,           // Machine + User
} wil_FeatureStore;

typedef enum wil_FeatureVariantPayloadKind
{
    wil_FeatureVariantPayloadKind_None = 0,
    wil_FeatureVariantPayloadKind_Resident = 1,
    wil_FeatureVariantPayloadKind_External = 2
} wil_FeatureVariantPayloadKind;

typedef struct wil_FeatureState
{
    wil_FeatureEnabledState enabledState;
    unsigned char variant;
    wil_FeatureVariantPayloadKind payloadKind;
    unsigned int payload;
    BOOL hasNotification;
    BOOL isVariantConfiguration;
} wil_FeatureState;

typedef struct wil_FeatureTestState *wil_PFeatureTestState;

inline void wil_CreateFeatureTestState(_Out_ wil_PFeatureTestState* testFeatureState, unsigned int featureId, wil_FeatureEnabledState state);
inline void wil_CreateFeatureVariantTestState(_Out_ wil_PFeatureTestState* testFeatureState, unsigned int featureId, unsigned char variant, unsigned int payload);
inline void __stdcall wil_FreeFeatureTestState(wil_PFeatureTestState testFeatureState);
inline BOOL wil_HasFeatureTestState(unsigned int featureId, _Out_ wil_FeatureEnabledState* state);
inline BOOL wil_HasFeatureVariantTestState(unsigned int featureId, _Out_ unsigned char* variant, _Out_ unsigned int* payload);

inline wil_ReportingKind wil_GetCustomReportingKind(unsigned char index);

//#define WIL_DISABLE_SRUM_WNF

#ifndef WIL_SUPPRESS_PRIVATE_API_USE

typedef struct wil_StagingConfig *wil_PStagingConfig;

typedef struct __WIL__WNF_STATE_NAME {
    ULONG Data[2];
} __WIL_WNF_STATE_NAME;
typedef struct __WIL__WNF_STATE_NAME* __WIL_PWNF_STATE_NAME;
typedef const struct __WIL__WNF_STATE_NAME* __WIL_PCWNF_STATE_NAME;

// Configuration load and free

inline NTSTATUS wil_LoadStagingConfig(wil_PStagingConfig* config, wil_FeatureStore store, BOOL forUpdate);
inline void __stdcall wil_FreeStagingConfig(_In_ wil_PStagingConfig configHandle);
inline NTSTATUS wil_SaveStagingConfig(_In_ wil_PStagingConfig configHandle, BOOL forceOverwrite);

inline NTSTATUS wil_CacheStagingConfig(BOOL supportPerUserStore);
inline void wil_FreeCachedStagingConfig();

// Configuration query

inline BOOL wil_StagingConfig_QueryFeatureState(_In_ wil_PStagingConfig configHandle, _Outptr_ wil_FeatureState* state, unsigned int featureId, BOOL featureRequiresSessionChange);
inline BOOL wil_QueryFeatureState(_Outptr_ wil_FeatureState* state, unsigned int featureId, BOOL featureRequiresSessionChange, wil_FeatureStore store, _In_opt_ BOOL* areAnyFeaturesConfigured);

// Configuration updates

inline BOOL wil_StagingConfig_SetFeatureEnabledState(_In_ wil_PStagingConfig configHandle, unsigned int featureId, wil_FeatureEnabledState state, wil_FeatureEnabledStateKind kind, wil_FeatureEnabledStateOptions options);
inline BOOL wil_StagingConfig_SetVariant(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned char variant, wil_FeatureVariantPayloadKind kind, unsigned int payload);

// Usage notifications
inline BOOL wil_StagingConfig_SubscribeFeatureReporting(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned short serviceReportingKind, unsigned short reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state);
inline BOOL wil_StagingConfig_UnsubscribeFeatureReporting(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned short serviceReportingKind, unsigned short reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state);

typedef BOOL NTAPI __WIL_UsageSubscriptionEnumerate(unsigned int featureId, unsigned short serviceReportingKind, void* context);
typedef __WIL_UsageSubscriptionEnumerate *__WIL_PUsageSubscriptionEnumerate;

inline BOOL wil_UsageSubscriptionEnumerate(_In_ __WIL_PCWNF_STATE_NAME state, _In_opt_ __WIL_PUsageSubscriptionEnumerate enumerateFunction, BOOL clear, void* context);

// Enablement checking macros

#if (!defined(__cplusplus) || defined(__WIL_MIN_KERNEL) || defined(WIL_ENABLE_C_FEATURES))
#define WI_C_IsFeatureEnabled(className) \
    ((((className##__private_usageReportingMode == wil_UsageReportingMode_Default) || ((className##__private_usageReportingMode == wil_UsageReportingMode_SuppressPotential) && className##_isAlwaysEnabled)) && className##__private_IsEnabledPreCheck()), \
    className##_isAlwaysEnabled || (!className##_isAlwaysDisabled && className##__private_IsEnabled()))

#define WI_C_IsFeatureEnabledWithReporting(className, reportingKind) \
    ((((className##__private_usageReportingMode != wil_UsageReportingMode_SuppressPotential) || className##_isAlwaysEnabled) && className##__private_IsEnabledPreCheckWithReporting(reportingKind)), className##_isAlwaysEnabled || (!className##_isAlwaysDisabled && className##__private_IsEnabledWithReporting(reportingKind)))

#define WI_C_IsFeatureEnabledWithDefault(className, defaultValue) \
    ((((className##__private_usageReportingMode == wil_UsageReportingMode_Default) || ((className##__private_usageReportingMode == wil_UsageReportingMode_SuppressPotential) && className##_isAlwaysEnabled)) && className##__private_IsEnabledPreCheck()), \
    className##_isAlwaysEnabled || (!className##_isAlwaysDisabled && className##__private_IsEnabledWithDefault(defaultValue)))

#define WI_C_IsFeatureEnabledWithReportingAndDefault(className, reportingKind, defaultValue) \
    ((((className##__private_usageReportingMode != wil_UsageReportingMode_SuppressPotential) || className##_isAlwaysEnabled) && className##__private_IsEnabledPreCheckWithReporting(reportingKind)), className##_isAlwaysEnabled || (!className##_isAlwaysDisabled && className##__private_IsEnabledWithReportingAndDefault(reportingKind, defaultValue)))
#endif
#endif // WIL_SUPPRESS_PRIVATE_API_USE

//*************************************************************************************************
// Implementation - Raw C
//*************************************************************************************************

// These limited set of feature traits from the XML are allowed to end up in the binary for logging
// and tracing. Fields like feature name should not be exposed here due to disclosure promises.
typedef struct FEATURE_LOGGED_TRAITS
{
    UINT16 version;
    UINT16 baseVersion;
    UINT8 stage;
} FEATURE_LOGGED_TRAITS;

// Note that this function pointer is overloaded with error, usage, and variant reporting types.  It
// would have been cleaner to separate them out into distinct function pointers / purposes, but that
// would add additional function pointers to every binary using WIL.  Since there technically only
// needs to be a single integration point, all parameters are used on this hook.

typedef void __stdcall __WIL_FeatureLoggingHook(unsigned int featureId, const FEATURE_LOGGED_TRAITS* traits, const FEATURE_ERROR* error,
    BOOL enabled, const wil_ReportingKind* kind, const wil_VariantReportingKind* variantKind, unsigned char variant, SIZE_T addend);
typedef __WIL_FeatureLoggingHook *__WIL_PFeatureLoggingHook;
__declspec(selectany) __WIL_PFeatureLoggingHook g_wil_details_pfnFeatureLoggingHook = NULL;

typedef enum wil_details_FeatureTestStateKind
{
    wil_details_FeatureTestStateKind_EnabledState,
    wil_details_FeatureTestStateKind_Variant
} wil_details_FeatureTestStateKind;

typedef struct wil_details_FeatureTestState
{
    wil_details_FeatureTestStateKind kind;
    unsigned int featureId;
    wil_FeatureEnabledState state;
    unsigned char variant;
    unsigned int payload;
    struct wil_details_FeatureTestState* next;
} wil_details_FeatureTestState;

__declspec(selectany) wil_details_FeatureTestState* g_wil_details_testStates = NULL;

__WIL_CONSTANT const size_t c_wil_details_maxCustomUsageReporting = 50;
__WIL_CONSTANT const unsigned char c_wil_details_ReportingKind_CustomBase = 100;
__WIL_CONSTANT const size_t c_wil_details_ReportingKindHonorVariantConfig = (((size_t)1) << 31);
__WIL_CONSTANT const size_t c_wil_details_Variant_IsVariantConfig = (((size_t)1) << 7);
__WIL_CONSTANT const size_t c_wil_details_usageFlushHandleFlag = (((size_t)1) << 31);
__WIL_CONSTANT const size_t c_wil_details_srumReportingFlag = (((size_t)1) << 30);
__WIL_CONSTANT void* const c_wil_details_usageFlushContext = (void*)-1;

#ifndef WIL_SUPPRESS_PRIVATE_API_USE

#if (!defined(__cplusplus) || defined(__WIL_MIN_KERNEL) || defined(WIL_ENABLE_C_FEATURES))
#define WI_C_DEFINE_FEATURE(name, id, stage, version, baseVersion, changeTime, usageReportingMode) \
    __WIL_CONSTANT const unsigned int name##_id = id; \
    __WIL_CONSTANT const BOOL name##_isAlwaysDisabled = (wil_FeatureStage_##stage == wil_FeatureStage_AlwaysDisabled); \
    __WIL_CONSTANT const BOOL name##_isAlwaysEnabled = (wil_FeatureStage_##stage == wil_FeatureStage_AlwaysEnabled); \
    __WIL_CONSTANT const BOOL name##_isEnabledByDefault = (wil_FeatureStage_##stage == wil_FeatureStage_AlwaysEnabled) || (wil_FeatureStage_##stage == wil_FeatureStage_EnabledByDefault); \
    __WIL_CONSTANT const wil_FeatureChangeTime name##__private_changeTime = wil_FeatureChangeTime_##changeTime; \
    __WIL_CONSTANT const wil_FeatureStore name##__private_featureStore = wil_FeatureStore_Machine; \
    __WIL_CONSTANT const wil_UsageReportingMode name##__private_usageReportingMode = wil_UsageReportingMode_##usageReportingMode; \
    __WIL_CONSTANT const FEATURE_LOGGED_TRAITS name##_logged_traits = { version, baseVersion, wil_FeatureStage_##stage }; \
    __declspec(selectany) wil_details_FeaturePropertyCache name##__private_propertyCache = { 0 }; \
    __pragma(warning(suppress:4505)) \
    inline BOOL name##__private_IsEnabledPreCheck() \
    { \
        __pragma(warning(suppress:4127)) \
        if (name##_isAlwaysEnabled || name##_isAlwaysDisabled) \
        { \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, name##_isAlwaysEnabled, wil_ReportingKind_DeviceUsage, 1); \
        } \
        return TRUE; \
    } \
    inline BOOL name##__private_IsEnabledPreCheckWithReporting(wil_ReportingKind kind) \
    { \
        __pragma(warning(suppress:4127)) \
        if (name##_isAlwaysEnabled || name##_isAlwaysDisabled) \
        { \
            WI_ASSERT((kind != wil_ReportingKind_TotalDuration) && (kind != wil_ReportingKind_PausedDuration)); \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, name##_isAlwaysEnabled, kind, 1); \
        } \
        return TRUE; \
    } \
    inline wil_details_CachedFeatureEnabledState name##__private_GetCachedFeatureEnabledState(BOOL isEnabledByDefault) \
    { \
        return wil_details_FeaturePropertyCache_GetCachedFeatureEnabledState(&name##__private_propertyCache, name##_id, \
            isEnabledByDefault, name##__private_changeTime, name##__private_featureStore, &name##__private_areDependenciesEnabled); \
    } \
    __declspec(noinline) inline BOOL name##__private_IsEnabled() \
    { \
        BOOL enabled = (wil_details_CachedFeatureEnabledState_Enabled == name##__private_GetCachedFeatureEnabledState(name##_isEnabledByDefault)); \
        __pragma(warning(suppress:4127)) \
        if ((name##__private_usageReportingMode == wil_UsageReportingMode_Default) || ((name##__private_usageReportingMode == wil_UsageReportingMode_SuppressPotential) && enabled)) \
        { \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, enabled, wil_ReportingKind_DeviceUsage, 1); \
        } \
        return enabled; \
    } \
    __declspec(noinline) inline BOOL name##__private_IsEnabledWithDefault(BOOL defaultValue) \
    { \
        BOOL enabled = (wil_details_CachedFeatureEnabledState_Enabled == name##__private_GetCachedFeatureEnabledState(defaultValue)); \
        __pragma(warning(suppress:4127)) \
        if ((name##__private_usageReportingMode == wil_UsageReportingMode_Default) || ((name##__private_usageReportingMode == wil_UsageReportingMode_SuppressPotential) && enabled)) \
        { \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, enabled, wil_ReportingKind_DeviceUsage, 1); \
        } \
        return enabled; \
    } \
    __declspec(noinline) static BOOL name##__private_IsEnabledWithReporting(wil_ReportingKind kind) \
    { \
        BOOL enabled = (wil_details_CachedFeatureEnabledState_Enabled == name##__private_GetCachedFeatureEnabledState(name##_isEnabledByDefault)); \
        __pragma(warning(suppress:4127)) \
        if ((name##__private_usageReportingMode != wil_UsageReportingMode_SuppressPotential) || enabled) \
        { \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, enabled, kind, 1); \
        } \
        return enabled; \
    } \
    __declspec(noinline) static BOOL name##__private_IsEnabledWithReportingAndDefault(wil_ReportingKind kind, BOOL defaultValue) \
    { \
        BOOL enabled = (wil_details_CachedFeatureEnabledState_Enabled == name##__private_GetCachedFeatureEnabledState(defaultValue)); \
        __pragma(warning(suppress:4127)) \
        if ((name##__private_usageReportingMode != wil_UsageReportingMode_SuppressPotential) || enabled) \
        { \
            wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, enabled, kind, 1); \
        } \
        return enabled; \
    } \
    __pragma(warning(push)) \
    __pragma(warning(error:4714)) \
    __forceinline BOOL name##_IsEnabled() \
    { \
        return WI_C_IsFeatureEnabled(name##); \
    } \
    __forceinline BOOL name##_IsEnabledWithReporting(wil_ReportingKind kind) \
    { \
        kind; return WI_C_IsFeatureEnabledWithReporting(name##, kind); \
    } \
    __forceinline BOOL name##_IsEnabledWithDefault(BOOL isEnabledByDefaultOverride) \
    { \
        isEnabledByDefaultOverride; \
        return WI_C_IsFeatureEnabledWithDefault(name##, isEnabledByDefaultOverride); \
    } \
    __pragma(warning(pop)) \
    inline void name##_AssertEnabled() \
    { \
        WI_ASSERT(WI_C_IsFeatureEnabledWithReporting(name##, wil_ReportingKind_None)); \
    } \
    inline BOOL name##_ShouldBeEnabled() \
    { \
        if (name##_isAlwaysEnabled) { return TRUE; } \
        if (name##_isAlwaysDisabled) { return FALSE; } \
        wil_details_CachedFeatureEnabledState state = name##__private_GetCachedFeatureEnabledState(name##_isEnabledByDefault); \
        return ((state == wil_details_CachedFeatureEnabledState_Enabled) || (state == wil_details_CachedFeatureEnabledState_Desired)); \
    } \
    inline void name##_ReportUsage(wil_ReportingKind kind, size_t addend) \
    { \
        wil_details_FeaturePropertyCache_ReportUsageToService(&name##__private_propertyCache, name##_id, &name##_logged_traits, WI_C_IsFeatureEnabledWithReporting(name##, wil_ReportingKind_None), kind, addend); \
    }
#endif

typedef struct __WIL__WNF_TYPE_ID {
    GUID TypeId;
} __WIL_WNF_TYPE_ID;
typedef struct __WIL__WNF_TYPE_ID* __WIL_PWNF_TYPE_ID;
typedef const struct __WIL__WNF_TYPE_ID* __WIL_PCWNF_TYPE_ID;

typedef struct _wil_details_UsageSubscriptionData
{
    unsigned int featureId;
    unsigned short serviceReportingKind;
} wil_details_UsageSubscriptionData;

typedef ULONG __WIL_WNF_CHANGE_STAMP, *__WIL_PWNF_CHANGE_STAMP;

typedef ULONG __WIL_LOGICAL;

typedef struct __WIL__WNF_USER_SUBSCRIPTION *__WIL_PWNF_USER_SUBSCRIPTION;

typedef NTSTATUS NTAPI __WIL_WNF_USER_CALLBACK(_In_ __WIL_WNF_STATE_NAME StateName, _In_ __WIL_WNF_CHANGE_STAMP ChangeStamp, _In_opt_ __WIL_PWNF_TYPE_ID TypeId,
    _In_opt_ PVOID CallbackContext, _In_reads_bytes_opt_(Length) const VOID* Buffer, _In_ ULONG Length);
typedef __WIL_WNF_USER_CALLBACK *__WIL_PWNF_USER_CALLBACK;


typedef NTSTATUS NTAPI __WIL_WNF_NtQueryWnfStateData(_In_ __WIL_PCWNF_STATE_NAME StateName, _In_opt_ __WIL_PCWNF_TYPE_ID TypeId, _In_opt_ const VOID* ExplicitScope,
    _Out_ __WIL_PWNF_CHANGE_STAMP ChangeStamp, _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer, _Inout_ PULONG BufferSize);
typedef __WIL_WNF_NtQueryWnfStateData *__WIL_PWNF_NtQueryWnfStateData;

typedef NTSTATUS NTAPI __WIL_WNF_NtUpdateWnfStateData(_In_ __WIL_PCWNF_STATE_NAME StateName, _In_reads_bytes_opt_(Length) const VOID* Buffer, _In_opt_ ULONG Length,
    _In_opt_ __WIL_PCWNF_TYPE_ID TypeId, _In_opt_ const VOID* ExplicitScope, _In_ __WIL_WNF_CHANGE_STAMP MatchingChangeStamp, _In_ __WIL_LOGICAL CheckStamp);
typedef __WIL_WNF_NtUpdateWnfStateData *__WIL_PWNF_NtUpdateWnfStateData;

typedef NTSTATUS NTAPI __WIL_WNF_RtlSubscribeWnfStateChangeNotification(_Outptr_ __WIL_PWNF_USER_SUBSCRIPTION* Subscription, _In_ __WIL_WNF_STATE_NAME StateName,
    __WIL_WNF_CHANGE_STAMP ChangeStamp, _In_ __WIL_PWNF_USER_CALLBACK Callback, _In_opt_ PVOID CallbackContext, _In_opt_ __WIL_PWNF_TYPE_ID TypeId,
    _In_opt_ ULONG SerializationGroupIndex, _In_opt_ ULONG DeliveryOptions);
typedef __WIL_WNF_RtlSubscribeWnfStateChangeNotification *__WIL_PWNF_RtlSubscribeWnfStateChangeNotification;

typedef NTSTATUS NTAPI __WIL_WNF_RtlUnsubscribeWnfNotificationWaitForCompletion(_In_ _Post_invalid_ __WIL_PWNF_USER_SUBSCRIPTION Subscription);
typedef __WIL_WNF_RtlUnsubscribeWnfNotificationWaitForCompletion *__WIL_PWNF_RtlUnsubscribeWnfNotificationWaitForCompletion;


#define __WIL_STATUS_SUCCESS                ((NTSTATUS)0x00000000L)
#define __WIL_STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023L)
#define __WIL_STATUS_INSUFFICIENT_RESOURCES ((NTSTATUS)0xC000009AL)
#define __WIL_STATUS_REVISION_MISMATCH      ((NTSTATUS)0xC0000059L)
#define __WIL_STATUS_UNKNOWN_REVISION       ((NTSTATUS)0xC0000058L)
#define __WIL_STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001L)

__declspec(selectany) __WIL_PWNF_NtQueryWnfStateData g_wil_details_pfnNtQueryWnfStateData = NULL;
__declspec(selectany) __WIL_PWNF_NtUpdateWnfStateData g_wil_details_pfnNtUpdateWnfStateData = NULL;
__declspec(selectany) __WIL_PWNF_RtlSubscribeWnfStateChangeNotification g_wil_details_pfnRtlSubscribeWnfStateChangeNotification = NULL;
__declspec(selectany) __WIL_PWNF_RtlUnsubscribeWnfNotificationWaitForCompletion g_wil_details_pfnRtlUnsubscribeWnfNotificationWaitForCompletion = NULL;

__declspec(selectany) BOOL g_wil_details_preventOnDemandStagingConfigReads = FALSE;
__declspec(selectany) wil_PStagingConfig g_wil_details_stagingConfigForMachine = NULL;
__declspec(selectany) wil_PStagingConfig g_wil_details_stagingConfigForUser = NULL;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

#ifndef WIL_NtQueryWnfStateData
#ifdef _KERNEL_MODE
#define WIL_NtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize) ZwQueryWnfStateData((PCWNF_STATE_NAME)StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize)
WI_ODR_PRAGMA("WIL_NtQueryWnfStateData", "2")
#else
inline NTSTATUS NTAPI wil_details_NtQueryWnfStateData(_In_ __WIL_PCWNF_STATE_NAME StateName, _In_opt_ __WIL_PCWNF_TYPE_ID TypeId, _In_opt_ const VOID* ExplicitScope,
    _Out_ __WIL_PWNF_CHANGE_STAMP ChangeStamp, _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer, _Inout_ PULONG BufferSize)
{
    if (!g_wil_details_pfnNtQueryWnfStateData)
    {
        g_wil_details_pfnNtQueryWnfStateData = (__WIL_PWNF_NtQueryWnfStateData)(GetProcAddress(wil_details_GetNtDllModuleHandle(), "NtQueryWnfStateData"));
    }
    return g_wil_details_pfnNtQueryWnfStateData ? g_wil_details_pfnNtQueryWnfStateData(StateName, TypeId, ExplicitScope, ChangeStamp, Buffer, BufferSize) : STATUS_ENTRYPOINT_NOT_FOUND;
}
#define WIL_NtQueryWnfStateData wil_details_NtQueryWnfStateData
WI_ODR_PRAGMA("WIL_NtQueryWnfStateData", "1")
#endif
#else
WI_ODR_PRAGMA("WIL_NtQueryWnfStateData", "0")
#endif

#ifndef WIL_NtUpdateWnfStateData
#ifdef _KERNEL_MODE
#define WIL_NtUpdateWnfStateData(StateName, Buffer, Length, TypeId, ExplicitScope, MatchingChangeStamp, CheckStamp) ZwUpdateWnfStateData((PCWNF_STATE_NAME)StateName, Buffer, Length, TypeId, ExplicitScope, MatchingChangeStamp, CheckStamp)
WI_ODR_PRAGMA("WIL_NtUpdateWnfStateData", "2")
#else
inline NTSTATUS NTAPI wil_details_NtUpdateWnfStateData(_In_ __WIL_PCWNF_STATE_NAME StateName, _In_reads_bytes_opt_(Length) const VOID* Buffer, _In_opt_ ULONG Length,
    _In_opt_ __WIL_PCWNF_TYPE_ID TypeId, _In_opt_ const VOID* ExplicitScope, _In_ __WIL_WNF_CHANGE_STAMP MatchingChangeStamp, _In_ __WIL_LOGICAL CheckStamp)
{
    if (!g_wil_details_pfnNtUpdateWnfStateData)
    {
        g_wil_details_pfnNtUpdateWnfStateData = (__WIL_PWNF_NtUpdateWnfStateData)(GetProcAddress(wil_details_GetNtDllModuleHandle(), "NtUpdateWnfStateData"));
    }
    return g_wil_details_pfnNtUpdateWnfStateData ? g_wil_details_pfnNtUpdateWnfStateData(StateName, Buffer, Length, TypeId, ExplicitScope, MatchingChangeStamp, CheckStamp) : STATUS_ENTRYPOINT_NOT_FOUND;
}
#define WIL_NtUpdateWnfStateData wil_details_NtUpdateWnfStateData
WI_ODR_PRAGMA("WIL_NtUpdateWnfStateData", "1")
#endif
#else
WI_ODR_PRAGMA("WIL_NtUpdateWnfStateData", "0")
#endif

#ifndef WIL_RtlSubscribeWnfStateChangeNotification
#ifdef _KERNEL_MODE
#define WIL_RtlSubscribeWnfStateChangeNotification(Subscription, StateName, ChangeStamp, Callback, CallbackContext, TypeId, SerializationGroupIndex, DeliveryOptions) STATUS_ENTRYPOINT_NOT_FOUND
WI_ODR_PRAGMA("WIL_RtlSubscribeWnfStateChangeNotification", "2")
#else
inline NTSTATUS NTAPI wil_details_RtlSubscribeWnfStateChangeNotification(_Outptr_ __WIL_PWNF_USER_SUBSCRIPTION* Subscription, _In_ __WIL_WNF_STATE_NAME StateName,
    __WIL_WNF_CHANGE_STAMP ChangeStamp, _In_ __WIL_PWNF_USER_CALLBACK Callback, _In_opt_ PVOID CallbackContext, _In_opt_ __WIL_PWNF_TYPE_ID TypeId,
    _In_opt_ ULONG SerializationGroupIndex, _In_opt_ ULONG DeliveryOptions)
{
    if (!g_wil_details_pfnRtlSubscribeWnfStateChangeNotification)
    {
        g_wil_details_pfnRtlSubscribeWnfStateChangeNotification = (__WIL_PWNF_RtlSubscribeWnfStateChangeNotification)(GetProcAddress(wil_details_GetNtDllModuleHandle(), "RtlSubscribeWnfStateChangeNotification"));
    }
    return g_wil_details_pfnRtlSubscribeWnfStateChangeNotification ? g_wil_details_pfnRtlSubscribeWnfStateChangeNotification(Subscription, StateName, ChangeStamp, Callback, CallbackContext, TypeId, SerializationGroupIndex, DeliveryOptions) : STATUS_ENTRYPOINT_NOT_FOUND;
}
#define WIL_RtlSubscribeWnfStateChangeNotification wil_details_RtlSubscribeWnfStateChangeNotification
WI_ODR_PRAGMA("WIL_RtlSubscribeWnfStateChangeNotification", "1")
#endif
#else
WI_ODR_PRAGMA("WIL_RtlSubscribeWnfStateChangeNotification", "0")
#endif

#ifndef WIL_RtlUnsubscribeWnfNotificationWaitForCompletion
#ifdef _KERNEL_MODE
#define WIL_RtlUnsubscribeWnfNotificationWaitForCompletion(Subscription) STATUS_ENTRYPOINT_NOT_FOUND
WI_ODR_PRAGMA("WIL_RtlUnsubscribeWnfNotificationWaitForCompletion", "2")
#else
inline NTSTATUS NTAPI wil_details_RtlUnsubscribeWnfNotificationWaitForCompletion(_In_ _Post_invalid_ __WIL_PWNF_USER_SUBSCRIPTION Subscription)
{
    if (!g_wil_details_pfnRtlUnsubscribeWnfNotificationWaitForCompletion)
    {
        g_wil_details_pfnRtlUnsubscribeWnfNotificationWaitForCompletion = (__WIL_PWNF_RtlUnsubscribeWnfNotificationWaitForCompletion)(GetProcAddress(wil_details_GetNtDllModuleHandle(), "RtlUnsubscribeWnfNotificationWaitForCompletion"));
    }
    return g_wil_details_pfnRtlUnsubscribeWnfNotificationWaitForCompletion ? g_wil_details_pfnRtlUnsubscribeWnfNotificationWaitForCompletion(Subscription) : STATUS_ENTRYPOINT_NOT_FOUND;
}
#define WIL_RtlUnsubscribeWnfNotificationWaitForCompletion wil_details_RtlUnsubscribeWnfNotificationWaitForCompletion
WI_ODR_PRAGMA("WIL_RtlUnsubscribeWnfNotificationWaitForCompletion", "1")
#endif
#else
WI_ODR_PRAGMA("WIL_RtlUnsubscribeWnfNotificationWaitForCompletion", "0")
#endif

#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

__WIL_CONSTANT const SIZE_T c_wil_details_maxStagingConfigSize = 4096;                 // MAX WNF size

__WIL_CONSTANT const unsigned char c_wil_details_stagingConfigVersion = 2;             // cannot read configs with version changes
__WIL_CONSTANT const unsigned char c_wil_details_stagingConfigMinorVersion = 2;        // minor version changes maintain compatibility (for reading)

typedef struct wil_details_StagingConfigHeaderProperties
{
    unsigned ignoreServiceState : 1;
    unsigned ignoreUserState : 1;
    unsigned ignoreTestState : 1;
    unsigned ignoreVariants : 1;
    unsigned unused : 28;
} wil_details_StagingConfigHeaderProperties;

typedef struct wil_details_StagingConfigHeader
{
    unsigned char version;
    unsigned char versionMinor;
    unsigned short headerSizeBytes;

    unsigned short featureCount;
    unsigned short featureUsageTriggerCount;

    wil_details_StagingConfigHeaderProperties sessionProperties;        // properties that were changed in this session
    wil_details_StagingConfigHeaderProperties properties;               // cache of the properties as they were prior to this session (for reboot focused features)
} wil_details_StagingConfigHeader;

typedef struct wil_details_StagingConfigWnfStateName
{
    unsigned int Data[2];
} wil_details_StagingConfigWnfStateName;

typedef struct wil_details_StagingConfigUsageTrigger
{
    unsigned int featureId;
    wil_details_StagingConfigWnfStateName trigger;
    unsigned serviceReportingKind : 16;
    unsigned isVariantConfig : 1;
    unsigned unused : 15;
} wil_details_StagingConfigUsageTrigger;
static_assert(sizeof(wil_details_StagingConfigUsageTrigger) == 16, "No accidental size changes");

typedef struct wil_details_StagingConfigFeature
{
    unsigned int featureId;

    unsigned changedInSession : 1;  // '1' if changed in the same session the configuration was last written
    unsigned isVariantConfig : 1;   // wil_FeatureEnabledStateOptions
    unsigned unused1 : 6;

    unsigned serviceState : 2;      // wil_FeatureEnabledState
    unsigned userState : 2;         // wil_FeatureEnabledState
    unsigned testState : 2;         // wil_FeatureEnabledState
    unsigned unused2 : 2;

    unsigned unused3 : 8;

    unsigned variant : 6;           // tied to c_wil_details_ServiceReportingKind_VariantMax
    unsigned payloadKind : 2;       // wil_FeatureVariantPayloadKind

    unsigned int payload;                          // Variant ID for External, Actual payload for Resident
} wil_details_StagingConfigFeature;
static_assert(sizeof(wil_details_StagingConfigFeature) == 12, "No accidental size changes");

typedef BOOL NTAPI __WIL_StagingConfigFeatureEnumeration(_Inout_ wil_details_StagingConfigFeature* feature, void* context);
typedef __WIL_StagingConfigFeatureEnumeration *__WIL_PStagingConfigFeatureEnumeration;

typedef enum wil_details_StagingConfigFeatureFields
{
    wil_details_StagingConfigFeatureFields_None = 0x00,
    wil_details_StagingConfigFeatureFields_ServiceState = 0x01,
    wil_details_StagingConfigFeatureFields_UserState = 0x02,
    wil_details_StagingConfigFeatureFields_TestState = 0x04,
    wil_details_StagingConfigFeatureFields_Variant = 0x08,
} wil_details_StagingConfigFeatureFields;
DEFINE_ENUM_FLAG_OPERATORS(wil_details_StagingConfigFeatureFields);

typedef struct wil_details_StagingConfig
{
    wil_FeatureStore store;
    BOOL forUpdate;
    ULONG readChangeStamp;
    unsigned char readVersion;
    BOOL modified;

    wil_details_StagingConfigHeader* header;
    wil_details_StagingConfigFeature* features;
    wil_details_StagingConfigUsageTrigger* triggers;
    BOOL changedInSession;

    void* buffer;
    SIZE_T bufferSize;
    SIZE_T bufferAlloc;
    BOOL bufferOwned;
} wil_details_StagingConfig;

typedef BOOL NTAPI __WIL_FeatureEnabledQuery();
typedef __WIL_FeatureEnabledQuery *__WIL_PFeatureEnabledQuery;
#endif // WIL_SUPPRESS_PRIVATE_API_USE

__WIL_CONSTANT const unsigned int c_wil_details_maxUsageCount = 0x000001FF;            // Tied to wil_details_FeatureProperties::usageCount
__WIL_CONSTANT const unsigned int c_wil_details_maxOpportunityCount = 0x0000007F;      // Tied to wil_details_FeatureProperties::opportunityCount

typedef struct wil_details_FeatureProperties
{
    // begin - must match wil_details_VariantProperties
    unsigned enabledState : 2;
    unsigned isVariant : 1;
    unsigned queuedForReporting : 1;
    unsigned hasNotificationState : 2;
    // end
    unsigned usageCount : 9;                               // Tied to c_wil_details_maxUsageCount
    unsigned usageCountRepresentsPotential : 1;
    unsigned reportedDeviceUsage : 1;
    unsigned reportedDevicePotential : 1;
    unsigned reportedDeviceOpportunity : 1;
    unsigned reportedDevicePotentialOpportunity : 1;
    unsigned recordedDeviceUsage : 1;
    unsigned recordedDevicePotential : 1;
    unsigned recordedDeviceOpportunity : 1;
    unsigned recordedDevicePotentialOpportunity : 1;
    unsigned opportunityCount : 7;                          // Tied to c_wil_details_maxOpportunityCount
    unsigned opportunityCountRepresentsPotential : 1;
} wil_details_FeatureProperties;

typedef struct wil_details_VariantProperties
{
    // begin - must match wil_details_FeatureProperties
    unsigned enabledState : 2;
    unsigned isVariant : 1;
    unsigned queuedForReporting : 1;
    unsigned hasNotificationState : 2;
    // end
    unsigned recordedDeviceUsage : 1;
    unsigned variant : 6;
    unsigned unused : 19;
} wil_details_VariantProperties;

// Stored on a per-feature level to cache various properties about a feature for fast access
typedef union wil_details_FeaturePropertyCache
{
    wil_details_FeatureProperties cache;
    wil_details_VariantProperties variant;
    volatile long var;
} wil_details_FeaturePropertyCache;

typedef struct wil_details_FeatureUsageSRUM
{
    unsigned int featureId;
    unsigned short serviceReportingKind;
    DWORD usageCount;
} wil_details_FeatureUsageSRUM;

typedef struct wil_details_FeatureVariantPropertyCache
{
    wil_details_FeaturePropertyCache propertyCache;
    unsigned int payloadId;
} wil_details_FeatureVariantPropertyCache;

typedef BOOL NTAPI __WIL_FeaturePropertyCacheModification(_Inout_ wil_details_FeaturePropertyCache* cache, void* context);
typedef __WIL_FeaturePropertyCacheModification *__WIL_PFeaturePropertyCacheModification;

typedef void NTAPI __WIL_FeaturePropertyCacheChangeNotification(_Inout_opt_ wil_details_FeaturePropertyCache* cache, wil_FeatureChangeTime changeTime);
typedef __WIL_FeaturePropertyCacheChangeNotification *__WIL_PFeaturePropertyCacheChangeNotification;

__declspec(selectany) __WIL_PFeaturePropertyCacheChangeNotification g_wil_details_featurePropertyCacheChangeNotification = NULL;

__WIL_CONSTANT const unsigned c_wil_details_ServiceReportingKind_VariantMax = 64;

typedef enum wil_details_ServiceReportingKind
{
    wil_details_ServiceReportingKind_UniqueUsage = 0,                       // DO NOT CHANGE ANY ENUMERATION VALUE
    wil_details_ServiceReportingKind_UniqueOpportunity = 1,                 // this is a service/reporting contract
    wil_details_ServiceReportingKind_DeviceUsage = 2,
    wil_details_ServiceReportingKind_DeviceOpportunity = 3,
    wil_details_ServiceReportingKind_PotentialUniqueUsage = 4,
    wil_details_ServiceReportingKind_PotentialUniqueOpportunity = 5,
    wil_details_ServiceReportingKind_PotentialDeviceUsage = 6,
    wil_details_ServiceReportingKind_PotentialDeviceOpportunity = 7,
    wil_details_ServiceReportingKind_EnabledTotalDuration = 8,
    wil_details_ServiceReportingKind_EnabledPausedDuration = 9,
    wil_details_ServiceReportingKind_DisabledTotalDuration = 10,
    wil_details_ServiceReportingKind_DisabledPausedDuration = 11,
    wil_details_ServiceReportingKind_CustomEnabledBase = 100,               // Custom data points for enabled features (100-149)
    wil_details_ServiceReportingKind_CustomDisabledBase = 150,              // Matching custom data point for tracking disabled features (150-199)
    wil_details_ServiceReportingKind_Store = 254,                           // Flush changes to disk
    wil_details_ServiceReportingKind_None = 255,                            // Indicate no change
    wil_details_ServiceReportingKind_VariantDevicePotentialBase = 256,      // 256 - 319 (inclusive) variant could have been used on device
    wil_details_ServiceReportingKind_VariantDeviceUsageBase = 320,          // 320 - 383 (inclusive) variant was used on device
    wil_details_ServiceReportingKind_VariantUniquePotentialBase = 384,      // 384 - 447 (inclusive) variant could have been used N times on device
    wil_details_ServiceReportingKind_VariantUniqueUsageBase = 448,          // 448 - 511 (inclusive) variant was used N times on device
} wil_details_ServiceReportingKind;

typedef enum wil_details_ServiceReportingOptions
{
    wil_details_ServiceReportingOptions_None = 0x00,
    wil_details_ServiceReportingOptions_VariantConfig = 0x01,
} wil_details_ServiceReportingOptions;
DEFINE_ENUM_FLAG_OPERATORS(wil_details_ServiceReportingOptions);

// Represent the type we cache to represent a feature's current state...
typedef enum wil_details_CachedFeatureEnabledState
{
    wil_details_CachedFeatureEnabledState_Unknown = 0,
    wil_details_CachedFeatureEnabledState_Disabled = 1,
    wil_details_CachedFeatureEnabledState_Enabled = 2,
    wil_details_CachedFeatureEnabledState_Desired = 3
    // Cannot add another state without updating wil_details_FeatureProperties
} wil_details_CachedFeatureEnabledState;

// Cached state of whether or not a feature has any subscriptions
typedef enum wil_details_CachedHasNotificationState
{
    wil_details_CachedHasNotificationState_Unknown = 0,
    wil_details_CachedHasNotificationState_DoesNotHaveNotifications = 1,
    wil_details_CachedHasNotificationState_HasNotification = 2
} wil_details_CachedHasNotificationState;

typedef struct wil_details_RecordUsageResult
{
    BOOL queueBackground;                           // true if wil_details_FeaturePropertyCache should be queued recording through a background thread
    unsigned int countImmediate;                    // > 0 for usage that needs immediate recording
    wil_details_ServiceReportingKind kindImmediate; // kind to report (when count > 0)
    unsigned int payloadId;                         // included when there is a relevant variant payload ID
    BOOL ignoredUse;                                // TRUE when we ignored usage we've already seen before
    BOOL isVariantConfiguration;                    // report usage to variant configuration subscriptions
} wil_details_RecordUsageResult;

typedef void NTAPI __WIL_RecordFeatureUsage(unsigned int featureId, wil_details_ServiceReportingKind kind, unsigned int addend, _In_opt_ wil_details_FeaturePropertyCache* cache, _In_ wil_details_RecordUsageResult* result);
typedef __WIL_RecordFeatureUsage *__WIL_PRecordFeatureUsage;

__declspec(selectany) __WIL_PRecordFeatureUsage g_wil_details_recordFeatureUsage = NULL;

typedef FEATURE_ENABLED_STATE NTAPI __WILAPI_GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime);
typedef __WILAPI_GetFeatureEnabledState *__WILAPI_PGetFeatureEnabledState;

__declspec(selectany) __WILAPI_PGetFeatureEnabledState g_wil_details_internalGetFeatureEnabledState = NULL;
__declspec(selectany) __WILAPI_PGetFeatureEnabledState g_wil_details_apiGetFeatureEnabledState = NULL;

typedef void NTAPI __WILAPI_RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, _In_ PCSTR originName);
typedef __WILAPI_RecordFeatureUsage *__WILAPI_PRecordFeatureUsage;

__declspec(selectany) __WILAPI_PRecordFeatureUsage g_wil_details_internalRecordFeatureUsage = NULL;
__declspec(selectany) __WILAPI_PRecordFeatureUsage g_wil_details_apiRecordFeatureUsage = NULL;

typedef void NTAPI __WILAPI_RecordSRUMFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend);
typedef __WILAPI_RecordSRUMFeatureUsage *__WILAPI_PRecordSRUMFeatureUsage;

#if !defined(WIL_DISABLE_SRUM_WNF)
__declspec(selectany) __WILAPI_PRecordSRUMFeatureUsage g_wil_details_RecordSRUMFeatureUsage = NULL;
#endif // WIL_DISABLE_SRUM_WNF

typedef void NTAPI __WILAPI_RecordFeatureError(UINT32 featureId, const FEATURE_ERROR* error);
typedef __WILAPI_RecordFeatureError *__WILAPI_PRecordFeatureError;

__declspec(selectany) __WILAPI_PRecordFeatureError g_wil_details_internalRecordFeatureError = NULL;
__declspec(selectany) __WILAPI_PRecordFeatureError g_wil_details_apiRecordFeatureError = NULL;

typedef void NTAPI __WILAPI_SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context);
typedef __WILAPI_SubscribeFeatureStateChangeNotification *__WILAPI_PSubscribeFeatureStateChangeNotification;

__declspec(selectany) __WILAPI_PSubscribeFeatureStateChangeNotification g_wil_details_internalSubscribeFeatureStateChangeNotification = NULL;
__declspec(selectany) __WILAPI_PSubscribeFeatureStateChangeNotification g_wil_details_apiSubscribeFeatureStateChangeNotification = NULL;

typedef void NTAPI __WILAPI_UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription);
typedef __WILAPI_UnsubscribeFeatureStateChangeNotification *__WILAPI_PUnsubscribeFeatureStateChangeNotification;

__declspec(selectany) __WILAPI_PUnsubscribeFeatureStateChangeNotification g_wil_details_internalUnsubscribeFeatureStateChangeNotification = NULL;
__declspec(selectany) __WILAPI_PUnsubscribeFeatureStateChangeNotification g_wil_details_apiUnsubscribeFeatureStateChangeNotification = NULL;

typedef UINT32 NTAPI __WILAPI_GetFeatureVariant(UINT32 featureId, FEATURE_CHANGE_TIME changeTime, _Out_ UINT32* payloadId, _Out_ BOOL* hasNotification);
typedef __WILAPI_GetFeatureVariant *__WILAPI_PGetFeatureVariant;

__declspec(selectany) __WILAPI_PGetFeatureVariant g_wil_details_internalGetFeatureVariant = NULL;
__declspec(selectany) __WILAPI_PGetFeatureVariant g_wil_details_apiGetFeatureVariant = NULL;

inline BOOL NTAPI wil_details_SetEnabledAndHasNotificationStateCallback(_Inout_ wil_details_FeaturePropertyCache* data, void* context)
{
    unsigned char state = (unsigned char)(ULONG_PTR)context;
    unsigned char hasNotificationState = (unsigned char)(((ULONG_PTR)context) >> 8);
    unsigned char isVariantConfiguration = (unsigned char)(((ULONG_PTR)context) >> 16);
    if (data->cache.enabledState == state && data->cache.hasNotificationState == hasNotificationState && data->cache.isVariant == isVariantConfiguration)
    {
        return FALSE;
    }
    data->cache.enabledState = state;
    data->cache.hasNotificationState = hasNotificationState;
    data->cache.isVariant = isVariantConfiguration;
    return TRUE;
}

inline BOOL NTAPI wil_details_SetHasNotificationStateCallback(_Inout_ wil_details_FeaturePropertyCache* data, void* context)
{
    unsigned char hasNotificationState = (unsigned char)(ULONG_PTR)context;
    if (data->cache.hasNotificationState == hasNotificationState)
    {
        return FALSE;
    }
    data->cache.hasNotificationState = hasNotificationState;
    return TRUE;
}

typedef struct wil_details_SetPropertyFlagContext
{
    wil_details_RecordUsageResult* result;
    unsigned long flags;
    BOOL ignoreReporting;
} wil_details_SetPropertyFlagContext;

inline BOOL NTAPI wil_details_SetPropertyFlagCallback(_Inout_ wil_details_FeaturePropertyCache* data, void* contextParam)
{
    wil_details_SetPropertyFlagContext* context = (wil_details_SetPropertyFlagContext*)contextParam;
    context->result->queueBackground = FALSE;
    if ((context->flags & data->var) == context->flags)
    {
        return FALSE;
    }
    data->var |= context->flags;
    if (!context->ignoreReporting && !data->cache.queuedForReporting)
    {
        data->cache.queuedForReporting = 1;
        context->result->queueBackground = TRUE;
    }
    return TRUE;
}

typedef struct wil_details_SetPropertyCacheUsageContext
{
    wil_details_RecordUsageResult* result;
    wil_details_ServiceReportingKind kind;
    SIZE_T addend;
} wil_details_SetPropertyCacheUsageContext;

inline BOOL NTAPI wil_details_SetPropertyCacheUsageCallback(_Inout_ wil_details_FeaturePropertyCache* data, void* contextParam)
{
    wil_details_SetPropertyCacheUsageContext* context = (wil_details_SetPropertyCacheUsageContext*)contextParam;
    context->result->countImmediate = 0;
    context->result->queueBackground = (!data->cache.queuedForReporting);
    data->cache.queuedForReporting = 1;
    if ((data->cache.usageCountRepresentsPotential != 0) != (context->kind == wil_details_ServiceReportingKind_PotentialUniqueUsage))
    {
        if (data->cache.usageCount > 0)
        {
            context->result->kindImmediate = (context->kind == wil_details_ServiceReportingKind_UniqueUsage) ? wil_details_ServiceReportingKind_PotentialUniqueUsage : wil_details_ServiceReportingKind_UniqueUsage;
            context->result->countImmediate = data->cache.usageCount;
            data->cache.usageCount = 0;
        }
        data->cache.usageCountRepresentsPotential = (context->kind == wil_details_ServiceReportingKind_PotentialUniqueUsage) ? 1 : 0;
    }
    SIZE_T newCount = data->cache.usageCount + context->addend;
    if ((newCount > c_wil_details_maxUsageCount) || (newCount < data->cache.usageCount))
    {
        newCount = context->addend;
        context->result->kindImmediate = context->kind;
        context->result->countImmediate = data->cache.usageCount;
    }
    data->cache.usageCount = (unsigned int)newCount;
    return TRUE;
}

inline BOOL NTAPI wil_details_SetPropertyCacheOpportunityCallback(_Inout_ wil_details_FeaturePropertyCache* data, void* contextParam)
{
    wil_details_SetPropertyCacheUsageContext* context = (wil_details_SetPropertyCacheUsageContext*)contextParam;
    context->result->countImmediate = 0;
    context->result->queueBackground = (!data->cache.queuedForReporting);
    data->cache.queuedForReporting = 1;
    if ((data->cache.opportunityCountRepresentsPotential != 0) != (context->kind == wil_details_ServiceReportingKind_PotentialUniqueOpportunity))
    {
        if (data->cache.opportunityCount > 0)
        {
            context->result->kindImmediate = (context->kind == wil_details_ServiceReportingKind_UniqueOpportunity) ? wil_details_ServiceReportingKind_PotentialUniqueOpportunity : wil_details_ServiceReportingKind_UniqueOpportunity;
            context->result->countImmediate = data->cache.opportunityCount;
            data->cache.opportunityCount = 0;
        }
        data->cache.opportunityCountRepresentsPotential = (context->kind == wil_details_ServiceReportingKind_PotentialUniqueOpportunity) ? 1 : 0;
    }
    SIZE_T newCount = data->cache.opportunityCount + context->addend;
    if ((newCount > c_wil_details_maxOpportunityCount) || (newCount < data->cache.opportunityCount))
    {
        newCount = context->addend;
        context->result->kindImmediate = context->kind;
        context->result->countImmediate = data->cache.opportunityCount;
    }
    data->cache.opportunityCount = (unsigned int)newCount;
    return TRUE;
}

inline BOOL wil_details_ModifyFeatureData(_Inout_ wil_details_FeaturePropertyCache* properties,
    __WIL_PFeaturePropertyCacheModification modifyFunction, void* context)
{
    wil_details_FeaturePropertyCache data;
    long var;
    do
    {
        data = *properties;
        var = data.var;
        if (!modifyFunction(&data, context))
        {
            return FALSE;
        }
    }
    while (InterlockedCompareExchange(&properties->var, data.var, var) != var);
    return TRUE;
}

inline void wil_details_SetEnabledAndHasNotificationStateProperties(_Inout_ wil_details_FeaturePropertyCache* properties,
    wil_details_CachedFeatureEnabledState state, wil_details_CachedHasNotificationState hasNotificationState, unsigned int isVariantConfiguration)
{
    UINT_PTR context = (ULONG_PTR)(state | (hasNotificationState << 8) | (isVariantConfiguration << 16));
    wil_details_ModifyFeatureData(properties, &wil_details_SetEnabledAndHasNotificationStateCallback, (void*)context);
}

inline void wil_details_SetHasNotificationStateProperty(_Inout_ wil_details_FeaturePropertyCache* properties,
    wil_details_CachedHasNotificationState hasNotificationState)
{
    UINT_PTR context = ((ULONG_PTR)hasNotificationState);
    wil_details_ModifyFeatureData(properties, &wil_details_SetHasNotificationStateCallback, (void*)context);
}

inline wil_details_ServiceReportingKind wil_details_MapReportingKind(wil_ReportingKind kind, BOOL enabled)
{
    switch (kind)
    {
    case wil_ReportingKind_None:
        return wil_details_ServiceReportingKind_None;
    case wil_ReportingKind_UniqueUsage:
        return enabled ? wil_details_ServiceReportingKind_UniqueUsage : wil_details_ServiceReportingKind_PotentialUniqueUsage;
    case wil_ReportingKind_UniqueOpportunity:
        return enabled ? wil_details_ServiceReportingKind_UniqueOpportunity : wil_details_ServiceReportingKind_PotentialUniqueOpportunity;
    case wil_ReportingKind_DeviceUsage:
        return enabled ? wil_details_ServiceReportingKind_DeviceUsage : wil_details_ServiceReportingKind_PotentialDeviceUsage;
    case wil_ReportingKind_DeviceOpportunity:
        return enabled ? wil_details_ServiceReportingKind_DeviceOpportunity : wil_details_ServiceReportingKind_PotentialDeviceOpportunity;
    case wil_ReportingKind_TotalDuration:
        return enabled ? wil_details_ServiceReportingKind_EnabledTotalDuration : wil_details_ServiceReportingKind_DisabledTotalDuration;
    case wil_ReportingKind_PausedDuration:
        return enabled ? wil_details_ServiceReportingKind_EnabledPausedDuration : wil_details_ServiceReportingKind_DisabledPausedDuration;
    default:    // product of GetCustomReportingKind
        if (((unsigned char)kind >= c_wil_details_ReportingKind_CustomBase) && ((unsigned char)kind < (c_wil_details_ReportingKind_CustomBase + c_wil_details_maxCustomUsageReporting)))
        {
            unsigned char index = (unsigned char)kind - c_wil_details_ReportingKind_CustomBase;
            return enabled ? (wil_details_ServiceReportingKind)(wil_details_ServiceReportingKind_CustomEnabledBase + index) :
                (wil_details_ServiceReportingKind)(wil_details_ServiceReportingKind_CustomDisabledBase + index);
        }
    }
    return wil_details_ServiceReportingKind_None;
}

inline wil_details_ServiceReportingKind wil_details_MapVariantReportingKind(wil_VariantReportingKind kind, BOOL enabled, unsigned char variant)
{
    switch (kind)
    {
    case wil_VariantReportingKind_DeviceUsage:
        return (wil_details_ServiceReportingKind)((enabled ? wil_details_ServiceReportingKind_VariantDeviceUsageBase :
            wil_details_ServiceReportingKind_VariantDevicePotentialBase) + variant);
    case wil_VariantReportingKind_UniqueUsage:
        return (wil_details_ServiceReportingKind)((enabled ? wil_details_ServiceReportingKind_VariantUniqueUsageBase :
            wil_details_ServiceReportingKind_VariantUniquePotentialBase) + variant);
    }
    return wil_details_ServiceReportingKind_None;
}

inline void wil_details_MapServiceReportingKindToReportingKind(wil_details_ServiceReportingKind serviceReportingKind, _Out_ wil_ReportingKind* kind, _Out_ BOOL* enabled)
{
    switch (serviceReportingKind)
    {
    case wil_details_ServiceReportingKind_UniqueUsage:
    case wil_details_ServiceReportingKind_PotentialUniqueUsage:
        *kind = wil_ReportingKind_UniqueUsage;
        break;

    case wil_details_ServiceReportingKind_UniqueOpportunity:
    case wil_details_ServiceReportingKind_PotentialUniqueOpportunity:
        *kind = wil_ReportingKind_UniqueOpportunity;
        break;

    case wil_details_ServiceReportingKind_DeviceUsage:
    case wil_details_ServiceReportingKind_PotentialDeviceUsage:
        *kind = wil_ReportingKind_DeviceUsage;
        break;

    case wil_details_ServiceReportingKind_DeviceOpportunity:
    case wil_details_ServiceReportingKind_PotentialDeviceOpportunity:
        *kind = wil_ReportingKind_DeviceOpportunity;
        break;

    case wil_details_ServiceReportingKind_EnabledTotalDuration:
    case wil_details_ServiceReportingKind_DisabledTotalDuration:
        *kind = wil_ReportingKind_TotalDuration;
        break;

    case wil_details_ServiceReportingKind_EnabledPausedDuration:
    case wil_details_ServiceReportingKind_DisabledPausedDuration:
        *kind = wil_ReportingKind_PausedDuration;
        break;

    default: // GetCustomReportingKind
        if (serviceReportingKind >= wil_details_ServiceReportingKind_CustomEnabledBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_CustomEnabledBase + c_wil_details_maxCustomUsageReporting)
        {
            *kind = (wil_ReportingKind)(serviceReportingKind - wil_details_ServiceReportingKind_CustomEnabledBase + c_wil_details_ReportingKind_CustomBase);
        }
        else if (serviceReportingKind >= wil_details_ServiceReportingKind_CustomDisabledBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_CustomDisabledBase + c_wil_details_maxCustomUsageReporting)
        {
            *kind = (wil_ReportingKind)(serviceReportingKind - wil_details_ServiceReportingKind_CustomDisabledBase + c_wil_details_ReportingKind_CustomBase);
        }
        else
        {
            *kind = wil_ReportingKind_None;
        }
        break;
    }

    switch (serviceReportingKind)
    {
    case wil_details_ServiceReportingKind_UniqueUsage:
    case wil_details_ServiceReportingKind_UniqueOpportunity:
    case wil_details_ServiceReportingKind_DeviceUsage:
    case wil_details_ServiceReportingKind_DeviceOpportunity:
    case wil_details_ServiceReportingKind_EnabledTotalDuration:
    case wil_details_ServiceReportingKind_EnabledPausedDuration:
        *enabled = TRUE;
        break;

    default:
        if (serviceReportingKind >= wil_details_ServiceReportingKind_CustomEnabledBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_CustomEnabledBase + c_wil_details_maxCustomUsageReporting)
        {
            *enabled = TRUE;
        }
        else
        {
            *enabled = FALSE;
        }
        break;
    }
}

inline void wil_details_MapServiceReportingKindToVariantReportingKind(wil_details_ServiceReportingKind serviceReportingKind, _Out_ wil_VariantReportingKind* kind, _Out_ BOOL* enabled, _Out_ unsigned char* variant)
{
    if (serviceReportingKind >= wil_details_ServiceReportingKind_VariantDevicePotentialBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_VariantDevicePotentialBase + c_wil_details_ServiceReportingKind_VariantMax)
    {
        *kind = wil_VariantReportingKind_DeviceUsage;
        *enabled = FALSE;
        *variant = (unsigned char)(serviceReportingKind - wil_details_ServiceReportingKind_VariantDevicePotentialBase);
    }
    else if (serviceReportingKind >= wil_details_ServiceReportingKind_VariantDeviceUsageBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_VariantDeviceUsageBase + c_wil_details_ServiceReportingKind_VariantMax)
    {
        *kind = wil_VariantReportingKind_DeviceUsage;
        *enabled = TRUE;
        *variant = (unsigned char)(serviceReportingKind - wil_details_ServiceReportingKind_VariantDeviceUsageBase);
    }
    else if (serviceReportingKind >= wil_details_ServiceReportingKind_VariantUniquePotentialBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_VariantUniquePotentialBase + c_wil_details_ServiceReportingKind_VariantMax)
    {
        *kind = wil_VariantReportingKind_UniqueUsage;
        *enabled = FALSE;
        *variant = (unsigned char)(serviceReportingKind - wil_details_ServiceReportingKind_VariantUniquePotentialBase);
    }
    else if (serviceReportingKind >= wil_details_ServiceReportingKind_VariantUniqueUsageBase && (size_t)serviceReportingKind < wil_details_ServiceReportingKind_VariantUniqueUsageBase + c_wil_details_ServiceReportingKind_VariantMax)
    {
        *kind = wil_VariantReportingKind_UniqueUsage;
        *enabled = TRUE;
        *variant = (unsigned char)(serviceReportingKind - wil_details_ServiceReportingKind_VariantUniqueUsageBase);
    }
    else
    {
        *kind = wil_VariantReportingKind_None;
        *enabled = FALSE;
        *variant = 0;
    }
}

inline BOOL wil_details_DoesServiceReportingKindRepresentVariantUsage(wil_details_ServiceReportingKind kind)
{
    if (kind >= wil_details_ServiceReportingKind_VariantDevicePotentialBase && (size_t)kind < (wil_details_ServiceReportingKind_VariantUniqueUsageBase + c_wil_details_ServiceReportingKind_VariantMax))
    {
        return TRUE;
    }
    return FALSE;
}

inline wil_details_RecordUsageResult wil_details_RecordUsageInPropertyCache(_Inout_ wil_details_FeaturePropertyCache* properties,
    wil_details_ServiceReportingKind kind, unsigned int payloadId, unsigned int addend)
{
    wil_details_RecordUsageResult result;
    RtlZeroMemory(&result, sizeof(result));
    switch (kind)
    {
    case wil_details_ServiceReportingKind_DeviceUsage:
    case wil_details_ServiceReportingKind_DeviceOpportunity:
    case wil_details_ServiceReportingKind_PotentialDeviceUsage:
    case wil_details_ServiceReportingKind_PotentialDeviceOpportunity:
        {
            wil_details_FeaturePropertyCache match = { 0 };
            switch (kind)
            {
            case wil_details_ServiceReportingKind_DeviceUsage:
                match.cache.reportedDeviceUsage = 1;
                break;
            case wil_details_ServiceReportingKind_DeviceOpportunity:
                match.cache.reportedDeviceOpportunity = 1;
                break;
            case wil_details_ServiceReportingKind_PotentialDeviceUsage:
                match.cache.reportedDevicePotential = 1;
                break;
            case wil_details_ServiceReportingKind_PotentialDeviceOpportunity:
                match.cache.reportedDevicePotentialOpportunity = 1;
                break;
            };

            wil_details_SetPropertyFlagContext context;
            context.result = &result;
            context.flags = match.var;
            context.ignoreReporting = FALSE;
            result.ignoredUse = !wil_details_ModifyFeatureData(properties, &wil_details_SetPropertyFlagCallback, &context);
        }
        break;

    case wil_details_ServiceReportingKind_UniqueUsage:
    case wil_details_ServiceReportingKind_PotentialUniqueUsage:
    case wil_details_ServiceReportingKind_UniqueOpportunity:
    case wil_details_ServiceReportingKind_PotentialUniqueOpportunity:
        {
            const BOOL usage = ((kind == wil_details_ServiceReportingKind_UniqueUsage) || (kind == wil_details_ServiceReportingKind_PotentialUniqueUsage));
            wil_details_SetPropertyCacheUsageContext context;
            context.result = &result;
            context.kind = kind;
            context.addend = addend;
            result.ignoredUse = !wil_details_ModifyFeatureData(properties, usage ? &wil_details_SetPropertyCacheUsageCallback : &wil_details_SetPropertyCacheOpportunityCallback, &context);
        }
        break;

    default:

        if ((kind >= wil_details_ServiceReportingKind_VariantDeviceUsageBase) &&
            (kind < (wil_details_ServiceReportingKind)(wil_details_ServiceReportingKind_VariantDeviceUsageBase + c_wil_details_ServiceReportingKind_VariantMax)) &&
            ((unsigned int)(kind - wil_details_ServiceReportingKind_VariantDeviceUsageBase) == properties->variant.variant))
        {
            // Note: Right now the only optimization for variant usage is this one which avoids any work for
            // repeatedly issuing usage on the current variant.  Further caching could be added to avoid any
            // substantive work for other scenarios (variant values) if or when the need arises.

            if (properties->variant.recordedDeviceUsage)
            {
                result.ignoredUse = TRUE;
                break;
            }

            wil_details_FeaturePropertyCache match = { 0 };
            match.variant.recordedDeviceUsage = 1;
            wil_details_SetPropertyFlagContext context;
            context.ignoreReporting = TRUE;
            context.result = &result;
            context.flags = match.var;
            wil_details_ModifyFeatureData(properties, &wil_details_SetPropertyFlagCallback, &context);
        }

        result.kindImmediate = kind;
        result.countImmediate = addend;
        result.payloadId = payloadId;
        break;
    };
    return result;
}

inline BOOL wil_details_FeaturePropertyCache_ReportUsageToServiceDirect(_Inout_ wil_details_FeaturePropertyCache* cache, unsigned int featureId,
    wil_details_ServiceReportingKind kind, unsigned int payloadId, SIZE_T addend)
{
    wil_details_RecordUsageResult result = wil_details_RecordUsageInPropertyCache(cache, kind, payloadId, (unsigned int)addend);
    if (g_wil_details_recordFeatureUsage)
    {
        g_wil_details_recordFeatureUsage(featureId, kind, (unsigned int)addend, cache, &result);

        if (cache->cache.hasNotificationState == wil_details_CachedHasNotificationState_HasNotification)
        {
            // Fire off any necessary notifications
            // Note: Supplying a null cache signals that we're only trying to fire any relevant notifications
            wil_details_RecordUsageResult kindOnlyResult;
            RtlZeroMemory(&kindOnlyResult, sizeof(kindOnlyResult));
            kindOnlyResult.kindImmediate = kind;
            kindOnlyResult.isVariantConfiguration = cache->cache.isVariant;
            g_wil_details_recordFeatureUsage(featureId, kind, (unsigned int)addend, NULL, &kindOnlyResult);
        }
    }

    return !result.ignoredUse;
}

inline void wil_details_FeaturePropertyCache_ReportUsageToService(_Inout_ wil_details_FeaturePropertyCache* cache, unsigned int featureId,
    const FEATURE_LOGGED_TRAITS* traits, BOOL enabled, wil_ReportingKind kindParam, SIZE_T addend)
{
    if (kindParam != wil_ReportingKind_None)
    {
        const wil_details_ServiceReportingKind kind = wil_details_MapReportingKind(kindParam, enabled);
        if (wil_details_FeaturePropertyCache_ReportUsageToServiceDirect(cache, featureId, kind, 0, addend))
        {
            if (g_wil_details_pfnFeatureLoggingHook)
            {
                g_wil_details_pfnFeatureLoggingHook(featureId, traits, NULL, enabled, &kindParam, NULL, 0, addend);
            }
        }
    }
}

inline void wil_details_FeaturePropertyCache_ReportVariantUsageToService(_Inout_ wil_details_FeatureVariantPropertyCache* cache, unsigned int featureId,
    const FEATURE_LOGGED_TRAITS* traits, BOOL enabled, unsigned char variant, wil_VariantReportingKind kindParam, SIZE_T addend)
{
    if (kindParam != wil_VariantReportingKind_None)
    {
        const wil_details_ServiceReportingKind kind = wil_details_MapVariantReportingKind(kindParam, enabled, variant);
        auto payloadId = (variant == 0) ? 0 : cache->payloadId;
        if (wil_details_FeaturePropertyCache_ReportUsageToServiceDirect(&cache->propertyCache, featureId, kind, payloadId, addend))
        {
            if (g_wil_details_pfnFeatureLoggingHook)
            {
                g_wil_details_pfnFeatureLoggingHook(featureId, traits, NULL, enabled, NULL, &kindParam, variant, addend);
            }
        }
    }
}

#ifndef WIL_SUPPRESS_PRIVATE_API_USE
inline wil_details_StagingConfigFeatureFields wil_details_StagingConfigFeature_SetEnabledState(_Inout_ wil_details_StagingConfigFeature* feature,
    wil_FeatureEnabledStateKind kind, wil_FeatureEnabledState state, wil_FeatureEnabledStateOptions options)
{
    unsigned int isVariantConfig = ((options & wil_details_ServiceReportingOptions_VariantConfig) != 0);
    switch (kind)
    {
    case wil_FeatureEnabledStateKind_All:
        feature->serviceState = state;
        feature->userState = state;
        feature->testState = state;
        feature->isVariantConfig = isVariantConfig;
        return (wil_details_StagingConfigFeatureFields_ServiceState | wil_details_StagingConfigFeatureFields_UserState |
            wil_details_StagingConfigFeatureFields_TestState);
    case wil_FeatureEnabledStateKind_Service:
        feature->serviceState = state;
        feature->isVariantConfig = isVariantConfig;
        return wil_details_StagingConfigFeatureFields_ServiceState;
    case wil_FeatureEnabledStateKind_User:
        feature->userState = state;
        feature->isVariantConfig = isVariantConfig;
        return wil_details_StagingConfigFeatureFields_UserState;
    case wil_FeatureEnabledStateKind_Test:
        feature->testState = state;
        feature->isVariantConfig = isVariantConfig;
        return wil_details_StagingConfigFeatureFields_TestState;
    }

    return wil_details_StagingConfigFeatureFields_None;
}

inline BOOL wil_details_StagingConfigFeature_SetFeatureState(_Inout_ wil_details_StagingConfigFeature* feature,
    const wil_details_StagingConfigFeature* newFeatureProperties, wil_details_StagingConfigFeatureFields changes)
{
    BOOL modified = FALSE;
    if (changes & wil_details_StagingConfigFeatureFields_ServiceState)
    {
        modified |= (feature->serviceState != newFeatureProperties->serviceState);
        feature->serviceState = newFeatureProperties->serviceState;
    }
    if (changes & wil_details_StagingConfigFeatureFields_UserState)
    {
        modified |= (feature->userState != newFeatureProperties->userState);
        feature->userState = newFeatureProperties->userState;
    }
    if (changes & wil_details_StagingConfigFeatureFields_TestState)
    {
        modified |= (feature->testState != newFeatureProperties->testState);
        feature->testState = newFeatureProperties->testState;
    }
    if (changes & wil_details_StagingConfigFeatureFields_Variant)
    {
        modified |= (feature->variant != newFeatureProperties->variant) ||
            (feature->payloadKind != newFeatureProperties->payloadKind) ||
            (feature->payload != newFeatureProperties->payload);
        feature->variant = newFeatureProperties->variant;
        feature->payloadKind = newFeatureProperties->payloadKind;
        feature->payload = newFeatureProperties->payload;
    }

    modified |= (feature->isVariantConfig != newFeatureProperties->isVariantConfig);
    feature->isVariantConfig = newFeatureProperties->isVariantConfig;

    return modified;
}

inline BOOL wil_details_StagingConfigFeature_HasUniqueState(const wil_details_StagingConfigFeature* feature)
{
    if (feature->featureId != 0)
    {
        if ((feature->serviceState != wil_FeatureEnabledState_Default) ||
            (feature->userState != wil_FeatureEnabledState_Default) ||
            (feature->testState != wil_FeatureEnabledState_Default) ||
            (feature->variant != 0) ||
            (feature->isVariantConfig != wil_FeatureEnabledStateOptions_None))
        {
            return TRUE;
        }
    }
    return FALSE;
}

inline void wil_details_StagingConfig_PrepareSessionChangeUpdate(_Inout_ wil_details_StagingConfig* config)
{
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigFeature* features = config->features;

    // Walk through the entire configuration and eliminate what we no longer need (old feature state)
    for (unsigned int index = 0; index < header->featureCount; index++)
    {
        if (features[index].changedInSession)
        {
            // If there's an old entry matching our new one, then we'll go ahead and delete that
            // old one...
            for (unsigned int find = 0; find < header->featureCount; find++)
            {
                if ((find != index) && (features[find].featureId == features[index].featureId))
                {
                    // Mark the feature id (meaning it no longer has unique state)
                    features[find].featureId = 0;
                }
            }
        }
    }

    // Now walk the features and remove any that no longer represent any unique state
    unsigned int newCount = 0;
    for (unsigned int index = 0; index < header->featureCount; index++)
    {
        if (wil_details_StagingConfigFeature_HasUniqueState(features + index))
        {
            if (newCount != index)
            {
                features[newCount] = features[index];
            }
            features[newCount++].changedInSession = 0;
        }
    }
    if (newCount != header->featureCount)
    {
        RtlMoveMemory(features + newCount, features + header->featureCount, header->featureUsageTriggerCount * sizeof(wil_details_StagingConfigUsageTrigger));
        config->triggers = (wil_details_StagingConfigUsageTrigger*)features + newCount;
        config->bufferSize -= (header->featureCount - newCount) * sizeof(wil_details_StagingConfigFeature);
        config->modified = TRUE;
        header->featureCount = (unsigned short)newCount;
    }

    // Remember the header properties as they were at the beginning of this session
    header->properties = header->sessionProperties;
}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

inline wil_details_CachedFeatureEnabledState wil_details_GetCurrentFeatureEnabledState(unsigned int featureId, BOOL isEnabledByDefault,
    wil_FeatureChangeTime changeTime, wil_FeatureStore store, __WIL_PFeatureEnabledQuery areDependenciesEnabled, _Out_ wil_details_CachedHasNotificationState* hasNotificationState)
{
    WI_ASSERT(store != wil_FeatureStore_All);
    BOOL featureRequiresSessionChange = (changeTime == wil_FeatureChangeTime_OnReboot) || (changeTime == wil_FeatureChangeTime_OnSession);
    BOOL shouldBeEnabled = FALSE;
    wil_FeatureState state;
    RtlZeroMemory(&state, sizeof(state));
    BOOL configured = wil_QueryFeatureState(&state, featureId, featureRequiresSessionChange, store, NULL) && (state.enabledState != wil_FeatureEnabledState_Default);
    *hasNotificationState = state.hasNotification ? wil_details_CachedHasNotificationState_HasNotification : wil_details_CachedHasNotificationState_DoesNotHaveNotifications;
    if (configured)
    {
        // Any explicitly configured feature controls the state (configuration wins)
        shouldBeEnabled = (state.enabledState == wil_FeatureEnabledState_Enabled);
    }
    else
    {
        // We're not configured: Are there features enabled that require our feature to first be enabled?
        shouldBeEnabled = isEnabledByDefault;
    }
    if (shouldBeEnabled)
    {
        return (areDependenciesEnabled() ? wil_details_CachedFeatureEnabledState_Enabled : wil_details_CachedFeatureEnabledState_Desired);
    }
    return wil_details_CachedFeatureEnabledState_Disabled;
}

inline wil_details_CachedFeatureEnabledState wil_details_FeaturePropertyCache_GetCachedFeatureEnabledState(
    _Inout_ wil_details_FeaturePropertyCache* cache, unsigned int featureId, BOOL isEnabledByDefault, wil_FeatureChangeTime changeTime,
    wil_FeatureStore store, __WIL_PFeatureEnabledQuery areDependenciesEnabled)
{
    WI_ASSERT(store != wil_FeatureStore_All);
    wil_details_CachedFeatureEnabledState cacheState = (wil_details_CachedFeatureEnabledState)cache->cache.enabledState;
    if (cacheState == wil_details_CachedFeatureEnabledState_Unknown)
    {
        wil_details_CachedHasNotificationState hasNotificationState;
        cacheState = wil_details_GetCurrentFeatureEnabledState(featureId, isEnabledByDefault, changeTime, store, areDependenciesEnabled, &hasNotificationState);

        BOOL shouldCacheState = TRUE;
        if (changeTime == wil_FeatureChangeTime_OnRead)
        {
            if (g_wil_details_featurePropertyCacheChangeNotification)
            {
                // When willing to change states at any point, we need to setup a subscription to invalidate our feature state in response
                // to a configuration change.
                g_wil_details_featurePropertyCacheChangeNotification(cache, changeTime);
            }
            else
            {
                // If we don't support change eventing, then we shoudn't cache state as we need to read each
                // time in order to be accurate.
                shouldCacheState = FALSE;
            }
        }

        if (shouldCacheState && (!g_wil_details_testStates || !wil_HasFeatureTestState(featureId, NULL)))
        {
            wil_details_SetEnabledAndHasNotificationStateProperties(cache, cacheState, hasNotificationState, cache->cache.isVariant);
        }
        else
        {
            wil_details_SetHasNotificationStateProperty(cache, hasNotificationState);
        }
    }
    return cacheState;
}

__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_MACHINE_FEATURE_STORE = { 0xa3bc7c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_MACHINE_FEATURE_STORE_MODIFIED = { 0xa3bc8075, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_USER_FEATURE_STORE = { 0xa3bc88f5, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_USER_FEATURE_STORE_MODIFIED = { 0xa3bc90f5, 0x418a073a };

WIL_PAGED_FUNCTION inline NTSTATUS wil_details_StagingConfig_Load(_Out_ wil_details_StagingConfig* config, wil_FeatureStore store, SIZE_T bufferSize,
    _Out_writes_bytes_opt_(bufferSize) void* buffer, BOOL forUpdate)
{
    WIL_PAGED_CODE();

    WI_ASSERT(store != wil_FeatureStore_All);
    WI_ASSERT((bufferSize == 0) || (bufferSize >= sizeof(wil_details_StagingConfigHeader)));
    RtlZeroMemory(config, sizeof(wil_details_StagingConfig));
    config->store = store;
    config->forUpdate = forUpdate;

    SIZE_T dataSize = buffer ? bufferSize : 0;
    void* allocation = NULL;
    void* data = NULL;

    const __WIL_WNF_STATE_NAME state = (store == wil_FeatureStore_Machine) ? __WIL_WNF_WIL_MACHINE_FEATURE_STORE :
        __WIL_WNF_WIL_USER_FEATURE_STORE;
    ULONG size = buffer ? (ULONG)bufferSize : 0;
    NTSTATUS status = WIL_NtQueryWnfStateData(&state, NULL, NULL, &config->readChangeStamp, buffer, &size);
    if (status == __WIL_STATUS_SUCCESS)
    {
        if (!buffer)
        {
            status = __WIL_STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            data = buffer;
        }
    }

    while (status == __WIL_STATUS_BUFFER_TOO_SMALL)
    {
        dataSize = (dataSize < bufferSize) ? bufferSize : dataSize;
        dataSize = (dataSize < size) ? size : dataSize;
        dataSize = (dataSize < sizeof(wil_details_StagingConfigHeader)) ? sizeof(wil_details_StagingConfigHeader) : dataSize;
        if (allocation)
        {
            WIL_FreeMemory(allocation);
        }
        WI_ASSERT(dataSize >= sizeof(wil_details_StagingConfigHeader));
        allocation = WIL_AllocateMemory(dataSize);
        if (!allocation)
        {
            return __WIL_STATUS_INSUFFICIENT_RESOURCES;
        }
        size = (ULONG)dataSize;
        status = WIL_NtQueryWnfStateData(&state, NULL, NULL, &config->readChangeStamp, allocation, &size);
        data = allocation;
    }

    if ((status == __WIL_STATUS_SUCCESS) && data)
    {
        wil_details_StagingConfigHeader* header = (wil_details_StagingConfigHeader*)data;

        if (size > sizeof(unsigned int))
        {
            config->readVersion = header->version;
        }

        const BOOL isEmpty = ((size < sizeof(wil_details_StagingConfigHeader)) ||
            (config->readVersion != c_wil_details_stagingConfigVersion) ||
            (header->headerSizeBytes < sizeof(wil_details_StagingConfigHeader)) ||
            (size < (header->headerSizeBytes + (sizeof(wil_details_StagingConfigFeature) * header->featureCount) + (sizeof(wil_details_StagingConfigUsageTrigger) * header->featureUsageTriggerCount))));
        if (isEmpty)
        {
            size = sizeof(wil_details_StagingConfigHeader);

            RtlZeroMemory(header, sizeof(wil_details_StagingConfigHeader));
            header->headerSizeBytes = sizeof(wil_details_StagingConfigHeader);
            header->version = c_wil_details_stagingConfigVersion;
            header->versionMinor = c_wil_details_stagingConfigMinorVersion;

            config->header = header;
            config->features = (wil_details_StagingConfigFeature*)((unsigned char *)header + header->headerSizeBytes);
            config->triggers = (wil_details_StagingConfigUsageTrigger*)(config->features + header->featureCount);
        }
        else
        {
            __WIL_WNF_CHANGE_STAMP updateChangeStamp = 0;
            if (header->featureCount > 0)
            {
                // If we have features, then we'll need to know whether or not the config was written
                // in this session to evaluate them properly.
                ULONG updateSize = 0;
                const __WIL_WNF_STATE_NAME stateUpdatedThisSession = (store == wil_FeatureStore_Machine) ?
                    __WIL_WNF_WIL_MACHINE_FEATURE_STORE_MODIFIED : __WIL_WNF_WIL_USER_FEATURE_STORE_MODIFIED;
                WIL_NtQueryWnfStateData(&stateUpdatedThisSession, NULL, NULL, &updateChangeStamp, NULL, &updateSize);
            }

            config->header = header;
            config->features = (wil_details_StagingConfigFeature*)(header + 1);
            config->triggers = (wil_details_StagingConfigUsageTrigger*)(config->features + header->featureCount);
            config->changedInSession = (updateChangeStamp != 0);

            // This code fixes a bug in existing clients that let the WNF buffer size grow beyond the actual size
            // of the data contained within... for configs written with an older version, compress the WNF state to
            // avoid that.
            if ((header->version == 2) && (header->versionMinor < 2))
            {
                size = (header->headerSizeBytes +
                    (sizeof(wil_details_StagingConfigFeature) * header->featureCount) +
                    (sizeof(wil_details_StagingConfigUsageTrigger) * header->featureUsageTriggerCount));
                config->modified = TRUE;
            }
        }

        config->buffer = data;
        config->bufferSize = size;
        config->bufferAlloc = (allocation ? dataSize : bufferSize);
        config->bufferOwned = ((data == allocation) ? TRUE : FALSE);

        if (forUpdate && !isEmpty && !config->changedInSession)
        {
            wil_details_StagingConfig_PrepareSessionChangeUpdate(config);
        }

        return __WIL_STATUS_SUCCESS;
    }

    if (allocation)
    {
        WIL_FreeMemory(allocation);
    }
    return status;
}

inline void wil_details_StagingConfig_Free(_In_ wil_details_StagingConfig* config)
{
    if (config->bufferOwned)
    {
        WIL_FreeMemory(config->buffer);
        config->buffer = NULL;
        config->header = NULL;
    }
}

WIL_PAGED_FUNCTION inline NTSTATUS wil_details_StagingConfig_Save(_In_ wil_details_StagingConfig* config, BOOL forceOverwrite)
{
    WIL_PAGED_CODE();

    if (!config->forUpdate)
    {
        WI_ASSERT(FALSE);
        // Can't write when we didn't prepare to do so from the load...
        return STATUS_INVALID_PARAMETER;
    }

    if (config->readVersion > c_wil_details_stagingConfigVersion)
    {
        // Can't overwrite a newer version with an older one...
        return __WIL_STATUS_REVISION_MISMATCH;
    }

    if (!config->modified)
    {
        return __WIL_STATUS_SUCCESS;
    }

    // Must always write with the current minor version, regardless of the version read...
    config->header->versionMinor = c_wil_details_stagingConfigMinorVersion;

    const __WIL_WNF_STATE_NAME state = (config->store == wil_FeatureStore_Machine) ? __WIL_WNF_WIL_MACHINE_FEATURE_STORE :
        __WIL_WNF_WIL_USER_FEATURE_STORE;

    NTSTATUS status = WIL_NtUpdateWnfStateData(&state, config->buffer, (ULONG)config->bufferSize, NULL, NULL, config->readChangeStamp, forceOverwrite ? FALSE : TRUE);
    if ((status == __WIL_STATUS_SUCCESS) && !config->changedInSession)
    {
        const __WIL_WNF_STATE_NAME updateState = (config->store == wil_FeatureStore_Machine) ? __WIL_WNF_WIL_MACHINE_FEATURE_STORE_MODIFIED : __WIL_WNF_WIL_USER_FEATURE_STORE_MODIFIED;
        WIL_NtUpdateWnfStateData(&updateState, NULL, 0, NULL, NULL, 0, FALSE);
    }
    return status;
}

inline BOOL wil_details_StagingConfig_QueryFeatureState(_In_ wil_details_StagingConfig* config, _Outptr_ wil_FeatureState* state,
    unsigned int featureId, BOOL featureRequiresSessionChange)
{
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigFeature* features = config->features;

    BOOL found = FALSE;
    wil_details_StagingConfigFeature feature;

    for (unsigned int index = 0; index < header->featureCount; index++)
    {
        if (features[index].featureId == featureId)
        {
            if (featureRequiresSessionChange && config->changedInSession)
            {
                if (!features[index].changedInSession)
                {
                    feature = features[index];
                    found = TRUE;
                    break;
                }
            }
            else if (features[index].changedInSession)
            {
                feature = features[index];
                found = TRUE;
                break;
            }
            else
            {
                feature = features[index];
                found = TRUE;
                // No break -- we're looking for (changedInSession = TRUE), but will use this
                // if we don't find it.
            }
        }
    }

    BOOL configured = FALSE;
    if (found)
    {
        wil_details_StagingConfigHeaderProperties* properties = (featureRequiresSessionChange && config->changedInSession) ?
            &header->properties : &header->sessionProperties;
        if (properties->ignoreTestState)
        {
            feature.testState = wil_FeatureEnabledState_Default;
        }
        if (properties->ignoreUserState)
        {
            feature.userState = wil_FeatureEnabledState_Default;
        }
        if (properties->ignoreServiceState)
        {
            feature.serviceState = wil_FeatureEnabledState_Default;
        }
        if (properties->ignoreVariants)
        {
            feature.variant = 0;
            feature.payload = 0;
        }

        if (wil_details_StagingConfigFeature_HasUniqueState(&feature))
        {
            state->payload = feature.payload;
            state->payloadKind = (wil_FeatureVariantPayloadKind)feature.payloadKind;
            state->variant = (unsigned char)feature.variant;
            state->isVariantConfiguration = feature.isVariantConfig;

            if (feature.testState != wil_FeatureEnabledState_Default)
            {
                state->enabledState = (wil_FeatureEnabledState)feature.testState;
            }
            else if (feature.userState != wil_FeatureEnabledState_Default)
            {
                state->enabledState = (wil_FeatureEnabledState)feature.userState;
            }
            else if (feature.serviceState != wil_FeatureEnabledState_Default)
            {
                state->enabledState = (wil_FeatureEnabledState)feature.serviceState;
            }

            configured = TRUE;
        }
    }

    // Regardless of whether or not the feature is configured, check to see if there are any usage triggers for it
    wil_details_StagingConfigUsageTrigger* triggers = config->triggers;
    BOOL hasNotification = FALSE;
    for (unsigned int index = 0; index < header->featureUsageTriggerCount; index++)
    {
        if (triggers[index].featureId == featureId)
        {
            hasNotification = TRUE;
            break;
        }
    }

    state->hasNotification = hasNotification;
    return configured;
}

inline void wil_details_StagingConfig_EnumerateFeatures(_In_ wil_details_StagingConfig* config,
    __WIL_PStagingConfigFeatureEnumeration enumerationFunction, void* context)
{
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigFeature* features = config->features;

    for (unsigned int index = 0; index < header->featureCount; index++)
    {
        if (wil_details_StagingConfigFeature_HasUniqueState(features + index))
        {
            // if the feature changed in session, then it's the newest copy and we have unique state
            // if it didn't, but the config says we changed in the current session, then this data is still relevant
            // for any reboot feature that must maintain the current state....

            if (features[index].changedInSession)
            {
                if (!enumerationFunction(features + index, context))
                {
                    break;
                }
            }
            else
            {
                // OK, so it wasn't changed in the current session and the config wasn't changed in the current session
                // either, so the only way this is relevant is if there is *not* a matching record for this property
                // also in the list.

                BOOL found = FALSE;
                for (unsigned int find = 0; find < header->featureCount; find++)
                {
                    if ((find != index) && (features[index].featureId == features[find].featureId))
                    {
                        found = TRUE;
                        break;
                    }
                }
                if (!found)
                {
                    if (!enumerationFunction(features + index, context))
                    {
                        break;
                    }
                }
            }
        }
    }
}

inline BOOL NTAPI wil_details_StagingConfig_AreAnyFeaturesConfigured_Callback(_Inout_ wil_details_StagingConfigFeature* feature, void* context)
{
    feature;
    *((BOOL*)context) = TRUE;
    return FALSE;
}

inline BOOL wil_details_StagingConfig_AreAnyFeaturesConfigured(_In_ wil_details_StagingConfig* config)
{
    BOOL found = FALSE;
    wil_details_StagingConfig_EnumerateFeatures(config, &wil_details_StagingConfig_AreAnyFeaturesConfigured_Callback, &found);
    return found || (config->header->featureUsageTriggerCount > 0);
}

inline BOOL wil_details_StagingConfig_SetFeatureState(_In_ wil_details_StagingConfig* config, const wil_details_StagingConfigFeature* feature,
    wil_details_StagingConfigFeatureFields changes)
{
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigFeature* features = config->features;

    // If we've already changed the feature in this session, then just overwrite that entry...
    BOOL foundId = FALSE;
    unsigned int existingIndex = 0;
    for (unsigned int index = 0; index < header->featureCount; index++)
    {
        if (features[index].featureId == feature->featureId)
        {
            if (features[index].changedInSession)
            {
                config->modified = wil_details_StagingConfigFeature_SetFeatureState(features + index, feature, changes) || config->modified;
                return TRUE;
            }
            foundId = TRUE;
            existingIndex = index;
        }
    }

    // Otherwise we need to add the feature
    if (wil_details_StagingConfigFeature_HasUniqueState(feature) || foundId)
    {
        if ((config->bufferAlloc - config->bufferSize) < sizeof(wil_details_StagingConfigFeature))
        {
            return FALSE;
        }

        RtlMoveMemory(features + header->featureCount + 1, features + header->featureCount, ((unsigned char*)config->buffer + config->bufferSize) - (unsigned char*)(features + header->featureCount));

        if (foundId)
        {
            features[header->featureCount] = features[existingIndex];
            wil_details_StagingConfigFeature_SetFeatureState(features + header->featureCount, feature, changes);
        }
        else
        {
            features[header->featureCount] = *feature;
        }

        features[header->featureCount++].changedInSession = 1;
        config->triggers = (wil_details_StagingConfigUsageTrigger*)features + header->featureCount;
        config->bufferSize += sizeof(wil_details_StagingConfigFeature);
        config->modified = TRUE;
    }

    return TRUE;
}

inline BOOL wil_details_StagingConfig_SetFeatureEnabledState(_In_ wil_details_StagingConfig* config, unsigned int featureId, wil_FeatureEnabledState state,
    wil_FeatureEnabledStateKind kind, wil_FeatureEnabledStateOptions options)
{
    wil_details_StagingConfigFeature feature;
    RtlZeroMemory(&feature, sizeof(feature));
    feature.featureId = featureId;
    wil_details_StagingConfigFeatureFields changes = wil_details_StagingConfigFeature_SetEnabledState(&feature, kind, state, options);
    return wil_details_StagingConfig_SetFeatureState(config, &feature, changes);
}

inline BOOL wil_details_StagingConfig_SetVariant(_Inout_ wil_details_StagingConfig* config, unsigned int featureId, unsigned char variant,
    wil_FeatureVariantPayloadKind kind, unsigned int payload)
{
    wil_details_StagingConfigFeature feature;
    RtlZeroMemory(&feature, sizeof(feature));
    feature.featureId = featureId;
    feature.variant = variant;
    feature.payloadKind = kind;
    feature.payload = payload;
    return wil_details_StagingConfig_SetFeatureState(config, &feature, wil_details_StagingConfigFeatureFields_Variant);
}

inline BOOL wil_details_StagingConfig_SubscribeFeatureReporting(_Inout_ wil_details_StagingConfig* config, unsigned int featureId,
    wil_details_ServiceReportingKind serviceReportingKind, wil_details_ServiceReportingOptions reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state)
{
    wil_details_StagingConfigUsageTrigger trigger;
    RtlZeroMemory(&trigger, sizeof(trigger));
    trigger.featureId = featureId;
    trigger.serviceReportingKind = (unsigned short)serviceReportingKind;
    trigger.isVariantConfig = ((reportingOptions & wil_details_ServiceReportingOptions_VariantConfig) != 0);
    trigger.trigger.Data[0] = state->Data[0];
    trigger.trigger.Data[1] = state->Data[1];

    // See if this trigger already exists
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigUsageTrigger* triggers = config->triggers;
    for (unsigned int index = 0; index < header->featureUsageTriggerCount; index++)
    {
        if (triggers[index].featureId == trigger.featureId
            && triggers[index].serviceReportingKind == trigger.serviceReportingKind
            && triggers[index].isVariantConfig == trigger.isVariantConfig
            && triggers[index].trigger.Data[0] == trigger.trigger.Data[0]
            && triggers[index].trigger.Data[1] == trigger.trigger.Data[1])
        {
            return TRUE;
        }
    }

    // Trigger didn't exist; add it
    if ((config->bufferAlloc - config->bufferSize) < sizeof(wil_details_StagingConfigUsageTrigger))
    {
        return FALSE;
    }

    triggers[header->featureUsageTriggerCount++] = trigger;
    config->bufferSize += sizeof(wil_details_StagingConfigUsageTrigger);
    config->modified = TRUE;

    return TRUE;
}

inline BOOL wil_details_StagingConfig_UnsubscribeFeatureReporting(_Inout_ wil_details_StagingConfig* config, unsigned int featureId,
    wil_details_ServiceReportingKind serviceReportingKind, wil_details_ServiceReportingOptions reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state)
{
    // Walk all triggers and removing matching ones
    wil_details_StagingConfigHeader* header = config->header;
    wil_details_StagingConfigUsageTrigger* triggers = config->triggers;
    unsigned short newCount = 0;
    unsigned short isVariantConfig = ((reportingOptions & wil_details_ServiceReportingOptions_VariantConfig) != 0);
    for (unsigned short index = 0; index < header->featureUsageTriggerCount; index++)
    {
        if (triggers[index].featureId != featureId
            || triggers[index].serviceReportingKind != (unsigned short)serviceReportingKind
            || triggers[index].isVariantConfig != isVariantConfig
            || triggers[index].trigger.Data[0] != state->Data[0]
            || triggers[index].trigger.Data[1] != state->Data[1])
        {
            if (newCount != index)
            {
                triggers[newCount] = triggers[index];
            }

            newCount++;
        }
    }

    if (newCount < header->featureUsageTriggerCount)
    {
        config->bufferSize -= ((header->featureUsageTriggerCount - newCount) * sizeof(wil_details_StagingConfigUsageTrigger));
        config->modified = TRUE;
        header->featureUsageTriggerCount = newCount;
    }

    return TRUE;
}

WIL_PAGED_FUNCTION inline NTSTATUS wil_details_StagingConfig_FireNotification(_In_ wil_details_StagingConfig* config, unsigned int featureId, unsigned short serviceReportingKind, BOOL fireVariantConfigNotifications)
{
    WIL_PAGED_CODE();

    NTSTATUS status = __WIL_STATUS_SUCCESS;
    wil_details_StagingConfigUsageTrigger* triggers = config->triggers;
    for (unsigned int index = 0; index < config->header->featureUsageTriggerCount; index++)
    {
        if (triggers[index].featureId == featureId && triggers[index].serviceReportingKind == serviceReportingKind)
        {
            if (triggers[index].isVariantConfig && !fireVariantConfigNotifications)
            {
                continue;
            }

            __WIL_WNF_STATE_NAME stateName;
            stateName.Data[0] = triggers[index].trigger.Data[0];
            stateName.Data[1] = triggers[index].trigger.Data[1];

            // Keep trying in case another process updates the state out from under us
            do
            {
                unsigned char buffer[4096];
                ULONG bufferSize = sizeof(buffer);
                ULONG dataSize = bufferSize;
                __WIL_WNF_CHANGE_STAMP readChangeStamp;
                status = WIL_NtQueryWnfStateData(&stateName, NULL, NULL, &readChangeStamp, buffer, &dataSize);
                if (status == __WIL_STATUS_SUCCESS)
                {
                    // Ensure buffer size makes sense
                    if ((dataSize % sizeof(wil_details_UsageSubscriptionData)) != 0)
                    {
                        // Treat invalid data as empty
                        dataSize = 0;
                    }

                    // Check to see if this data point has already been logged
                    BOOL shouldAdd = TRUE;
                    wil_details_UsageSubscriptionData* items = (wil_details_UsageSubscriptionData*)buffer;
                    unsigned int count = dataSize / sizeof(wil_details_UsageSubscriptionData);
                    for (unsigned int i = 0; i < count; i++)
                    {
                        if (items[i].featureId == featureId && items[i].serviceReportingKind == serviceReportingKind)
                        {
                            shouldAdd = FALSE;
                            break;
                        }
                    }

                    if (shouldAdd && (dataSize + sizeof(wil_details_UsageSubscriptionData)) <= bufferSize)
                    {
                        items[count].featureId = featureId;
                        items[count].serviceReportingKind = serviceReportingKind;
                        dataSize += sizeof(wil_details_UsageSubscriptionData);
                    }

                    // Always publish (in case a previous update was missed)
                    status = WIL_NtUpdateWnfStateData(&stateName, (void*)buffer, dataSize, NULL, NULL, readChangeStamp, TRUE);
                }
            } while (status == __WIL_STATUS_UNSUCCESSFUL);
        }
    }

    return status;
}

WIL_PAGED_FUNCTION inline BOOL wil_UsageSubscriptionEnumerate(_In_ __WIL_PCWNF_STATE_NAME state, _In_opt_ __WIL_PUsageSubscriptionEnumerate enumerateFunction, BOOL clear, void* context)
{
    WIL_PAGED_CODE();
    // Read (and clear, if necessary)
    NTSTATUS status = __WIL_STATUS_SUCCESS;
    unsigned char buffer[4096];
    ULONG bufferSize = sizeof(buffer);
    ULONG dataSize = 0;
    do
    {
        __WIL_WNF_CHANGE_STAMP readChangeStamp;
        dataSize = bufferSize;
        status = WIL_NtQueryWnfStateData(state, NULL, NULL, &readChangeStamp, buffer, &dataSize);
        if (status == __WIL_STATUS_SUCCESS && clear && dataSize > 0)
        {
            status = WIL_NtUpdateWnfStateData(state, NULL, 0, NULL, NULL, readChangeStamp, TRUE);
        }
    } while (clear && status == __WIL_STATUS_UNSUCCESSFUL);

    if (status == __WIL_STATUS_SUCCESS)
    {
        // Ensure buffer size makes sense
        if ((dataSize % sizeof(wil_details_UsageSubscriptionData)) != 0)
        {
            // Treat invalid data as empty
            dataSize = 0;
        }

        wil_details_UsageSubscriptionData* items = (wil_details_UsageSubscriptionData*)buffer;
        unsigned int count = dataSize / sizeof(wil_details_UsageSubscriptionData);
        for (unsigned int i = 0; i < count; i++)
        {
            if (!enumerateFunction(items[i].featureId, items[i].serviceReportingKind, context))
            {
                break;
            }
        }
    }

    return (status == __WIL_STATUS_SUCCESS);
}

inline void wil_details_StagingConfig_SetHeaderProperties(_Inout_ wil_details_StagingConfig* config, _In_ wil_details_StagingConfigHeaderProperties* pProperties)
{
    if (RtlCompareMemory(pProperties, &config->header->sessionProperties, sizeof(wil_details_StagingConfigHeaderProperties)) != sizeof(wil_details_StagingConfigHeaderProperties))
    {
        config->header->sessionProperties = *pProperties;
        config->modified = TRUE;
    }
}


//*************************************************************************************************
// Public Interface Implementation
//*************************************************************************************************

inline NTSTATUS wil_LoadStagingConfig(wil_PStagingConfig* config, wil_FeatureStore store, BOOL forUpdate)
{
    WI_ASSERT(store != wil_FeatureStore_All);
    *config = NULL;
    SIZE_T preAllocationSize = forUpdate ? c_wil_details_maxStagingConfigSize : 0;

    NTSTATUS status = __WIL_STATUS_INSUFFICIENT_RESOURCES;
    wil_details_StagingConfig* buffer = (wil_details_StagingConfig*)WIL_AllocateMemory(sizeof(wil_details_StagingConfig) + preAllocationSize);
    if (buffer)
    {
        status = wil_details_StagingConfig_Load(buffer, store, preAllocationSize, forUpdate ? buffer + 1 : NULL, forUpdate);
        if (status == __WIL_STATUS_SUCCESS)
        {
            *config = (wil_PStagingConfig)buffer;
            return __WIL_STATUS_SUCCESS;
        }
    }

    WIL_FreeMemory(buffer);
    return status;
}

inline void __stdcall wil_FreeStagingConfig(wil_PStagingConfig configHandle)
{
    wil_details_StagingConfig* config = (wil_details_StagingConfig*)configHandle;
    if (config)
    {
        wil_details_StagingConfig_Free(config);
        WIL_FreeMemory(config);
    }
}

inline NTSTATUS wil_SaveStagingConfig(wil_PStagingConfig configHandle, BOOL forceOverwrite)
{
    return wil_details_StagingConfig_Save((wil_details_StagingConfig*)configHandle, forceOverwrite);
}

inline NTSTATUS wil_CacheStagingConfig(BOOL supportPerUserStore)
{
    g_wil_details_preventOnDemandStagingConfigReads = TRUE;
    NTSTATUS status = wil_LoadStagingConfig(&g_wil_details_stagingConfigForMachine, wil_FeatureStore_Machine, FALSE);
    if ((status == __WIL_STATUS_SUCCESS) && supportPerUserStore)
    {
        status = wil_LoadStagingConfig(&g_wil_details_stagingConfigForUser, wil_FeatureStore_User, FALSE);
    }
    return status;
}

inline void wil_FreeCachedStagingConfig()
{
    wil_FreeStagingConfig(g_wil_details_stagingConfigForMachine);
    g_wil_details_stagingConfigForMachine = NULL;
    wil_FreeStagingConfig(g_wil_details_stagingConfigForUser);
    g_wil_details_stagingConfigForUser = NULL;
}

inline BOOL wil_StagingConfig_QueryFeatureState(_In_ wil_PStagingConfig configHandle, _Outptr_ wil_FeatureState* state, unsigned int featureId, BOOL featureRequiresSessionChange)
{
    return wil_details_StagingConfig_QueryFeatureState((wil_details_StagingConfig*)configHandle, state, featureId, featureRequiresSessionChange);
}

inline BOOL wil_QueryFeatureState(_Outptr_ wil_FeatureState* state, unsigned int featureId, BOOL featureRequiresSessionChange, wil_FeatureStore store, _In_opt_ BOOL* areAnyFeaturesConfigured)
{
    WI_ASSERT(store != wil_FeatureStore_All);
    BOOL found = FALSE;
    wil_PStagingConfig configStore = (store == wil_FeatureStore_Machine) ? g_wil_details_stagingConfigForMachine : g_wil_details_stagingConfigForUser;
    if (configStore)
    {
        if (areAnyFeaturesConfigured)
        {
            *areAnyFeaturesConfigured = wil_details_StagingConfig_AreAnyFeaturesConfigured((wil_details_StagingConfig*)configStore);
        }
        found = wil_StagingConfig_QueryFeatureState(configStore, state, featureId, featureRequiresSessionChange);
    }
    else
    {
        if (areAnyFeaturesConfigured)
        {
            *areAnyFeaturesConfigured = FALSE;
        }

        if (!g_wil_details_preventOnDemandStagingConfigReads)
        {
            unsigned char buffer[200];      // small stack buffer to avoid the vast majority of allocations
            wil_details_StagingConfig config;
            if (__WIL_STATUS_SUCCESS == wil_details_StagingConfig_Load(&config, store, ARRAYSIZE(buffer), buffer, FALSE))
            {
                found = wil_details_StagingConfig_QueryFeatureState(&config, state, featureId, featureRequiresSessionChange);
                if (areAnyFeaturesConfigured)
                {
                    *areAnyFeaturesConfigured = wil_details_StagingConfig_AreAnyFeaturesConfigured(&config);
                }
                wil_details_StagingConfig_Free(&config);
            }
        }
    }

    if (g_wil_details_testStates)
    {
        wil_FeatureEnabledState testState;
        if (wil_HasFeatureTestState(featureId, &testState))
        {
            if (!found)
            {
                RtlZeroMemory(state, sizeof(wil_FeatureState));
                found = TRUE;
            }
            state->enabledState = testState;
        }
        unsigned char variant;
        unsigned int payload;
        if (wil_HasFeatureVariantTestState(featureId, &variant, &payload))
        {
            if (!found)
            {
                RtlZeroMemory(state, sizeof(wil_FeatureState));
                found = TRUE;
            }
            state->variant = variant;
            state->payload = payload;
        }
    }

    return found;
}

inline BOOL wil_StagingConfig_SetFeatureEnabledState(_In_ wil_PStagingConfig configHandle, unsigned int featureId, wil_FeatureEnabledState state, wil_FeatureEnabledStateKind kind, wil_FeatureEnabledStateOptions options)
{
    return wil_details_StagingConfig_SetFeatureEnabledState((wil_details_StagingConfig*)configHandle, featureId, state, kind, options);
}

inline BOOL wil_StagingConfig_SetVariant(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned char variant, wil_FeatureVariantPayloadKind kind, unsigned int payload)
{
    return wil_details_StagingConfig_SetVariant((wil_details_StagingConfig*)configHandle, featureId, variant, kind, payload);
}

inline BOOL wil_StagingConfig_SubscribeFeatureReporting(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned short serviceReportingKind, unsigned short reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state)
{
    return wil_details_StagingConfig_SubscribeFeatureReporting((wil_details_StagingConfig*)configHandle, featureId, (wil_details_ServiceReportingKind)serviceReportingKind, (wil_details_ServiceReportingOptions)reportingOptions, state);
}

inline BOOL wil_StagingConfig_UnsubscribeFeatureReporting(_In_ wil_PStagingConfig configHandle, unsigned int featureId, unsigned short serviceReportingKind, unsigned short reportingOptions, _In_ __WIL_PCWNF_STATE_NAME state)
{
    return wil_details_StagingConfig_UnsubscribeFeatureReporting((wil_details_StagingConfig*)configHandle, featureId, (wil_details_ServiceReportingKind)serviceReportingKind, (wil_details_ServiceReportingOptions)reportingOptions, state);
}

#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
#endif // WIL_SUPPRESS_PRIVATE_API_USE

#ifndef __WIL_MIN_KERNEL

#ifndef WIL_AcquireSRWLockExclusive
#if defined(__cplusplus)
#define WIL_AcquireSRWLockExclusive(SRWLock) ::AcquireSRWLockExclusive(SRWLock)
#else
#define WIL_AcquireSRWLockExclusive(SRWLock) AcquireSRWLockExclusive(SRWLock)
#endif
WI_ODR_PRAGMA("WIL_AcquireSRWLockExclusive", "1")
#else
WI_ODR_PRAGMA("WIL_AcquireSRWLockExclusive", "0")
#endif

#ifndef WIL_AcquireSRWLockShared
#if defined(__cplusplus)
#define WIL_AcquireSRWLockShared(SRWLock) ::AcquireSRWLockShared(SRWLock)
#else
#define WIL_AcquireSRWLockShared(SRWLock) AcquireSRWLockShared(SRWLock)
#endif
WI_ODR_PRAGMA("WIL_AcquireSRWLockShared", "1")
#else
WI_ODR_PRAGMA("WIL_AcquireSRWLockShared", "0")
#endif

#ifndef WIL_ReleaseSRWLockExclusive
#define WIL_ReleaseSRWLockExclusive(SRWLock) ReleaseSRWLockExclusive(SRWLock)
WI_ODR_PRAGMA("WIL_ReleaseSRWLockExclusive", "1")
#else
WI_ODR_PRAGMA("WIL_ReleaseSRWLockExclusive", "0")
#endif

#ifndef WIL_ReleaseSRWLockShared
#define WIL_ReleaseSRWLockShared(SRWLock) ReleaseSRWLockShared(SRWLock)
WI_ODR_PRAGMA("WIL_ReleaseSRWLockShared", "1")
#else
WI_ODR_PRAGMA("WIL_ReleaseSRWLockShared", "0")
#endif

#ifndef WIL_Critical_Section
#define WIL_Critical_Section critical_section
WI_ODR_PRAGMA("WIL_Critical_Section", "1")
#else
WI_ODR_PRAGMA("WIL_Critical_Section", "0")
#endif

#ifndef WIL_SRWLock
#define WIL_SRWLock srwlock
WI_ODR_PRAGMA("WIL_SRWLock", "1")
#else
WI_ODR_PRAGMA("WIL_SRWLock", "0")
#endif

#ifndef WIL_UniqueThreadpoolTimer
#define WIL_UniqueThreadpoolTimer wil::unique_threadpool_timer
WI_ODR_PRAGMA("WIL_UniqueThreadpoolTimer", "1")
#else
WI_ODR_PRAGMA("WIL_UniqueThreadpoolTimer", "0")
#endif

#ifndef WIL_SetThreadpoolTimer
#define WIL_SetThreadpoolTimer SetThreadpoolTimer
WI_ODR_PRAGMA("WIL_SetThreadpoolTimer", "1")
#else
WI_ODR_PRAGMA("WIL_SetThreadpoolTimer", "0")
#endif

// Copied from winnt.h to avoid conflicting definition from nturtl.h
typedef struct __WIL_RTL_SRWLOCK {
    PVOID Ptr;
} __WIL_RTL_SRWLOCK;

static_assert(sizeof(__WIL_RTL_SRWLOCK) == sizeof(SRWLOCK), "SRWLOCK definition should not change");

__declspec(selectany) __WIL_RTL_SRWLOCK g_wil_details_testFeatureStateLock = SRWLOCK_INIT;

inline void wil_details_CreateTestState(_Out_ wil_PFeatureTestState* testFeatureState, wil_details_FeatureTestState* testState)
{
    wil_details_FeatureTestState* test = (wil_details_FeatureTestState*)WIL_AllocateMemory(sizeof(wil_details_FeatureTestState));
    if (test)
    {
        *test = *testState;

        WIL_AcquireSRWLockExclusive((PSRWLOCK)(&g_wil_details_testFeatureStateLock));

        test->next = g_wil_details_testStates;
        g_wil_details_testStates = test;

        WIL_ReleaseSRWLockExclusive((PSRWLOCK)(&g_wil_details_testFeatureStateLock));

        if (g_wil_details_featurePropertyCacheChangeNotification)
        {
            g_wil_details_featurePropertyCacheChangeNotification(NULL, wil_FeatureChangeTime_OnRead);
        }
    }
    *testFeatureState = (wil_PFeatureTestState)test;
}

inline BOOL wil_details_HasTestState(unsigned int featureId, wil_details_FeatureTestStateKind kind, _Out_ wil_details_FeatureTestState* test)
{
    BOOL found = FALSE;
    if (g_wil_details_testStates)
    {
        WIL_AcquireSRWLockShared((PSRWLOCK)(&g_wil_details_testFeatureStateLock));

        wil_details_FeatureTestState* stateList = g_wil_details_testStates;
        while (stateList)
        {
            if ((stateList->featureId == featureId) && (stateList->kind == kind))
            {
                *test = *stateList;
                found = TRUE;
                break;
            }
            stateList = stateList->next;
        }

        WIL_ReleaseSRWLockShared((PSRWLOCK)(&g_wil_details_testFeatureStateLock));
    }
    return found;
}

inline void wil_CreateFeatureTestState(_Out_ wil_PFeatureTestState* testFeatureState, unsigned int featureId, wil_FeatureEnabledState state)
{
    wil_details_FeatureTestState test;
    RtlZeroMemory(&test, sizeof(test));

    test.kind = wil_details_FeatureTestStateKind_EnabledState;
    test.featureId = featureId;
    test.state = state;

    wil_details_CreateTestState(testFeatureState, &test);
}

inline void wil_CreateFeatureVariantTestState(_Out_ wil_PFeatureTestState* testFeatureState, unsigned int featureId, unsigned char variant, unsigned int payload)
{
    wil_details_FeatureTestState test;
    RtlZeroMemory(&test, sizeof(test));

    test.kind = wil_details_FeatureTestStateKind_Variant;
    test.featureId = featureId;
    test.variant = variant;
    test.payload = payload;

    wil_details_CreateTestState(testFeatureState, &test);
}

inline void __stdcall wil_FreeFeatureTestState(wil_PFeatureTestState testFeatureState)
{
    wil_details_FeatureTestState* state = (wil_details_FeatureTestState*)testFeatureState;
    if (state)
    {
        WIL_AcquireSRWLockExclusive((PSRWLOCK)(&g_wil_details_testFeatureStateLock));

        wil_details_FeatureTestState** stateList = &g_wil_details_testStates;
        while (*stateList)
        {
            if (*stateList == state)
            {
                *stateList = state->next;
                state->next = NULL;
                break;
            }
            stateList = &((*stateList)->next);
        }

        WIL_ReleaseSRWLockExclusive((PSRWLOCK)(&g_wil_details_testFeatureStateLock));

        WIL_FreeMemory(state);

        if (g_wil_details_featurePropertyCacheChangeNotification)
        {
            g_wil_details_featurePropertyCacheChangeNotification(NULL, wil_FeatureChangeTime_OnRead);
        }
    }
};

inline BOOL wil_HasFeatureTestState(unsigned int featureId, _Out_opt_ wil_FeatureEnabledState* state)
{
    wil_details_FeatureTestState test;
    BOOL found = wil_details_HasTestState(featureId, wil_details_FeatureTestStateKind_EnabledState, &test);
    if (state)
    {
        *state = found ? test.state : wil_FeatureEnabledState_Default;
    }
    return found;
}

inline BOOL wil_HasFeatureVariantTestState(unsigned int featureId, _Out_ unsigned char* variant, _Out_ unsigned int* payload)
{
    wil_details_FeatureTestState test;
    BOOL found = wil_details_HasTestState(featureId, wil_details_FeatureTestStateKind_Variant, &test);
    *variant = found ? test.variant : 0;
    *payload = found ? test.payload : 0;
    return found;
}
#else
inline BOOL wil_HasFeatureTestState(unsigned int featureId, _Out_opt_ wil_FeatureEnabledState* state)
{
    featureId;
    state;
    return FALSE;
}

inline BOOL wil_HasFeatureVariantTestState(unsigned int featureId, _Out_ unsigned char* variant, _Out_ unsigned int* payload)
{
    featureId;
    variant;
    payload;
    return FALSE;
}
#endif //__WIL_MIN_KERNEL

inline wil_ReportingKind wil_GetCustomReportingKind(unsigned char index)
{
    WI_ASSERT(index < c_wil_details_maxCustomUsageReporting);
    return (wil_ReportingKind)(index + c_wil_details_ReportingKind_CustomBase);
}


#if defined(__cplusplus) && !defined(__WIL_MIN_KERNEL)

#pragma warning(push)
#pragma warning(disable:4714 6262)    // __forceinline not honored, stack size

#include "Result.h"

namespace wil
{
    //! Represents the desired state of a feature for configuration.
    enum class FeatureEnabledState : unsigned char
    {
        Default     = FEATURE_ENABLED_STATE_DEFAULT,
        Disabled    = FEATURE_ENABLED_STATE_DISABLED,
        Enabled     = FEATURE_ENABLED_STATE_ENABLED,
    };

    //! Represents the priority of a feature configuration change.
    enum class FeatureControlKind : unsigned char
    {
        All         = wil_FeatureEnabledStateKind_All,      //!< use to reset state for all 'kinds' for a feature
        Service     = wil_FeatureEnabledStateKind_Service,  //!< use to set the feature state on behalf of the velocity service (in absense of user setting)
        User        = wil_FeatureEnabledStateKind_User,     //!< use to explicitly configure a feature on or off on behalf of the user
        Testing     = wil_FeatureEnabledStateKind_Test,     //!< use to adjust the state of a feature for testing
    };

    //! Represents a requested change to a particular feature or (with id == 0) a requested change across
    //! all features representing the given rolloutId or priority.
    struct StagingControl
    {
        unsigned int id;                //!< feature ID
        FeatureEnabledState state;      //!< setting (enabled or disabled), default to bypass (delete the setting)
    };

    //! Actions to perform when calling ModifyStagingControls
    enum class StagingControlActions
    {
        Default = 0x00,                 //!< Changes the given controls without affecting others
        Replace = 0x01,                 //!< The group of controls replaces all existing controls of the given FeatureControlKind
    };
    DEFINE_ENUM_FLAG_OPERATORS(StagingControlActions);

    /// @cond
    namespace details
    {
        // Modifies a feature's property cache in a thread-safe manner
        template <typename TLambda>
        void ModifyFeatureData(wil_details_FeaturePropertyCache& properties, TLambda&& modifyFunction)
        {
            __WIL_PFeaturePropertyCacheModification fn = [](_Inout_ wil_details_FeaturePropertyCache* cache, void* context) -> BOOL
            {
                return (*reinterpret_cast<TLambda*>(context))(*cache) ? TRUE : FALSE;
            };
            wil_details_ModifyFeatureData(&properties, fn, &modifyFunction);
        }

        inline void EnsureCoalescedTimer_SetTimer(WIL_UniqueThreadpoolTimer& timer, bool& timerSet, long long delay)
        {
            if (timer)
            {
                const auto allowedWindow = static_cast<DWORD>(delay) / 4;
                FILETIME dueTime;
                *reinterpret_cast<PLONGLONG>(&dueTime) = -static_cast<LONGLONG>(delay * 10000);
                WIL_SetThreadpoolTimer(timer.get(), &dueTime, 0, allowedWindow);
                timerSet = true;
            }
        }

        template <typename T>
        void EnsureCoalescedTimer(WIL_UniqueThreadpoolTimer& timer, bool& timerSet, T* callbackObject)
        {
            if (!timerSet)
            {
                if (!timer)
                {
                    timer.reset(::CreateThreadpoolTimer([](PTP_CALLBACK_INSTANCE, PVOID context, PTP_TIMER)
                    {
                        reinterpret_cast<T*>(context)->OnTimer();
                    }, callbackObject, nullptr));
                }
                const long long delay = 5 * 60 * 1000;  // 5 mins
                EnsureCoalescedTimer_SetTimer(timer, timerSet, delay);
            }
        }

#if !defined(WIL_DISABLE_SRUM_WNF)
        template <typename T>
        void EnsureCoalescedTimerSRUM(WIL_UniqueThreadpoolTimer& timer, bool& timerSet, T* callbackObject)
        {
            if (!timerSet)
            {
                if (!timer)
                {
                    timer.reset(::CreateThreadpoolTimer([](PTP_CALLBACK_INSTANCE, PVOID context, PTP_TIMER)
                    {
                        reinterpret_cast<T*>(context)->OnSRUMTimer();
                    }, callbackObject, nullptr));
                }
                const long long delay = 5 * 1000;  // 5 sec
                EnsureCoalescedTimer_SetTimer(timer, timerSet, delay);
            }
        }
#endif // WIL_DISABLE_SRUM_WNF

        class StagingFailureInformation : public FEATURE_ERROR
        {
        public:
            StagingFailureInformation()
            {
                ::ZeroMemory(static_cast<FEATURE_ERROR*>(this), sizeof(FEATURE_ERROR));
            }

            StagingFailureInformation(const FailureInfo& info, const DiagnosticsInfo& diagnostics, void* returnAddress)
            {
                ::ZeroMemory(static_cast<FEATURE_ERROR*>(this), sizeof(FEATURE_ERROR));

                // Note: Currently we are (by design) dropping the return addresses (returnAddress and info.returnAddress) as
                // ALMOST EVERYONE includes file and line information already making these redundant.
                // If it ever becomes vogue NOT to include that information, then we persist these addresses.
                returnAddress;

                hr = info.hr;
                lineNumber = static_cast<unsigned short>(info.uLineNumber);
                file = info.pszFile;
                if (GetModuleInformationFromAddress(nullptr, nullptr, m_processName, ARRAYSIZE(m_processName)))
                {
                    process = m_processName;
                }
                modulePath = info.pszModule;
                if (GetModuleInformationFromAddress(info.callerReturnAddress, &callerReturnAddressOffset, m_callerModule, ARRAYSIZE(m_callerModule)))
                {
                    callerModule = m_callerModule;
                }
                if (info.pszMessage && *info.pszMessage)
                {
                    ::wil::details::StringCchPrintfA(m_message, ARRAYSIZE(m_message), "%ws", info.pszMessage);
                    message = m_message;
                }

                originLineNumber = diagnostics.line;
                originFile = diagnostics.file;
                if (g_pfnGetModuleName)
                {
                    originModule = g_pfnGetModuleName();
                }
                if (GetModuleInformationFromAddress(diagnostics.returnAddress, &originCallerReturnAddressOffset, m_originCallerModule, ARRAYSIZE(m_originCallerModule)))
                {
                    originCallerModule = m_originCallerModule;
                }
                originName = diagnostics.name;
            }

        private:
            char m_processName[64];
            char m_callerModule[64];
            char m_originCallerModule[64];
            char m_message[96];
        };
    }

    // WARNING: EVERYTHING in this namespace must be handled WITH CARE as the entities defined within
    //          are used as an in-proc ABI contract between binaries that utilize WIL.  Making changes
    //          that add v-tables or change the storage semantics of anything herein needs to be done
    //          with care and respect to versioning.
    namespace details_abi
    {
        struct buffer_range
        {
            unsigned char* buffer = nullptr;
            unsigned char* bufferEnd = nullptr;
            unsigned char* bufferAllocEnd = nullptr;

            buffer_range() = default;
            buffer_range(void* bufferParam, size_t allocatedSize, size_t size = 0) :
                buffer(static_cast<unsigned char*>(bufferParam)), bufferEnd(buffer + size), bufferAllocEnd(buffer + allocatedSize)
            {
                WI_ASSERT(size <= allocatedSize);
            }

            size_t size() const { return (bufferEnd - buffer); }
            size_t capacity() const { return (bufferAllocEnd - buffer); }
            size_t remaining_capacity() const { return (bufferEnd >= bufferAllocEnd) ? 0 : (bufferAllocEnd - bufferEnd); }
            void clear() { bufferEnd = buffer; }
            bool empty() { return (bufferEnd == buffer); }
        };

        struct heap_buffer : buffer_range
        {
            unique_process_heap_ptr<void> allocation;

            heap_buffer() = default;

            // Note: Does not take ownership of the given buffer, so may be used to pre-seed with a stack buffer and then subsequently
            // reserve or ensure space to grow into the heap.
            heap_buffer(void* bufferParam, size_t allocatedSize, size_t size = 0) : buffer_range(bufferParam, allocatedSize, size)
            {
            }

            bool reserve(size_t newCapacity)
            {
                static_assert(sizeof(allocation) == sizeof(void*), "ABI contract, must not carry any code or be changed");
                if (capacity() < newCapacity)
                {
                    newCapacity += (64 - (newCapacity % 64));

                    auto newBuffer = static_cast<unsigned char*>(::HeapAlloc(::GetProcessHeap(), 0, newCapacity));
                    if (!newBuffer)
                    {
                        return false;
                    }

                    auto currentSize = size();
                    memcpy_s(newBuffer, newCapacity, buffer, size());
                    allocation.reset(newBuffer);
                    buffer = newBuffer;
                    bufferEnd = newBuffer + currentSize;
                    bufferAllocEnd = newBuffer + newCapacity;
                }
                return true;
            }

            bool ensure(size_t appendSize)
            {
                auto requiredSize = (size() + appendSize);
                auto currentCapacity = capacity();
                if (requiredSize < currentCapacity)
                {
                    return true;
                }
                currentCapacity *= 2;
                return reserve(appendSize < currentCapacity ? currentCapacity : appendSize);
            }

            void set_size(size_t newSize)
            {
                WI_ASSERT(buffer + newSize <= bufferAllocEnd);
                auto newEnd = buffer + newSize;
                if (WI_VERIFY(newEnd <= bufferAllocEnd))
                {
                    bufferEnd = newEnd;
                }
            }

            bool push_back(_In_reads_bytes_(appendSize) const void* data, size_t appendSize)
            {
                if (!ensure(appendSize))
                {
                    return false;
                }
                WI_ASSERT(appendSize <= remaining_capacity());
                memcpy_s(bufferEnd, remaining_capacity(), data, appendSize);
                bufferEnd += appendSize;
                return true;
            }
        };

        // NOTE: POD types only, no constructors, destrucotrs, v-tables, etc
        template <typename T>
        class heap_vector
        {
        public:
            size_t size() { return (m_data.size() / sizeof(T)); }
            T* data() { return reinterpret_cast<T*>(m_data.buffer); }
            void clear() { m_data.clear(); }

            bool push_back(const T& value)
            {
                return m_data.push_back(&value, sizeof(value));
            }
        private:
            heap_buffer m_data;
        };
    }
    /// @endcond
}

/// @cond
#ifndef __STAGING_TEST_HOOK_USAGE
#define __STAGING_TEST_HOOK_USAGE(isDeviceUsage, id, kind, addend)
#define __STAGING_TEST_HOOK_ERROR(id, info, diagnostics, returnAddress)
#define __STAGING_TEST_HOOK_USAGE_UNFILTERED false
#endif
/// @endcond

#if !defined(WIL_ENABLE_STAGING_API)
#ifndef WIL_STAGING_SUPPRESS_TRACELOGGING
#include "TraceLogging.h"
#endif

/// @cond

// WNF states used by this staging header -- removes the dependency on the generated WNF states header
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_1 = { 0xa3bc1c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_2 = { 0xa3bc2475, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_3 = { 0xa3bc2c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_1 = { 0xa3bc3475, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_2 = { 0xa3bc3c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_3 = { 0xa3bc4475, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_1 = { 0xa3bc4c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_2 = { 0xa3bc5475, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_3 = { 0xa3bc5c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_4 = { 0xa3bc6475, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_5 = { 0xa3bc6c75, 0x418a073a };
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_6 = { 0xa3bc7475, 0x418a073a };

/// @endcond
__WIL_CONSTANT const __WIL_WNF_STATE_NAME __WIL_WNF_WIL_FEATURE_USAGE_FOR_SRUM = { 0xa3bc9835, 0x418a073a };

#if !defined(WIL_DISABLE_SRUM_WNF)
WIL_PAGED_FUNCTION inline NTSTATUS wil_details_WriteSRUMWnfUsageBuffer(wil::details_abi::heap_vector<wil_details_FeatureUsageSRUM>* cachedSRUMUsageTrackingData)
{
    WIL_PAGED_CODE();

    NTSTATUS queryStatus = __WIL_STATUS_SUCCESS;
    NTSTATUS updateStatus = __WIL_STATUS_SUCCESS;
    
    if (cachedSRUMUsageTrackingData->size())
    {
        int loopCount = 0;
        do
        {
            unsigned char buffer[4096];
            ULONG bufferSize = sizeof(buffer);
            ULONG dataSize = bufferSize;
            __WIL_WNF_CHANGE_STAMP readChangeStamp;
            queryStatus = WIL_NtQueryWnfStateData(&__WIL_WNF_WIL_FEATURE_USAGE_FOR_SRUM, NULL, NULL, &readChangeStamp, buffer, &dataSize);
            if (queryStatus == __WIL_STATUS_SUCCESS)
            {
                if ((dataSize % sizeof(wil_details_FeatureUsageSRUM)) != 0)
                {
                    // Treat invalid data as empty
                    dataSize = 0;
                }

                // Check to see if this data point has already been logged, if it's unique usage update the cache, if it's custom just append.
                wil_details_FeatureUsageSRUM* items = reinterpret_cast<wil_details_FeatureUsageSRUM*>(buffer);
                unsigned int count = dataSize / sizeof(wil_details_FeatureUsageSRUM);

                for (auto const& feature : wil::make_range(cachedSRUMUsageTrackingData->data(), cachedSRUMUsageTrackingData->size()))
                {
                    bool shouldAdd = true;
                    for (auto &featureUsageCount : wil::make_range(items, count))
                    {
                        if ((featureUsageCount.featureId == feature.featureId) && (featureUsageCount.serviceReportingKind == feature.serviceReportingKind))
                        {
                            featureUsageCount.usageCount += feature.usageCount;
                            shouldAdd = false;
                            break;
                        }
                    }

                    if (shouldAdd && (dataSize + sizeof(wil_details_FeatureUsageSRUM)) <= bufferSize)
                    {
                        items[count] = feature;
                        dataSize += sizeof(wil_details_FeatureUsageSRUM);
                        count++;
                    }
                }

                // Always publish (in case a previous update was missed)
                updateStatus = WIL_NtUpdateWnfStateData(&__WIL_WNF_WIL_FEATURE_USAGE_FOR_SRUM, reinterpret_cast<void*>(buffer), dataSize, nullptr, NULL, readChangeStamp, TRUE);
            }
            loopCount++;
            WI_ASSERT(loopCount < 100);
        } while ((updateStatus == __WIL_STATUS_UNSUCCESSFUL) && (loopCount < 100) && (queryStatus == __WIL_STATUS_SUCCESS));
    }

    NTSTATUS status = (queryStatus != __WIL_STATUS_SUCCESS) ? queryStatus : updateStatus;
    return status;
}
#endif // WIL_DISABLE_SRUM_WNF

namespace wil
{
    typedef unique_any<wil_PStagingConfig, decltype(&::wil_FreeStagingConfig), ::wil_FreeStagingConfig> unique_staging_config;

    /// @cond
    namespace details
    {
        inline void __stdcall UnsubscribeWilWnf(_In_ _Post_invalid_ __WIL_PWNF_USER_SUBSCRIPTION subscription)
        {
            WIL_RtlUnsubscribeWnfNotificationWaitForCompletion(subscription);
        }

        typedef unique_any<__WIL_PWNF_USER_SUBSCRIPTION, decltype(&UnsubscribeWilWnf), UnsubscribeWilWnf> unique_wil_pwnf_user_subscription;
    }

    // WARNING: EVERYTHING in this namespace must be handled WITH CARE as the entities defined within
    //          are used as an in-proc ABI contract between binaries that utilize WIL.  Making changes
    //          that add v-tables or change the storage semantics of anything herein needs to be done
    //          with care and respect to versioning.
    namespace details_abi
    {
        enum class CountSize : unsigned char
        {
            None,
            UnsignedShort,
            UnsignedLong
        };

        struct UsageIndexProperty
        {
            const unsigned short c_size;
            const CountSize c_countSize;

            unsigned int count = 0;
            unsigned short size = 0;
            void* countBuffer = nullptr;
            void* value = nullptr;

            UsageIndexProperty(unsigned short size, CountSize countSize) : c_size(size), c_countSize(countSize)
            {
            }

            int Compare(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize) const
            {
                return ((indexSize == size) ? memcmp(indexData, value, indexSize) : (static_cast<int>(indexSize) - size));
            }

            void Attach(unsigned int newCount, void* valueData, unsigned short valueSize)
            {
                count = newCount;
                size = valueSize;
                countBuffer = nullptr;
                value = valueData;
            }

            void UpdateCount(unsigned int newCount)
            {
                WI_ASSERT(countBuffer);
                WI_ASSERT(c_countSize != CountSize::None);
                if (count != newCount)
                {
                    count = newCount;
                    if (c_countSize == CountSize::UnsignedShort)
                    {
                        unsigned short tempValue = static_cast<unsigned short>(count);
                        memcpy_s(countBuffer, sizeof(unsigned short), &tempValue, sizeof(tempValue));
                    }
                    else if (c_countSize == CountSize::UnsignedLong)
                    {
                        memcpy_s(countBuffer, sizeof(unsigned long), &count, sizeof(unsigned long));
                    }
                }
            }

            bool AddToCount(unsigned int addend)
            {
                const bool update = (c_countSize != CountSize::None);
                if (update)
                {
                    UpdateCount(count + addend);
                }
                return update;
            }

            size_t GetAndValidateCount(size_t maxCount)
            {
                if (count > maxCount)       // data is untrusted, so we truncate count if the buffer was not large enough
                {
                    UpdateCount(static_cast<unsigned int>(maxCount));
                }
                return count;
            }

            size_t GetSize() const
            {
                size_t result = (c_size == 0) ? (sizeof(size) + size) : c_size;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    return (result + sizeof(unsigned short));
                }
                if (c_countSize == CountSize::UnsignedLong)
                {
                    return (result + sizeof(unsigned long));
                }
                return result;
            }

            static size_t GetFixedSize(unsigned short fixedSize, CountSize countSize)
            {
                WI_ASSERT(fixedSize != 0);
                return UsageIndexProperty(fixedSize, countSize).GetSize();
            }

            bool Write(unsigned char*& bufferParam, unsigned char* bufferEnd) const
            {
                unsigned char* buffer = bufferParam;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    if ((buffer + sizeof(unsigned short)) > bufferEnd)
                    {
                        return false;
                    }
                    unsigned short tempValue = static_cast<unsigned short>(count);
                    memcpy_s(buffer, sizeof(unsigned short), &tempValue, sizeof(unsigned short));
                    buffer += sizeof(unsigned short);
                }
                else if (c_countSize == CountSize::UnsignedLong)
                {
                    if ((static_cast<unsigned char*>(buffer) + sizeof(unsigned long)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(buffer, sizeof(unsigned long), &count, sizeof(unsigned long));
                    buffer += sizeof(unsigned long);
                }

                if (c_size == 0)
                {
                    if ((buffer + sizeof(size)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(buffer, bufferEnd - buffer, &size, sizeof(size));
                    buffer += sizeof(size);
                }

                if ((buffer + size) > bufferEnd)
                {
                    return false;
                }
                memcpy_s(buffer, bufferEnd - buffer, value, size);
                buffer += size;
                bufferParam = buffer;
                return true;
            }

            bool Read(unsigned char*& bufferParam, unsigned char* bufferEnd)
            {
                unsigned char* buffer = bufferParam;
                if (c_countSize == CountSize::UnsignedShort)
                {
                    if ((buffer + sizeof(unsigned short)) > bufferEnd)
                    {
                        return false;
                    }
                    countBuffer = buffer;
                    unsigned short tempValue;
                    memcpy_s(&tempValue, sizeof(unsigned short), buffer, sizeof(unsigned short));
                    buffer += sizeof(unsigned short);
                    count = tempValue;
                }
                else if (c_countSize == CountSize::UnsignedLong)
                {
                    if ((buffer + sizeof(unsigned long)) > bufferEnd)
                    {
                        return false;
                    }
                    countBuffer = buffer;
                    memcpy_s(&count, sizeof(unsigned long), buffer, sizeof(unsigned long));
                    buffer += sizeof(unsigned long);
                }

                size = c_size;
                if (c_size == 0)
                {
                    if ((buffer + sizeof(size)) > bufferEnd)
                    {
                        return false;
                    }
                    memcpy_s(&size, sizeof(size), buffer, sizeof(size));
                    buffer += sizeof(size);
                }

                if ((buffer + size) > bufferEnd)
                {
                    return false;
                }
                value = buffer;
                buffer += size;
                bufferParam = buffer;
                return true;
            }
        };

        // Manages an efficient serialized, contiguous sorted index over two properties allowing the representation
        // of various forms of usage data in WNF state (or other contiguous memory).  Format:
        //
        // Header: header
        // [Repeating until end of buffer]:
        //      IndexEntry:  UsageIndexProperty [Index]
        //      [Repeating for IndexEntry.count]
        //          ValueEntry:  UsageIndexProperty [Value]
        class RawUsageIndex
        {
        public:
            static const unsigned short c_formatVersion = 0;
            const unsigned short c_usageDataVersion;

            // Note that this implementation uses non-static constants, rather than template parameters to minimize
            // template bloat for this (fairly large) class to manage varying forms of data.
            const unsigned short c_indexSize;
            const CountSize c_indexCountSize;
            const unsigned short c_valueSize;
            const CountSize c_valueCountSize;

            RawUsageIndex(unsigned short version, unsigned short indexSize, CountSize indexCountSize, unsigned short valueSize, CountSize valueCountSize) :
                c_usageDataVersion(version), c_indexSize(indexSize), c_indexCountSize(indexCountSize), c_valueSize(valueSize), c_valueCountSize(valueCountSize),
                c_fixedValueSize((valueSize > 0) ? UsageIndexProperty::GetFixedSize(c_valueSize, c_valueCountSize) : 0)
            {
            }

            void Swap(RawUsageIndex& other)
            {
                WI_ASSERT((c_indexSize == other.c_indexSize) && (c_indexCountSize == other.c_indexCountSize) && (c_valueSize == other.c_valueSize) && (c_valueCountSize == other.c_valueCountSize));

                wistd::swap_wil(m_buffer, other.m_buffer);
                wistd::swap_wil(m_isDirty, other.m_isDirty);
                wistd::swap_wil(m_sourceNewer, other.m_sourceNewer);
            }

            bool IsDirty() const { return m_isDirty; }
            void* GetData() const { return m_buffer.buffer; }
            size_t GetDataSize() const { return m_buffer.size(); }
            unsigned short IsSourceNewer() const { return m_sourceNewer; }
            void SetAllowGrowth(bool allowGrowth) { m_allowGrowth = allowGrowth; }

            void EnsureAllocated()
            {
                if (m_buffer.buffer != m_buffer.allocation.get())
                {
                    heap_buffer newBuffer;
                    newBuffer.push_back(m_buffer.buffer, m_buffer.size());
                    m_buffer = wistd::move(newBuffer);
                }
            }

            void SetBuffer(_In_reads_bytes_(size) void* buffer, size_t size, size_t alloc)
            {
                FAIL_FAST_IF(alloc < sizeof(Header));

                m_buffer.allocation.reset();
                m_buffer.buffer = static_cast<unsigned char *>(buffer);
                m_buffer.bufferEnd = m_buffer.buffer + size;
                m_buffer.bufferAllocEnd = m_buffer.buffer + alloc;
                m_sourceNewer = false;

                auto header = static_cast<Header*>(buffer);
                bool valid = (size >= sizeof(*header));
                if (valid)
                {
                    m_sourceNewer = ((header->formatVersion > c_formatVersion) || (header->version > c_usageDataVersion));

                    if ((header->formatVersion != c_formatVersion) || (header->version != c_usageDataVersion) ||
                        (header->indexSize != c_indexSize) || (header->indexCountSize != c_indexCountSize) ||
                        (header->valueSize != c_valueSize) || (header->valueCountSize != c_valueCountSize))
                    {
                        valid = false;
                    }
                }

                if (!valid)
                {
                    header->formatVersion = c_formatVersion;
                    header->version = c_usageDataVersion;
                    header->indexSize = c_indexSize;
                    header->indexCountSize = c_indexCountSize;
                    header->valueSize = c_valueSize;
                    header->valueCountSize = c_valueCountSize;

                    m_buffer.bufferEnd = m_buffer.buffer + sizeof(Header);
                }
            }

            bool Iterate(wistd::function<bool(void*, size_t, void*, size_t, unsigned int)> callback) const
            {
                auto buffer = m_buffer.buffer + sizeof(Header);
                UsageIndexProperty index(c_indexSize, c_indexCountSize);
                UsageIndexProperty value(c_valueSize, c_valueCountSize);

                while (index.Read(buffer, m_buffer.bufferEnd))
                {
                    for (unsigned int valueEntry = 0; (valueEntry < index.count) && value.Read(buffer, m_buffer.bufferEnd); valueEntry++)
                    {
                        if (!callback(index.value, index.size, value.value, value.size, value.count))
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            template <typename TIndex, typename TValue>
            bool RecordUsage(TIndex index, TValue value, unsigned int addend = 1)
            {
                return RecordUsage(&index, sizeof(index), &value, sizeof(value), addend);
            }

            template <typename TIndex>
            bool RecordUsage(TIndex index, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                return RecordUsage(&index, sizeof(index), value, valueSize, addend);
            }

            bool RecordUsage(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                if (RecordUsageInternal(indexData, indexSize, valueData, valueSize, addend))
                {
                    return true;
                }

                const size_t maxRequiredSize = (indexSize + valueSize) + 32;      // 32 for overhead approximation (counts, sizes)
                if (!m_buffer.buffer)
                {
                    heap_buffer heapBuffer;
                    if (heapBuffer.ensure(maxRequiredSize + sizeof(Header)))
                    {
                        SetBuffer(heapBuffer.buffer, 0, heapBuffer.capacity());
                        m_buffer.allocation = wistd::move(heapBuffer.allocation);
                        SetAllowGrowth(true);
                    }
                }
                else if (m_allowGrowth)
                {
                    m_buffer.ensure(maxRequiredSize);
                }

                return RecordUsageInternal(indexData, indexSize, valueData, valueSize, addend);
            }

        private:
            const size_t c_fixedValueSize;

            struct Header
            {
                unsigned short formatVersion;
                unsigned short version;
                unsigned short indexSize;
                unsigned short valueSize;
                CountSize indexCountSize;
                CountSize valueCountSize;
            };

            heap_buffer m_buffer;

            bool m_isDirty = false;
            bool m_sourceNewer = false;
            bool m_allowGrowth = false;

            // NOTE: Directly adapted from STL's version only accounting for our variable size buffers
            unsigned char* LowerBound(unsigned char* buffer, size_t count, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize)
            {
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                unsigned char* first = buffer;
                while (count > 0)
                {
                    size_t count2 = count / 2;
                    auto middle = first;
                    middle += (count2 * c_fixedValueSize);

                    WI_VERIFY(value.Read(middle, m_buffer.bufferEnd));
                    if (value.Compare(valueData, valueSize) > 0)
                    {
                        first = middle;
                        count -= count2 + 1;
                    }
                    else
                    {
                        count = count2;
                    }
                }
                return first;
            }

            unsigned char* FindInsertionPointOrIncrement(UsageIndexProperty& index, unsigned char* buffer, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend)
            {
                int compare = -1;
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                if (c_fixedValueSize > 0)
                {
                    size_t count = index.GetAndValidateCount(m_buffer.size() / c_fixedValueSize);
                    unsigned char* last = buffer + (count * c_fixedValueSize);
                    buffer = LowerBound(buffer, count, valueData, valueSize);
                    if (buffer < last)
                    {
                        auto next = buffer;
                        WI_VERIFY(value.Read(next, m_buffer.bufferEnd));
                        compare = value.Compare(valueData, valueSize);
                    }
                }
                else
                {
                    for (unsigned int valuesRead = 0; (valuesRead < index.count); valuesRead++)
                    {
                        auto next = buffer;
                        if (!value.Read(next, m_buffer.bufferEnd))
                        {
                            index.UpdateCount(valuesRead);
                            break;
                        }
                        compare = value.Compare(valueData, valueSize);
                        if (compare <= 0)
                        {
                            break;
                        }
                        buffer = next;
                    }
                }

                if (compare == 0)
                {
                    // We found the match -- add to it if we need to and we're done
                    m_isDirty = (value.AddToCount(addend) || m_isDirty);
                    return nullptr;
                }

                return buffer;
            }

            unsigned char* SkipValues(UsageIndexProperty& index, unsigned char* buffer)
            {
                if (c_fixedValueSize > 0)
                {
                    auto indexCount = index.GetAndValidateCount(m_buffer.size() / c_fixedValueSize);
                    buffer += (c_fixedValueSize * indexCount);
                }
                else
                {
                    // We didn't find the index item, so continue on to the next index item after skipping the values...
                    UsageIndexProperty value(c_valueSize, c_valueCountSize);
                    unsigned int valuesRead = 0;
                    for (; (valuesRead < index.count) && value.Read(buffer, m_buffer.bufferEnd); valuesRead++)
                    {
                    }
                    index.UpdateCount(valuesRead);
                }
                return buffer;
            }

            bool RecordUsageInternal(_In_reads_bytes_(indexSize) void* indexData, size_t indexSize, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                if (!m_buffer.buffer)
                {
                    return false;
                }

                auto buffer = m_buffer.buffer + sizeof(Header);

                UsageIndexProperty index(c_indexSize, c_indexCountSize);
                bool foundIndex = false;
                bool validRead = false;
                auto prev = buffer;
                while ((validRead = index.Read(buffer, m_buffer.bufferEnd)), validRead)
                {
                    int compare = index.Compare(indexData, indexSize);
                    if (compare < 0)
                    {
                        // break so that we can do the index and value insertion below...
                        buffer = prev;
                        break;
                    }
                    if (compare == 0)
                    {
                        buffer = FindInsertionPointOrIncrement(index, buffer, valueData, valueSize, addend);
                        if (!buffer)
                        {
                            // No insertion pointer, we're done (incremented).
                            return true;
                        }

                        // Break so that we can do the value insertion below...
                        foundIndex = true;
                        break;
                    }

                    // Skip all the values in the buffer to the next index item...
                    buffer = SkipValues(index, buffer);
                    prev = buffer;
                }

                WI_ASSERT(buffer <= m_buffer.bufferEnd);
                WI_ASSERT(m_buffer.bufferEnd <= m_buffer.bufferAllocEnd);
                if (!validRead)
                {
                    // we don't trust our input data, so if we got to the end and there is any more garbage remaining then
                    // truncate at that position...
                    m_buffer.bufferEnd = buffer;
                }

                // Figure out how much space we need for insertion...
                size_t requiredIndexBytes = 0;
                if (!foundIndex)
                {
                    index.Attach(1, indexData, static_cast<unsigned short>(indexSize));
                    requiredIndexBytes = index.GetSize();
                }
                UsageIndexProperty value(c_valueSize, c_valueCountSize);
                value.Attach(addend, valueData, static_cast<unsigned short>(valueSize));
                const size_t requiredBytes = requiredIndexBytes + value.GetSize();

                // Ensure we have enough space for what we want to insert
                const size_t availableBytes = m_buffer.remaining_capacity();
                if (availableBytes < requiredBytes)
                {
                    return false;
                }

                // Move the memory and do the insertion in the leftover hole
                const size_t moveBytes = m_buffer.bufferEnd - buffer;
                memmove_s(buffer + requiredBytes, (m_buffer.bufferAllocEnd - buffer) - requiredBytes, buffer, moveBytes);
                m_buffer.bufferEnd += requiredBytes;
                if (!foundIndex)
                {
                    WI_VERIFY(index.Write(buffer, m_buffer.bufferEnd));
                }
                else
                {
                    index.AddToCount(1);
                }
                WI_VERIFY(value.Write(buffer, m_buffer.bufferEnd));
                WI_ASSERT((buffer + moveBytes) == m_buffer.bufferEnd);
                m_isDirty = true;
                return true;
            }
        };

        template <typename TIndex, typename TValue, CountSize countSize>
        class UsageIndex : public RawUsageIndex
        {
        public:
            UsageIndex(unsigned short version) : RawUsageIndex(version, sizeof(TIndex), CountSize::UnsignedShort, sizeof(TValue), countSize)
            {
            }

            __declspec(noinline) bool RecordUsage(TIndex index, TValue value, unsigned int addend = 1)
            {
                return RawUsageIndex::RecordUsage(&index, sizeof(index), &value, sizeof(value), addend);
            }

            bool Iterate(wistd::function<bool(TIndex, TValue, unsigned int)> callback) const
            {
                return RawUsageIndex::Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int count) -> bool
                {
                    if (WI_VERIFY((indexSize == sizeof(TIndex)) && (valueSize == sizeof(TValue))))
                    {
                        if (!callback(*reinterpret_cast<TIndex*>(index), *reinterpret_cast<TValue*>(value), count))
                        {
                            return false;
                        }
                    }
                    return true;
                });
            }
        };

        template <typename TIndex, CountSize countSize>
        class UsageIndexBuffer : public RawUsageIndex
        {
        public:
            UsageIndexBuffer(unsigned short version) : RawUsageIndex(version, sizeof(TIndex), CountSize::UnsignedShort, 0, countSize)
            {
            }

            bool RecordUsage(TIndex index, _In_reads_bytes_(valueSize) void* valueData, size_t valueSize, unsigned int addend = 1)
            {
                return RawUsageIndex::RecordUsage(&index, sizeof(index), valueData, valueSize, addend);
            }

            bool Iterate(wistd::function<bool(TIndex, void*, size_t, unsigned int)> callback) const
            {
                return RawUsageIndex::Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int count) -> bool
                {
                    if (WI_VERIFY(indexSize == sizeof(TIndex)))
                    {
                        if (!callback(*reinterpret_cast<TIndex*>(index), value, valueSize, count))
                        {
                            return false;
                        }
                    }
                    return true;
                });
            }
        };

        struct SerializedFailure
        {
            // NOTE: structure is serialized; variable order maintains packing efficiency
            HRESULT hr;
            unsigned short lineNumber;
            unsigned short file;
            unsigned short modulePath;
            unsigned short callerModule;
            unsigned int callerReturnAddressOffset;
            unsigned short message;
            unsigned short originLineNumber;
            unsigned short originFile;
            unsigned short originModule;
            unsigned int originCallerReturnAddressOffset;
            unsigned short originCallerModule;
            unsigned short originName;
            unsigned short process;

            _Success_return_ static bool Serialize(const FEATURE_ERROR& info, _Out_ size_t* requiredParam, _Out_writes_bytes_to_opt_(capacity, *requiredParam) void* bufferParam, size_t capacity)
            {
                auto bufferStart = static_cast<unsigned char*>(bufferParam);
                auto buffer = bufferStart;
                auto bufferEnd = buffer + capacity;

                SerializedFailure headerStack;
                size_t& required = *requiredParam;
                required = sizeof(SerializedFailure);
                SerializedFailure& header = (required <= capacity) ? *reinterpret_cast<SerializedFailure*>(buffer) : headerStack;
                if (required <= capacity)
                {
                    ::ZeroMemory(&header, sizeof(header));
                    header.hr = info.hr;
                    header.lineNumber = info.lineNumber;
                    header.callerReturnAddressOffset = info.callerReturnAddressOffset;
                    header.originLineNumber = info.originLineNumber;
                    header.originCallerReturnAddressOffset = info.originCallerReturnAddressOffset;
                    buffer += sizeof(header);
                }

                auto writeString = [&](_In_opt_ PCSTR string, unsigned short& offsetVar, unsigned short alternateOffset = 0)
                {
                    if (string)
                    {
                        // We may be able to collapse the string
                        if ((alternateOffset > 0) && (0 == strcmp(string, reinterpret_cast<PCSTR>(bufferStart + alternateOffset))))
                        {
                            offsetVar = alternateOffset;
                        }
                        else
                        {
                            auto stringSize = strlen(string) + 1;
                            required += stringSize;
                            if (required <= capacity)
                            {
                                memcpy_s(buffer, bufferEnd - buffer, string, stringSize);
                                offsetVar = static_cast<unsigned short>(buffer - bufferStart);
                                buffer += stringSize;
                            }
                        }
                    }
                };

                writeString(info.file, header.file);
                writeString(info.process, header.process);
                writeString(info.modulePath, header.modulePath, header.process);
                writeString(info.callerModule, header.callerModule, header.modulePath);
                writeString(info.message, header.message);
                writeString(info.originFile, header.originFile, header.file);
                writeString(info.callerModule, header.callerModule, header.modulePath);
                writeString(info.originModule, header.originModule, header.modulePath);
                writeString(info.originCallerModule, header.originCallerModule, header.originModule);
                writeString(info.originName, header.originName);

                return (required <= capacity);
            }

            _Success_return_ static bool Deserialize(details::StagingFailureInformation& info, _In_reads_bytes_(size) void* bufferParam, size_t size)
            {
                auto bufferStart = static_cast<char*>(bufferParam);
                auto buffer = bufferStart;
                auto bufferEnd = buffer + size;

                if ((buffer + sizeof(SerializedFailure)) < bufferEnd)
                {
                    const SerializedFailure& header = *reinterpret_cast<const SerializedFailure*>(buffer);

                    info.hr = header.hr;
                    info.lineNumber = header.lineNumber;
                    info.callerReturnAddressOffset = header.callerReturnAddressOffset;
                    info.originLineNumber = header.originLineNumber;
                    info.originCallerReturnAddressOffset = header.originCallerReturnAddressOffset;

                    auto readString = [&](PCSTR& value, unsigned short offsetVar)
                    {
                        if (offsetVar > 0)
                        {
                            auto start = buffer + offsetVar;
                            if ((start < bufferEnd) && SUCCEEDED(wil::details::StringCchLengthA(start, bufferEnd - start, nullptr)))
                            {
                                value = start;
                            }
                        }
                    };

                    readString(info.file, header.file);
                    readString(info.process, header.process);
                    readString(info.modulePath, header.modulePath);
                    readString(info.callerModule, header.callerModule);
                    readString(info.message, header.message);
                    readString(info.originFile, header.originFile);
                    readString(info.callerModule, header.callerModule);
                    readString(info.originModule, header.originModule);
                    readString(info.originCallerModule, header.originCallerModule);
                    readString(info.originName, header.originName);

                    return true;
                }
                return false;
            }
        };

        inline bool ReadWnfUsageBuffer(const __WIL_WNF_STATE_NAME* state, _Out_writes_bytes_(countBytes) unsigned char* buffer, size_t countBytes, _Inout_ RawUsageIndex& usage, _Out_ __WIL_WNF_CHANGE_STAMP* changeStamp)
        {
            ULONG size = static_cast<ULONG>(countBytes);
            const auto status = WIL_NtQueryWnfStateData(state, nullptr, nullptr, changeStamp, buffer, &size);
            const auto hr = wil::details::NtStatusToHr(status); hr;
            if (STATUS_SUCCESS != status)
            {
                size = 0;
                *changeStamp = 0;
            }

            usage.SetBuffer(buffer, size, countBytes);
            return (!usage.IsSourceNewer());
        }

        // Returns false when the buffer has been updated outside of the request
        inline bool WriteWnfUsageBuffer(const __WIL_WNF_STATE_NAME* state, __WIL_WNF_CHANGE_STAMP match, RawUsageIndex& usage)
        {
            if (usage.IsDirty())
            {
                const auto status = WIL_NtUpdateWnfStateData(state, usage.GetData(), static_cast<ULONG>(usage.GetDataSize()), nullptr, nullptr, match, TRUE);
                if (STATUS_UNSUCCESSFUL == status)
                {
                    return false;
                }

                if (status != __WIL_STATUS_SUCCESS)
                {
                    // if there's something wrong with the key, we try to just overwrite anyway... if it doesn't work, then we drop the usage
                    // and return 'true' to cease retries.
                    WIL_NtUpdateWnfStateData(state, usage.GetData(), static_cast<ULONG>(usage.GetDataSize()), nullptr, nullptr, 0, FALSE);
                }
            }
            return true;
        }

        inline void RecordWnfUsageIndex(_In_reads_(count) const __WIL_WNF_STATE_NAME* states, size_t count, const RawUsageIndex& usage)
        {
            WI_ASSERT(count > 0);
            auto statesEnd = states + count;

            size_t updateAttempts = 0;
            size_t valuesWritten = 0;
            bool fullyWritten = false;
            do
            {
                RawUsageIndex persistedUsage(usage.c_usageDataVersion, usage.c_indexSize, usage.c_indexCountSize, usage.c_valueSize, usage.c_valueCountSize);
                unsigned char buffer[4096];
                __WIL_WNF_CHANGE_STAMP readChangeStamp;
                if (!ReadWnfUsageBuffer(states, buffer, ARRAYSIZE(buffer), persistedUsage, &readChangeStamp))
                {
                    break;
                }
                WI_ASSERT(!persistedUsage.IsDirty());

                size_t read = 0;
                fullyWritten = usage.Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int addend) -> bool
                {
                    if (read >= valuesWritten)
                    {
                        if (!persistedUsage.RecordUsage(index, indexSize, value, valueSize, addend))
                        {
                            return false;
                        }
                    }
                    read++;
                    return true;
                });

                if (WriteWnfUsageBuffer(states, readChangeStamp, persistedUsage))
                {
                    states++;
                    valuesWritten = read;
                }
                else
                {
                    updateAttempts++;
                    fullyWritten = false;
                }
            }
            while (!fullyWritten && (states < statesEnd) && WI_VERIFY(updateAttempts < 50));
        }

        enum class UsageIndexesLoadOptions
        {
            None = 0x0,
            Clear = 0x1,
        };
        DEFINE_ENUM_FLAG_OPERATORS(UsageIndexesLoadOptions);

        inline void LoadWnfUsageIndex(UsageIndexesLoadOptions options, _In_reads_(count) const __WIL_WNF_STATE_NAME* states, size_t count, RawUsageIndex& usage)
        {
            unsigned char rootBuffer[4096];
            __WIL_WNF_CHANGE_STAMP readChangeStamp;
            details_abi::ReadWnfUsageBuffer(states, rootBuffer, ARRAYSIZE(rootBuffer), usage, &readChangeStamp);
            usage.SetAllowGrowth(true);

            if (WI_IsFlagSet(options, UsageIndexesLoadOptions::Clear))
            {
                WIL_NtUpdateWnfStateData(&states[0], nullptr, 0ul, nullptr, nullptr, 0, FALSE);
            }

            for (auto& name : make_range(states + 1, states + count))
            {
                details_abi::RawUsageIndex persistedUsage(usage.c_usageDataVersion, usage.c_indexSize, usage.c_indexCountSize, usage.c_valueSize, usage.c_valueCountSize);
                unsigned char buffer[4096];
                details_abi::ReadWnfUsageBuffer(&name, buffer, ARRAYSIZE(buffer), persistedUsage, &readChangeStamp);
                bool hasData = false;
                persistedUsage.Iterate([&](void* index, size_t indexSize, void* value, size_t valueSize, unsigned int addend) -> bool
                {
                    usage.RecordUsage(index, indexSize, value, valueSize, addend);
                    hasData = true;
                    return true;
                });

                if (hasData && WI_IsFlagSet(options, UsageIndexesLoadOptions::Clear))
                {
                    WIL_NtUpdateWnfStateData(&name, nullptr, 0ul, nullptr, nullptr, 0, FALSE);
                }
            }

            usage.EnsureAllocated();
        }

        struct UsageIndexes
        {
            UsageIndex<wil_details_ServiceReportingKind, unsigned int, CountSize::None> device;
            UsageIndex<wil_details_ServiceReportingKind, unsigned int, CountSize::UnsignedLong> unique;
            UsageIndexBuffer<unsigned int, CountSize::UnsignedShort> error;

            // ZEROs below are VERSIONING information for any changes to these index types
            UsageIndexes() : device(0), unique(0), error(0)
            {
            }

            void Record()
            {
                if (device.IsDirty())
                {
                    __WIL_WNF_STATE_NAME names[] = { __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_1, __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_2,
                        __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_3 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), device);
                }

                if (unique.IsDirty())
                {
                    __WIL_WNF_STATE_NAME names[] = { __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_1, __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_2,
                        __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_3 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), unique);
                }

                if (error.IsDirty())
                {
                    __WIL_WNF_STATE_NAME names[] = { __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_1, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_2,
                        __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_3, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_4, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_5,
                        __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_6 };
                    RecordWnfUsageIndex(names, ARRAYSIZE(names), error);
                }
            }

            void Load(UsageIndexesLoadOptions options)
            {
                __WIL_WNF_STATE_NAME deviceNames[] = { __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_1, __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_2,
                    __WIL_WNF_WIL_FEATURE_DEVICE_USAGE_TRACKING_3 };
                LoadWnfUsageIndex(options, deviceNames, ARRAYSIZE(deviceNames), device);

                __WIL_WNF_STATE_NAME uniqueNames[] = { __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_1, __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_2,
                    __WIL_WNF_WIL_FEATURE_USAGE_TRACKING_3 };
                LoadWnfUsageIndex(options, uniqueNames, ARRAYSIZE(uniqueNames), unique);

                __WIL_WNF_STATE_NAME errorNames[] = { __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_1, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_2,
                    __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_3, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_4, __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_5,
                    __WIL_WNF_WIL_FEATURE_HEALTH_TRACKING_6 };
                LoadWnfUsageIndex(options, errorNames, ARRAYSIZE(errorNames), error);
            }
        };

        // This class manages a list of subscriptions (e.g. EnabledStateManager subscribing to state changes via FeatureStateManager)
        class SubscriptionList
        {
        public:
            void SubscribeUnderLock(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
            {
                // Note:  Handle value is always (Index + 1) to retain slot ZERO as invalid (nullptr)
                *subscription = nullptr;
                for (size_t index = 0; index < m_subscriptions.size(); index++)
                {
                    if (m_subscriptions.data()[index].callback == nullptr)
                    {
                        m_subscriptions.data()[index] = Subscription{ callback, context };
                        *subscription = reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(index + 1);
                        return;
                    }
                }
                if (m_subscriptions.push_back(Subscription{ callback, context }))
                {
                    *subscription = reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(m_subscriptions.size());
                }
            }

            void Subscribe(WIL_SRWLock& srwlock, _Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
            {
                auto lock = srwlock.lock_exclusive();
                SubscribeUnderLock(subscription, callback, context);
            }

            void Unsubscribe(WIL_SRWLock& srwlock, _In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                if (subscription)
                {
                    auto unsubscribeLock = m_unsubscribeLock.lock();
                    auto lock = srwlock.lock_exclusive();
                    size_t index = (reinterpret_cast<size_t>(subscription) - 1);
                    if (index < m_subscriptions.size())
                    {
                        m_subscriptions.data()[index] = Subscription{};
                    }
                }
            }

            void OnSignaled(WIL_SRWLock& srwlock) WI_NOEXCEPT
            {
                size_t subscriptionCount = 0;
                {
                    auto lock = srwlock.lock_shared();
                    subscriptionCount = m_subscriptions.size();
                }

                size_t subscription = 0;
                while (subscription < subscriptionCount)
                {
                    Subscription callback = {};
                    auto unsubscribeLock = m_unsubscribeLock.lock();

                    {
                        auto lock = srwlock.lock_exclusive();
                        for (; (subscription < subscriptionCount); subscription++)
                        {
                            if (m_subscriptions.data()[subscription].callback)
                            {
                                callback = m_subscriptions.data()[subscription++];
                                break;
                            }
                        }
                    }

                    if (callback.callback)
                    {
                        // Note: Reentrant unsubscribeLock allows caller to unsubscribe in their callback while preventing multiple callbacks from executing simultaneously
                        callback.callback(callback.context);
                    }
                }
            }

        private:
            struct Subscription
            {
                PFEATURE_STATE_CHANGE_CALLBACK callback;
                void* context;
            };

            WIL_Critical_Section m_unsubscribeLock; // ordering:  must always be taken ahead of srwlock
            details_abi::heap_vector<Subscription> m_subscriptions;
        };


        // This class manages a cache of the globally tracked usage and error information so that every module
        // doesn't itself end up with a cache of the same information.
        class FeatureStateData
        {
        private:
            // Declared here to enable use of decltype(m_lock.lock_exclusive()) below
            WIL_SRWLock m_lock;
            UsageIndexes m_usage;
            SubscriptionList m_processWideUsageFlushSubscriptions;

        public:
            // note: not called during process shutdown
            ~FeatureStateData()
            {
                ProcessShutdown();
            }

            // called by virtue of being held by process shared memory (final release when terminating)
            void ProcessShutdown()
            {
                UsageIndexes indexes;
                RetrieveUsageUnderLock(indexes);
                indexes.Record();
            }

            bool RecordFeatureError(unsigned int featureId, const FEATURE_ERROR& failure) WI_NOEXCEPT
            {
                unsigned char bufferStack[256];
                details_abi::heap_buffer buffer(reinterpret_cast<void*>(bufferStack), ARRAYSIZE(bufferStack));
                size_t required = 0;
                if (!SerializedFailure::Serialize(failure, &required, buffer.buffer, buffer.capacity()))
                {
                    if (!buffer.ensure(required) || !SerializedFailure::Serialize(failure, &required, buffer.buffer, buffer.capacity()))
                    {
                        // best effort -- memory failure can't record feature error
                        return false;
                    }
                }
                buffer.set_size(required);

                auto lock = m_lock.lock_exclusive();
                return m_usage.error.RecordUsage(featureId, buffer.buffer, buffer.size());
            }

            // we allow external locking for repeat callers of RecordFeatureUsageUnderLock
            decltype(m_lock.lock_exclusive()) lock_exclusive() WI_NOEXCEPT
            {
                return m_lock.lock_exclusive();
            }

            bool RecordFeatureUsageUnderLock(unsigned int featureId, wil_details_ServiceReportingKind kind, size_t addend)
            {
                if ((kind == wil_details_ServiceReportingKind_DeviceUsage) ||
                    (kind == wil_details_ServiceReportingKind_DeviceOpportunity) ||
                    (kind == wil_details_ServiceReportingKind_PotentialDeviceUsage) ||
                    (kind == wil_details_ServiceReportingKind_PotentialDeviceOpportunity) ||
                    ((kind >= wil_details_ServiceReportingKind_VariantDevicePotentialBase) && (kind < wil_details_ServiceReportingKind_VariantUniquePotentialBase)))
                {
                    __STAGING_TEST_HOOK_USAGE(true /* isDeviceUsage */, featureId, kind, addend);
                    m_usage.device.RecordUsage(kind, featureId);
                    return m_usage.device.IsDirty();
                }

                __STAGING_TEST_HOOK_USAGE(false /* isDeviceUsage */, featureId, kind, addend);
                return m_usage.unique.RecordUsage(kind, featureId, static_cast<unsigned int>(addend));
            }

            bool RecordFeatureUsage(unsigned int featureId, wil_details_ServiceReportingKind kind, size_t addend)
            {
                switch (kind)
                {
                case wil_details_ServiceReportingKind_Store:
                    RecordUsage();
                    return true;

                default:
                    if (static_cast<size_t>(kind) < (static_cast<size_t>(wil_details_ServiceReportingKind_CustomDisabledBase) + c_wil_details_maxCustomUsageReporting)
                        || ((kind >= wil_details_ServiceReportingKind_VariantDevicePotentialBase)
                            && (static_cast<size_t>(kind) < (static_cast<size_t>(wil_details_ServiceReportingKind_VariantUniqueUsageBase) + static_cast<size_t>(c_wil_details_ServiceReportingKind_VariantMax)))))
                    {
                        auto lock = m_lock.lock_exclusive();
                        return RecordFeatureUsageUnderLock(featureId, kind, addend);
                    }
                }
                return false;
            }

            void RecordUsage()
            {
                UsageIndexes usageToRecord;
                {
                    auto lock = m_lock.lock_exclusive();
                    RetrieveUsageUnderLock(usageToRecord);
                }
                usageToRecord.Record();
            }

            void SubscribeToProcessWideUsageFlush(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
            {
                m_processWideUsageFlushSubscriptions.Subscribe(m_lock, subscription, callback, context);
            }

            void UnsubscribeToProcessWideUsageFlush(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                m_processWideUsageFlushSubscriptions.Unsubscribe(m_lock, subscription);
            }

            void OnProcessWideUsageFlush() WI_NOEXCEPT
            {
                m_processWideUsageFlushSubscriptions.OnSignaled(m_lock);
            }

        private:
            void RetrieveUsageUnderLock(UsageIndexes& usageToRecord)
            {
                static_assert(sizeof(m_lock) == sizeof(SRWLOCK), "ABI contract, must not carry any code or be changed");

                // Now we pull any dirty records and assign them to our out pointer so that they can be
                // used to persist data to the backing store outside of our lock.

                if (m_usage.device.IsDirty())
                {
                    usageToRecord.device.Swap(m_usage.device);
                }
                if (m_usage.unique.IsDirty())
                {
                    usageToRecord.unique.Swap(m_usage.unique);
                }
                if (m_usage.error.IsDirty())
                {
                    usageToRecord.error.Swap(m_usage.error);
                }
            }
        };
    } // details_abi

    namespace details
    {
        inline void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_opt_ wil_details_FeaturePropertyCache* state, wil_FeatureChangeTime changeTime) WI_NOEXCEPT;

        void __stdcall UnsubscribeProcessWideUsageFlush(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription);

        typedef unique_any<FEATURE_STATE_CHANGE_SUBSCRIPTION, decltype(&UnsubscribeProcessWideUsageFlush), UnsubscribeProcessWideUsageFlush> unique_wil_process_wide_usage_flush_subscription;

        // This class manages the central relationship between staging features and reporting/eventing.
        // It keeps in-memory data structures to track usage and creates timers to occasionally push
        // that usage information into WNF state so that it can be centrally reported.  It also manages
        // the same kind of relationship for reporting feature errors and subscribing to feature notification
        // changes.
        class FeatureStateManager
        {
        public:
            FeatureStateManager() WI_NOEXCEPT : m_dataStorage("WilStaging_02")
            {
                // This object will be held in a zero-initialized global variable.  We want this line to
                // run at construction time so we can distinguish when someone is trying to call us ahead
                // of construction (CRT init order)
                m_fInitialized = true;
            }

            // note: not called during process shutdown
            ~FeatureStateManager() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_timer.reset();
#if !defined(WIL_DISABLE_SRUM_WNF)
                m_SRUMtimer.reset();
#endif // WIL_DISABLE_SRUM_WNF
            }

            // called by virtue of FeatureStateManager being held by shutdown_aware_object
            void ProcessShutdown() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_dataStorage.~ProcessLocalStorage();
            }

            void RecordFeatureError(unsigned int featureId, const FEATURE_ERROR& error) WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    if (g_wil_details_pfnFeatureLoggingHook)
                    {
                        g_wil_details_pfnFeatureLoggingHook(featureId, NULL, &error, TRUE, NULL, NULL, 0, 1);
                    }

                    if (EnsureStateData() && m_data->RecordFeatureError(featureId, error))
                    {
                        if (!ProcessShutdownInProgress())
                        {
                            auto lock = m_lock.lock_exclusive();
                            EnsureTimerUnderLock();
                        }
                    }
                }
            }

            void RecordFeatureUsage(unsigned int featureId, wil_details_ServiceReportingKind kind, size_t addend) WI_NOEXCEPT
            {
                if (m_fInitialized && EnsureStateData() && m_data->RecordFeatureUsage(featureId, kind, addend))
                {
                    if (!ProcessShutdownInProgress())
                    {
                        auto lock = m_lock.lock_exclusive();
                        EnsureTimerUnderLock();
                    }
                }
            }

            inline void SubscribeToEnabledStateChanges(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
            {
                // Note:  Handle value is always (Index + 1) to retain slot ZERO as invalid (nullptr)
                *subscription = nullptr;
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    EnsureSubscribedToStateChangesUnderLock();
                    m_stateChangeSubscriptions.SubscribeUnderLock(subscription, callback, context);
                }
            }

            inline void UnsubscribeToEnabledStateChanges(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                if (m_fInitialized)
                {
                    m_stateChangeSubscriptions.Unsubscribe(m_lock, subscription);
                }
            }

            inline void SubscribeToUsageFlush(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback)
            {
                *subscription = nullptr;
                if (m_fInitialized && EnsureStateData())
                {
                    auto lock = m_lock.lock_exclusive();
                    EnsureSubscribedToProcessWideUsageFlushUnderLock();
                    m_usageFlushSubscriptions.SubscribeUnderLock(subscription, callback, nullptr);

                    if (*subscription != nullptr)
                    {
                        *subscription = reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(reinterpret_cast<size_t>(*subscription) | c_wil_details_usageFlushHandleFlag);
                    }
                }
            }

            inline void UnsubscribeToUsageFlush(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                if (m_fInitialized)
                {
                    m_usageFlushSubscriptions.Unsubscribe(m_lock, subscription);
                }
            }

            inline void UnsubscribeToProcessWideUsageFlush(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
            {
                if (subscription && m_data)
                {
                    m_data->UnsubscribeToProcessWideUsageFlush(subscription);
                }
            }

            void OnTimer() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    {
                        auto lock = m_lock.lock_exclusive();
                        m_timerSet = false;
                    }
                    if (m_data)
                    {
                        m_data->RecordUsage();
                    }
                }
            }

            void OnStateChange() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    m_stateChangeSubscriptions.OnSignaled(m_lock);
                }
            }

            void OnUsageFlushed() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    m_usageFlushSubscriptions.OnSignaled(m_lock);
                }
            }

            void FlushUsage() WI_NOEXCEPT
            {
                if (!ProcessShutdownInProgress() && EnsureStateData())
                {
                    m_data->OnProcessWideUsageFlush();
                }
            }

#if !defined(WIL_DISABLE_SRUM_WNF)
            void QueueBackgroundSRUMUsageReporting(unsigned int featureId, unsigned short serviceReportingKind, UINT32 addend) WI_NOEXCEPT
            {
                if (m_fInitialized && !ProcessShutdownInProgress())
                {
                    auto lock = m_SRUMlock.lock_exclusive();
                    wil_details_FeatureUsageSRUM properties = {};
                    properties.featureId = featureId;
                    properties.serviceReportingKind = serviceReportingKind;
                    properties.usageCount = addend;
                    m_cachedSRUMUsageTrackingData.push_back(properties);
                    EnsureSRUMTimerUnderLock();
                }
            }

            void OnSRUMTimer() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_SRUMlock.lock_exclusive();
                    RecordCachedSRUMUsageUnderLock();
                    m_SRUMtimerSet = false;
                }
            }
#endif // WIL_DISABLE_SRUM_WNF

        private:
            bool m_fInitialized = false;

            details_abi::ProcessLocalStorage<details_abi::FeatureStateData> m_dataStorage;
            details_abi::FeatureStateData* m_data = nullptr;

            WIL_SRWLock m_lock;
#if !defined(WIL_DISABLE_SRUM_WNF)
            WIL_SRWLock m_SRUMlock;
#endif // WIL_DISABLE_SRUM_WNF

            WIL_UniqueThreadpoolTimer m_timer;
#if !defined(WIL_DISABLE_SRUM_WNF)
            WIL_UniqueThreadpoolTimer m_SRUMtimer;
            bool m_SRUMtimerSet = false;
#endif // WIL_DISABLE_SRUM_WNF
            bool m_timerSet = false;
            wil::details::unique_wil_pwnf_user_subscription m_wnfMachineSubscription;
            wil::details::unique_wil_pwnf_user_subscription m_wnfUserSubscription;
            wil::details::unique_wil_process_wide_usage_flush_subscription m_processWideUsageFlushSubscription;

            details_abi::SubscriptionList m_stateChangeSubscriptions;
            details_abi::SubscriptionList m_usageFlushSubscriptions;

#if !defined(WIL_DISABLE_SRUM_WNF)
            details_abi::heap_vector<wil_details_FeatureUsageSRUM> m_cachedSRUMUsageTrackingData;
#endif // WIL_DISABLE_SRUM_WNF

            bool EnsureStateData() WI_NOEXCEPT
            {
                if (!m_data)
                {
                    auto data = m_data ? nullptr : m_dataStorage.GetShared();
                    auto lock = m_lock.lock_exclusive();
                    if (!m_data)
                    {
                        m_data = data;
                    }
                }
                return (m_data != nullptr);
            }

            void EnsureTimerUnderLock() WI_NOEXCEPT
            {
                EnsureCoalescedTimer(m_timer, m_timerSet, this);
            }

#if !defined(WIL_DISABLE_SRUM_WNF)
            void EnsureSRUMTimerUnderLock() WI_NOEXCEPT
            {
                EnsureCoalescedTimerSRUM(m_SRUMtimer, m_SRUMtimerSet, this);
            }

            void RecordCachedSRUMUsageUnderLock() WI_NOEXCEPT
            {
                if (m_cachedSRUMUsageTrackingData.size())
                {
                    wil_details_WriteSRUMWnfUsageBuffer(&m_cachedSRUMUsageTrackingData);

                    m_cachedSRUMUsageTrackingData.clear();
                }
            }
#endif // WIL_DISABLE_SRUM_WNF

            static void EnsureSubscribedToStateChangesUnderLock(wil::details::unique_wil_pwnf_user_subscription& subscription, __WIL_WNF_STATE_NAME state, void* context) WI_NOEXCEPT
            {
                if (!subscription)
                {
                    // Retrieve the latest changestamp and use that as the basis for change notifications
                    __WIL_WNF_CHANGE_STAMP subscribeFrom = 0;
                    ULONG bufferSize = 0;
                    ::WIL_NtQueryWnfStateData(&state, 0, nullptr, &subscribeFrom, nullptr, &bufferSize);

                    WIL_RtlSubscribeWnfStateChangeNotification(&subscription, state, subscribeFrom,
                        [](__WIL_WNF_STATE_NAME, __WIL_WNF_CHANGE_STAMP, __WIL_WNF_TYPE_ID*, void* context, const void*, ULONG) -> NTSTATUS
                    {
                        reinterpret_cast<FeatureStateManager*>(context)->OnStateChange();
                        return STATUS_SUCCESS;
                    }, context, nullptr, 0, 0);
                }
            }

            void EnsureSubscribedToStateChangesUnderLock() WI_NOEXCEPT
            {
                EnsureSubscribedToStateChangesUnderLock(m_wnfMachineSubscription, __WIL_WNF_WIL_MACHINE_FEATURE_STORE, this);
                EnsureSubscribedToStateChangesUnderLock(m_wnfUserSubscription, __WIL_WNF_WIL_USER_FEATURE_STORE, this);
            }

            void EnsureSubscribedToProcessWideUsageFlushUnderLock() WI_NOEXCEPT
            {
                if (!m_processWideUsageFlushSubscription && m_data)
                {
                    m_data->SubscribeToProcessWideUsageFlush(&m_processWideUsageFlushSubscription, [](void* context)
                    {
                        reinterpret_cast<FeatureStateManager*>(context)->OnUsageFlushed();
                    }, this);
                }
            }
        };

#ifndef STAGING_SUPPRESS_STATIC_INITIALIZERS
WI_ODR_PRAGMA("WilApiImpl_FeatureStateManager", "1")
        __declspec(selectany) shutdown_aware_object<FeatureStateManager> g_featureStateManager;
#else
WI_ODR_PRAGMA("WilApiImpl_FeatureStateManager", "0")
        __declspec(selectany) manually_managed_shutdown_aware_object<FeatureStateManager> g_featureStateManager;
#endif

        inline void __stdcall UnsubscribeProcessWideUsageFlush(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
        {
            g_featureStateManager.get().UnsubscribeToProcessWideUsageFlush(subscription);
        }

        _Success_return_ inline bool IsFeatureConfigured(_Outptr_ wil_FeatureState* state, unsigned int featureId, bool featureRequiresSessionChange, wil_FeatureStore store)
        {
            static wil_details_FeaturePropertyCache userStoreProbe = {};
            static wil_details_FeaturePropertyCache machineStoreProbe = {};

            auto& probe = (store == wil_FeatureStore_Machine) ? machineStoreProbe : userStoreProbe;

            if (static_cast<wil_details_CachedFeatureEnabledState>(probe.cache.enabledState) == wil_details_CachedFeatureEnabledState_Disabled)
            {
                return false;
            }

            const bool checkForAnyFeaturesConfigured = (!g_wil_details_testStates && (static_cast<wil_details_CachedFeatureEnabledState>(probe.cache.enabledState) == wil_details_CachedFeatureEnabledState_Unknown));
            BOOL areAnyFeaturesConfigured = TRUE;
            const bool found = !!wil_QueryFeatureState(state, featureId, featureRequiresSessionChange ? TRUE : FALSE, store, checkForAnyFeaturesConfigured ? &areAnyFeaturesConfigured : NULL);
            if (checkForAnyFeaturesConfigured)
            {
                wil_details_SetEnabledAndHasNotificationStateProperties(&probe,
                    areAnyFeaturesConfigured ? wil_details_CachedFeatureEnabledState_Enabled : wil_details_CachedFeatureEnabledState_Disabled, wil_details_CachedHasNotificationState_Unknown, 0);

                if (!areAnyFeaturesConfigured)
                {
                    SubscribeFeaturePropertyCacheToEnabledStateChanges(&probe, wil_FeatureChangeTime_OnRead);
                }
            }
            return found;
        }

        enum class StagingConfigurationFlags
        {
            Default                 = 0x0,
            IgnoreServiceControls   = 0x1,
            IgnoreUserControls      = 0x2,
            IgnoreTestControls      = 0x4,
            IgnoreVariants          = 0x8
        };
        DEFINE_ENUM_FLAG_OPERATORS(StagingConfigurationFlags);

        //! Set staging configuration flags (e.g. whether or not service-based controls are being ignored)
        inline void SetStagingConfigurationFlags(StagingConfigurationFlags flags)
        {
            // Code is best effort...

            wil_FeatureStore stores[] = { wil_FeatureStore_User, wil_FeatureStore_Machine };
            for (auto& store : stores)
            {
                unique_staging_config configHandle;
                if (__WIL_STATUS_SUCCESS == wil_LoadStagingConfig(&configHandle, store, TRUE))
                {
                    auto config = reinterpret_cast<wil_details_StagingConfig*>(configHandle.get());

                    wil_details_StagingConfigHeaderProperties headerProperties = config->header->sessionProperties;

                    headerProperties.ignoreServiceState     = WI_IsFlagSet(flags, StagingConfigurationFlags::IgnoreServiceControls) ? 1 : 0;
                    headerProperties.ignoreUserState        = WI_IsAnyFlagSet(flags, StagingConfigurationFlags::IgnoreUserControls) ? 1 : 0;
                    headerProperties.ignoreTestState        = WI_IsAnyFlagSet(flags, StagingConfigurationFlags::IgnoreTestControls) ? 1 : 0;
                    headerProperties.ignoreVariants         = WI_IsAnyFlagSet(flags, StagingConfigurationFlags::IgnoreVariants) ? 1 : 0;

                    wil_details_StagingConfig_SetHeaderProperties(config, &headerProperties);

                    wil_SaveStagingConfig(configHandle.get(), TRUE);
                }
            }

            // Synchronously kick our change notification callback so that setting controls and then responding to changes is predictable/stable within the same module (tests)
            details::g_featureStateManager.get().OnStateChange();
        }

        //! Get staging configuration flags (e.g. whether or not service-based controls are being ignored)
        inline StagingConfigurationFlags GetStagingConfigurationFlags()
        {
            unique_staging_config config;
            if (__WIL_STATUS_SUCCESS == wil_LoadStagingConfig(&config, wil_FeatureStore_Machine, FALSE))
            {
                wil_details_StagingConfigHeaderProperties headerProperties = reinterpret_cast<wil_details_StagingConfig*>(config.get())->header->sessionProperties;
                return ((headerProperties.ignoreServiceState ? StagingConfigurationFlags::IgnoreServiceControls : StagingConfigurationFlags::Default) |
                    (headerProperties.ignoreUserState ? StagingConfigurationFlags::IgnoreUserControls : StagingConfigurationFlags::Default) |
                    (headerProperties.ignoreTestState ? StagingConfigurationFlags::IgnoreTestControls : StagingConfigurationFlags::Default) |
                    (headerProperties.ignoreVariants ? StagingConfigurationFlags::IgnoreVariants : StagingConfigurationFlags::Default));
            }
            return StagingConfigurationFlags::Default;
        }

        template <typename Callback>
        inline void EnumerateFeatures(wil_details_StagingConfig* config, Callback&& lambda)
        {
            __WIL_PStagingConfigFeatureEnumeration fn = [](_Inout_ wil_details_StagingConfigFeature* feature, void* context) -> BOOL
            {
                return (*((Callback*)context))(*feature) ? TRUE : FALSE;
            };
            wil_details_StagingConfig_EnumerateFeatures(config, fn, &lambda);
        }

        template <typename Callback>
        inline void EnumerateStagingControls(FeatureControlKind kind, Callback&& lambda, wil_FeatureStore store)
        {
            WI_ASSERT(store != wil_FeatureStore_All);

            unique_staging_config configHandle;
            if (__WIL_STATUS_SUCCESS == wil_LoadStagingConfig(&configHandle, store, FALSE))
            {
                auto config = reinterpret_cast<wil_details_StagingConfig*>(configHandle.get());

                EnumerateFeatures(config, [&](wil_details_StagingConfigFeature& feature)
                {
                    StagingControl control = {};
                    control.id = feature.featureId;
                    if ((kind == FeatureControlKind::Testing) && (feature.testState != wil_FeatureEnabledState_Default))
                    {
                        control.state = static_cast<FeatureEnabledState>(feature.testState);
                        lambda(control);
                    }
                    else if ((kind == FeatureControlKind::User) && (feature.userState != wil_FeatureEnabledState_Default))
                    {
                        control.state = static_cast<FeatureEnabledState>(feature.userState);
                        lambda(control);
                    }
                    else if ((kind == FeatureControlKind::Service) && (feature.serviceState != wil_FeatureEnabledState_Default))
                    {
                        control.state = static_cast<FeatureEnabledState>(feature.serviceState);
                        lambda(control);
                    }
                    return true;
                });
            }
        }

        // Public API Contract Implementation in WIL

        inline FEATURE_ENABLED_STATE NTAPI WilApiImpl_GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime)
        {
            wil_FeatureStore store = WI_IsFlagSet(changeTime, FEATURE_CHANGE_TIME_USER_FLAG) ? wil_FeatureStore_User : wil_FeatureStore_Machine;
            WI_ClearFlag(changeTime, FEATURE_CHANGE_TIME_USER_FLAG);
            const bool requiresSessionChange = ((changeTime == FEATURE_CHANGE_TIME_REBOOT) || (changeTime == FEATURE_CHANGE_TIME_SESSION));

            wil_FeatureState state;
            RtlZeroMemory(&state, sizeof(state));
            FEATURE_ENABLED_STATE enabledState = FEATURE_ENABLED_STATE_DEFAULT;
            if (IsFeatureConfigured(&state, featureId, requiresSessionChange, store))
            {
                enabledState = static_cast<FEATURE_ENABLED_STATE>(state.enabledState);
            }

            WI_SetFlagIf(enabledState, FEATURE_ENABLED_STATE_HAS_NOTIFICATION, state.hasNotification);
            WI_SetFlagIf(enabledState, FEATURE_ENABLED_STATE_HAS_VARIANT_CONFIGURATION, state.isVariantConfiguration);

            return enabledState;
        }

        inline void NTAPI WilApiImpl_RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, PCSTR originName)
        {
            originName;
            BOOL fireVariantConfigNotifications = WI_IsFlagSet(kind, c_wil_details_ReportingKindHonorVariantConfig);
            WI_ClearFlag(kind, c_wil_details_ReportingKindHonorVariantConfig);

            if (featureId == 0 && addend == 0 && kind == 0)
            {
                // Special case to request a flush of all cached usage data
                g_featureStateManager.get().FlushUsage();
            }
#if !defined(WIL_DISABLE_SRUM_WNF)
            else if (WI_IsFlagSet(kind, c_wil_details_srumReportingFlag))
            {
                WI_ClearFlag(kind, c_wil_details_srumReportingFlag);
                g_featureStateManager.get().QueueBackgroundSRUMUsageReporting((unsigned int)featureId, (unsigned short)kind, addend);
            }
#endif // WIL_DISABLE_SRUM_WNF
            else if (addend == 0 && kind != (UINT32)wil_details_ServiceReportingKind_Store)
            {
                // Special case to handle firing notifications
                if (!g_wil_details_preventOnDemandStagingConfigReads)
                {
                    unsigned char buffer[200];      // small stack buffer to avoid the vast majority of allocations
                    wil_details_StagingConfig config;
                    wil_FeatureStore store = wil_FeatureStore_Machine; // Revision needed when we start supporting per-user stores
                    if (__WIL_STATUS_SUCCESS == wil_details_StagingConfig_Load(&config, store, ARRAYSIZE(buffer), buffer, FALSE))
                    {
                        wil_details_StagingConfig_FireNotification(&config, featureId, (unsigned short)kind, fireVariantConfigNotifications);
                        wil_details_StagingConfig_Free(&config);
                    }
                }
            }
            else
            {
                g_featureStateManager.get().RecordFeatureUsage(featureId, static_cast<wil_details_ServiceReportingKind>(kind), static_cast<size_t>(addend));
            }
        }

        inline void NTAPI WilApiImpl_RecordFeatureError(UINT32 featureId, const FEATURE_ERROR* error)
        {
            g_featureStateManager.get().RecordFeatureError(featureId, *error);
        }

        inline void NTAPI WilApiImpl_SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
        {
            if (context == c_wil_details_usageFlushContext)
            {
                // Special case: subscribe to usage data flush requests instead of enabled state changes
                g_featureStateManager.get().SubscribeToUsageFlush(subscription, callback);
            }
            else
            {
                g_featureStateManager.get().SubscribeToEnabledStateChanges(subscription, callback, context);
            }
        }

        inline void NTAPI WilApiImpl_UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
        {
            if (WI_IsFlagSet(reinterpret_cast<size_t>(subscription), c_wil_details_usageFlushHandleFlag))
            {
                // Special case: this is a usage data flush request subscription
                size_t subscriptionIndex = (size_t)subscription;
                WI_ClearFlag(subscriptionIndex, c_wil_details_usageFlushHandleFlag);
                g_featureStateManager.get().UnsubscribeToUsageFlush(reinterpret_cast<FEATURE_STATE_CHANGE_SUBSCRIPTION>(subscriptionIndex));
            }
            else
            {
                g_featureStateManager.get().UnsubscribeToEnabledStateChanges(subscription);
            }
        }

        inline UINT32 NTAPI WilApiImpl_GetFeatureVariant(UINT32 featureId, FEATURE_CHANGE_TIME changeTime, _Out_ UINT32* payloadId, _Out_ BOOL* hasNotification)
        {
            wil_FeatureStore store = WI_IsFlagSet(changeTime, FEATURE_CHANGE_TIME_USER_FLAG) ? wil_FeatureStore_User : wil_FeatureStore_Machine;
            WI_ClearFlag(changeTime, FEATURE_CHANGE_TIME_USER_FLAG);
            const bool requiresSessionChange = ((changeTime == FEATURE_CHANGE_TIME_REBOOT) || (changeTime == FEATURE_CHANGE_TIME_SESSION));

            wil_FeatureState state;
            RtlZeroMemory(&state, sizeof(state));
            bool configured = IsFeatureConfigured(&state, featureId, requiresSessionChange, store);
            *hasNotification = state.hasNotification;

            if (configured)
            {
                *payloadId = state.payload;
                WI_SetFlagIf(state.variant, c_wil_details_Variant_IsVariantConfig, state.isVariantConfiguration);
                return state.variant;
            }
            *payloadId = 0;
            return (UINT32)0;
        }
    }
    /// @endcond

    _Success_return_ inline bool IsFeatureConfigured(_Outptr_ wil_FeatureState* state, unsigned int featureId, FEATURE_CHANGE_TIME changeTime)
    {
        wil_FeatureStore store = WI_IsFlagSet(changeTime, FEATURE_CHANGE_TIME_USER_FLAG) ? wil_FeatureStore_User : wil_FeatureStore_Machine;
        WI_ClearFlag(changeTime, FEATURE_CHANGE_TIME_USER_FLAG);
        const bool requiresSessionChange = ((changeTime == FEATURE_CHANGE_TIME_REBOOT) || (changeTime == FEATURE_CHANGE_TIME_SESSION));
        return details::IsFeatureConfigured(state, featureId, requiresSessionChange, store);
    }

    template <typename Callback>
    inline void EnumerateStagingControls(FeatureControlKind kind, Callback&& lambda, wil_FeatureStore store = wil_FeatureStore_All)
    {
        switch (store)
        {
        case wil_FeatureStore_All:
            details::EnumerateStagingControls(kind, wistd::forward<Callback>(lambda), wil_FeatureStore_Machine);
            details::EnumerateStagingControls(kind, wistd::forward<Callback>(lambda), wil_FeatureStore_User);
            break;
        case wil_FeatureStore_User:
            details::EnumerateStagingControls(kind, wistd::forward<Callback>(lambda), wil_FeatureStore_User);
            break;
        case wil_FeatureStore_Machine:
            details::EnumerateStagingControls(kind, wistd::forward<Callback>(lambda), wil_FeatureStore_Machine);
            break;
        }
    }

    //! Adjust persistent staging controls by marking features as enabled, disabled, or default.
    inline void ModifyStagingControls(FeatureControlKind kind, size_t count, _In_reads_(count) StagingControl* changes, StagingControlActions actions = StagingControlActions::Replace, wil_FeatureStore store = wil_FeatureStore_Machine)
    {
        WI_ASSERT(store != wil_FeatureStore_All);

        unique_staging_config configHandle;
        if (__WIL_STATUS_SUCCESS == wil_LoadStagingConfig(&configHandle, store, TRUE))
        {
            auto config = reinterpret_cast<wil_details_StagingConfig*>(configHandle.get());

            // first, reset all features if we're replacing them....
            if (WI_IsFlagSet(actions, StagingControlActions::Replace))
            {
                details::EnumerateFeatures(config, [&](wil_details_StagingConfigFeature& feature)
                {
                    if (((kind == FeatureControlKind::Testing) && (feature.testState != wil_FeatureEnabledState_Default)) ||
                        ((kind == FeatureControlKind::User) && (feature.userState != wil_FeatureEnabledState_Default)) ||
                        ((kind == FeatureControlKind::Service) && (feature.serviceState != wil_FeatureEnabledState_Default)))
                    {
                        wil_details_StagingConfig_SetFeatureEnabledState(config, feature.featureId, wil_FeatureEnabledState_Default, static_cast<wil_FeatureEnabledStateKind>(kind), wil_FeatureEnabledStateOptions_None);
                    }
                    return true;
                });
            }

            for (auto& change : wil::make_range(changes, count))
            {
                wil_details_StagingConfig_SetFeatureEnabledState(config, change.id, static_cast<wil_FeatureEnabledState>(change.state), static_cast<wil_FeatureEnabledStateKind>(kind), wil_FeatureEnabledStateOptions_None);
            }

            wil_SaveStagingConfig(configHandle.get(), TRUE);
        }

        // Synchronously kick our change notification callback so that setting controls and then responding to changes is predictable/stable within the same module (tests)
        details::g_featureStateManager.get().OnStateChange();
    }

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline void ModifyStagingControl(unsigned int id, FeatureEnabledState state, FeatureControlKind kind = FeatureControlKind::User, StagingControlActions actions = StagingControlActions::Default, wil_FeatureStore store = wil_FeatureStore_Machine)
    {
        StagingControl control{ id, state };
        ModifyStagingControls(kind, 1, &control, actions, store);
    }

    //! Adjust test staging controls by marking features as enabled, disabled, or default; test changes override "normal" feature controls, but are not stable (they may be reset at reboot or the request of other tests).
    //! Note: does not honor the reboot required state for making the change.
    inline void ModifyTestStagingControls(size_t count, _In_reads_(count) StagingControl* changes, StagingControlActions actions = StagingControlActions::Replace, wil_FeatureStore store = wil_FeatureStore_Machine)
    {
        ModifyStagingControls(FeatureControlKind::Testing, count, changes, actions, store);
    }

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline void ModifyTestStagingControl(unsigned int id, FeatureEnabledState state, StagingControlActions actions = StagingControlActions::Replace, wil_FeatureStore store = wil_FeatureStore_Machine)
    {
        StagingControl control{ id, state };
        ModifyStagingControls(FeatureControlKind::Testing, 1, &control, actions, store);
    }

    //! Clears all test staging controls immediately.
    inline void ClearTestStagingControls(wil_FeatureStore store = wil_FeatureStore_Machine)
    {
        // Default is replace, so modifying ZERO is identical to clearing
        ModifyTestStagingControls(0, nullptr, StagingControlActions::Replace, store);
    }
}

#endif // WIL_ENABLE_STAGING_API

//! This macro checks whether a feature is enabled or not; use when Feature::IsEnabled is rejected due to inlining failure.
#define WI_IsFeatureEnabled(className) \
    ((((className::featureTraits::usageReportingMode == ::wil::UsageReportingMode::Default) || ((className::featureTraits::usageReportingMode == ::wil::UsageReportingMode::SuppressPotential) && className::featureTraits::isAlwaysEnabled)) && className::__private_IsEnabledPreCheck()), \
    className::featureTraits::isAlwaysEnabled || (!className::featureTraits::isAlwaysDisabled && className::__private_IsEnabled()))

//! This macro checks whether a feature is enabled or not while controlling reporting; use when Feature::IsEnabled is rejected due to inlining failure.
#define WI_IsFeatureEnabled_ReportUsage(className, reportingKind) \
    ((((className::featureTraits::usageReportingMode != ::wil::UsageReportingMode::SuppressPotential) || className::featureTraits::isAlwaysEnabled) && className::__private_IsEnabledPreCheck(reportingKind)), className::featureTraits::isAlwaysEnabled || (!className::featureTraits::isAlwaysDisabled && className::__private_IsEnabled(reportingKind)))

//! This macro checks whether a feature is enabled or not, while allowing calling code to specify the default enabled state; use when integrating with other systems of configuring features.
#define WI_IsFeatureEnabledWithDefault(className, isEnabledByDefaultOverride) \
    (className::featureTraits::isAlwaysEnabled || (!className::featureTraits::isAlwaysDisabled && className::__private_IsEnabledWithDefault(isEnabledByDefaultOverride)))

//! This is an alias for WI_IsFeatureEnabled_ReportUsage with ReportingKind::None to check enablement without reporting; use when Feature::IsEnabled(kind) is rejected due to inlining failure.
#define WI_IsFeatureEnabled_SuppressUsage(className) \
    WI_IsFeatureEnabled_ReportUsage(className, ::wil::ReportingKind::None)


#define WI_IsVariantEqual(className, variant) \
    ((className::featureTraits::isAlwaysDisabled && (className::featureTraits::usageReportingMode == ::wil::UsageReportingMode::Default) && className::__private_ReportVariantDisabled(variant)), \
     (!className::featureTraits::isAlwaysDisabled && className::__private_IsVariantEqual(variant)))

#define WI_IsVariantEqual_ReportUsage(className, variant, variantReportingKind) \
    ((className::featureTraits::isAlwaysDisabled && (className::featureTraits::usageReportingMode != ::wil::UsageReportingMode::SuppressPotential) && className::__private_ReportVariantDisabled(variant, variantReportingKind)), \
     (!className::featureTraits::isAlwaysDisabled && className::__private_IsVariantEqual(variant, variantReportingKind)))

#define WI_GetVariant(className) \
    ((className::featureTraits::isAlwaysDisabled && (className::featureTraits::usageReportingMode == ::wil::UsageReportingMode::Default) && className::__private_ReportVariantDisabled()), \
     (!className::featureTraits::isAlwaysDisabled ? className::__private_GetVariant() : className::Variant::None))

#define WI_GetVariant_ReportUsage(className, variantReportingKind) \
    ((className::featureTraits::isAlwaysDisabled && (className::featureTraits::usageReportingMode != ::wil::UsageReportingMode::SuppressPotential) && className::__private_ReportVariantDisabled(variantReportingKind)), \
     (!className::featureTraits::isAlwaysDisabled ? className::__private_GetVariant(variantReportingKind) : className::Variant::None))

#define WI_CalculateActiveStage(stage, stageOverride, stageChkOverride) \
    (((stageChkOverride) != ::wil::details::UnknownFeatureStage) ? (stageChkOverride) : (((stageOverride) != ::wil::details::UnknownFeatureStage) ? (stageOverride) : (stage)))

#define WI_CalculateIsAlwaysDisabled(activeStage) \
    ((activeStage) == ::wil::FeatureStage::AlwaysDisabled)

#define WI_CalculateIsAlwaysEnabled(activeStage) \
    ((activeStage) == ::wil::FeatureStage::AlwaysEnabled)

#define WI_CalculateEnabledByDefault(activeStage) \
    (WI_CalculateIsAlwaysEnabled(activeStage) || ((activeStage) == ::wil::FeatureStage::EnabledByDefault))


/// @cond
// Following are various staging control macros that are generated by an XML compilation tool for consumption by the
// staging header.  These settings are documented in the example XML file used to generate them.

#define __WilStaging_CompletePreviousMacro  WI_PASTE(___dummy, __COUNTER__)
#define __WilStaging_StartNewMacro          static const char WI_PASTE(___dummy, __COUNTER__)

#define WI_DEFINE_FEATURE(className_, id_, stage_, name_, description_, ...) \
    struct __WilFeatureTraits_##className_ : ::wil::details::FeatureTraitsBase \
    { \
        static const unsigned int id = id_; \
        static const ::wil::FeatureStage stage = ::wil::FeatureStage:: stage_; \
        static PCSTR GetName()          { return name_; } \
        static PCSTR GetDescription()   { return description_; } \
        __WilStaging_StartNewMacro, \
        __VA_ARGS__ ; \
        static const ::wil::FeatureStage activeStage = WI_CalculateActiveStage(stage, stageOverride, stageChkOverride); \
        /* True if this feature is always disabled; this is based on the stage and whether or not there are any branch overrides. */ \
        static const bool isAlwaysDisabled = WI_CalculateIsAlwaysDisabled(activeStage); \
        /* True if this feature is always enabled (it cannot be turned off) */ \
        static const bool isAlwaysEnabled = WI_CalculateIsAlwaysEnabled(activeStage); \
        /*True if this feature is enabled by default (either always or conditionally based on stage or branch overrides)*/ \
        static const bool isEnabledByDefault = WI_CalculateEnabledByDefault(activeStage); \
    }; \
    using className_ = ::wil::Feature<__WilFeatureTraits_##className_>;

#define WilStagingVariant(variantTypeName_, defaultVariantValue_, payloadType_, defaultPayloadValue_) \
    __WilStaging_CompletePreviousMacro; \
    typedef variantTypeName_ Variant; \
    static const Variant defaultVariant = variantTypeName_ :: defaultVariantValue_; \
    static const ::wil_VariantPayloadType payloadType = wil_VariantPayloadType_##payloadType_; \
    static const unsigned int defaultPayloadValue = defaultPayloadValue_; \
    __WilStaging_StartNewMacro

#define WilStagingCustomReportingKind(customReportingKindTypeName_) \
    __WilStaging_CompletePreviousMacro; \
    typedef customReportingKindTypeName_ CustomReportingKind; \
    __WilStaging_StartNewMacro

#define WilStagingGroup(name_, description_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetGroupName() { return name_; } \
    static PCSTR GetGroupDescription() { return description_; } \
    __WilStaging_StartNewMacro

#define WilStagingEmail(alias_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetEMail() { return alias_; } \
    __WilStaging_StartNewMacro

#define WilStagingLink(link_) \
    __WilStaging_CompletePreviousMacro; \
    static PCSTR GetLink() { return link_; } \
    __WilStaging_StartNewMacro

#define WilStagingVersion(version_) \
    __WilStaging_CompletePreviousMacro; \
    static const unsigned short version = version_; \
    __WilStaging_StartNewMacro

#define WilStagingBaseVersion(baseVersion_) \
    __WilStaging_CompletePreviousMacro; \
    static const unsigned short baseVersion = baseVersion_; \
    __WilStaging_StartNewMacro

#define WilStagingBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
        static const ::wil::FeatureStage stageOverride = ::wil::FeatureStage:: stage_; \
    __WilStaging_StartNewMacro

#ifdef RESULT_DEBUG
#define WilStagingChkBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureStage stageChkOverride = ::wil::FeatureStage:: stage_; \
    __WilStaging_StartNewMacro
#else
#define WilStagingChkBranchOverride(stage_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureStage stageChkOverride = ::wil::details::UnknownFeatureStage; \
    __WilStaging_StartNewMacro
#endif

#define WilStagingRequiredBy(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool ShouldBeEnabledForDependentFeature_DefaultDisabled() \
    { \
        return ::wil::details::DependentFeatures<__VA_ARGS__>::ShouldBeEnabledForDependentFeature(false); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingRequiredByEnabled(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool ShouldBeEnabledForDependentFeature_DefaultEnabled() \
    { \
        return ::wil::details::DependentFeatures<__VA_ARGS__>::ShouldBeEnabledForDependentFeature(true); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingRequiresFeature(...) \
    __WilStaging_CompletePreviousMacro; \
    static bool AreDependenciesEnabled() \
    { \
        return ::wil::details::RequiredFeatures<__VA_ARGS__>::IsEnabled(); \
    } \
    __WilStaging_StartNewMacro

#define WilStagingChangeTime(changeTime_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::FeatureChangeTime changeTime = ::wil::FeatureChangeTime:: changeTime_; \
    __WilStaging_StartNewMacro

#define WilStagingUsageReportingMode(usageReportingMode_) \
    __WilStaging_CompletePreviousMacro; \
    static const ::wil::UsageReportingMode usageReportingMode = ::wil::UsageReportingMode:: usageReportingMode_; \
    __WilStaging_StartNewMacro


/// @endcond

namespace wil
{
    //! Indicates the stage of a feature in the current production build.
    enum class FeatureStage
    {
        AlwaysDisabled = wil_FeatureStage_AlwaysDisabled,       //!< The feature is always excluded from the production build and cannot be enabled
        DisabledByDefault = wil_FeatureStage_DisabledByDefault, //!< The feature is included, but off by default; it can be switched on through configuration
        EnabledByDefault = wil_FeatureStage_EnabledByDefault,   //!< The feature is included and on by default; it can be switched off through configuration
        AlwaysEnabled = wil_FeatureStage_AlwaysEnabled,         //!< The feature is always included in the production build and cannot be disabled
    };

    //! Controls precisely when feature changes are observed
    enum class FeatureChangeTime : unsigned char
    {
        OnRead = FEATURE_CHANGE_TIME_READ,              //!< Changes to the feature's enabled state are observed the next time Feature::IsEnabled() is called.
        OnReload = FEATURE_CHANGE_TIME_MODULE_RELOAD,   //!< Changes observed only after the containing binary has been reloaded.
        OnReboot = FEATURE_CHANGE_TIME_REBOOT           //!< Changes observed only after reboot.
    };

    //! Identifies what kind of reporting should be done in response to the primitive accepting this parameters as
    enum class ReportingKind : unsigned char
    {
        None = wil_ReportingKind_None,                              //!< Do not report any usage information from this primitive; use for module initialization and other unrelated actions to usage.
        UniqueUsage = wil_ReportingKind_UniqueUsage,                //!< Report a unique usage of this feature (when enabled) or a potential usage of the feature (when disabled).
        UniqueOpportunity = wil_ReportingKind_UniqueOpportunity,    //!< Report that the user was given an opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
        DeviceUsage = wil_ReportingKind_DeviceUsage,                //!< [Default for most primitives] Report that this device has used the feature (when enabled) or had the potential to use the feature (when disabled).
        DeviceOpportunity = wil_ReportingKind_DeviceOpportunity,    //!< Report that this device has presented the opportunity to use this feature (when enabled) or had the potential to show opportunity to use the feature (when disabled).
        TotalDuration = wil_ReportingKind_TotalDuration,            //!< Report the overall total time (in milliseconds) spent executing the feature (including time spent waiting on user input)
        PausedDuration = wil_ReportingKind_PausedDuration,          //!< Report the time (in milliseconds) spent waiting on user input while the feature is being executed
                                                                    // Custom data points start at enum value 100
    };

    //! Identifies what kind of reporting should be done for a variant
    enum class VariantReportingKind
    {
        None = wil_VariantReportingKind_None,                       //!< No usage reporting at all
        UniqueUsage = wil_VariantReportingKind_UniqueUsage,         //!< Report a unique usage of this variant
        DeviceUsage = wil_VariantReportingKind_DeviceUsage,         //!< Report the notion that there was usage of this variant on this device
    };

    //! Identifies what kind of reporting should be allowed
    enum class UsageReportingMode
    {
        Default = wil_UsageReportingMode_Default,                       //!< Normal mode. Everything is reported
        SuppressPotential = wil_UsageReportingMode_SuppressPotential,   //!< Suppress potential usage reporting
        SuppressImplicit = wil_UsageReportingMode_SuppressImplicit      //!< Suppress implicit usage reporting. Any reporting not explicitly specified should not occur
    };

    enum class CustomReportingKind
    {
        Custom1 = 0,
        Custom2 = 1,
        Custom3 = 2
    };

    inline ReportingKind GetCustomReportingKind(unsigned char index)
    {
        return static_cast<ReportingKind>(wil_GetCustomReportingKind(index));
    }

    //! Identifies one of several string values that can be retrieved from a feature
    enum class FeatureString
    {
        Name,
        Description,
        GroupName,
        GroupDescription,
        EMail,
        Link
    };

    //! Represents a mask of possible properties to retrieve from a feature
    enum class FeaturePropertyGroupFlags
    {
        None = 0x0000,
        StaticProperties = 0x0001,   //!< Retrieves all compile-time specified properties for a Feature
        FeatureEnabledState = 0x0002    //!< Retrieves the runtime enabled state of a feature
    };
    DEFINE_ENUM_FLAG_OPERATORS(FeaturePropertyGroupFlags)

    //! Holds basic information describing a feature for runtime inspection
    struct FeatureProperties
    {
        // Retrieved via FeaturePropertyGroupFlags::StaticProperties
        FeatureChangeTime changeTime;
        FeatureStage stage;
        bool isEnabledByDefault;
        unsigned short version;
        unsigned short baseVersion;

        // Retrieved via FeaturePropertyGroupFlags::FeatureEnabledState
        bool isEnabled;
    };

    /// @cond
    // Depracated: Use ReportingKind. TODO: Remove after the next RI/FI
    enum class FeatureUsageKind
    {
        ImplicitOpportunityCount = 0,
        ImplicitUsageCount = 1,
        OpportunityCount = 2,
        UsageCount = 3,
        TotalDuration = 4,
        PausedDuration = 5,
        Custom1 = 11,
        Custom2 = 12,
        Custom3 = 13,
        Custom4 = 14,
        Custom5 = 15,
    };

    struct VariantUsage
    {
        unsigned int featureId;
        unsigned int variantId;
        unsigned long long usageFileTime;
    };

    /// @cond
    namespace details
    {
        struct FeatureTestState;
        __declspec(selectany) FeatureTestState* g_testStates = nullptr;
        __declspec(selectany) SRWLOCK g_testLock = SRWLOCK_INIT;

        struct FeatureTestState
        {
            unsigned int featureId;
            FeatureEnabledState state;
            FeatureTestState* next;

            static void __stdcall Reset(FeatureTestState* state)
            {
                auto lock = wil::AcquireSRWLockExclusive(&g_testLock);
                auto stateList = &g_testStates;
                while (*stateList)
                {
                    if (*stateList == state)
                    {
                        *stateList = state->next;
                        state->next = nullptr;
                        return;
                    }
                    stateList = &((*stateList)->next);
                }

                ::HeapFree(::GetProcessHeap(), 0, state);
            }
        };

        inline FEATURE_ENABLED_STATE GetFeatureTestState(UINT32 featureId, FEATURE_CHANGE_TIME)
        {
            if (g_testStates)
            {
                auto lock = wil::AcquireSRWLockShared(&g_testLock);
                auto state = g_testStates;
                while (state)
                {
                    if (state->featureId == featureId)
                    {
                        return static_cast<FEATURE_ENABLED_STATE>(state->state);
                    }
                    state = state->next;
                }
            }
            return FEATURE_ENABLED_STATE_DEFAULT;
        }

        inline FEATURE_ENABLED_STATE WilApi_GetFeatureEnabledState(UINT32 featureId, FEATURE_CHANGE_TIME changeTime)
        {
            if (g_wil_details_internalGetFeatureEnabledState)
            {
                return g_wil_details_internalGetFeatureEnabledState(featureId, changeTime);
            }
            if (g_wil_details_apiGetFeatureEnabledState)
            {
                wil_FeatureEnabledState state;
                if (wil_HasFeatureTestState(featureId, &state))
                {
                    return static_cast<FEATURE_ENABLED_STATE>(state);
                }
                return g_wil_details_apiGetFeatureEnabledState(featureId, changeTime);
            }
            WI_ASSERT(FALSE);
            return FEATURE_ENABLED_STATE_DEFAULT;
        }

        inline void WilApi_RecordFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend, PCSTR originName)
        {
            if (g_wil_details_internalRecordFeatureUsage)
            {
                g_wil_details_internalRecordFeatureUsage(featureId, kind, addend, originName);
                return;
            }
            if (g_wil_details_apiRecordFeatureUsage)
            {
                g_wil_details_apiRecordFeatureUsage(featureId, kind, addend, originName);
                return;
            }
            WI_ASSERT(FALSE);
        }

        inline void WilApi_RecordFeatureError(UINT32 featureId, const FEATURE_ERROR* error)
        {
            if (g_wil_details_internalRecordFeatureError)
            {
                g_wil_details_internalRecordFeatureError(featureId, error);
                return;
            }
            if (g_wil_details_apiRecordFeatureError)
            {
                g_wil_details_apiRecordFeatureError(featureId, error);
                return;
            }
            WI_ASSERT(FALSE);
        }

        inline void WilApi_SubscribeFeatureStateChangeNotification(_Outptr_ FEATURE_STATE_CHANGE_SUBSCRIPTION* subscription, _In_ PFEATURE_STATE_CHANGE_CALLBACK callback, _In_opt_ void* context)
        {
            if (g_wil_details_internalSubscribeFeatureStateChangeNotification)
            {
                g_wil_details_internalSubscribeFeatureStateChangeNotification(subscription, callback, context);
                return;
            }
            if (g_wil_details_apiSubscribeFeatureStateChangeNotification)
            {
                g_wil_details_apiSubscribeFeatureStateChangeNotification(subscription, callback, context);
                return;
            }
            WI_ASSERT(FALSE);
        }

        inline void WilApi_UnsubscribeFeatureStateChangeNotification(_In_ _Post_invalid_ FEATURE_STATE_CHANGE_SUBSCRIPTION subscription)
        {
            if (g_wil_details_internalUnsubscribeFeatureStateChangeNotification)
            {
                g_wil_details_internalUnsubscribeFeatureStateChangeNotification(subscription);
                return;
            }
            if (g_wil_details_apiUnsubscribeFeatureStateChangeNotification)
            {
                g_wil_details_apiUnsubscribeFeatureStateChangeNotification(subscription);
                return;
            }
            WI_ASSERT(FALSE);
        }

        inline UINT32 WilApi_GetFeatureVariant(UINT32 featureId, FEATURE_CHANGE_TIME changeTime, _Out_ UINT32* payloadId, _Out_ BOOL* hasNotification)
        {
            if (g_wil_details_internalGetFeatureVariant)
            {
                return g_wil_details_internalGetFeatureVariant(featureId, changeTime, payloadId, hasNotification);
            }
            if (g_wil_details_apiGetFeatureVariant)
            {
                return g_wil_details_apiGetFeatureVariant(featureId, changeTime, payloadId, hasNotification);
            }
            WI_ASSERT(FALSE);
            return 0;
        }


        //! Manage the lifetime of a user subscription.
        typedef unique_any<FEATURE_STATE_CHANGE_SUBSCRIPTION, decltype(&::wil::details::WilApi_UnsubscribeFeatureStateChangeNotification), ::wil::details::WilApi_UnsubscribeFeatureStateChangeNotification, details::pointer_access_all> unique_feature_state_change_subscription;

        inline void RecordFeatureError(unsigned int featureId, const FailureInfo& info, const DiagnosticsInfo& diagnostics, void* returnAddress)
        {
            __STAGING_TEST_HOOK_ERROR(featureId, info, diagnostics, returnAddress);

            details::StagingFailureInformation failure(info, diagnostics, returnAddress);
            WilApi_RecordFeatureError(featureId, &failure);
        }
        class EnabledStateManager
        {
        public:
            EnabledStateManager() WI_NOEXCEPT
            {
                // This object will be held in a zero-initialized global variable.  We want this line to
                // run at construction time so we can distinguish when someone is trying to call us ahead
                // of construction (CRT init order)
                m_fInitialized = true;
            }

            // note: not called during process shutdown
            ~EnabledStateManager() WI_NOEXCEPT
            {
                m_fInitialized = false;
                m_timer.reset();
                ProcessShutdown();
            }

            // called by virtue of EnabledStateManager being held by shutdown_aware_object
            void ProcessShutdown() WI_NOEXCEPT
            {
                m_fInitialized = false;
                RecordCachedUsageUnderLock();
            }

            void QueueBackgroundUsageReporting(unsigned int id, _Inout_ wil_details_FeaturePropertyCache& state) WI_NOEXCEPT
            {
                if (m_fInitialized && !ProcessShutdownInProgress())
                {
                    auto lock = m_lock.lock_exclusive();
                    m_cachedUsageTrackingData.push_back(CachedUsageData{ id, &state });
                    EnsureTimerUnderLock();
                }
            }

            void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_opt_ wil_details_FeaturePropertyCache* state, wil_FeatureChangeTime changeTime) WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    if (state)
                    {
                        auto lock = m_lock.lock_exclusive();
                        CachedFeaturePropertyData properties;
                        properties.changeTime = changeTime;
                        properties.data = state;
                        m_cachedFeatureProperties.push_back(properties);
                        EnsureSubscribedToStateChangesUnderLock();
                    }
                    else
                    {
                        OnStateChange();
                    }
                }
            }

            void EnsureSubscribedToUsageFlush(_In_ PFEATURE_STATE_CHANGE_CALLBACK callback) WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    if (!m_usageFlushSubscription)
                    {
                        // We want to subscribe to usage data flush requests instead of feature state changes
                        WilApi_SubscribeFeatureStateChangeNotification(&m_usageFlushSubscription, callback, c_wil_details_usageFlushContext);
                    }
                }
            }

            void OnTimer() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    RecordCachedUsageUnderLock();
                    m_timerSet = false;
                }
            }

            void OnStateChange() WI_NOEXCEPT
            {
                if (m_fInitialized)
                {
                    auto lock = m_lock.lock_exclusive();
                    for (auto& properties : wil::make_range(m_cachedFeatureProperties.data(), m_cachedFeatureProperties.size()))
                    {
                        if (properties.changeTime == wil_FeatureChangeTime_OnRead)
                        {
                            wil_details_SetEnabledAndHasNotificationStateProperties(properties.data, wil_details_CachedFeatureEnabledState_Unknown, wil_details_CachedHasNotificationState_Unknown, 0);
                        }
                        else
                        {
                            wil_details_SetHasNotificationStateProperty(properties.data, wil_details_CachedHasNotificationState_Unknown);
                        }
                    }
                    m_cachedFeatureProperties.clear();
                }
            }

        private:
            bool m_fInitialized = false;

            WIL_SRWLock m_lock;

            WIL_UniqueThreadpoolTimer m_timer;
            bool m_timerSet = false;
            unique_feature_state_change_subscription m_subscription;
            unique_feature_state_change_subscription m_usageFlushSubscription;

            struct CachedUsageData
            {
                unsigned int id;
                wil_details_FeaturePropertyCache* data;
            };

            details_abi::heap_vector<CachedUsageData> m_cachedUsageTrackingData;

            struct CachedFeaturePropertyData
            {
                wil_FeatureChangeTime changeTime;
                wil_details_FeaturePropertyCache *data;
            };

            details_abi::heap_vector<CachedFeaturePropertyData> m_cachedFeatureProperties;

            void RecordCachedUsageUnderLock() WI_NOEXCEPT
            {
                if (m_cachedUsageTrackingData.size())
                {
                    for (auto& feature : wil::make_range(m_cachedUsageTrackingData.data(), m_cachedUsageTrackingData.size()))
                    {
                        struct Change
                        {
                            wil_details_ServiceReportingKind kind;
                            unsigned int count;
                        };
                        Change changes[8] = {};
                        size_t count = 0;

                        ModifyFeatureData(*feature.data, [&](wil_details_FeaturePropertyCache& change)
                        {
                            count = 0;
                            changes[count++] = Change{ wil_details_ServiceReportingKind_DeviceUsage, (!change.cache.recordedDeviceUsage && change.cache.reportedDeviceUsage) ? 1ul : 0ul };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_PotentialDeviceUsage, (!change.cache.recordedDevicePotential && change.cache.reportedDevicePotential) ? 1ul : 0ul };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_DeviceOpportunity, (!change.cache.recordedDeviceOpportunity && change.cache.reportedDeviceOpportunity) ? 1ul : 0ul };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_PotentialDeviceOpportunity, (!change.cache.recordedDevicePotentialOpportunity && change.cache.reportedDevicePotentialOpportunity) ? 1ul : 0ul };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_UniqueUsage, change.cache.usageCountRepresentsPotential ? 0ul : change.cache.usageCount };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_PotentialUniqueUsage, change.cache.usageCountRepresentsPotential ? change.cache.usageCount : 0ul };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_UniqueOpportunity, change.cache.opportunityCountRepresentsPotential ? 0ul : change.cache.opportunityCount };
                            changes[count++] = Change{ wil_details_ServiceReportingKind_PotentialUniqueOpportunity, change.cache.opportunityCountRepresentsPotential ? change.cache.opportunityCount : 0ul };

                            change.cache.recordedDeviceUsage = change.cache.reportedDeviceUsage;
                            change.cache.recordedDevicePotential = change.cache.reportedDevicePotential;
                            change.cache.recordedDeviceOpportunity = change.cache.reportedDeviceOpportunity;
                            change.cache.recordedDevicePotentialOpportunity = change.cache.reportedDevicePotentialOpportunity;
                            change.cache.usageCount = 0;
                            change.cache.opportunityCount = 0;
                            change.cache.queuedForReporting = 0;
                            return true;
                        });
                        WI_ASSERT(count == ARRAYSIZE(changes));

                        for (auto& change : changes)
                        {
                            if (change.count > 0)
                            {
                                WilApi_RecordFeatureUsage(feature.id, static_cast<UINT32>(change.kind), static_cast<UINT32>(change.count), nullptr);
                            }
                        }
                    }

                    m_cachedUsageTrackingData.clear();

                    WilApi_RecordFeatureUsage(0, static_cast<UINT32>(wil_details_ServiceReportingKind_Store), 0, nullptr);
                }
            }

            void EnsureTimerUnderLock() WI_NOEXCEPT
            {
                EnsureCoalescedTimer(m_timer, m_timerSet, this);
            }

            void EnsureSubscribedToStateChangesUnderLock() WI_NOEXCEPT
            {
                if (!m_subscription)
                {
                    WilApi_SubscribeFeatureStateChangeNotification(&m_subscription, [](void* context)
                    {
                        reinterpret_cast<EnabledStateManager*>(context)->OnStateChange();
                    }, this);
                }
            }
        };

#ifndef STAGING_SUPPRESS_STATIC_INITIALIZERS
        WI_ODR_PRAGMA("WilApiImpl_EnabledStateManager", "1")
            __declspec(selectany) shutdown_aware_object<EnabledStateManager> g_enabledStateManager;
#else
        WI_ODR_PRAGMA("WilApiImpl_EnabledStateManager", "0")
            __declspec(selectany) manually_managed_shutdown_aware_object<EnabledStateManager> g_enabledStateManager;
#endif

        inline void SubscribeFeaturePropertyCacheToEnabledStateChanges(_Inout_opt_ wil_details_FeaturePropertyCache* state, wil_FeatureChangeTime changeTime) WI_NOEXCEPT
        {
            g_enabledStateManager.get().SubscribeFeaturePropertyCacheToEnabledStateChanges(state, changeTime);
        }

        inline wil_details_ServiceReportingKind MapReportingKind(ReportingKind kind, bool enabled) WI_NOEXCEPT
        {
            return wil_details_MapReportingKind(static_cast<wil_ReportingKind>(kind), enabled ? TRUE : FALSE);
        }

        inline void ReportFeatureCaughtException(ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics, void* returnAddress) WI_NOEXCEPT
        {
            FailureInfo info = {};
            if (context.GetCaughtExceptionError(info, &diagnostics))
            {
                RecordFeatureError(featureId, info, diagnostics, returnAddress);
            }
        }

        class FeatureFunctorHost : public IFunctorHost
        {
        public:
            FeatureFunctorHost(unsigned int featureId, const DiagnosticsInfo& diagnostics) : m_featureId(featureId), m_diagnostics(diagnostics) {}

            HRESULT Run(IFunctor& functor) override
            {
                return functor.Run();
            }

#pragma warning(push)
#pragma warning(disable:4702) // unreachable code
            HRESULT ExceptionThrown(void* returnAddress) override
            {
                ReportFeatureCaughtException(m_context, m_featureId, m_diagnostics, returnAddress);
                RethrowCaughtException();
                FAIL_FAST_IMMEDIATE();
                return E_UNEXPECTED;
            }
#pragma warning(pop)

        private:
            ThreadErrorContext m_context;
            const DiagnosticsInfo& m_diagnostics;
            unsigned int m_featureId;
        };

        __declspec(noinline) inline void ReportFeatureError(HRESULT hr, ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics) WI_NOEXCEPT
        {
            FailureInfo info;

            if (!context.GetLastError(info, hr))
            {
                // WIL has never seen this failure, so it's being introduced here.  Let the GetLastError mechanism in WIL know about it
                // so that we're able to offer it up as the source if we have other wrapped contexts.

                ::ZeroMemory(&info, sizeof(info));
                info.uLineNumber = diagnostics.line;
                info.pszFile = diagnostics.file;
                info.returnAddress = diagnostics.returnAddress;
                info.hr = hr;

                SetLastError(info);
                context.GetLastError(info, hr);
            }

            RecordFeatureError(featureId, info, diagnostics, _ReturnAddress());
        }

        __declspec(noinline) inline void ReportFeatureCaughtException(wil::ThreadErrorContext& context, unsigned int featureId, const DiagnosticsInfo& diagnostics) WI_NOEXCEPT
        {
            ReportFeatureCaughtException(context, featureId, diagnostics, _ReturnAddress());
        }

        // WARNING: may throw an exception (even with an HRESULT return)...
        template <typename TFunctor>
        __forceinline HRESULT RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_HRESULT)
        {
            ThreadErrorContext context;
            HRESULT hr = functor();
            if (FAILED(hr))
            {
                ReportFeatureError(hr, context, featureId, diagnostics);
            }
            return hr;
        }

        template <typename TFunctor>
        __forceinline void RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_void)
        {
#if (WIL_EXCEPTION_MODE == 0)       // switchable
            functor_wrapper_void<TFunctor> wrapper(functor);
            FeatureFunctorHost host(featureId, diagnostics);
            RunFunctor(wrapper, host);
#elif (WIL_EXCEPTION_MODE == 1)     // exception-based only
            ThreadErrorContext context;
            try
            {
                functor();
            }
            catch (...)
            {
                ReportFeatureCaughtException(context, featureId, diagnostics);
                throw;
            }
#else                               // error-code only
            featureId; diagnostics; functor;
            functor();
#endif
        }

        template <typename TFunctor>
        inline auto RunFeatureDispatch(unsigned int featureId, const DiagnosticsInfo& diagnostics, TFunctor&& functor, tag_return_other) -> decltype(functor())
        {
#if (WIL_EXCEPTION_MODE == 0)       // switchable
            decltype(functor()) result;
            functor_wrapper_other<TFunctor, decltype(functor())> wrapper(functor, result);
            FeatureFunctorHost host(featureId, diagnostics);
            RunFunctor(wrapper, host);
            return result;
#elif (WIL_EXCEPTION_MODE == 1)     // exception-based only
            ThreadErrorContext context;
            try
            {
                return functor();
            }
            catch (...)
            {
                ReportFeatureCaughtException(context, featureId, diagnostics);
                throw;
            }
#else                               // error-code only
            featureId; diagnostics; functor;
            return functor();
#endif
        }

        //! Holds information necessary to report feature durations; use FeatureUsageDuration, rather than using directly.
        struct FeatureUsageDurationData
        {
            unsigned int featureId;
            bool enabled;
            ReportingKind kind;
            LARGE_INTEGER counterStart;

            FeatureUsageDurationData(unsigned int featureId_, bool enabled_, ReportingKind kind_) : featureId(featureId_), enabled(enabled_), kind(kind_)
            {
                QueryPerformanceCounter(&counterStart);
            }

            static void __stdcall Stop(FeatureUsageDurationData* data) WI_NOEXCEPT
            {
                FeatureUsageDurationData& duration = *data;

                if (duration.counterStart.QuadPart != 0)
                {
                    LARGE_INTEGER counterEnd;
                    QueryPerformanceCounter(&counterEnd);
                    WI_ASSERT(duration.counterStart.QuadPart <= counterEnd.QuadPart);
                    counterEnd.QuadPart -= duration.counterStart.QuadPart;

                    LARGE_INTEGER frequency;
                    QueryPerformanceFrequency(&frequency);

                    auto durationInMs = (counterEnd.QuadPart / (frequency.QuadPart / 1000));
                    WilApi_RecordFeatureUsage(duration.featureId, static_cast<UINT32>(MapReportingKind(duration.kind, duration.enabled)), static_cast<UINT32>(durationInMs), nullptr);

                    duration.counterStart.QuadPart = 0;
                }
            }
        };
    }
    /// @endcond

    typedef unique_any<wil_PFeatureTestState, decltype(&wil_FreeFeatureTestState), &wil_FreeFeatureTestState> unique_feature_test_state;

    //! Adjust a single persistent staging control by marking the feature as enabled, disabled, or default.
    inline unique_feature_test_state TestFeatureEnabledState(unsigned int featureId, FeatureEnabledState state = FeatureEnabledState::Enabled)
    {
        unique_feature_test_state testState;
        wil_CreateFeatureTestState(&testState, featureId, static_cast<wil_FeatureEnabledState>(state));
        return testState;
    }

    //! Adjust a single persistent staging control by providing it a variant override.
    template <typename VariantType>
    inline unique_feature_test_state TestFeatureVariantState(unsigned int featureId, VariantType variant, unsigned int payload = 0)
    {
        unique_feature_test_state testState;
        wil_CreateFeatureVariantTestState(&testState, featureId, static_cast<unsigned char>(variant), payload);
        return testState;
    }

    /** Provides the RAII class returned from a feature's ReportUsageDuration; automatically reports duration on destruction.
    You can report the duration explicitly ahead of destruction with the reset() method and abort reporting with the
    release() method. */
    using FeatureUsageDuration = unique_struct<details::FeatureUsageDurationData, decltype(&::wil::details::FeatureUsageDurationData::Stop), ::wil::details::FeatureUsageDurationData::Stop>;

    //! A feature for use with WIL staging; features control whether or not a piece of code is available for use.
    //! This is the common base for a native Feature and Managed feature.  It must not have any 'static const' variables.
    template <typename traits>
    class Feature
    {
    public:
        //! Access to feature traits defined in the header
        typedef traits featureTraits;
        typedef typename traits::Variant Variant;
        typedef typename traits::CustomReportingKind CustomReportingKind;

#ifndef WIL_STAGING_NOCONST
        //! Globally unique integer based id for this feature.
        //! Deprecated - use Feature::featureTraits::id instead
        static const unsigned int id = traits::id;

        //! True if this feature is always disabled; this is based on the stage and whether or not there are any branch overrides.
        //! Deprecated - use Feature::featureTraits::isAlwaysDisabled instead
        static const bool isAlwaysDisabled = traits::isAlwaysDisabled;

        //! True if this feature is always enabled (it cannot be turned off)
        //! Deprecated - use Feature::featureTraits::isAlwaysEnabled instead
        static const bool isAlwaysEnabled = traits::isAlwaysEnabled;

        //! True if this feature is enabled by default (either always or conditionally based on stage or branch overrides)
        //! Deprecated - use Feature::featureTraits::isEnabledByDefault instead
        static const bool isEnabledByDefault = traits::isEnabledByDefault;
#endif

        //! Retrieves the given string for use in tooling scenarios.
        static PCSTR GetFeatureString(FeatureString kind)
        {
            switch (kind)
            {
            case FeatureString::Name:              return traits::GetName();
            case FeatureString::Description:       return traits::GetDescription();
            case FeatureString::GroupName:         return traits::GetGroupName();
            case FeatureString::GroupDescription:  return traits::GetGroupDescription();
            case FeatureString::EMail:             return traits::GetEMail();
            case FeatureString::Link:              return traits::GetLink();
            };
            return nullptr;
        }

        //! Retrieves basic properties about the feature for use in configuration scenarios.
        static FeatureProperties GetFeatureProperties(FeaturePropertyGroupFlags featurePropertyGroups)
        {
            FeatureProperties featureProperties = {};

            if (WI_IsFlagSet(featurePropertyGroups, FeaturePropertyGroupFlags::StaticProperties))
            {
                featureProperties.changeTime = traits::changeTime;
                featureProperties.stage = traits::stage;
                featureProperties.isEnabledByDefault = traits::isEnabledByDefault;
                featureProperties.version = traits::version;
                featureProperties.baseVersion = traits::baseVersion;
            }

            if (WI_IsFlagSet(featurePropertyGroups, FeaturePropertyGroupFlags::FeatureEnabledState))
            {
                featureProperties.isEnabled = WI_IsFeatureEnabled_SuppressUsage(FeatureType);
            }

            return featureProperties;
        }

        //! Retrieves the logged set of traits.
        static FEATURE_LOGGED_TRAITS GetFeatureLoggedTraits()
        {
            return FEATURE_LOGGED_TRAITS{ traits::version, traits::baseVersion, static_cast<UINT8>(traits::stage) };
        }

#pragma warning(push)
#pragma warning(error:4714)
        /** This is the primary primitive to determine whether a feature is enabled.
        If the feature is in the AlwaysDisabled stage, this will evaluate at compile-time to 'false' and the forceinline
        will guarantee that the optimizer just sees a static 'false' evaluation of any conditional that includes this
        primitive.  The same (opposite bool) is true for the AlwaysEnabled stage.  When in the DisabledByDefault or
        EnabledByDefault stages, this results in a runtime enabled configuration check.

        This routine also automatically records implicit usage and potential usage statistics for the device.  Use
        the overload accepting ReportingKind to suppress that usage or report unique usage, rather than device usage.

        This routine ensures that warning 4714 (failure to honor __forceinline) is an error.  If you get this error, use
        WI_IsFeatureEnabled(Feature) rather than Feature::IsEnabled(). */
        __forceinline static bool IsEnabled()
        {
            return WI_IsFeatureEnabled(FeatureType);
        }

        /** This primitive determines whether a feature is enabled while providing control of the kind of usage that is reported.
        The default reporting of a normal IsEnabled() check is ReportingKind::ReportDeviceUsage.  This routine allows you
        to specify the kind of usage reporting that you want done in response to this call being made.  By specifing an alternate
        ReportingKind you can suppress usage information (with ReportingKind::None) or report on the unique uses of the feature
        (with ReportingKind::UniqueUsage) rather than device usage.

        This routine ensures that warning 4714 (failure to honor __forceinline) is an error.  If you get this error, use
        WI_IsFeatureEnabled_ReportUsage(Feature) rather than Feature::IsEnabled().

        See IsEnabled for more complete details. */
        __forceinline static bool IsEnabled(ReportingKind kind)
        {
            kind;
            return WI_IsFeatureEnabled_ReportUsage(FeatureType, kind);
        }

        __forceinline static bool IsEnabled(CustomReportingKind kind)
        {
            return WI_IsFeatureEnabled_ReportUsage(FeatureType, GetCustomReportingKind(static_cast<unsigned char>(kind)));
        }

        __forceinline static bool IsVariantEqual(Variant variant)
        {
            variant;
            return WI_IsVariantEqual(FeatureType, variant);
        }

        __forceinline static bool IsVariantEqual(Variant variant, VariantReportingKind kind)
        {
            variant; kind;
            return WI_IsVariantEqual_ReportUsage(FeatureType, variant, kind);
        }

        __forceinline static Variant GetVariant()
        {
            return WI_GetVariant(FeatureType);
        }

        __forceinline static Variant GetVariant(VariantReportingKind kind)
        {
            kind;
            return WI_GetVariant_ReportUsage(FeatureType, kind);
        }

        static unsigned int GetVariantData()
        {
            static_assert(!wistd::is_same<Variant, details::EmptyVariant>::value, "No variants for this feature");
#ifndef WIL_STAGING_NOCONST
            static_assert(traits::payloadType == wil_VariantPayloadType_Fixed, "No payload configured for this feature");
#endif

            GetCachedVariantState();
            return GetFeatureVariantPropertyCache().payloadId;
        }

        /** This primitive will run the given functor if the feature is enabled while capturing usage and error (exception) information and associating it with your feature.
        This routine give you three core capabilities rolled all into one primitive:

        1)  Enablement Checking:  This primitive will only run the given lambda if the feature is enabled.  If the feature isn't enabled, the
        lambda won't be run and if the feature can't be enabled (AlwaysDisabled) the code in the lambda will be dropped from optimized builds.

        2)  Usage Reporting:  Like the IsEnabled primitive this routine will also calculate potential and actual device usage (based upon whether the feature
        is enabled).  It also has an overload accepting ReportingKind that allows you to specify that you want to track UniqueUsage, rather than
        device usage or to avoid counting this primitive towards usage at all.

        3)  Error Reporting:  This primitive also encapsualtes a lightweight model for capturing any error or exception information that happens
        while executing the lambda, aggregating that information, and attributing that to your feature.  With this information, you can know
        through your feature reporting dashboard that a specific error was hit and on what line of code that error originated and through which
        RunIfEnabled call the error originated.

        To use the routine, the first parameter should always be WI_DIAGNOSTICS_INFO as it provides the needed context (file, line and address information)
        to the error reporting when an error occurs.  The second parameter is an optional ReportingKind parameter to allow specifying the exact type of
        usage to be tracked, and the final parameter is the lambda.

        The lambda must not accept any parameters and should use [&] as the capture mechanism.  The return value of the lambda is reflected as the return
        value of the RunIfEnabled call itself.  So if your lambda returns a ComPtr<IFoo> the RunIfEnabled call returns a ComPtr<IFoo>.  In the event your
        lambda is not run, the routine will return a default constructed ComPtr<IFoo>().

        When the lambda returns an HRESULT, the return value is inspected and failures are reported based upon that HRESULT value.  For any other return
        value a try/catch is used internally to similarly report thrown failures.  Note that this routine will not block exceptions, it only observes
        them and rethrows the original exception.

        If the lambda returns a long value, that value is presumed to be an HRESULT (since unfortunately they are type equivalent).  To return an actual
        `long` or any other typedef which maps to a `long`, use `RunIfEnabled<wil::ErrorReturn::None>(...)` so that the long isn't interpreted as an HRESULT.

        Examples:
        ~~~
        Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]
        {
        // Your feature code
        });

        HRESULT hr = Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]() -> HRESULT
        {
        // Your feature code
        RETURN_IF_FAILED(...);
        return S_OK;
        });

        wil::com_ptr<IFoo> foo = Feature_MyFeature::RunIfEnabled(WI_DIAGNOSTICS_INFO, [&]() -> wil::com_ptr<IFoo>
        {
        return wil::com_query<IFoo>(...);
        });
        ~~~ */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto RunIfEnabled(const DiagnosticsInfo& diagnostics, TFunctor&& functor) -> decltype(functor())
        {
            if (WI_IsFeatureEnabled(FeatureType))
            {
                return details::RunFeatureDispatch(traits::id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
            }
            return decltype(functor())();
        }

        /** This primitive will run the given functor if the feature is enabled capturing error (exception) statistics without recording implicit usage.
        See RunIfEnabled for more details. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto RunIfEnabled(const DiagnosticsInfo& diagnostics, ReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            if (WI_IsFeatureEnabled_ReportUsage(FeatureType, kind))
            {
                return details::RunFeatureDispatch(traits::id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
            }
            return decltype(functor())();
        }

        /** This primitive will run the given functor if the feature is enabled capturing error (exception) statistics without recording implicit usage.
        See RunIfEnabled for more details. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto RunIfEnabled(const DiagnosticsInfo& diagnostics, CustomReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            if (WI_IsFeatureEnabled_ReportUsage(FeatureType, GetCustomReportingKind(static_cast<unsigned char>(kind))))
            {
                return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
            }
            return decltype(functor())();
        }

        /// @cond
        // Do not use - will be removed
        __forceinline static bool IsEnabled_SuppressUsage()
        {
            return WI_IsFeatureEnabled_SuppressUsage(FeatureType);
        }
        /// @endcond

        /** This primitive checks whether a feature is enabled or not, while allowing calling code to specify the default enabled state; use when
        integrating with other systems of configuring features. */
        __forceinline static bool IsEnabledWithDefault(bool isEnabledByDefaultOverride)
        {
            return WI_IsFeatureEnabledWithDefault(FeatureType, isEnabledByDefaultOverride);
        }
#pragma warning(pop)

        /** Call this method at the top of feature-specific methods to avoid accidental dependencies on features. */
        static void AssertEnabled()
        {
            WI_ASSERT(WI_IsFeatureEnabled_SuppressUsage(FeatureType));
        }

        /** Returns true if the given staging feature is already enabled, or if it should be enabled, but is currently blocked
        from being enabled by a disabled dependent feature.  NOTE: this does not cover scnearios where the enabled state might
        be pending (such as if the feature has been enabled after the next reboot); these will still return false as they shouldn't
        currently be enabled. */
        static bool ShouldBeEnabled()
        {
            if (traits::isAlwaysEnabled)
            {
                return true;
            }
            if (traits::isAlwaysDisabled)
            {
                return false;
            }
            auto state = GetCachedFeatureEnabledState();
            return ((state == wil_details_CachedFeatureEnabledState_Enabled) || (state == wil_details_CachedFeatureEnabledState_Desired));
        }

        /** Use this method to explicitly record telemetry regarding a feature's usage, such as an explicit usage signal or usage duration.
        Normally, most "IsEnabled" or "RunIfEnabled" primitives provide an override that allows you to also pass
        the ReportKind into the primitive to do this reporting as an artifact of enablement checking.  That method is
        preferred to this method when it can be used as it handles both Actual and Potential usage, but if you're not
        simultaneously controlling enablement with this primitives, this method allows you to directly report usage.

        This method also enables callers to explicitly provide additional information associated and reporting on behalf of
        a given feature.  Examples:
        ~~~
        // Report a single unique usage of the feature:
        Feature_MyFeature::ReportUsage();

        // Report a single unique offering of the feature to the user (an offered entry point such as a menu) independent
        // from actual use of the feature:
        Feature_MyFeature::ReportUsage(ReportingKind::UniqueOpportunity);
        ~~~
        You can also explicitly provide feature usage duration (though this is better done through ReportUsageDuration)
        as well as custom data points.  These data points can be usage counts, outcome counts, items per operation or
        any other count that makes sense to view in aggregate relation to usage.  */
        static void ReportUsage(ReportingKind kind = ReportingKind::UniqueUsage, size_t addend = 1)
        {
            ReportUsageToService(WI_IsFeatureEnabled_SuppressUsage(FeatureType), kind, addend);
        }

        static void ReportUsage(CustomReportingKind kind, size_t addend = 1)
        {
            ReportUsageToService(WI_IsFeatureEnabled_SuppressUsage(FeatureType), GetCustomReportingKind(static_cast<unsigned char>(kind)), addend);
        }

        /** Use this method to record telemetry regarding a feature's usage duration.
        This method returns an RAII class that will automatically record the duration between the call to this method
        and the destruction of the returned object and report that duration via telemetry.
        ~~~
        auto usage = Feature_MyFeature::ReportUsageDuration();
        // your feature code goes here
        // telemetry will be reported when 'usage' goes out of scope
        ~~~
        If the total duration of your feature involves some user prompting then you should capture that time via
        UsageKind::PausedDuration and keep the TotalDuration inclusive of this time.  Note that you can also use
        this method to record duration for custom data points as well. */
        static FeatureUsageDuration ReportUsageDuration(ReportingKind kind = ReportingKind::TotalDuration)
        {
            WI_ASSERT((kind != ReportingKind::None) && (kind != ReportingKind::UniqueUsage) && (kind != ReportingKind::UniqueOpportunity) &&
                (kind != ReportingKind::DeviceUsage) && (kind != ReportingKind::DeviceOpportunity));
            return FeatureUsageDuration(details::FeatureUsageDurationData(traits::id, WI_IsFeatureEnabled_SuppressUsage(FeatureType), kind));
        }

        /** Use this method to record telemetry for the current variant. This should be called when a user has been exposed to the variant behavior. */
        __forceinline static void ReportCurrentVariantUsage(VariantReportingKind kind = VariantReportingKind::DeviceUsage)
        {
            kind;
            WI_GetVariant_ReportUsage(FeatureType, kind);
        }

        /// @cond
        // Depracated: Will be removed after RI/FI cycle; use ReportingKind, not FeatureUsageKind
        static void ReportUsage(FeatureUsageKind kind, size_t addend = 1)
        {
            switch (kind)
            {
            case FeatureUsageKind::ImplicitOpportunityCount:
                ReportUsage(ReportingKind::DeviceOpportunity, addend);
                break;
            case FeatureUsageKind::ImplicitUsageCount:
                ReportUsage(ReportingKind::DeviceUsage, addend);
                break;
            case FeatureUsageKind::OpportunityCount:
                ReportUsage(ReportingKind::UniqueOpportunity, addend);
                break;
            case FeatureUsageKind::UsageCount:
                ReportUsage(ReportingKind::UniqueUsage, addend);
                break;
            case FeatureUsageKind::TotalDuration:
                ReportUsage(ReportingKind::TotalDuration, addend);
                break;
            case FeatureUsageKind::PausedDuration:
                ReportUsage(ReportingKind::PausedDuration, addend);
                break;
            case FeatureUsageKind::Custom1:
            case FeatureUsageKind::Custom2:
            case FeatureUsageKind::Custom3:
            case FeatureUsageKind::Custom4:
            case FeatureUsageKind::Custom5:
                ReportUsage(GetCustomReportingKind(static_cast<unsigned char>(kind) - static_cast<unsigned char>(FeatureUsageKind::Custom1)), addend);
                break;
            }
        }

        // Depracated: Will be removed after RI/FI cycle; use ReportingKind, not FeatureUsageKind
        static FeatureUsageDuration ReportUsageDuration(FeatureUsageKind kind)
        {
            return ReportUsageDuration((kind == FeatureUsageKind::TotalDuration) ? ReportingKind::TotalDuration : ReportingKind::PausedDuration);
        }
        /// @endcond

        /** This primitive will run the given functor while automatically capturing error (exception) statistics and associating them with the feature.
        See RunIfEnabled for more information.  This routine behaves identically with the exception that there is
        no enablement checking and no usage information reported.  The feature should already be enabled when this
        routine is used. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto Run(const DiagnosticsInfo& diagnostics, TFunctor&& functor) -> decltype(functor())
        {
            AssertEnabled();
            return details::RunFeatureDispatch(traits::id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
        }

        /** This primitive will run the given functor while automatically capturing error (exception) statistics and associating them with the feature and reporting usage.
        See RunIfEnabled for more information.  This routine behaves identically with the exception that there is
        no enablement checking.  The feature should already be enabled when this routine is used. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto Run(const DiagnosticsInfo& diagnostics, ReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            AssertEnabled();
            ReportUsageToService(true, kind);
            return details::RunFeatureDispatch(traits::id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());
        }

        /** This primitive will run the given functor while automatically capturing error (exception) statistics and associating them with the feature and reporting usage.
        See RunIfEnabled for more information.  This routine behaves identically with the exception that there is
        no enablement checking.  The feature should already be enabled when this routine is used. */
        template <ErrorReturn errorReturn = ErrorReturn::Auto, typename TFunctor>
        __forceinline static auto Run(const DiagnosticsInfo& diagnostics, CustomReportingKind kind, TFunctor&& functor) -> decltype(functor())
        {
            AssertEnabled();
            ReportUsageToService(true, GetCustomReportingKind(static_cast<unsigned char>(kind)));
            return details::RunFeatureDispatch(id, diagnostics, functor, details::functor_tag<errorReturn, TFunctor>());           
        }

        /** Use this primitive in case of inlining failure for Run/RunIfEnabled:
        ~~~
        wil::ThreadErrorContext context;
        wil::DiagnosticsInfo diagnostics = WI_DIAGNOSTICS_INFO;
        try
        {
        // your code
        }
        catch (...)
        {
        Feature_YourFeature::ReportCaughtException(diagnostics, context);
        }
        ~~~
        */
        static void ReportCaughtException(const DiagnosticsInfo& diagnostics, ThreadErrorContext& context)
        {
            details::ReportFeatureCaughtException(context, traits::id, diagnostics);
        }

        /// @cond
        // Supports WI_* feature macros; do not use directly
        inline static bool __private_IsEnabledPreCheck(ReportingKind kind)
        {
            #pragma warning(suppress:4127)  // conditional expression is constant
            if (traits::isAlwaysEnabled || traits::isAlwaysDisabled)
            {
                WI_ASSERT((kind != ReportingKind::TotalDuration) && (kind != ReportingKind::PausedDuration));
                ReportUsageToService(traits::isAlwaysEnabled, kind);
            }
            return true;
        }
        inline static bool __private_IsEnabledPreCheck()
        {
            return __private_IsEnabledPreCheck(ReportingKind::DeviceUsage);
        }

        // Supports WI_* feature macros; do not use directly
        inline static bool __private_ReportVariantDisabled(Variant variant, VariantReportingKind kind = VariantReportingKind::DeviceUsage)
        {
            ReportVariantUsageToService((variant == Variant::None), variant, kind);
            return true;
        }
        inline static bool __private_ReportVariantDisabled(VariantReportingKind kind = VariantReportingKind::DeviceUsage)
        {
            return __private_ReportVariantDisabled(Variant::None, kind);
        }

        // Supports WI_* feature macros; do not use directly
        __declspec(noinline) static bool __private_IsEnabled()
        {
            const auto enabled = (GetCachedFeatureEnabledState() == wil_details_CachedFeatureEnabledState_Enabled);
            #pragma warning(suppress:4127)  // conditional expression is constant
            if ((traits::usageReportingMode == wil::UsageReportingMode::Default) || ((traits::usageReportingMode == wil::UsageReportingMode::SuppressPotential) && enabled))
            {
                ReportUsageToService(enabled, ReportingKind::DeviceUsage);
            }
            return enabled;
        }
        __declspec(noinline) static bool __private_IsEnabled(ReportingKind kind)
        {
            const auto enabled = (GetCachedFeatureEnabledState() == wil_details_CachedFeatureEnabledState_Enabled);
#pragma warning(suppress:4127)  // conditional expression is constant
            if ((traits::usageReportingMode != wil::UsageReportingMode::SuppressPotential) || enabled)
            {
                ReportUsageToService(enabled, kind);
            }
            return enabled;
        }

        // Supports WI_* feature macros; do not use directly
        __declspec(noinline) static bool __private_IsVariantEqual(Variant variant, VariantReportingKind kind, bool implicit = false)
        {
            static_assert(!wistd::is_same<Variant, details::EmptyVariant>::value, "No variants for this feature");
            const Variant currentVariant = GetCachedVariantState();
            const auto isVariantEqual = (currentVariant == variant);

#pragma warning(push)
#pragma warning(disable:4127)  // conditional expression is constant
            implicit; variant;
            if ((traits::usageReportingMode == ::wil::UsageReportingMode::Default) ||
                ((traits::usageReportingMode == ::wil::UsageReportingMode::SuppressImplicit) && !implicit) ||
                ((traits::usageReportingMode == ::wil::UsageReportingMode::SuppressPotential) && isVariantEqual))
            {
                ReportVariantUsageToService(isVariantEqual, variant, kind);
                if (currentVariant == Variant::None && variant != Variant::None)
                {
                    ReportVariantUsageToService(TRUE, Variant::None, kind);
                }
            }
#pragma warning(pop)
            return isVariantEqual;
        }
        inline static bool __private_IsVariantEqual(Variant variant)
        {
            return __private_IsVariantEqual(variant, VariantReportingKind::DeviceUsage, true);  // true = implicit
        }

        // Supports WI_* feature macros; do not use directly
        __declspec(noinline) static Variant __private_GetVariant(VariantReportingKind kind, bool implicit = false)
        {
            static_assert(!wistd::is_same<Variant, details::EmptyVariant>::value, "No variants for this feature");
            const auto variant = GetCachedVariantState();
            const auto isVariantEqual = (variant != Variant::None);

#pragma warning(push)
#pragma warning(disable:4127)  // conditional expression is constant
            implicit; isVariantEqual;
            if ((traits::usageReportingMode == ::wil::UsageReportingMode::Default) ||
                ((traits::usageReportingMode == ::wil::UsageReportingMode::SuppressImplicit) && !implicit) ||
                ((traits::usageReportingMode == ::wil::UsageReportingMode::SuppressPotential) && isVariantEqual))
            {
                ReportVariantUsageToService(true, variant, kind);
            }
            return variant;
#pragma warning(pop)
        }
        inline static Variant __private_GetVariant()
        {
            return __private_GetVariant(VariantReportingKind::DeviceUsage, true);  // true = implicit
        }

        // Supports testing; do not use directly (will 'forget' about usage)
        static void __private_ClearCache()
        {
            auto& cache = GetFeaturePropertyCache();
            cache.var = 0;

            auto& variantCache = GetFeatureVariantPropertyCache();
            variantCache.propertyCache.var = 0;
        }

        __declspec(noinline) static bool __private_IsEnabledWithDefault(bool isEnabledByDefaultOverride)
        {
            return (GetCachedFeatureEnabledState(isEnabledByDefaultOverride) == wil_details_CachedFeatureEnabledState_Enabled);
        }
        /// @endcond

    private:
        typedef Feature<traits> FeatureType;

        static bool ShouldBeEnabledForDependentFeature()
        {
            return ((traits::ShouldBeEnabledForDependentFeature_DefaultDisabled() || traits::ShouldBeEnabledForDependentFeature_DefaultEnabled()));
        }

        static wil_details_CachedFeatureEnabledState GetCurrentFeatureEnabledState(bool isEnabledByDefaultOverride, _Out_ wil_details_CachedHasNotificationState* hasNotificationState, _Out_ BOOL* hasVariantConfig)
        {
            bool shouldBeEnabled = false;

            FEATURE_ENABLED_STATE state = ::wil::details::WilApi_GetFeatureEnabledState(traits::id, static_cast<FEATURE_CHANGE_TIME>(traits::changeTime));
            *hasNotificationState = WI_IsFlagSet(state, FEATURE_ENABLED_STATE_HAS_NOTIFICATION) ? wil_details_CachedHasNotificationState_HasNotification : wil_details_CachedHasNotificationState_DoesNotHaveNotifications;
            *hasVariantConfig = WI_IsFlagSet(state, FEATURE_ENABLED_STATE_HAS_VARIANT_CONFIGURATION);

            WI_ClearAllFlags(state, FEATURE_ENABLED_STATE_HAS_NOTIFICATION | FEATURE_ENABLED_STATE_HAS_VARIANT_CONFIGURATION);
            if (state != FEATURE_ENABLED_STATE_DEFAULT)
            {
                // Any explicitly configured feature controls the state (configuration wins)
                shouldBeEnabled = (state == FEATURE_ENABLED_STATE_ENABLED);
            }
            else
            {
                // We're not configured: Are there features enabled that require our feature to first be enabled?
                shouldBeEnabled = (isEnabledByDefaultOverride || ShouldBeEnabledForDependentFeature());
            }

            if (shouldBeEnabled)
            {
                return (traits::AreDependenciesEnabled() ? wil_details_CachedFeatureEnabledState_Enabled : wil_details_CachedFeatureEnabledState_Desired);
            }

            return wil_details_CachedFeatureEnabledState_Disabled;
        }

        static Variant GetCurrentVariantState(unsigned int* payloadId, BOOL* hasNotification, BOOL* hasVariantConfiguration)
        {
            unsigned int variantVal = ::wil::details::WilApi_GetFeatureVariant(traits::id, static_cast<FEATURE_CHANGE_TIME>(traits::changeTime), payloadId, hasNotification);
            *hasVariantConfiguration = WI_IsFlagSet(variantVal, c_wil_details_Variant_IsVariantConfig);
            WI_ClearFlag(variantVal, c_wil_details_Variant_IsVariantConfig);

            auto variant = static_cast<Variant>(variantVal);
            if (variant == Variant::None)
            {
                *payloadId = traits::defaultPayloadValue;
                variant = traits::defaultVariant;
            }
            if (!traits::AreDependenciesEnabled())
            {
                *payloadId = traits::defaultPayloadValue;
                variant = Variant::None;
            }

            return variant;
        }

        inline static wil_details_FeaturePropertyCache& GetFeaturePropertyCache()
        {
            static wil_details_FeaturePropertyCache data = {};
            return data;
        }

        inline static wil_details_FeatureVariantPropertyCache& GetFeatureVariantPropertyCache()
        {
            static wil_details_FeatureVariantPropertyCache data = {};
            return data;
        }

        static wil_details_CachedFeatureEnabledState GetCachedFeatureEnabledState(bool isEnabledByDefaultOverride = traits::isEnabledByDefault)
        {
            auto& cache = GetFeaturePropertyCache();
            wil_details_CachedFeatureEnabledState cacheState = static_cast<wil_details_CachedFeatureEnabledState>(cache.cache.enabledState);
            bool enabledStateUnknown = (cacheState == wil_details_CachedFeatureEnabledState_Unknown);
            if (enabledStateUnknown || cache.cache.hasNotificationState == wil_details_CachedHasNotificationState_Unknown)
            {
                wil_details_CachedHasNotificationState hasNotificationState = wil_details_CachedHasNotificationState_Unknown;
                BOOL hasVariantConfiguration;
                wil_details_CachedFeatureEnabledState currentState = GetCurrentFeatureEnabledState(isEnabledByDefaultOverride, &hasNotificationState, &hasVariantConfiguration);
                BOOL cacheHasVariantConfig = cache.cache.isVariant;
                if (enabledStateUnknown)
                {
                    cacheState = currentState;
                    cacheHasVariantConfig = hasVariantConfiguration;
                }

                // We need to subscribe to state changes regardless of the change time trait in order to detect changes to usage subscriptions. Enablement
                // changes will only take effect for OnRead features
                ::wil::details::SubscribeFeaturePropertyCacheToEnabledStateChanges(&cache, static_cast<wil_FeatureChangeTime>(traits::changeTime));

                if (!g_wil_details_testStates || !wil_HasFeatureTestState(traits::id, nullptr))
                {
                    if (enabledStateUnknown)
                    {
                        wil_details_SetEnabledAndHasNotificationStateProperties(&cache, cacheState, hasNotificationState, cacheHasVariantConfig);
                    }
                    else
                    {
                        wil_details_SetHasNotificationStateProperty(&cache, hasNotificationState);
                    }
                }
            }
            return cacheState;
        }

        static Variant GetCachedVariantState()
        {
            auto& cache = GetFeatureVariantPropertyCache();
            Variant result = static_cast<Variant>(cache.propertyCache.variant.variant);
            wil_details_CachedFeatureEnabledState cacheState = static_cast<wil_details_CachedFeatureEnabledState>(cache.propertyCache.variant.enabledState);
            bool enabledStateUnknown = (cacheState == wil_details_CachedFeatureEnabledState_Unknown);
            if (enabledStateUnknown || cache.propertyCache.variant.hasNotificationState == wil_details_CachedHasNotificationState_Unknown)
            {
                unsigned int payloadId;
                BOOL hasNotification = FALSE;
                BOOL hasVariantConfiguration = FALSE;
                Variant currentVariant = GetCurrentVariantState(&payloadId, &hasNotification, &hasVariantConfiguration);
                wil_details_CachedFeatureEnabledState currentState = (result == Variant::None) ? wil_details_CachedFeatureEnabledState_Disabled :
                    wil_details_CachedFeatureEnabledState_Enabled;
                BOOL cacheHasVariantConfig = cache.propertyCache.variant.isVariant;

                if (enabledStateUnknown)
                {
                    result = currentVariant;
                    cacheState = currentState;
                    cacheHasVariantConfig = hasVariantConfiguration;
                }

                // We need to subscribe to state changes regardless of the change time trait in order to detect changes to usage subscriptions. Enablement
                // changes will only take effect for OnRead features
                ::wil::details::SubscribeFeaturePropertyCacheToEnabledStateChanges(&cache.propertyCache, static_cast<wil_FeatureChangeTime>(traits::changeTime));

                if (g_wil_details_testStates && wil_HasFeatureTestState(traits::id, nullptr))
                {
                    cacheState = wil_details_CachedFeatureEnabledState_Unknown;
                }

                wil_details_FeaturePropertyCache newCache;
                newCache.var = cache.propertyCache.var;
                newCache.variant.hasNotificationState = (hasNotification ? wil_details_CachedHasNotificationState_HasNotification : wil_details_CachedHasNotificationState_DoesNotHaveNotifications);
                if (enabledStateUnknown)
                {
                    newCache.variant.isVariant = cacheHasVariantConfig;
                    newCache.variant.enabledState = cacheState;
                    if ((cache.payloadId != payloadId) || (cache.propertyCache.variant.variant != static_cast<unsigned char>(result)))
                    {
                        newCache.variant.recordedDeviceUsage = 0;
                        newCache.variant.variant = static_cast<unsigned char>(result);
                        cache.payloadId = payloadId;
                    }
                }

                if (cache.propertyCache.var != newCache.var)
                {
                    ::InterlockedExchange(&cache.propertyCache.var, newCache.var);
                }
            }
            return result;
        }

        static void ReportUsageToService(bool enabled, ReportingKind kind, size_t addend = 1)
        {
            // Ensure we have checked for notifications
            auto &cache = GetFeaturePropertyCache();
            if (cache.cache.hasNotificationState == wil_details_CachedHasNotificationState_Unknown)
            {
                // We are reporting usage but we have not loaded the configuration for this feature yet. Force load it here so we know whether or not there
                // are any subscribers.
                GetCachedFeatureEnabledState();
            }

            const FEATURE_LOGGED_TRAITS loggedTraits = GetFeatureLoggedTraits();
            wil_details_FeaturePropertyCache_ReportUsageToService(&cache, traits::id, &loggedTraits, enabled,
                static_cast<wil_ReportingKind>(kind), addend);
        }

        static void ReportVariantUsageToService(bool enabled, Variant variant, VariantReportingKind kind, size_t addend = 1)
        {
            // Ensure we have checked for notifications
            auto &cache = GetFeatureVariantPropertyCache();
            if (cache.propertyCache.variant.hasNotificationState == wil_details_CachedHasNotificationState_Unknown)
            {
                // We are reporting usage but we have not loaded the configuration for this feature yet. Force load it here so we know whether or not there
                // are any subscribers.
                GetCachedVariantState();
            }

            const FEATURE_LOGGED_TRAITS loggedTraits = GetFeatureLoggedTraits();
            wil_details_FeaturePropertyCache_ReportVariantUsageToService(&cache, traits::id, &loggedTraits, enabled ? TRUE : FALSE,
                static_cast<unsigned char>(variant), static_cast<wil_VariantReportingKind>(kind), addend);
        }
    };

    /// @cond
    namespace details
    {
        static const FeatureStage UnknownFeatureStage = static_cast<FeatureStage>(-1);

        enum class EmptyVariant : unsigned char
        {
            None = 0
        };

        // Provides the defaults for optional properties that can be specified from the XML configuration
        struct FeatureTraitsBase
        {
            typedef wil::CustomReportingKind CustomReportingKind;
            typedef EmptyVariant Variant;
            static const Variant defaultVariant = Variant::None;
            static const ::wil_VariantPayloadType payloadType = wil_VariantPayloadType_None;
            static const unsigned int defaultPayloadValue = 0;
            static const unsigned short version = 0;
            static const unsigned short baseVersion = 0;
            static const FeatureStage stageOverride = UnknownFeatureStage;
            static const FeatureStage stageChkOverride = UnknownFeatureStage;
            static const FeatureChangeTime changeTime = FeatureChangeTime::OnRead;
            static const UsageReportingMode usageReportingMode = UsageReportingMode::Default;
            static bool ShouldBeEnabledForDependentFeature_DefaultDisabled() { return false; }
            static bool ShouldBeEnabledForDependentFeature_DefaultEnabled() { return false; }
            static bool AreDependenciesEnabled() { return true; }
            static PCSTR GetEMail() { return nullptr; }
            static PCSTR GetLink() { return nullptr; }
        };

        __declspec(noinline) inline bool ShouldBeEnabledFromConfigAndDefault(unsigned int featureId, bool isEnabledByDefault)
        {
            FEATURE_ENABLED_STATE state = WilApi_GetFeatureEnabledState(featureId, FEATURE_CHANGE_TIME_READ);
            WI_ClearAllFlags(state, FEATURE_ENABLED_STATE_HAS_NOTIFICATION | FEATURE_ENABLED_STATE_HAS_VARIANT_CONFIGURATION);
            if (state != FEATURE_ENABLED_STATE_DEFAULT)
            {
                // Any explicitly configured feature controls the state (configuration wins)
                return (state == FEATURE_ENABLED_STATE_ENABLED);
            }
            return isEnabledByDefault;
        }

        // Used to provide the recursive evaluation of whether or not a feature should be enabled because there is a child feature that depends upon it that is already enabled.
        template <unsigned int... Args>
        struct DependentFeatures;

        template <unsigned int featureId>
        struct DependentFeatures<featureId>
        {
            static bool ShouldBeEnabledForDependentFeature(bool isEnabledByDefault) { return ShouldBeEnabledFromConfigAndDefault(featureId, isEnabledByDefault); }
        };

        template <unsigned int featureId, unsigned int... Args>
        struct DependentFeatures<featureId, Args...>
        {
            static bool ShouldBeEnabledForDependentFeature(bool isEnabledByDefault) { return (ShouldBeEnabledFromConfigAndDefault(featureId, isEnabledByDefault) || DependentFeatures<Args...>::ShouldBeEnabledForDependentFeature(isEnabledByDefault)); }
        };

        // Used to provide the recursive evaluation of whether or not required features are enabled so that a parent can be enabled.
        template <typename... Args>
        struct RequiredFeatures;

        template <typename TFeature>
        struct RequiredFeatures<TFeature>
        {
            static bool IsEnabled() { return WI_IsFeatureEnabled_SuppressUsage(TFeature); }
        };

        template <typename TFeature, typename... Args>
        struct RequiredFeatures<TFeature, Args...>
        {
            static bool IsEnabled() { return (WI_IsFeatureEnabled_SuppressUsage(TFeature) || RequiredFeatures<Args...>::IsEnabled()); }
        };

        inline void NTAPI RecordSRUMFeatureUsage(UINT32 featureId, UINT32 kind, UINT32 addend)
        {
            // Special case: we're logging unique and custom usage to SRUM.
            WI_SetFlag(kind, c_wil_details_srumReportingFlag);
            WilApi_RecordFeatureUsage(featureId, kind, addend, nullptr);
        }

        inline void NTAPI RecordFeatureUsageCallback(unsigned int featureId, wil_details_ServiceReportingKind kind, unsigned int addend, _In_opt_ wil_details_FeaturePropertyCache* cache, _In_ wil_details_RecordUsageResult* result)
        {
            if (cache != nullptr)
            {
                if (g_wil_details_RecordSRUMFeatureUsage)
                {
                    UINT32 reportingKind = static_cast<UINT32>(kind);
                    if ((reportingKind == wil_details_ServiceReportingKind_UniqueUsage) || ((reportingKind >= wil_details_ServiceReportingKind_CustomEnabledBase) && ((size_t)reportingKind < wil_details_ServiceReportingKind_CustomDisabledBase)))
                    {
                        g_wil_details_RecordSRUMFeatureUsage(featureId, reportingKind, addend);
                    }
                }

                // Record usage normally
                if (result->queueBackground)
                {
                    g_enabledStateManager.get().QueueBackgroundUsageReporting(featureId, *cache);
                }

                if (result->countImmediate > 0)
                {
                    // REVISION NEEDED:
                    // At this point, the result contains payloadId for variants that also needs
                    // to be communicated throught the API.

                    WilApi_RecordFeatureUsage(featureId, static_cast<UINT32>(result->kindImmediate), static_cast<UINT32>(result->countImmediate), nullptr);
                }

                // Subscribe to process-wide notifications to ensure we flush data on modern app suspend or background task completion (desktop
                // processes flush data when g_enabledStateManager's destructor is run)
                if (!result->ignoredUse)
                {
                    g_enabledStateManager.get().EnsureSubscribedToUsageFlush([](void*)
                    {
                        g_enabledStateManager.get().OnTimer();
                    });
                }
            }
            else
            {
                // Special case: we're trying to fire notifications
                UINT32 reportingKind = static_cast<UINT32>(result->kindImmediate);
                WI_SetFlagIf(reportingKind, c_wil_details_ReportingKindHonorVariantConfig, result->isVariantConfiguration);

                WilApi_RecordFeatureUsage(featureId, reportingKind, 0, nullptr);
            }
        }

        inline void NTAPI FeaturePropertyChangeNotificationCallback(_Inout_opt_ wil_details_FeaturePropertyCache* cache, wil_FeatureChangeTime changeTime)
        {
            g_enabledStateManager.get().SubscribeFeaturePropertyCacheToEnabledStateChanges(cache, changeTime);
        }

        //! Persists cached feature usage and error data (process wide). This should be called by framework code when DllMain won't be called for DLL unload (e.g. when a modern app is suspended or when a background task completes).
        inline void NTAPI RecordCachedUsage() WI_NOEXCEPT
        {
            // Feature id, kind, and addend of zero is a special case indicating that we should request all cached usage data be flushed
            WilApi_RecordFeatureUsage(0, 0, 0, nullptr);
        }

        inline UINT32 NTAPI GetFeatureVariantHelper(UINT32 featureId, FEATURE_CHANGE_TIME changeTime, _Out_ UINT32* payloadId, _Out_ BOOL* hasNotification)
        {
            unsigned char variant;
            unsigned int payload;
            if (wil_HasFeatureVariantTestState(featureId, &variant, &payload))
            {
                *payloadId = payload;
                *hasNotification = FALSE;
                return variant;
            }

            return ::GetFeatureVariant(featureId, changeTime, payloadId, hasNotification);
        }

        inline void WilInitialize_Staging_Common()
        {
            g_wil_details_recordFeatureUsage = RecordFeatureUsageCallback;
            g_wil_details_featurePropertyCacheChangeNotification = FeaturePropertyChangeNotificationCallback;
        }

        inline void WilInitialize_Staging_SRUMFeatureReporting()
        {
            g_wil_details_RecordSRUMFeatureUsage = RecordSRUMFeatureUsage;
        }

#ifdef WIL_ENABLE_STAGING_API
        //! Call this method to initialize WIL manually in a module where STAGING_SUPPRESS_STATIC_INITIALIZERS is required. WIL will
        //! only use publicly documented APIs.
        inline void WilInitialize_Staging_Api()
        {
            WilInitialize_Staging_Common();
            g_wil_details_apiGetFeatureEnabledState = ::GetFeatureEnabledState;
            g_wil_details_apiRecordFeatureUsage = ::RecordFeatureUsage;
            g_wil_details_apiRecordFeatureError = ::RecordFeatureError;
            g_wil_details_apiSubscribeFeatureStateChangeNotification = ::SubscribeFeatureStateChangeNotification;
            g_wil_details_apiUnsubscribeFeatureStateChangeNotification = ::UnsubscribeFeatureStateChangeNotification;
            g_wil_details_apiGetFeatureVariant = GetFeatureVariantHelper;
        }
#else
        //! Call this method to initialize WIL manually in a module where STAGING_SUPPRESS_STATIC_INITIALIZERS is required. WIL will
        //! use internal methods
        inline void WilInitialize_Staging_InternalApi()
        {
            WilInitialize_Staging_Common();
            g_wil_details_internalGetFeatureEnabledState = WilApiImpl_GetFeatureEnabledState;
            g_wil_details_internalRecordFeatureUsage = WilApiImpl_RecordFeatureUsage;
            g_wil_details_internalRecordFeatureError = WilApiImpl_RecordFeatureError;
            g_wil_details_internalSubscribeFeatureStateChangeNotification = WilApiImpl_SubscribeFeatureStateChangeNotification;
            g_wil_details_internalUnsubscribeFeatureStateChangeNotification = WilApiImpl_UnsubscribeFeatureStateChangeNotification;
            g_wil_details_internalGetFeatureVariant = WilApiImpl_GetFeatureVariant;
        }
#endif // WIL_ENABLE_STAGING_API

#ifndef STAGING_SUPPRESS_STATIC_INITIALIZERS
#ifdef WIL_ENABLE_STAGING_API
        WI_HEADER_INITITALIZATION_FUNCTION(InitializeStagingHeaderApi, []
        {
            WilInitialize_Staging_Api();
            return 1;
        });
#else
        WI_HEADER_INITITALIZATION_FUNCTION(InitializeStagingHeaderInternalApi, []
        {
            WilInitialize_Staging_InternalApi();
            return 1;
        });
#endif // WIL_ENABLE_STAGING_API
#if (NTDDI_VERSION >= NTDDI_WIN10_RS4)
#if !defined(WIL_DISABLE_SRUM_WNF)
        WI_HEADER_INITITALIZATION_FUNCTION(InitializeStagingSRUMFeatureReporting, []
        {
            WilInitialize_Staging_SRUMFeatureReporting();
            return 1;
        });
#endif // WIL_DISABLE_SRUM_WNF
#endif // NTDDI_WIN10_RS4
#endif // STAGING_SUPPRESS_STATIC_INITIALIZERS

        // To light up the feature-based integration with WRL, WRL's module header must be included PRIOR to the staging header
#ifdef _WRL_MODULE_H_

        inline void OriginateApiNotAvailable(HRESULT hr)
        {
            ::RoOriginateError(hr, ::Microsoft::WRL::Wrappers::HStringReference(L"This API is for evaluation purposes only and is subject to change or removal in future updates.").Get());
        }

        template <typename Feature, typename FeatureEnabledClass, ::Microsoft::WRL::FactoryCacheFlags cacheFlagValue = ::Microsoft::WRL::FactoryCacheDefault>
        class IfFeatureActivationFactory : public ::Microsoft::WRL::ActivationFactory<::Microsoft::WRL::Details::Nil, ::Microsoft::WRL::Details::Nil, ::Microsoft::WRL::Details::Nil, cacheFlagValue>
        {
        public:
            STDMETHOD(ActivateInstance)(_Outptr_result_nullonfailure_ IInspectable **ppvObject)
            {
                if (Feature::IsEnabled())
                {
                    return ::Microsoft::WRL::MakeAndInitialize<FeatureEnabledClass>(ppvObject);
                }
                *ppvObject = nullptr;
                OriginateApiNotAvailable(E_NOTIMPL);
                return E_NOTIMPL;
            }
        };

        template <typename FeatureInterface1>
        bool IsFeatureInterfaceEnabled(REFIID riid)
        {
            if (__uuidof(FeatureInterface1::interface_type) == riid) { return FeatureInterface1::feature_type::IsEnabled(); }
            return true;
        }

        template <typename FeatureInterface1, typename FeatureInterface2>
        bool IsFeatureInterfaceEnabled(REFIID riid)
        {
            if (__uuidof(FeatureInterface1::interface_type) == riid) { return FeatureInterface1::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface2::interface_type) == riid) { return FeatureInterface2::feature_type::IsEnabled(); }
            return true;
        }

        template <typename FeatureInterface1, typename FeatureInterface2, typename FeatureInterface3>
        bool IsFeatureInterfaceEnabled(REFIID riid)
        {
            if (__uuidof(FeatureInterface1::interface_type) == riid) { return FeatureInterface1::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface2::interface_type) == riid) { return FeatureInterface2::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface3::interface_type) == riid) { return FeatureInterface3::feature_type::IsEnabled(); }
            return true;
        }

        template <typename FeatureInterface1, typename FeatureInterface2, typename FeatureInterface3, typename FeatureInterface4>
        bool IsFeatureInterfaceEnabled(REFIID riid)
        {
            if (__uuidof(FeatureInterface1::interface_type) == riid) { return FeatureInterface1::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface2::interface_type) == riid) { return FeatureInterface2::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface3::interface_type) == riid) { return FeatureInterface3::feature_type::IsEnabled(); }
            if (__uuidof(FeatureInterface4::interface_type) == riid) { return FeatureInterface4::feature_type::IsEnabled(); }
            return true;
        }

        template <typename Interface, bool>
        struct SelectFeatureInterface
        {
            using type = Interface;
        };

        template <typename Interface>
        struct SelectFeatureInterface<Interface, true>
        {
            using type = IInspectable;
        };

        template <typename Feature, typename Factory>
        inline HRESULT STDMETHODCALLTYPE CreateActivationFactoryWithFeature(_In_ unsigned int *flags, _In_ const ::Microsoft::WRL::Details::CreatorMap* entry, REFIID riid, _Outptr_ IUnknown **ppFactory) throw()
        {
            *ppFactory = nullptr;
            if (Feature::IsEnabled())
            {
                return ::Microsoft::WRL::Details::CreateActivationFactory<Factory>(flags, entry, riid, ppFactory);
            }
            OriginateApiNotAvailable(E_NOTIMPL);
            return E_NOTIMPL;
        }
#endif // __WRL_MODULE_H_
    }
    /// @endcond

    //! Prepares for process suspension/termination by persisting in-memory data (e.g. usage and error data).
    inline void PrepareForTerminate()
    {
        details::RecordCachedUsage();
    }

// To light up the feature-based integration with WRL, WRL's module header must be included PRIOR to the staging header
#ifdef _WRL_MODULE_H_
    template <typename Feature, typename Interface>
    struct FeatureInterface
    {
        using feature_type = Feature;
        using interface_type = Interface;
    };

    template <typename Feature, typename Interface>
    using FeatureInterfaceIfPresent = typename ::wil::details::SelectFeatureInterface<Interface, Feature::featureTraits::isAlwaysDisabled>::type;
#endif // __WRL_MODULE_H_
} // wil

#define RETURN_IF_API_DISABLED(Feature) \
    if (!Feature::IsEnabled()) \
    { \
        ::wil::details::OriginateApiNotAvailable(E_NOTIMPL); \
        return E_NOTIMPL; \
    }

#define ActivatableClassWithFactory_IfFeatureEnabled(feature, className, factory) \
    InternalWrlCreateCreatorMap(className, reinterpret_cast<const IID*>(&className::InternalGetRuntimeClassName), &className::InternalGetTrustLevel, (::wil::details::CreateActivationFactoryWithFeature<feature, factory>), "minATL$__r")

#define ActivatableClass_IfFeatureEnabled(feature, enabledClassName) \
    using feature##enabledClassName##Factory = ::wil::details::IfFeatureActivationFactory<feature, enabledClassName>; \
    ActivatableClassWithFactory(enabledClassName, feature##enabledClassName##Factory)

#define ActivatableStaticOnlyFactory_IfFeatureEnabled(feature, factory) \
    InternalWrlCreateCreatorMap(factory, reinterpret_cast<const IID*>(&factory::InternalGetRuntimeClassNameStatic), &factory::InternalGetTrustLevelStatic, (::wil::details::CreateActivationFactoryWithFeature<feature, factory>), "minATL$__r")

#define InspectableClass_WithFeatureInterfaces(runtimeClassName, trustLevel, ...) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassName() throw() \
        { \
            static_assert((RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRt || \
                (RuntimeClassT::ClassFlags::value & ::Microsoft::WRL::WinRtClassicComMix) == ::Microsoft::WRL::WinRtClassicComMix, \
                    "'InspectableClass' macro must not be used with ClassicCom clasess."); \
            static_assert(__is_base_of(::Microsoft::WRL::Details::RuntimeClassBase, RuntimeClassT), "'InspectableClass' macro can only be used with ::Windows::WRL::RuntimeClass types"); \
            static_assert(!__is_base_of(IActivationFactory, RuntimeClassT), "Incorrect usage of IActivationFactory interface. Make sure that your RuntimeClass doesn't implement IActivationFactory interface use ::Windows::WRL::ActivationFactory instead or 'InspectableClass' macro is not used on ::Windows::WRL::ActivationFactory"); \
            return runtimeClassName; \
        } \
        static TrustLevel STDMETHODCALLTYPE InternalGetTrustLevel() throw() \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            HRESULT hr = S_OK; \
            const wchar_t *name = InternalGetRuntimeClassName(); \
            if (name != nullptr) \
            { \
                hr = ::WindowsCreateString(name, static_cast<UINT32>(::wcslen(name)), runtimeName); \
            } \
            return hr; \
        } \
        STDMETHOD(GetTrustLevel)(_Out_ TrustLevel* trustLvl) \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD(GetIids)(_Out_ ULONG *iidCount, \
            _When_(*iidCount == 0, _At_(*iids, _Post_null_)) \
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_)) \
            _Result_nullonfailure_ IID **iids) \
        { \
            return RuntimeClassT::GetIids(iidCount, iids); \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, _Outptr_result_nullonfailure_ void **ppvObject) override \
        { \
            if (!::wil::details::IsFeatureInterfaceEnabled<__VA_ARGS__>(riid)) \
            { \
                *ppvObject = nullptr; \
                ::wil::details::OriginateApiNotAvailable(E_NOINTERFACE); \
                return E_NOINTERFACE; \
            } \
            return RuntimeClassT::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return RuntimeClassT::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return RuntimeClassT::AddRef(); \
        } \
    private:

#define InspectableClassStatic_WithFeatureInterfaces(runtimeClassName, trustLevel, ...) \
    public: \
        static const wchar_t* STDMETHODCALLTYPE InternalGetRuntimeClassNameStatic() throw() \
        { \
            static_assert(__is_base_of(IActivationFactory, ActivationFactoryT) && __is_base_of(::Microsoft::WRL::Details::FactoryBase, ActivationFactoryT), "'InspectableClassStatic' macro can only be used with ::Windows::WRL::ActivationFactory types"); \
            static_assert(!__is_base_of(ActivationFactoryT::FirstInterface, ::Microsoft::WRL::Details::Nil), "ActivationFactory with 'InspectableClassStatic' macro requires to specify custom interfaces"); \
            return runtimeClassName; \
        } \
        static TrustLevel STDMETHODCALLTYPE InternalGetTrustLevelStatic() throw() \
        { \
            return trustLevel; \
        } \
        STDMETHOD(GetRuntimeClassName)(_Out_ HSTRING* runtimeName) \
        { \
            *runtimeName = nullptr; \
            return E_ILLEGAL_METHOD_CALL; \
        } \
        STDMETHOD(GetTrustLevel)(_Out_ TrustLevel* trustLvl) \
        { \
            *trustLvl = trustLevel; \
            return S_OK; \
        } \
        STDMETHOD(GetIids)(_Out_ ULONG *iidCount, \
            _When_(*iidCount == 0, _At_(*iids, _Post_null_)) \
            _When_(*iidCount > 0, _At_(*iids, _Post_notnull_)) \
            _Result_nullonfailure_ IID **iids) \
        { \
            return ActivationFactoryT::GetIids(iidCount, iids); \
        } \
        STDMETHOD(QueryInterface)(REFIID riid, _Outptr_result_nullonfailure_ void **ppvObject) \
        { \
            if (!::wil::details::IsFeatureInterfaceEnabled<__VA_ARGS__>(riid)) \
            { \
                *ppvObject = nullptr; \
                ::wil::details::OriginateApiNotAvailable(E_NOINTERFACE); \
                return E_NOINTERFACE; \
            } \
            return ActivationFactoryT::QueryInterface(riid, ppvObject); \
        } \
        STDMETHOD_(ULONG, Release)() \
        { \
            return ActivationFactoryT::Release(); \
        } \
        STDMETHOD_(ULONG, AddRef)() \
        { \
            return ActivationFactoryT::AddRef(); \
        } \
    private:



#pragma warning(pop)

#endif // defined(__cplusplus) && !defined(__WIL_MIN_KERNEL)

#pragma warning(pop)

#endif // FEATURE_STAGING_LEGACY_MODE
#endif // __WIL_STAGING_INCLUDED
