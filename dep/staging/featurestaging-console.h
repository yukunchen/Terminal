// Windows Internal Libraries (wil)
//
// wil Feature Definitions.
// Automatically generated from: FeatureStaging-console.xml
// Do not make manual changes to this header
//
// wil Discussion alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)
//
// For more information on wil Staging, see:
// https://osgwiki.com/wiki/Feature_Withholding

#ifndef __features_FeatureStaging_console
#define __features_FeatureStaging_console

#ifndef FEATURE_STAGING_LEGACY_MODE
#include <wil/Staging.h>
#include <wil/StagingEvents.h>
#endif

#if (defined(__cplusplus) && !defined(__WIL_MIN_KERNEL) && !defined(FEATURE_STAGING_LEGACY_MODE))

#pragma detect_mismatch("ODR_violation_Feature_DxRenderer_mismatch", "EnabledByDefault--")
#pragma detect_mismatch("ODR_violation_Feature_TerminalPropSheet_mismatch", "EnabledByDefault--")

// Console
// Features related to the console subsystem or console host window

WI_DEFINE_FEATURE(
    Feature_DxRenderer, 12745851, EnabledByDefault,
    "Console DirectX Renderer",
    "Uses DirectX/DirectWrite for drawing to the main console host window instead of GDI",
    WilStagingEmail("conhost@microsoft.com"),
    WilStagingGroup("Console", "Features related to the console subsystem or console host window"));

WI_DEFINE_FEATURE(
    Feature_TerminalPropSheet, 18451277, EnabledByDefault,
    "Terminal Property Sheet",
    "This adds a new property sheet tab for configuring terminal settings.",
    WilStagingEmail("conhost@microsoft.com"),
    WilStagingGroup("Console", "Features related to the console subsystem or console host window"));

#endif

#ifdef FEATURE_STAGING_LEGACY_MODE
// Do not use:  These are included for compatability with consumers of the legacy holdback system.
#define FEATURE_STAGING_DISABLED_Feature_DxRenderer 0
#define FEATURE_STAGING_DISABLED_Feature_TerminalPropSheet 0
#ifndef FEATURE_STAGING_IS_FEATURE_ENABLED
#define FEATURE_STAGING_IS_FEATURE_ENABLED(f) (FEATURE_STAGING_DISABLED_##f == 0)
#endif
#endif

// Defines used to power the WI_IS_FEATURE_PRESENT(FeatureName) macro for (rare) #ifdef work based on features
#define __INTERNAL_FEATURE_PRESENT_Feature_DxRenderer 1
#define __INTERNAL_FEATURE_PRESENT_Feature_TerminalPropSheet 1

#if ((!defined(__cplusplus) || defined(WIL_ENABLE_C_FEATURES)) && !defined(FEATURE_STAGING_LEGACY_MODE))

inline BOOL NTAPI Feature_DxRenderer__private_areDependenciesEnabled() { return (TRUE); }
WI_C_DEFINE_FEATURE(Feature_DxRenderer, 12745851, EnabledByDefault, 0, 0, OnRead, Default);

inline BOOL NTAPI Feature_TerminalPropSheet__private_areDependenciesEnabled() { return (TRUE); }
WI_C_DEFINE_FEATURE(Feature_TerminalPropSheet, 18451277, EnabledByDefault, 0, 0, OnRead, Default);

#endif

#endif  // include guard
