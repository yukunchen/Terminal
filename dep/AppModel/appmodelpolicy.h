//
//    Copyright (C) Microsoft.  All rights reserved.
//
//    This header contains the AppModelPolicy APIs and associated enums. All policies have
//    a C++ version, implemented by C-compatible, untyped versions. C callers, and non-exception
//    handling C++ callers, should reference the untyped implementations directly.
//
//    These APIs support both real token handles and pseudo tokens.
//
//    Example user-mode usage:
//
//      LifecycleManagerPolicy policy = GetLifecycleManagerPolicy(GetCurrentThreadEffectiveToken());
//
//    Example kernel-mode usage:
//
//      PACCESS_TOKEN token = PsReferencePrimaryToken(EProcess);
//      LifecycleManagerPolicy policy = GetLifecycleManagerPolicy(token);
//      PsDereferencePrimaryToken(token);
//
//    Required include paths:
//
//      INCLUDES = $(MINWIN_PRIV_SDK_INC_PATH); \
//                 $(MINCORE_PRIV_SDK_INC_PATH);
//
//    Required headers:
//
//      #include <nt.h>
//      #include <ntrtl.h>
//      #include <nturtl.h>
//      #include <windef.h>
//      #include <winerror.h>
//      #include <ntassert.h>
//
//    Required lib:
//
//      TARGETLIBS = $(SDK_LIB_PATH)\ntdll.lib
//

#pragma once

#include <AppModelPolicyImplementation.h>

// Only include C++/exception-based APIs if exceptions are enabled
#if defined(_CPPUNWIND) && !defined(WIL_SUPPRESS_EXCEPTIONS)

#include <wil\result.h>

namespace AppModelPolicy {

// Specifies the lifecycle manager policy for the process.
enum class LifecycleManagerPolicy {
    Unmanaged    = AppModelPolicy_LifecycleManager_Unmanaged,
    ManagedByPLM = AppModelPolicy_LifecycleManager_ManagedByPLM, // Managed by PLM (Process Lifecycle Manager)
    ManagedByEM  = AppModelPolicy_LifecycleManager_ManagedByEM,  // Managed by EM (Execution Manager)
};

inline LifecycleManagerPolicy GetLifecycleManagerPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue lifecycleManager;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_LifecycleManager, &lifecycleManager));
    return static_cast<LifecycleManagerPolicy>(lifecycleManager);
}

// AppDataAccessPolicy represents whether Application Data should be used for the process to store system data.
enum class AppDataAccessPolicy {
    Allowed = AppModelPolicy_AppDataAccess_Allowed,
    Denied  = AppModelPolicy_AppDataAccess_Denied,
};

inline AppDataAccessPolicy GetAppDataAccessPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue appDataAccess;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AppDataAccess, &appDataAccess));
    return static_cast<AppDataAccessPolicy>(appDataAccess);
}

// WindowingModelPolicy is used by components that need to know the internal UI details of the application,
// such as the root Win32 HWND, or information directly off of the App's CoreWindow.
enum class WindowingModelPolicy {
    Hwnd        = AppModelPolicy_WindowingModel_Hwnd,        // Application uses classic Win32 HWND to host the UI
    CoreWindow  = AppModelPolicy_WindowingModel_CoreWindow,  // Application uses CoreWindow to host the UI
    LegacyPhone = AppModelPolicy_WindowingModel_LegacyPhone, // Application uses neither HWND nor CoreWindow to host the UI (e.g, Splash, Silverlight)
    None        = AppModelPolicy_WindowingModel_None,        // Application does not host UI (e.g., service)
};

inline WindowingModelPolicy GetWindowingModelPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue windowingModel;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_WindowingModel, &windowingModel));
    return static_cast<WindowingModelPolicy>(windowingModel);
}

// DllSearchOrderPolicy specifies which search order policy to use according to:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms682586(v=vs.85).aspx
enum class DllSearchOrderPolicy {
    Traditional                 = AppModelPolicy_DllSearchOrder_Traditional,                 // Adhere's to MSDN's "Search Order for Desktop Applications"
    PackageGraphBased           = AppModelPolicy_DllSearchOrder_PackageGraphBased,           // Adhere's to MSDN's "Search Order for Windows Store apps"
};

inline DllSearchOrderPolicy GetDllSearchOrderPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue dllSearchOrder;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_DllSearchOrder, &dllSearchOrder));
    return static_cast<DllSearchOrderPolicy>(dllSearchOrder);
}

// Specifies Fusion/SxS permission
enum class FusionPolicy {
    Full    = AppModelPolicy_Fusion_Full,    // Full Fusion/SxS functionality allowed
    Limited = AppModelPolicy_Fusion_Limited, // Universal subset of Fusion/SxS functionality allowed
};

inline FusionPolicy GetFusionPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue fusion;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_Fusion, &fusion));
    return static_cast<FusionPolicy>(fusion);
}

// Specifies whether the non-Windows code is allowed to be loaded
enum class NonWindowsCodeLoadingPolicy {
    Allowed = AppModelPolicy_NonWindowsCodeLoading_Allowed,
    Denied  = AppModelPolicy_NonWindowsCodeLoading_Denied,
};

inline NonWindowsCodeLoadingPolicy GetNonWindowsCodeLoadingPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue nonWindowsCode;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_NonWindowsCodeLoading, &nonWindowsCode));
    return static_cast<NonWindowsCodeLoadingPolicy>(nonWindowsCode);
}

// Specifies method used to end a process
enum class ProcessEndPolicy {
    TerminateProcess = AppModelPolicy_ProcessEnd_TerminateProcess, // Immediately ends process
    ExitProcess      = AppModelPolicy_ProcessEnd_ExitProcess,      // Allows code execution at shutdown
};

inline ProcessEndPolicy GetProcessEndPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue processEnd;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ProcessEnd, &processEnd));
    return static_cast<ProcessEndPolicy>(processEnd);
}

// Specifies automatic initialization performed when BeginThread creates a thread
enum class BeginThreadInitPolicy {
    RoInitialize = AppModelPolicy_BeginThreadInit_RoInitialize,
    None         = AppModelPolicy_BeginThreadInit_None,
};

inline BeginThreadInitPolicy GetBeginThreadInitPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue beginThreadInit;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_BeginThreadInit, &beginThreadInit));
    return static_cast<BeginThreadInitPolicy>(beginThreadInit);
}

// Specifies method used to surface developer information such as asserts to the user
enum class DeveloperInformationPolicy {
    UI   = AppModelPolicy_DeveloperInformation_UI,
    None = AppModelPolicy_DeveloperInformation_None,
};

inline DeveloperInformationPolicy GetDeveloperInformationPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue devInfo;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_DeveloperInformation, &devInfo));
    return static_cast<DeveloperInformationPolicy>(devInfo);
}

// Specifies create file access
enum class CreateFileAccessPolicy {
    Full    = AppModelPolicy_CreateFileAccess_Full,
    Limited = AppModelPolicy_CreateFileAccess_Limited,
};

inline CreateFileAccessPolicy GetCreateFileAccessPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue fileAccess;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_CreateFileAccess, &fileAccess));
    return static_cast<CreateFileAccessPolicy>(fileAccess);
}

// Specifies whether child processes are allowed to implicitly break away from the package of their parent process
enum class ImplicitPackageBreakawayPolicy {
    Allowed     = AppModelPolicy_ImplicitPackageBreakaway_Allowed,
    Denied      = AppModelPolicy_ImplicitPackageBreakaway_Denied,
    DeniedByApp = AppModelPolicy_ImplicitPackageBreakaway_DeniedByApp,
};

inline ImplicitPackageBreakawayPolicy GetImplicitPackageBreakawayPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue implicitPackageBreakaway;
    THROW_IF_FAILED(AppModelPolicy_GetProcessPolicy_ImplicitPackageBreakaway(token, &implicitPackageBreakaway));
    return static_cast<ImplicitPackageBreakawayPolicy>(implicitPackageBreakaway);
}

// Specifies whether the application requires a tailored shim for activation
enum class ProcessActivationShimPolicy {
    None                = AppModelPolicy_ProcessActivationShim_None,
    PackagedCWALauncher = AppModelPolicy_ProcessActivationShim_PackagedCWALauncher,  // Use PackagedCWALauncher.exe for activation
};

inline ProcessActivationShimPolicy GetProcessActivationShimPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue activationShim;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ProcessActivationShim, &activationShim));
    return static_cast<ProcessActivationShimPolicy>(activationShim);
}

// Specifies whether the State Repository can be queried for information about this process/app
enum class AppKnownToStateRepositoryPolicy {
    Known   = AppModelPolicy_AppKnownToStateRepository_Known,
    Unknown = AppModelPolicy_AppKnownToStateRepository_Unknown,
};

inline AppKnownToStateRepositoryPolicy GetAppKnownToStateRepositoryPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue known;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AppKnownToStateRepository, &known));
    return static_cast<AppKnownToStateRepositoryPolicy>(known);
}

// Specifies the Audio Management policy for the process.
enum class AudioManagementPolicy {
    Unmanaged    = AppModelPolicy_AudioManagement_Unmanaged,
    ManagedByPBM = AppModelPolicy_AudioManagement_ManagedByPBM, // Managed by PBM (Playback Manager)
};

inline AudioManagementPolicy GetAudioManagementPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue AudioManagement;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AudioManagement, &AudioManagement));
    return static_cast<AudioManagementPolicy>(AudioManagement);
}

// Specifies whether the application type supports deployment of COM registrations to the public packaged store
enum class PackageMayContainPublicComRegistrationsPolicy {
    Yes = AppModelPolicy_PackageMayContainPublicComRegistrations_Yes,
    No  = AppModelPolicy_PackageMayContainPublicComRegistrations_No,
};

inline PackageMayContainPublicComRegistrationsPolicy GetPackageMayContainPublicComRegistrationsPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue mayContain;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_PackageMayContainPublicComRegistrations, &mayContain));
    return static_cast<PackageMayContainPublicComRegistrationsPolicy>(mayContain);
}

// Specifies whether the application type supports some kind of private packaged COM registration
enum class PackageMayContainPrivateComRegistrationsPolicy {
    None        = AppModelPolicy_PackageMayContainPrivateComRegistrations_None,
    PrivateHive = AppModelPolicy_PackageMayContainPrivateComRegistrations_PrivateHive,
};

inline PackageMayContainPrivateComRegistrationsPolicy GetPackageMayContainPrivateComRegistrationsPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue mayContain;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_PackageMayContainPrivateComRegistrations, &mayContain));
    return static_cast<PackageMayContainPrivateComRegistrationsPolicy>(mayContain);
}

// Specifies whether the application type supports some kind of private packaged COM registration
enum class ComLaunchCreateProcessExtensionsPolicy {
    None                    = AppModelPolicy_ComLaunchCreateProcessExtensions_None,
    RegisterWithPsm         = AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,
    RegisterWithDesktopAppX = AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithDesktopAppX,
};

inline ComLaunchCreateProcessExtensionsPolicy GetComLaunchCreateProcessExtensionsPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue createProcessExtensions;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComLaunchCreateProcessExtensions, &createProcessExtensions));
    return static_cast<ComLaunchCreateProcessExtensionsPolicy>(createProcessExtensions);
}

// Specifies whether the loader should ignore the LOAD_WITH_ALTERED_SEARCH_PATH flag set,
// which indicates an aboslute path, during an attempt to load a library from a relative path
enum class LoaderIgnoreAlteredSearchForRelativePathPolicy {
    False = AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,
    True  = AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_True,
};

inline LoaderIgnoreAlteredSearchForRelativePathPolicy GetLoaderIgnoreAlteredSearchForRelativePathPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue ignore;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_LoaderIgnoreAlteredSearchForRelativePath, &ignore));
    return static_cast<LoaderIgnoreAlteredSearchForRelativePathPolicy>(ignore);
}

// Specifies whether an Activate As Activator COM registration in the classic registry locations
// activated by an application of this type is implicitly activated with Interactive User semantics
enum class ImplicitlyActivateClassicAAAServersAsIUPolicy {
    Yes = AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes,
    No = AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,
};

inline ImplicitlyActivateClassicAAAServersAsIUPolicy GetImplicitlyActivateClassicAAAServersAsIUPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue implicitlyActivateAsIU;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ImplicitlyActivateClassicAAAServersAsIU, &implicitlyActivateAsIU));
    return static_cast<ImplicitlyActivateClassicAAAServersAsIUPolicy>(implicitlyActivateAsIU);
}

// Specifies whether to search COM catalog in both HKLM and HKCU, or HKLM only.
enum class ComClassicCatalogPolicy {
    MachineHiveAndUserHive = AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,
    MachineHiveOnly        = AppModelPolicy_ComClassicCatalog_MachineHiveOnly,
};

inline ComClassicCatalogPolicy GetComClassicCatalogPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue comClassicCatalog;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComClassicCatalog, &comClassicCatalog));
    return static_cast<ComClassicCatalogPolicy>(comClassicCatalog);
}

// Specifies whether to force strong unmarshaling policy for the process, or allow application to choose whether or not
// to use strong unmarshaling policy.
enum class ComUnmarshalingPolicy {
    ForceStrongUnmarshaling = AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling,
    ApplicationManaged      = AppModelPolicy_ComUnmarshaling_ApplicationManaged,
};

inline ComUnmarshalingPolicy GetComUnmarshalingPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue comUnmarshaling;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComUnmarshaling, &comUnmarshaling));
    return static_cast<ComUnmarshalingPolicy>(comUnmarshaling);
}

// Specifies whether to enable perf enhancement feature designed for UWA launch
enum class ComAppLaunchPerfEnhancementsPolicy {
    Enabled  = AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled,
    Disabled = AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,
};

inline ComAppLaunchPerfEnhancementsPolicy GetComAppLaunchPerfEnhancementsPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue appLaunchPerfEnhancements;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComAppLaunchPerfEnhancements, &appLaunchPerfEnhancements));
    return static_cast<ComAppLaunchPerfEnhancementsPolicy>(appLaunchPerfEnhancements);
}

// Specifies whether COM expects the app to perform its own security initialization, or provides a default security
// initialization under the assumption that the app will not.
enum class ComSecurityInitializationPolicy {
    SystemManaged      = AppModelPolicy_ComSecurityInitialization_SystemManaged,
    ApplicationManaged = AppModelPolicy_ComSecurityInitialization_ApplicationManaged,
};

inline ComSecurityInitializationPolicy GetComSecurityInitializationPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue comSecurityInitialization;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComSecurityInitialization, &comSecurityInitialization));
    return static_cast<ComSecurityInitializationPolicy>(comSecurityInitialization);
}

// Specifies whether RoInitialize should create ASTA or STA for Single Threaded Apartment
enum class RoInitializeSingleThreadedBehaviorPolicy {
    STA  = AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,
    ASTA = AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA,
};

inline RoInitializeSingleThreadedBehaviorPolicy GetRoInitializeSingleThreadedBehaviorPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue roInitializeSingleThreadedBehavior;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_RoInitializeSingleThreadedBehavior, &roInitializeSingleThreadedBehavior));
    return static_cast<RoInitializeSingleThreadedBehaviorPolicy>(roInitializeSingleThreadedBehavior);
}

// Specifies whether COM will handle non-fatal exceptions and continue, or terminate the server process.
enum class ComDefaultExceptionHandlingPolicy {
    HandleAll  = AppModelPolicy_ComDefaultExceptionHandling_HandleAll,
    HandleNone = AppModelPolicy_ComDefaultExceptionHandling_HandleNone,
};

inline ComDefaultExceptionHandlingPolicy GetComDefaultExceptionHandlingPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue comDefaultExceptionHandling;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComDefaultExceptionHandling, &comDefaultExceptionHandling));
    return static_cast<ComDefaultExceptionHandlingPolicy>(comDefaultExceptionHandling);
}

// Specifies whether proxies to out-of-proc COM objects are agile or non-agile.
enum class ComOopProxyAgilityPolicy {
    Agile    = AppModelPolicy_ComOopProxyAgility_Agile,
    NonAgile = AppModelPolicy_ComOopProxyAgility_NonAgile,
};

inline ComOopProxyAgilityPolicy GetComOopProxyAgilityPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue comOopProxyAgility;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ComOopProxyAgility, &comOopProxyAgility));
    return static_cast<ComOopProxyAgilityPolicy>(comOopProxyAgility);
}

enum class AppServiceLifetimePolicy {
    StandardTimeout = AppModelPolicy_AppServiceLifetime_StandardTimeout,
    ExtendedForSamePackage = AppModelPolicy_AppServiceLifetime_ExtendedForSamePackage,
};

inline AppServiceLifetimePolicy GetAppServiceLifetimePolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue appServiceLifetime;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AppServiceLifetime, &appServiceLifetime));
    return static_cast<AppServiceLifetimePolicy>(appServiceLifetime);
}

// Specifies if Edge or Legacy WebPlatform Components should be used.
enum class WebPlatformPolicy {
    Edge   = AppModelPolicy_WebPlatform_Edge,
    Legacy = AppModelPolicy_WebPlatform_Legacy,
};

inline WebPlatformPolicy GetWebPlatformPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue webPlatformPolicyValue;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_WebPlatform, &webPlatformPolicyValue));
    return static_cast<WebPlatformPolicy>(webPlatformPolicyValue);
}

enum class WinInetStoragePartitioningPolicy {
    Isolated = AppModelPolicy_WinInetStoragePartitioning_Isolated,
    SharedWithAppContainer = AppModelPolicy_WinInetStoragePartitioning_SharedWithAppContainer
};

inline WinInetStoragePartitioningPolicy GetWinInetStoragePartitioningPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue wininetStoragePolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_WinInetStoragePartitioning, &wininetStoragePolicy));
    return static_cast<WinInetStoragePartitioningPolicy>(wininetStoragePolicy);
}

// Specifies how app provided protocol handler is hosted: per-user host process or per-app host process.
enum class IndexerProtocolHandlerHostPolicy {
    PerUser = AppModelPolicy_IndexerProtocolHandlerHost_PerUser,
    PerApp  = AppModelPolicy_IndexerProtocolHandlerHost_PerApp,
};

inline IndexerProtocolHandlerHostPolicy GetIndexerProtocolHandlerHostPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue indexerProtocolHandlerHostPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_IndexerProtocolHandlerHost, &indexerProtocolHandlerHostPolicy));
    return static_cast<IndexerProtocolHandlerHostPolicy>(indexerProtocolHandlerHostPolicy);
}

// Specifies whether the loader should support user dll directories (e.g., SetDllDirectory)
enum class LoaderIncludeUserDirectoriesPolicy {
    False = AppModelPolicy_LoaderIncludeUserDirectories_False,
    True  = AppModelPolicy_LoaderIncludeUserDirectories_True,
};

inline LoaderIncludeUserDirectoriesPolicy GetLoaderIncludeUserDirectoriesPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue loaderIncludeUserDirectoriesPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_LoaderIncludeUserDirectories, &loaderIncludeUserDirectoriesPolicy));
    return static_cast<LoaderIncludeUserDirectoriesPolicy>(loaderIncludeUserDirectoriesPolicy);
}

// Specifies whether creation of AppContainer processes implicitly convert to Restricted AppContainer
enum class ConvertAppContainerToRestrictedAppContainerPolicy {
    False = AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,
    True  = AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_True,
};

inline ConvertAppContainerToRestrictedAppContainerPolicy GetConvertAppContainerToRestrictedAppContainerPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue convertAppContainerToRestrictedAppContainerPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ConvertAppContainerToRestrictedAppContainer, &convertAppContainerToRestrictedAppContainerPolicy));
    return static_cast<ConvertAppContainerToRestrictedAppContainerPolicy>(convertAppContainerToRestrictedAppContainerPolicy);
}

// Specifies whether the application type supports some kind of private packaged MAPI Provider
enum class PackageMayContainPrivateMapiProviderPolicy {
    None        = AppModelPolicy_PackageMayContainPrivateMapiProvider_None,
    PrivateHive = AppModelPolicy_PackageMayContainPrivateMapiProvider_PrivateHive,
};

inline PackageMayContainPrivateMapiProviderPolicy GetPackageMayContainPrivateMapiProviderPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue mayContain;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_PackageMayContainPrivateMapiProvider, &mayContain));
    return static_cast<PackageMayContainPrivateMapiProviderPolicy>(mayContain);
}

// The package claims that should be applied to the token used to launch an admin process
enum class AdminProcessPackageClaimsPolicy {
    None = AppModelPolicy_AdminProcessPackageClaims_None,
    Caller = AppModelPolicy_AdminProcessPackageClaims_Caller,
};

inline AdminProcessPackageClaimsPolicy GetAdminProcessPackageClaimsPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue policy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AdminProcessPackageClaims, &policy));
    return static_cast<AdminProcessPackageClaimsPolicy>(policy);
}

// The package claims that should be applied to the token used to launch an admin process
enum class RegistryRedirectionBehaviorPolicy {
    None = AppModelPolicy_RegistryRedirectionBehavior_None,
    CopyOnWrite = AppModelPolicy_RegistryRedirectionBehavior_CopyOnWrite,
};

inline RegistryRedirectionBehaviorPolicy GetRegistryRedirectionBehaviorPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue policy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_RegistryRedirectionBehavior, &policy));
    return static_cast<RegistryRedirectionBehaviorPolicy>(policy);
}

// The package claims that should be applied to the token used to launch an admin process
enum class BypassCreateProcessAppxExtensionPolicy {
    False = AppModelPolicy_BypassCreateProcessAppxExtension_False,
    True = AppModelPolicy_BypassCreateProcessAppxExtension_True,
};

inline BypassCreateProcessAppxExtensionPolicy GetBypassCreateProcessAppxExtensionPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue policy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_BypassCreateProcessAppxExtension, &policy));
    return static_cast<BypassCreateProcessAppxExtensionPolicy>(policy);
}

// Specifies how known folders can be redirected by SHGetKnownFolderPath
enum class KnownFolderRedirectionPolicy {
    Isolated = AppModelPolicy_KnownFolderRedirection_Isolated,
    RedirectToPackage = AppModelPolicy_KnownFolderRedirection_RedirectToPackage,
};

inline KnownFolderRedirectionPolicy GetKnownFolderRedirectionPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue redirectionPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_KnownFolderRedirection, &redirectionPolicy));
    return static_cast<KnownFolderRedirectionPolicy>(redirectionPolicy);
}

// Specifies which types of private ActivateAsPackage Winrt classes are visible
enum class PrivateActivateAsPackageWinrtClassesPolicy {
    AllowNone = AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,
    AllowFullTrust = AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowFullTrust,
    AllowNonFullTrust = AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNonFullTrust,
};

inline PrivateActivateAsPackageWinrtClassesPolicy GetPrivateActivateAsPackageWinrtClassesPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue policy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_PrivateActivateAsPackageWinrtClasses, &policy));
    return static_cast<PrivateActivateAsPackageWinrtClassesPolicy>(policy);
}

// Specifies how folders are redirected by SHGetKnownFolderPath to per-app locations for packaged app
enum class AppPrivateFolderRedirectionPolicy {
    None = AppModelPolicy_AppPrivateFolderRedirection_None,
    AppPrivate = AppModelPolicy_AppPrivateFolderRedirection_AppPrivate,
};

inline AppPrivateFolderRedirectionPolicy GetAppPrivateFolderRedirectionPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue redirectionPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_AppPrivateFolderRedirection, &redirectionPolicy));
    return static_cast<AppPrivateFolderRedirectionPolicy>(redirectionPolicy);
}

// Specifies what happens when the application tries to access the Global SystemAppData in the registry
enum class GlobalSystemAppDataAccessPolicy {
    Normal = AppModelPolicy_GlobalSystemAppDataAccess_Normal,
    Virtualized = AppModelPolicy_GlobalSystemAppDataAccess_Virtualized,
};

inline GlobalSystemAppDataAccessPolicy GetGlobalSystemAppDataAccessPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue accessPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_GlobalSystemAppDataAccess, &accessPolicy));
    return static_cast<GlobalSystemAppDataAccessPolicy>(accessPolicy);
}

// Specifies what happens when a process inherits console handles from its parent process
enum class ConsoleHandleInheritancePolicy {
    ConsoleOnly = AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,
    All = AppModelPolicy_ConsoleHandleInheritance_All,
};

inline ConsoleHandleInheritancePolicy GetConsoleHandleInheritancePolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue accessPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ConsoleHandleInheritance, &accessPolicy));
    return static_cast<ConsoleHandleInheritancePolicy>(accessPolicy);
}

// Specifies what happens when a process attempts to access buffers in the console subsystem
enum class ConsoleBufferAccessPolicy {
    RestrictedUnidirectional = AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional,
    Unrestricted = AppModelPolicy_ConsoleBufferAccess_Unrestricted,
};

inline ConsoleBufferAccessPolicy GetConsoleBufferAccessPolicy(_In_ HANDLE token)
{
    AppModelPolicy_PolicyValue accessPolicy;
    THROW_IF_FAILED(AppModelPolicy_GetPolicy(token, AppModelPolicy_Type_ConsoleBufferAccess, &accessPolicy));
    return static_cast<ConsoleBufferAccessPolicy>(accessPolicy);
}

} // namespace AppModelPolicy

#endif // defined(_CPPUNWIND) && !defined(WIL_SUPPRESS_EXCEPTIONS)
