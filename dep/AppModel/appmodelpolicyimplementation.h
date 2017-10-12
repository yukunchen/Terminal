//
//    Copyright (C) Microsoft.  All rights reserved.
//
//    This header contains the untyped implementation of the App Model Policy APIs.
//    C callers should consume this layer directly via AppModelPolicy_GetPolicy().
//
//    These APIs return a single policy value for the single specified policy type associated
//    with the specified token. Both real token handles and pseudo tokens are supported.
//
//    Example user-mode usage:
//
//      AppModelPolicy_PolicyValue lifecyclePolicy;
//      hr = AppModelPolicy_GetPolicy(GetCurrentThreadEffectiveToken(),
//                                    AppModelPolicy_Type_LifecycleManager,
//                                    &lifecyclePolicy)));
//
//    Example kernel-mode usage:
//
//      PACCESS_TOKEN token = PsReferencePrimaryToken(EProcess);
//      AppModelPolicy_PolicyValue lifecyclePolicy;
//      hr = AppModelPolicy_GetPolicy(token,
//                                    AppModelPolicy_Type_LifecycleManager,
//                                    &lifecyclePolicy)));
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

#include <psmregisterapi.h>

// Policy types
typedef enum AppModelPolicy_Type {
    AppModelPolicy_Type_LifecycleManager                            = 0x01, // Value range: (0x00010000-0x0001FFFF)
    AppModelPolicy_Type_AppDataAccess                               = 0x02, // Value range: (0x00020000-0x0002FFFF)
    AppModelPolicy_Type_WindowingModel                              = 0x03, // Value range: (0x00030000-0x0003FFFF)
    AppModelPolicy_Type_DllSearchOrder                              = 0x04, // Value range: (0x00040000-0x0004FFFF)
    AppModelPolicy_Type_Fusion                                      = 0x05, // Value range: (0x00050000-0x0005FFFF)
    AppModelPolicy_Type_NonWindowsCodeLoading                       = 0x06, // Value range: (0x00060000-0x0006FFFF)
    AppModelPolicy_Type_ProcessEnd                                  = 0x07, // Value range: (0x00070000-0x0007FFFF)
    AppModelPolicy_Type_BeginThreadInit                             = 0x08, // Value range: (0x00080000-0x0008FFFF)
    AppModelPolicy_Type_DeveloperInformation                        = 0x09, // Value range: (0x00090000-0x0009FFFF)
    AppModelPolicy_Type_CreateFileAccess                            = 0x0A, // Value range: (0x000A0000-0x000AFFFF)
    AppModelPolicy_Type_ImplicitPackageBreakaway_Internal           = 0x0B, // Value range: (0x000B0000-0x000BFFFF)
    AppModelPolicy_Type_ProcessActivationShim                       = 0x0C, // Value range: (0x000C0000-0x000CFFFF)
    AppModelPolicy_Type_AppKnownToStateRepository                   = 0x0D, // Value range: (0x000D0000-0x000DFFFF)
    AppModelPolicy_Type_AudioManagement                             = 0x0E, // Value range: (0x000E0000-0x000EFFFF)
    AppModelPolicy_Type_PackageMayContainPublicComRegistrations     = 0x0F, // Value range: (0x000F0000-0x000FFFFF)
    AppModelPolicy_Type_PackageMayContainPrivateComRegistrations    = 0x10, // Value range: (0x00100000-0x0010FFFF)
    AppModelPolicy_Type_ComLaunchCreateProcessExtensions            = 0x11, // Value range: (0x00110000-0x0011FFFF)
    /*DO NOT USE*/AppModelPolicy_Type_ClrCompat                     = 0x12, // Value range: (0x00120000-0x0012FFFF)
    AppModelPolicy_Type_LoaderIgnoreAlteredSearchForRelativePath    = 0x13, // Value range: (0x00130000-0x0013FFFF)
    AppModelPolicy_Type_ImplicitlyActivateClassicAAAServersAsIU     = 0x14, // Value range: (0x00140000-0x0014FFFF)
    AppModelPolicy_Type_ComClassicCatalog                           = 0x15, // Value range: (0x00150000-0x0015FFFF)
    AppModelPolicy_Type_ComUnmarshaling                             = 0x16, // Value range: (0x00160000-0x0016FFFF)
    AppModelPolicy_Type_ComAppLaunchPerfEnhancements                = 0x17, // Value range: (0x00170000-0x0017FFFF)
    AppModelPolicy_Type_ComSecurityInitialization                   = 0x18, // Value range: (0x00180000-0x0018FFFF)
    AppModelPolicy_Type_RoInitializeSingleThreadedBehavior          = 0x19, // Value range: (0x00190000-0x0019FFFF)
    AppModelPolicy_Type_ComDefaultExceptionHandling                 = 0x1A, // Value range: (0x001A0000-0x001AFFFF)
    AppModelPolicy_Type_ComOopProxyAgility                          = 0x1B, // Value range: (0x001B0000-0x001BFFFF)
    AppModelPolicy_Type_AppServiceLifetime                          = 0x1C, // Value range: (0x001C0000-0x001CFFFF)
    AppModelPolicy_Type_WebPlatform                                 = 0x1D, // Value range: (0x001D0000-0x001DFFFF)
    AppModelPolicy_Type_WinInetStoragePartitioning                  = 0x1E, // Value range: (0x001E0000-0x001EFFFF)
    AppModelPolicy_Type_IndexerProtocolHandlerHost                  = 0x1F, // Value range: (0x001F0000-0x001FFFFF)
    AppModelPolicy_Type_LoaderIncludeUserDirectories                = 0x20, // Value range: (0x00200000-0x0020FFFF)
    AppModelPolicy_Type_ConvertAppContainerToRestrictedAppContainer = 0x21, // Value range: (0x00210000-0x0021FFFF)
    AppModelPolicy_Type_PackageMayContainPrivateMapiProvider        = 0x22, // Value range: (0x00220000-0x0022FFFF)
    AppModelPolicy_Type_AdminProcessPackageClaims                   = 0x23, // Value range: (0x00230000-0x0023FFFF)
    AppModelPolicy_Type_RegistryRedirectionBehavior                 = 0x24, // Value range: (0x00240000-0x0024FFFF)
    AppModelPolicy_Type_BypassCreateProcessAppxExtension            = 0x25, // Value range: (0x00250000-0x0025FFFF)
    AppModelPolicy_Type_KnownFolderRedirection                      = 0x26, // Value range: (0x00260000-0x0026FFFF)
    AppModelPolicy_Type_PrivateActivateAsPackageWinrtClasses        = 0x27, // Value range: (0x00270000-0x0027FFFF)
    AppModelPolicy_Type_AppPrivateFolderRedirection                 = 0x28, // Value range: (0x00280000-0x0028FFFF)
    AppModelPolicy_Type_GlobalSystemAppDataAccess                   = 0x29, // Value range: (0x00290000-0x0029FFFF)
    AppModelPolicy_Type_ConsoleHandleInheritance                    = 0x2A, // Value range: (0x002A0000-0x002AFFFF)
    AppModelPolicy_Type_ConsoleBufferAccess                         = 0x2B, // Value range: (0x002B0000-0x002BFFFF)
    AppModelPolicy_Type_Count = AppModelPolicy_Type_ConsoleBufferAccess
} AppModelPolicy_Type;

// Policy Values. Each policy type has a range, to allow each policy to grow:
// 0xTTTTVVVV, where the high word is the policy type, and the low word is the policy value
typedef enum AppModelPolicy_PolicyValue {

    // Specifies the lifecycle manager policy for the process
    AppModelPolicy_LifecycleManager_Unmanaged                       = (AppModelPolicy_Type_LifecycleManager << 16),                             // 0x00010000
    AppModelPolicy_LifecycleManager_ManagedByPLM,
    AppModelPolicy_LifecycleManager_ManagedByEM,

    // Represents whether Application Data should be used for the process to store system data.
    AppModelPolicy_AppDataAccess_Allowed                            = (AppModelPolicy_Type_AppDataAccess << 16),                                // 0x00020000
    AppModelPolicy_AppDataAccess_Denied,

    // WindowingModel is used by components that need to know the internal UI details of the application,
    // such as the root HWND, or information directly off of the App's CoreWindow.
    AppModelPolicy_WindowingModel_Hwnd                              = (AppModelPolicy_Type_WindowingModel << 16),                               // 0x00030000
    AppModelPolicy_WindowingModel_CoreWindow,
    AppModelPolicy_WindowingModel_LegacyPhone,
    AppModelPolicy_WindowingModel_None,

    // DllSearchOrder specifies which search order policy to use according to:
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682586(v=vs.85).aspx
    AppModelPolicy_DllSearchOrder_Traditional                       = (AppModelPolicy_Type_DllSearchOrder << 16),                               // 0x00040000
    AppModelPolicy_DllSearchOrder_PackageGraphBased,

    // Specifies Fusion/SxS permission
    AppModelPolicy_Fusion_Full                                      = (AppModelPolicy_Type_Fusion << 16),                                       // 0x00050000
    AppModelPolicy_Fusion_Limited,

    // Specifies whether the non-Windows code is allowed to be loaded
    AppModelPolicy_NonWindowsCodeLoading_Allowed                    = (AppModelPolicy_Type_NonWindowsCodeLoading << 16),                        // 0x00060000
    AppModelPolicy_NonWindowsCodeLoading_Denied,

    // Specifies method used to end a process
    AppModelPolicy_ProcessEnd_TerminateProcess                      = (AppModelPolicy_Type_ProcessEnd << 16),                                   // 0x00070000
    AppModelPolicy_ProcessEnd_ExitProcess,

    // Specifies automatic initialization performed when BeginThread creates a thread
    AppModelPolicy_BeginThreadInit_RoInitialize                     = (AppModelPolicy_Type_BeginThreadInit << 16),                              // 0x00080000
    AppModelPolicy_BeginThreadInit_None,

    // Specifies method used to surface developer information such as asserts to the user
    AppModelPolicy_DeveloperInformation_UI                          = (AppModelPolicy_Type_DeveloperInformation << 16),                         // 0x00090000
    AppModelPolicy_DeveloperInformation_None,

    // Specifies create file access
    AppModelPolicy_CreateFileAccess_Full                            = (AppModelPolicy_Type_CreateFileAccess << 16),                             // 0x000A0000
    AppModelPolicy_CreateFileAccess_Limited,

    // Specifies whether child processes are allowed to implicitly break away from the package of their parent process
    AppModelPolicy_ImplicitPackageBreakaway_Allowed                 = (AppModelPolicy_Type_ImplicitPackageBreakaway_Internal << 16),            // 0x000B0000
    AppModelPolicy_ImplicitPackageBreakaway_Denied,
    AppModelPolicy_ImplicitPackageBreakaway_DeniedByApp,

    // Specifies whether the application requires a tailored shim for activation
    AppModelPolicy_ProcessActivationShim_None                       = (AppModelPolicy_Type_ProcessActivationShim << 16),                        // 0x000C0000
    AppModelPolicy_ProcessActivationShim_PackagedCWALauncher,

    // Specifies whether the State Repository can be queried for information about this process/app
    AppModelPolicy_AppKnownToStateRepository_Known                  = (AppModelPolicy_Type_AppKnownToStateRepository << 16),                    // 0x000D0000
    AppModelPolicy_AppKnownToStateRepository_Unknown,

    // Specifies the audio management policy for the process
    AppModelPolicy_AudioManagement_Unmanaged                        = (AppModelPolicy_Type_AudioManagement << 16),                              // 0x000E0000
    AppModelPolicy_AudioManagement_ManagedByPBM,

    // Specifies whether the application type supports deployment of COM registrations to the public packaged store
    AppModelPolicy_PackageMayContainPublicComRegistrations_Yes      = (AppModelPolicy_Type_PackageMayContainPublicComRegistrations << 16),      // 0x000F0000
    AppModelPolicy_PackageMayContainPublicComRegistrations_No,

    // Specifies whether the application type supports some kind of private packaged COM registration
    AppModelPolicy_PackageMayContainPrivateComRegistrations_None    = (AppModelPolicy_Type_PackageMayContainPrivateComRegistrations << 16),     // 0x00100000
    AppModelPolicy_PackageMayContainPrivateComRegistrations_PrivateHive,

    // Specifies what extensions, if any, should be called in the CreateProcess path when launching a COM server in an application of this type
    AppModelPolicy_ComLaunchCreateProcessExtensions_None            = (AppModelPolicy_Type_ComLaunchCreateProcessExtensions << 16),             // 0x00110000
    AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,
    AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithDesktopAppX,

    // DO NOT USE - Intended only for CLR. All other callers should use more appropriate policies
    AppModelPolicy_ClrCompat_Others                                 = (AppModelPolicy_Type_ClrCompat << 16),                                    // 0x00120000
    AppModelPolicy_ClrCompat_ClassicDesktop,
    AppModelPolicy_ClrCompat_Universal,
    AppModelPolicy_ClrCompat_PackagedDesktop,

    // Specifies whether the loader should ignore the LOAD_WITH_ALTERED_SEARCH_PATH flag set,
    // which indicates an aboslute path, during an attempt to load a library from a relative path
    AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False   = (AppModelPolicy_Type_LoaderIgnoreAlteredSearchForRelativePath << 16),     // 0x00130000
    AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_True,

    // Specifies whether an Activate As Activator COM registration in the classic registry locations
    // activated by an application of this type is implicitly activated with Interactive User semantics
    AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes      = (AppModelPolicy_Type_ImplicitlyActivateClassicAAAServersAsIU << 16),      // 0x00140000
    AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,

    // Specifies whether to search COM catalog in both HKLM and HKCU, or HKLM only.
    AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive         = (AppModelPolicy_Type_ComClassicCatalog << 16),                            // 0x00150000
    AppModelPolicy_ComClassicCatalog_MachineHiveOnly,

    // Specifies whether to force strong unmarshaling policy for the process, or allow application to choose whether
    // or not to use strong unmarshaling policy.
    AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling          = (AppModelPolicy_Type_ComUnmarshaling << 16),                              // 0x00160000
    AppModelPolicy_ComUnmarshaling_ApplicationManaged,

    // Specifies whether to enable perf enhancement feature designed for UWA launch
    AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled             = (AppModelPolicy_Type_ComAppLaunchPerfEnhancements << 16),                 // 0x00170000
    AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,

    // Specifies whether COM expects the app to perform its own security initialization, or provides a default
    // security initialization under the assumption that the app will not.
    AppModelPolicy_ComSecurityInitialization_ApplicationManaged     = (AppModelPolicy_Type_ComSecurityInitialization << 16),                    // 0x00180000
    AppModelPolicy_ComSecurityInitialization_SystemManaged,

    // Specifies whether RoInitialize should create ASTA or STA for Single Threaded Apartment
    AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA          = (AppModelPolicy_Type_RoInitializeSingleThreadedBehavior << 16),           // 0x00190000
    AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,

    // Specifies whether COM will handle non-fatal exceptions and continue, or terminate the server process.
    AppModelPolicy_ComDefaultExceptionHandling_HandleAll            = (AppModelPolicy_Type_ComDefaultExceptionHandling << 16),                  // 0x001A0000
    AppModelPolicy_ComDefaultExceptionHandling_HandleNone,

    // Specifies whether proxies to out-of-proc COM objects are agile or non-agile.
    AppModelPolicy_ComOopProxyAgility_Agile                         = (AppModelPolicy_Type_ComOopProxyAgility << 16),                           // 0x001B0000
    AppModelPolicy_ComOopProxyAgility_NonAgile,

    // Specifies whether we use standard resource management policy for the
    // execution time of the invoked appservice, or use a client controlled policy
    AppModelPolicy_AppServiceLifetime_StandardTimeout               = (AppModelPolicy_Type_AppServiceLifetime << 16),                           // 0x001C0000
    AppModelPolicy_AppServiceLifetime_ExtendedForSamePackage,

    // Specifies whether consumers of WebPlatform components get Edge or Legacy versions.
    AppModelPolicy_WebPlatform_Edge                                 = (AppModelPolicy_Type_WebPlatform << 16),                                  // 0x001D0000
    AppModelPolicy_WebPlatform_Legacy,

    // Specifies whether wininet caches are shared across AC and non-AC application environments
    AppModelPolicy_WinInetStoragePartitioning_Isolated               = (AppModelPolicy_Type_WinInetStoragePartitioning << 16),                  // 0x001E0000
    AppModelPolicy_WinInetStoragePartitioning_SharedWithAppContainer,

    // Specifies how app provided protocol handler is hosted: per-user host process or per-app host process
    AppModelPolicy_IndexerProtocolHandlerHost_PerUser               = (AppModelPolicy_Type_IndexerProtocolHandlerHost << 16),                   // 0x001F0000
    AppModelPolicy_IndexerProtocolHandlerHost_PerApp,

    // Specifies whether the loader should support user dll directories (e.g., SetDllDirectory)
    AppModelPolicy_LoaderIncludeUserDirectories_False               = (AppModelPolicy_Type_LoaderIncludeUserDirectories << 16),                 // 0x00200000
    AppModelPolicy_LoaderIncludeUserDirectories_True,

    // Specifies whether creation of AppContainer processes implicitly convert to Restricted AppContainer
    AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False = (AppModelPolicy_Type_ConvertAppContainerToRestrictedAppContainer << 16), // 0x00210000
    AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_True,

    // Specifies whether the application type supports some kind of private packaged Mapi Provider
    AppModelPolicy_PackageMayContainPrivateMapiProvider_None        = (AppModelPolicy_Type_PackageMayContainPrivateMapiProvider << 16),         // 0x00220000
    AppModelPolicy_PackageMayContainPrivateMapiProvider_PrivateHive,

    // The package claims that should be applied to the token used to launch an admin process
    AppModelPolicy_AdminProcessPackageClaims_None                   = (AppModelPolicy_Type_AdminProcessPackageClaims << 16),                    // 0x00230000
    AppModelPolicy_AdminProcessPackageClaims_Caller,

    // Specifies how the registry operations are redirected, if at all
    AppModelPolicy_RegistryRedirectionBehavior_None                 = (AppModelPolicy_Type_RegistryRedirectionBehavior << 16),                  // 0x00240000
    AppModelPolicy_RegistryRedirectionBehavior_CopyOnWrite,

    // Specifies whether or not the AppX extensions should be invoked by CreateProcess
    AppModelPolicy_BypassCreateProcessAppxExtension_False           = (AppModelPolicy_Type_BypassCreateProcessAppxExtension << 16),             // 0x00250000
    AppModelPolicy_BypassCreateProcessAppxExtension_True,

    // Specifies how known folders are redirected by SHGetKnownFolderPath
    AppModelPolicy_KnownFolderRedirection_Isolated                  = (AppModelPolicy_Type_KnownFolderRedirection << 16),                       // 0x00260000
    AppModelPolicy_KnownFolderRedirection_RedirectToPackage,

    // Specifies which types of private ActivateAsPackage Winrt classes are visible
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone   = (AppModelPolicy_Type_PrivateActivateAsPackageWinrtClasses << 16),         // 0x00270000
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowFullTrust,
    AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNonFullTrust,

    // Specifies how folders are redirected by SHGetKnownFolderPath to per-app locations for packaged app
    AppModelPolicy_AppPrivateFolderRedirection_None                  = (AppModelPolicy_Type_AppPrivateFolderRedirection << 16),                 // 0x00280000
    AppModelPolicy_AppPrivateFolderRedirection_AppPrivate,

    // Specifies what happens when the application tries to access the Global SystemAppData in the registry
    AppModelPolicy_GlobalSystemAppDataAccess_Normal                   = (AppModelPolicy_Type_GlobalSystemAppDataAccess << 16),                 // 0x00290000
    AppModelPolicy_GlobalSystemAppDataAccess_Virtualized,

    // Specifies what happens when a process inherits console handles from its parent process
    AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly               = (AppModelPolicy_Type_ConsoleHandleInheritance << 16),                  // 0x002A0000
    AppModelPolicy_ConsoleHandleInheritance_All,

    // Specifies what happens when a process attempts to access buffers in the console subsystem
    AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional       = (AppModelPolicy_Type_ConsoleBufferAccess << 16),                       // 0x002B0000
    AppModelPolicy_ConsoleBufferAccess_Unrestricted,

} AppModelPolicy_PolicyValue;

__inline HRESULT AppModelPolicy_GetPolicy_Internal(
    _In_ HANDLE token,
    _In_ AppModelPolicy_Type policyType,
    _Out_ AppModelPolicy_PolicyValue* policyValue,
    _Out_ PPS_PKG_CLAIM PkgClaim,
    _Out_ PULONG64 AttributesPresent)
{
    // App types used for the policy logic
    // NOTE: this needs to be in sync with minkernel\appmodel\test\policyapis.cpp
    typedef enum AppModelPolicy_AppType {
        AppModelPolicy_AppType_Universal = 1,
        AppModelPolicy_AppType_Centennial,
        AppModelPolicy_AppType_Desktop,
        AppModelPolicy_AppType_Silverlight_8_1, // Silverlight 8.1
        AppModelPolicy_AppType_Silverlight_8_0, // Silverlight 8.0 or prior
        AppModelPolicy_AppType_Splash,
        AppModelPolicy_AppType_PackagedService,
        AppModelPolicy_AppType_PerAppBroker,

        AppModelPolicy_AppType_Count = AppModelPolicy_AppType_PerAppBroker
    } AppModelPolicy_AppType;

    // NOTE: this needs to be in sync with minkernel\appmodel\test\policyapis.cpp
    static const AppModelPolicy_PolicyValue AppModelPolicy_PolicyValue_Table[AppModelPolicy_Type_Count][AppModelPolicy_AppType_Count] =
    {
        // LifecycleManager
        {
            AppModelPolicy_LifecycleManager_ManagedByPLM,                               // Universal
            AppModelPolicy_LifecycleManager_Unmanaged,                                  // Centennial
            AppModelPolicy_LifecycleManager_Unmanaged,                                  // Desktop
            AppModelPolicy_LifecycleManager_ManagedByPLM,                               // Silverlight_8_1
            AppModelPolicy_LifecycleManager_ManagedByEM,                                // Silverlight_8_0
            AppModelPolicy_LifecycleManager_ManagedByEM,                                // Splash
            AppModelPolicy_LifecycleManager_Unmanaged,                                  // PackagedService
            AppModelPolicy_LifecycleManager_Unmanaged,                                  // PerAppBroker
        },

        // AppDataAccess
        {
            AppModelPolicy_AppDataAccess_Allowed,                                       // Universal
            AppModelPolicy_AppDataAccess_Allowed,                                       // Centennial
            AppModelPolicy_AppDataAccess_Denied,                                        // Desktop
            AppModelPolicy_AppDataAccess_Allowed,                                       // Silverlight_8_1
            AppModelPolicy_AppDataAccess_Denied,                                        // Silverlight_8_0
            AppModelPolicy_AppDataAccess_Denied,                                        // Splash
            AppModelPolicy_AppDataAccess_Denied,                                        // PackagedService
            AppModelPolicy_AppDataAccess_Denied,                                        // PerAppBroker
        },

        // WindowingModel
        {
            AppModelPolicy_WindowingModel_CoreWindow,                                   // Universal
            AppModelPolicy_WindowingModel_Hwnd,                                         // Centennial
            AppModelPolicy_WindowingModel_Hwnd,                                         // Desktop
            AppModelPolicy_WindowingModel_CoreWindow,                                   // Silverlight_8_1
            AppModelPolicy_WindowingModel_LegacyPhone,                                  // Silverlight_8_0
            AppModelPolicy_WindowingModel_LegacyPhone,                                  // Splash
            AppModelPolicy_WindowingModel_None,                                         // PackagedService
            AppModelPolicy_WindowingModel_Hwnd,                                         // PerAppBroker
        },

        // DllSearchOrder
        {
            AppModelPolicy_DllSearchOrder_PackageGraphBased,                            // Universal
            AppModelPolicy_DllSearchOrder_PackageGraphBased,                            // Centennial
            AppModelPolicy_DllSearchOrder_Traditional,                                  // Desktop
            AppModelPolicy_DllSearchOrder_PackageGraphBased,                            // Silverlight_8_1
            AppModelPolicy_DllSearchOrder_Traditional,                                  // Silverlight_8_0
            AppModelPolicy_DllSearchOrder_Traditional,                                  // Splash
            AppModelPolicy_DllSearchOrder_Traditional,                                  // PackagedService
            AppModelPolicy_DllSearchOrder_Traditional,                                  // PerAppBroker
        },

        // Fusion
        {
            AppModelPolicy_Fusion_Limited,                                              // Universal
            AppModelPolicy_Fusion_Full,                                                 // Centennial
            AppModelPolicy_Fusion_Full,                                                 // Desktop
            AppModelPolicy_Fusion_Limited,                                              // Silverlight_8_1
            AppModelPolicy_Fusion_Full,                                                 // Silverlight_8_0
            AppModelPolicy_Fusion_Full,                                                 // Splash
            AppModelPolicy_Fusion_Full,                                                 // PackagedService
            AppModelPolicy_Fusion_Full,                                                 // PerAppBroker
        },

        // NonWindowsCodeLoading
        {
            AppModelPolicy_NonWindowsCodeLoading_Denied,                                // Universal
            AppModelPolicy_NonWindowsCodeLoading_Allowed,                               // Centennial
            AppModelPolicy_NonWindowsCodeLoading_Allowed,                               // Desktop
            AppModelPolicy_NonWindowsCodeLoading_Denied,                                // Silverlight_8_1
            AppModelPolicy_NonWindowsCodeLoading_Denied,                                // Silverlight_8_0
            AppModelPolicy_NonWindowsCodeLoading_Denied,                                // Splash
            AppModelPolicy_NonWindowsCodeLoading_Allowed,                               // PackagedService
            AppModelPolicy_NonWindowsCodeLoading_Allowed,                               // PerAppBroker
        },

        // ProcessEnd
        {
            AppModelPolicy_ProcessEnd_TerminateProcess,                                 // Universal
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // Centennial
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // Desktop
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // Silverlight_8_1
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // Silverlight_8_0
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // Splash
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // PackagedService
            AppModelPolicy_ProcessEnd_ExitProcess,                                      // PerAppBroker
        },

        // BeginThreadInit
        {
            AppModelPolicy_BeginThreadInit_RoInitialize,                                // Universal
            AppModelPolicy_BeginThreadInit_None,                                        // Centennial
            AppModelPolicy_BeginThreadInit_None,                                        // Desktop
            AppModelPolicy_BeginThreadInit_None,                                        // Silverlight_8_1
            AppModelPolicy_BeginThreadInit_None,                                        // Silverlight_8_0
            AppModelPolicy_BeginThreadInit_None,                                        // Splash
            AppModelPolicy_BeginThreadInit_None,                                        // PackagedService
            AppModelPolicy_BeginThreadInit_None,                                        // PerAppBroker
        },

        // DeveloperInformation
        {
            AppModelPolicy_DeveloperInformation_None,                                   // Universal
            AppModelPolicy_DeveloperInformation_UI,                                     // Centennial
            AppModelPolicy_DeveloperInformation_UI,                                     // Desktop
            AppModelPolicy_DeveloperInformation_None,                                   // Silverlight_8_1
            AppModelPolicy_DeveloperInformation_None,                                   // Silverlight_8_0
            AppModelPolicy_DeveloperInformation_None,                                   // Splash
            AppModelPolicy_DeveloperInformation_None,                                   // PackagedService
            AppModelPolicy_DeveloperInformation_UI,                                     // PerAppBroker
        },

        // CreateFileAccess
        {
            AppModelPolicy_CreateFileAccess_Limited,                                    // Universal
            AppModelPolicy_CreateFileAccess_Full,                                       // Centennial
            AppModelPolicy_CreateFileAccess_Full,                                       // Desktop
            AppModelPolicy_CreateFileAccess_Limited,                                    // Silverlight_8_1
            AppModelPolicy_CreateFileAccess_Limited,                                    // Silverlight_8_0
            AppModelPolicy_CreateFileAccess_Full,                                       // Splash
            AppModelPolicy_CreateFileAccess_Full,                                       // PackagedService
            AppModelPolicy_CreateFileAccess_Full,                                       // PerAppBroker
        },

        // ImplicitPackageBreakaway
        {
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // Universal
            AppModelPolicy_ImplicitPackageBreakaway_Allowed,                            // Centennial
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // Desktop
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // Silverlight_8_1
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // Silverlight_8_0
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // Splash
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // PackagedService
            AppModelPolicy_ImplicitPackageBreakaway_Denied,                             // PerAppBroker
        },

        // ProcessActivationShim
        {
            AppModelPolicy_ProcessActivationShim_None,                                  // Universal
            AppModelPolicy_ProcessActivationShim_PackagedCWALauncher,                   // Centennial
            AppModelPolicy_ProcessActivationShim_None,                                  // Desktop
            AppModelPolicy_ProcessActivationShim_None,                                  // Silverlight_8_1
            AppModelPolicy_ProcessActivationShim_None,                                  // Silverlight_8_0
            AppModelPolicy_ProcessActivationShim_None,                                  // Splash
            AppModelPolicy_ProcessActivationShim_None,                                  // PackagedService
            AppModelPolicy_ProcessActivationShim_None,                                  // PerAppBroker
        },

        // AppKnownToStateRepository
        {
            AppModelPolicy_AppKnownToStateRepository_Known,                             // Universal
            AppModelPolicy_AppKnownToStateRepository_Known,                             // Centennial
            AppModelPolicy_AppKnownToStateRepository_Unknown,                           // Desktop
            AppModelPolicy_AppKnownToStateRepository_Known,                             // Silverlight_8_1
            AppModelPolicy_AppKnownToStateRepository_Known,                             // Silverlight_8_0
            AppModelPolicy_AppKnownToStateRepository_Known,                             // Splash
            AppModelPolicy_AppKnownToStateRepository_Unknown,                           // PackagedService
            AppModelPolicy_AppKnownToStateRepository_Unknown,                           // PerAppBroker
        },

        // AudioManagement
        {
            AppModelPolicy_AudioManagement_ManagedByPBM,                                // Universal
            AppModelPolicy_AudioManagement_Unmanaged,                                   // Centennial
            AppModelPolicy_AudioManagement_Unmanaged,                                   // Desktop
            AppModelPolicy_AudioManagement_ManagedByPBM,                                // Silverlight_8_1
            AppModelPolicy_AudioManagement_ManagedByPBM,                                // Silverlight_8_0
            AppModelPolicy_AudioManagement_ManagedByPBM,                                // Splash
            AppModelPolicy_AudioManagement_Unmanaged,                                   // PackagedService
            AppModelPolicy_AudioManagement_Unmanaged,                                   // PerAppBroker
        },

        // PackageMayContainPublicComRegistrations
        {
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // Universal
            AppModelPolicy_PackageMayContainPublicComRegistrations_Yes,                 // Centennial
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // Desktop
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // Silverlight_8_1
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // Silverlight_8_0
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // Splash
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // PackagedService
            AppModelPolicy_PackageMayContainPublicComRegistrations_No,                  // PerAppBroker
        },

        // PackageMayContainPrivateComRegistrations
        {
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // Universal
            AppModelPolicy_PackageMayContainPrivateComRegistrations_PrivateHive,        // Centennial
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // Desktop
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // Silverlight_8_1
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // Silverlight_8_0
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // Splash
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // PackagedService
            AppModelPolicy_PackageMayContainPrivateComRegistrations_None,               // PerAppBroker
        },

        // ComLaunchCreateProcessExtensions
        {
            AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,            // Universal
            AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithDesktopAppX,    // Centennial
            AppModelPolicy_ComLaunchCreateProcessExtensions_None,                       // Desktop
            AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,            // Silverlight_8_1
            AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,            // Silverlight_8_0
            AppModelPolicy_ComLaunchCreateProcessExtensions_RegisterWithPsm,            // Splash
            AppModelPolicy_ComLaunchCreateProcessExtensions_None,                       // PackagedService
            AppModelPolicy_ComLaunchCreateProcessExtensions_None,                       // PerAppBroker
        },

        // ClrCompat
        // DO NOT USE - Intended only for Clr. All other callers should use more appropriate policies
        {
            AppModelPolicy_ClrCompat_Universal,                                         // Universal
            AppModelPolicy_ClrCompat_PackagedDesktop,                                   // Centennial
            AppModelPolicy_ClrCompat_ClassicDesktop,                                    // Desktop
            AppModelPolicy_ClrCompat_Others,                                            // Silverlight_8_1
            AppModelPolicy_ClrCompat_Others,                                            // Silverlight_8_0
            AppModelPolicy_ClrCompat_Others,                                            // Splash
            AppModelPolicy_ClrCompat_Others,                                            // PackagedService
            AppModelPolicy_ClrCompat_ClassicDesktop,                                    // PerAppBroker
        },

        // LoaderIgnoreAlteredSearchForRelativePath
        {
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // Universal
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_True,               // Centennial
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // Desktop
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // Silverlight_8_1
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // Silverlight_8_0
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // Splash
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // PackagedService
            AppModelPolicy_LoaderIgnoreAlteredSearchForRelativePath_False,              // PerAppBroker
        },

        // ImplicitlyActivateClassicAAAServersAsIU
        {
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // Universal
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes,                 // Centennial
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // Desktop
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // Silverlight_8_1
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // Silverlight_8_0
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // Splash
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_No,                  // PackagedService
            AppModelPolicy_ImplicitlyActivateClassicAAAServersAsIU_Yes,                 // PerAppBroker
        },

        // ComClassicCatalog
        {
            AppModelPolicy_ComClassicCatalog_MachineHiveOnly,                           // Universal
            AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,                    // Centennial
            AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,                    // Desktop
            AppModelPolicy_ComClassicCatalog_MachineHiveOnly,                           // Silverlight_8_1
            AppModelPolicy_ComClassicCatalog_MachineHiveOnly,                           // Silverlight_8_0
            AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,                    // Splash
            AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,                    // PackagedService
            AppModelPolicy_ComClassicCatalog_MachineHiveAndUserHive,                    // PerAppBroker
        },

        // ComUnmarshaling
        {
            AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling,                     // Universal
            AppModelPolicy_ComUnmarshaling_ApplicationManaged,                          // Centennial
            AppModelPolicy_ComUnmarshaling_ApplicationManaged,                          // Desktop
            AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling,                     // Silverlight_8_1
            AppModelPolicy_ComUnmarshaling_ForceStrongUnmarshaling,                     // Silverlight_8_0
            AppModelPolicy_ComUnmarshaling_ApplicationManaged,                          // Splash
            AppModelPolicy_ComUnmarshaling_ApplicationManaged,                          // PackagedService
            AppModelPolicy_ComUnmarshaling_ApplicationManaged,                          // PerAppBroker
        },

        // ComAppLaunchPerfEnhancements
        {
            AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled,                        // Universal
            AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,                       // Centennial
            AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,                       // Desktop
            AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled,                        // Silverlight_8_1
            AppModelPolicy_ComAppLaunchPerfEnhancements_Enabled,                        // Silverlight_8_0
            AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,                       // Splash
            AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,                       // PackagedService
            AppModelPolicy_ComAppLaunchPerfEnhancements_Disabled,                       // PerAppBroker
        },

        // ComSecurityInitialization
        {
            AppModelPolicy_ComSecurityInitialization_SystemManaged,                     // Universal
            AppModelPolicy_ComSecurityInitialization_ApplicationManaged,                // Centennial
            AppModelPolicy_ComSecurityInitialization_ApplicationManaged,                // Desktop
            AppModelPolicy_ComSecurityInitialization_SystemManaged,                     // Silverlight_8_1
            AppModelPolicy_ComSecurityInitialization_SystemManaged,                     // Silverlight_8_0
            AppModelPolicy_ComSecurityInitialization_SystemManaged,                     // Splash
            AppModelPolicy_ComSecurityInitialization_ApplicationManaged,                // PackagedService
            AppModelPolicy_ComSecurityInitialization_ApplicationManaged,                // PerAppBroker
        },

        // RoInitializeSingleThreadedBehavior
        {
            AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA,                     // Universal
            AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,                      // Centennial
            AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,                      // Desktop
            AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA,                     // Silverlight_8_1
            AppModelPolicy_RoInitializeSingleThreadedBehavior_ASTA,                     // Silverlight_8_0
            AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,                      // Splash
            AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,                      // PackagedService
            AppModelPolicy_RoInitializeSingleThreadedBehavior_STA,                      // PerAppBroker
        },

        // ComDefaultExceptionHandling
        {
            AppModelPolicy_ComDefaultExceptionHandling_HandleNone,                      // Universal
            AppModelPolicy_ComDefaultExceptionHandling_HandleAll,                       // Centennial
            AppModelPolicy_ComDefaultExceptionHandling_HandleAll,                       // Desktop
            AppModelPolicy_ComDefaultExceptionHandling_HandleNone,                      // Silverlight_8_1
            AppModelPolicy_ComDefaultExceptionHandling_HandleNone,                      // Silverlight_8_0
            AppModelPolicy_ComDefaultExceptionHandling_HandleAll,                       // Splash
            AppModelPolicy_ComDefaultExceptionHandling_HandleAll,                       // PackagedService
            AppModelPolicy_ComDefaultExceptionHandling_HandleAll,                       // PerAppBroker
        },

        // ComOopProxyAgility
        {
            AppModelPolicy_ComOopProxyAgility_Agile,                                    // Universal
            AppModelPolicy_ComOopProxyAgility_NonAgile,                                 // Centennial
            AppModelPolicy_ComOopProxyAgility_NonAgile,                                 // Desktop
            AppModelPolicy_ComOopProxyAgility_Agile,                                    // Silverlight_8_1
            AppModelPolicy_ComOopProxyAgility_Agile,                                    // Silverlight_8_0
            AppModelPolicy_ComOopProxyAgility_NonAgile,                                 // Splash
            AppModelPolicy_ComOopProxyAgility_NonAgile,                                 // PackagedService
            AppModelPolicy_ComOopProxyAgility_NonAgile,                                 // PerAppBroker
        },

        // AppServiceLifetime
        {
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // Universal
            AppModelPolicy_AppServiceLifetime_ExtendedForSamePackage,                   // Centennial
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // Desktop
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // Silverlight_8_1
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // Silverlight_8_0
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // Splash
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // PackagedService
            AppModelPolicy_AppServiceLifetime_StandardTimeout,                          // PerAppBroker
        },

        // WebPlatform
        {
            AppModelPolicy_WebPlatform_Edge,                                            // Universal
            AppModelPolicy_WebPlatform_Legacy,                                          // Centennial
            AppModelPolicy_WebPlatform_Legacy,                                          // Desktop
            AppModelPolicy_WebPlatform_Legacy,                                          // Silverlight_8_1
            AppModelPolicy_WebPlatform_Legacy,                                          // Silverlight_8_0
            AppModelPolicy_WebPlatform_Legacy,                                          // Splash
            AppModelPolicy_WebPlatform_Legacy,                                          // PackagedService
            AppModelPolicy_WebPlatform_Legacy,                                          // PerAppBroker
        },

        // WinInetStoragePartitioning
        {
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // Universal
            AppModelPolicy_WinInetStoragePartitioning_SharedWithAppContainer,      // Centennial
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // Desktop
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // Silverlight_8_1
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // Silverlight_8_0
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // Splash
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // PackagedService
            AppModelPolicy_WinInetStoragePartitioning_Isolated,                    // PerAppBroker
        },

        // IndexerProtocolHandlerHost
        {
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // Universal
            AppModelPolicy_IndexerProtocolHandlerHost_PerApp,                       // Centennial
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // Desktop
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // Silverlight_8_1
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // Silverlight_8_0
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // Splash
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // PackagedService
            AppModelPolicy_IndexerProtocolHandlerHost_PerUser,                      // PerAppBroker
        },

        // LoaderIncludeUserDirectories
        {
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // Universal
            AppModelPolicy_LoaderIncludeUserDirectories_True,                       // Centennial
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // Desktop
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // Silverlight_8_1
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // Silverlight_8_0
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // Splash
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // PackagedService
            AppModelPolicy_LoaderIncludeUserDirectories_False,                      // PerAppBroker
        },

        // ConvertAppContainerToRestrictedAppContainer
        {
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // Universal
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_True,        // Centennial
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // Desktop
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // Silverlight_8_1
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // Silverlight_8_0
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // Splash
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // PackagedService
            AppModelPolicy_ConvertAppContainerToRestrictedAppContainer_False,       // PerAppBroker
        },

        // PackageMayContainPrivateMapiProvider
        {
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // Universal
            AppModelPolicy_PackageMayContainPrivateMapiProvider_PrivateHive,        // Centennial
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // Desktop
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // Silverlight_8_1
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // Silverlight_8_0
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // Splash
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // PackagedService
            AppModelPolicy_PackageMayContainPrivateMapiProvider_None,               // PerAppBroker
        },

        // AdminProcessPackageClaims
        {
            AppModelPolicy_AdminProcessPackageClaims_None,                          // Universal
            AppModelPolicy_AdminProcessPackageClaims_Caller,                        // Centennial
            AppModelPolicy_AdminProcessPackageClaims_None,                          // Desktop
            AppModelPolicy_AdminProcessPackageClaims_None,                          // Silverlight_8_1
            AppModelPolicy_AdminProcessPackageClaims_None,                          // Silverlight_8_0
            AppModelPolicy_AdminProcessPackageClaims_None,                          // Splash
            AppModelPolicy_AdminProcessPackageClaims_None,                          // PackagedService
            AppModelPolicy_AdminProcessPackageClaims_None,                          // PerAppBroker
        },

        // RegistryRedirectionBehavior
        {
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // Universal
            AppModelPolicy_RegistryRedirectionBehavior_CopyOnWrite,                 // Centennial
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // Desktop
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // Silverlight_8_1
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // Silverlight_8_0
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // Splash
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // PackagedService
            AppModelPolicy_RegistryRedirectionBehavior_None,                        // PerAppBroker
        },

        // BypassCreateProcessAppxExtension
        {
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Universal
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Centennial
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Desktop
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Silverlight_8_1
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Silverlight_8_0
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // Splash
            AppModelPolicy_BypassCreateProcessAppxExtension_False,                  // PackagedService
            AppModelPolicy_BypassCreateProcessAppxExtension_True,                   // PerAppBroker
        },

        // KnownFolderRedirection
        {
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // Universal
            AppModelPolicy_KnownFolderRedirection_RedirectToPackage,                // Centennial
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // Desktop
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // Silverlight_8_1
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // Silverlight_8_0
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // Splash
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // PackagedService
            AppModelPolicy_KnownFolderRedirection_Isolated,                         // PerAppBroker
        },

        // PrivateActivateAsPackageWinrtClasses
        {
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNonFullTrust,  // Universal
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowFullTrust,     // Centennial
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // Desktop
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // Silverlight_8_1
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // Silverlight_8_0
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // Splash
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // PackagedService
            AppModelPolicy_PrivateActivateAsPackageWinrtClasses_AllowNone,          // PerAppBroker
        },
        
        // AppPrivateFolderRedirection
        {
            AppModelPolicy_AppPrivateFolderRedirection_AppPrivate,                  // Universal
            AppModelPolicy_AppPrivateFolderRedirection_AppPrivate,                  // Centennial
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // Desktop
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // Silverlight_8_1
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // Silverlight_8_0
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // Splash
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // PackagedService
            AppModelPolicy_AppPrivateFolderRedirection_None,                        // PerAppBroker
        },

        // GlobalSystemAppDataAccess
        {
            AppModelPolicy_GlobalSystemAppDataAccess_Virtualized,                 // Universal
            AppModelPolicy_GlobalSystemAppDataAccess_Virtualized,                 // Centennial
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // Desktop
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // Silverlight_8_1
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // Silverlight_8_0
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // Splash
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // PackagedService
            AppModelPolicy_GlobalSystemAppDataAccess_Normal,                      // PerAppBroker
        },

        // ConsoleHandleInheritance
        {
            AppModelPolicy_ConsoleHandleInheritance_All,                          // Universal
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // Centennial
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // Desktop
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // Silverlight_8_1
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // Silverlight_8_0
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // Splash
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // PackagedService
            AppModelPolicy_ConsoleHandleInheritance_ConsoleOnly,                  // PerAppBroker
        },
        
        // ConsoleBufferAccess
        {
            AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional,          // Universal
            AppModelPolicy_ConsoleBufferAccess_Unrestricted,                      // Centennial
            AppModelPolicy_ConsoleBufferAccess_Unrestricted,                      // Desktop
            AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional,          // Silverlight_8_1
            AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional,          // Silverlight_8_0
            AppModelPolicy_ConsoleBufferAccess_RestrictedUnidirectional,          // Splash
            AppModelPolicy_ConsoleBufferAccess_Unrestricted,                      // PackagedService
            AppModelPolicy_ConsoleBufferAccess_Unrestricted,                      // PerAppBroker
        },
        
        // NOTE: this needs to be in sync with minkernel\appmodel\test\policyapis.cpp
    };

    NTSTATUS status = RtlQueryPackageClaims(token, NULL, NULL, NULL, NULL, NULL, PkgClaim, AttributesPresent);

    // STATUS_NOT_FOUND is expected if neither WIN://SYSAPPID nor WIN://PKG are present
    if (status == STATUS_NOT_FOUND)
    {
        *AttributesPresent = 0;
        PkgClaim->Flags = 0;
        status = STATUS_SUCCESS;
    }

    *policyValue = (AppModelPolicy_PolicyValue)0;
    NT_ASSERT(policyType > (AppModelPolicy_Type)(0) && policyType <= AppModelPolicy_Type_Count);

    // Note: order of below if/else if statements is significant.
    //   Desktop:           !(WIN://SYSAPPID)
    //   Splash:             (WIN://SYSAPPID) && !(WIN://PKG) &&  (WP://SKUID)
    //   Silverlight <= 8.0: (WIN://SYSAPPID) &&  (WIN://PKG) &&  (WP://SKUID)
    //   Centennial:         (WIN://SYSAPPID) &&  (WIN://PKG) && !(WP://SKUID) && (Full Trust)
    //   Packaged Service:   (WIN://SYSAPPID) &&  (WIN://PKG) && !(WP://SKUID) && (Native Service)
    //   Per-app broker:     (WIN://SYSAPPID) &&  (WIN://PKG) && !(WP://SKUID) && (Per-app broker)
    //   Universal:          (WIN://SYSAPPID) &&  (WIN://PKG) && !(WP://SKUID)

    if (NT_SUCCESS(status))
    {
        AppModelPolicy_AppType appType;
        if ((*AttributesPresent & ACCESS_CLAIM_WIN_SYSAPPID_PRESENT) == 0)
        {
            appType = AppModelPolicy_AppType_Desktop;
        }
        else if ((*AttributesPresent & ACCESS_CLAIM_WIN_PKG_PRESENT) == 0)
        {
            NT_ASSERT((*AttributesPresent & ACCESS_CLAIM_WIN_SKUID_PRESENT) != 0);
            appType = AppModelPolicy_AppType_Splash;
        }
        else if ((*AttributesPresent & ACCESS_CLAIM_WIN_SKUID_PRESENT) != 0)
        {
            NT_ASSERT((PkgClaim->Flags & PSM_ACTIVATION_TOKEN_FULL_TRUST) == 0);
            appType = AppModelPolicy_AppType_Silverlight_8_0;
        }
        else if ((PkgClaim->Flags & PSM_ACTIVATION_TOKEN_FULL_TRUST) != 0)
        {
            appType = AppModelPolicy_AppType_Centennial;
        }
        else if ((PkgClaim->Flags & PSM_ACTIVATION_TOKEN_NATIVE_SERVICE) != 0)
        {
            appType = AppModelPolicy_AppType_PackagedService;
        }
        else if ((PkgClaim->Flags & PSM_ACTIVATION_TOKEN_PER_APP_BROKER) != 0)
        {
            appType = AppModelPolicy_AppType_PerAppBroker;
        }        
        else
        {
            // TODO: MSFT 5190375 Currently, UWAs and SL81 Apps are indistinguishable,
            // so SL81 will be marked as Universal
            appType = AppModelPolicy_AppType_Universal;
        }

        *policyValue = AppModelPolicy_PolicyValue_Table[(UINT32)(policyType - 1)][(UINT32)(appType - 1)];
    }

    return HRESULT_FROM_NT(status);
}

__inline HRESULT AppModelPolicy_GetPolicy(
    _In_ HANDLE token,
    _In_ AppModelPolicy_Type policyType,
    _Out_ AppModelPolicy_PolicyValue* policyValue)
{
    ULONG64 AttributesPresent;
    PS_PKG_CLAIM PkgClaim;

    return AppModelPolicy_GetPolicy_Internal(token, policyType, policyValue, &PkgClaim, &AttributesPresent);
}

__inline HRESULT AppModelPolicy_GetProcessPolicy_ImplicitPackageBreakaway(
    _In_ HANDLE token,
    _Out_ AppModelPolicy_PolicyValue* policyValue)
{
    ULONG64 AttributesPresent;
    PS_PKG_CLAIM PkgClaim;

    HRESULT hr = AppModelPolicy_GetPolicy_Internal(token, AppModelPolicy_Type_ImplicitPackageBreakaway_Internal, policyValue, &PkgClaim, &AttributesPresent);
    if (FAILED(hr))
    {
        return hr;
    }

    // A process can only override breakaway behavior; it cannot cause 'stay with' to become breakaway.
    if (*policyValue != AppModelPolicy_ImplicitPackageBreakaway_Allowed)
    {
        return S_OK;
    }

    // This process is allowed to breakaway from the parent's package by the default policy.
    // Now check to see if the process itself has asked to disable breakaway, aka
    // are we in a centennial process (full trust) and is breakaway disabled by the application?
    const ULONG64 flags = PSM_ACTIVATION_TOKEN_FULL_TRUST | PSM_ACTIVATION_TOKEN_DESKTOP_APP_INHIBIT_BREAKAWAY;
    if ((PkgClaim.Flags & flags) == flags)
    {
        *policyValue = AppModelPolicy_ImplicitPackageBreakaway_DeniedByApp;
    }

    return S_OK;
}
