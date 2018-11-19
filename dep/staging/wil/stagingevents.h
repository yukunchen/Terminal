// Windows Internal Libraries (wil)
// StagingEvents.h:  Automatic ETW event provider for feature usage and information
//
// Usage Guidelines:
// https://osgwiki.com/wiki/Windows_Internal_Libraries_(wil)
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)

#pragma once

#include <telemetry\MicrosoftTelemetry.h>
#include "TraceLogging.h"
#include "Staging.h"

namespace wil
{
    namespace details
    {
        class __declspec(uuid("dcef5411-1f98-5ee7-238b-5abd0e078e97")) 
        FeatureLogging : public wil::TraceLoggingProvider
        {
            IMPLEMENT_TRACELOGGING_CLASS_WITHOUT_TELEMETRY(FeatureLogging, "Microsoft.Windows.Wil.FeatureLogging", (0xdcef5411, 0x1f98, 0x5ee7, 0x23, 0x8b, 0x5a, 0xbd, 0x0e, 0x07, 0x8e, 0x97));

        public:
            static const UINT64 Keyword_Error = 0x00000001;
            static const UINT64 Keyword_Usage = 0x00000002;
            static const UINT64 Keyword_VariantUsage = 0x00000004;
            static const UINT64 Keyword_EnablementOverridden = 0x00000008;
        };

        inline void __stdcall FeatureLoggingHook(unsigned int featureId, const FEATURE_LOGGED_TRAITS* traits, const FEATURE_ERROR* error, BOOL enabled, const wil_ReportingKind* kind,
            const wil_VariantReportingKind* variantKind, unsigned char variant, SIZE_T addend)
        {
            if (FeatureLogging::IsEnabled())
            {
                if (kind)
                {
                    TraceLoggingProviderWrite(FeatureLogging, "FeatureUsage", TraceLoggingDescription("Feature usage"), TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingKeyword(FeatureLogging::Keyword_Usage),
                        // Feature information
                        TraceLoggingValue(featureId, "featureId", "Feature Id associated with this error"),
                        TraceLoggingInt32(traits ? traits->version : -1, "featureVersion", "Feature version"),
                        TraceLoggingInt32(traits ? traits->baseVersion : -1, "featureBaseVersion", "Feature base version"),
                        TraceLoggingInt16(traits ? traits->stage : -1, "featureStage", "Feature stage"),
                        // Usage information
                        TraceLoggingValue(!!enabled, "enabled", "Feature enabled state"),
                        TraceLoggingValue(static_cast<unsigned int>(*kind), "kind", "Usage kind"),
                        TraceLoggingValue(addend, "addend", "Usage count"));

                    if (traits->stage == static_cast<UINT8>(FeatureStage::DisabledByDefault) && enabled)
                    {
                        // This feature has been explicitly enabled on this device, so log a separate event (that is much less verbose and has
                        // Keyword_EnablementOverridden). This allows for efficient subscriptions to usage for features that are specifically being
                        // tested/rolled out to an audience.

                        TraceLoggingProviderWrite(FeatureLogging, "EnabledFeatureUsage", TraceLoggingDescription("Feature usage for a feature that has been explicitly enabled"), TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingKeyword(FeatureLogging::Keyword_Usage | FeatureLogging::Keyword_EnablementOverridden),
                            // Feature information
                            TraceLoggingValue(featureId, "featureId", "Feature Id"),
                            TraceLoggingInt32(traits ? traits->version : -1, "featureVersion", "Feature version"),
                            TraceLoggingInt32(traits ? traits->baseVersion : -1, "featureBaseVersion", "Feature base version"),
                            TraceLoggingInt16(traits ? traits->stage : -1, "featureStage", "Feature stage"),
                            // Usage information
                            TraceLoggingValue(!!enabled, "enabled", "Feature enabled state"),
                            TraceLoggingValue(static_cast<unsigned int>(*kind), "kind", "Usage kind"),
                            TraceLoggingValue(addend, "addend", "Usage count"));
                    }
                }
                else if (variantKind)
                {
                    TraceLoggingProviderWrite(FeatureLogging, "FeatureVariantUsage", TraceLoggingDescription("Feature variant usage"), TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingKeyword(FeatureLogging::Keyword_VariantUsage),
                        // Feature information
                        TraceLoggingValue(featureId, "featureId", "Feature Id associated with this error"),
                        TraceLoggingInt32(traits ? traits->version : -1, "featureVersion", "Feature version"),
                        TraceLoggingInt32(traits ? traits->baseVersion : -1, "featureBaseVersion", "Feature base version"),
                        TraceLoggingInt16(traits ? traits->stage : -1, "featureStage", "Feature stage"),
                        // Usage information
                        TraceLoggingValue(!!enabled, "enabled", "Feature enabled state"),
                        TraceLoggingValue(static_cast<unsigned int>(*variantKind), "variantKind", "Usage kind"),
                        TraceLoggingValue(variant, "variant", "Variant value"),
                        TraceLoggingValue(addend, "addend", "Usage count"));
                }
                else if (error)
                {
                    TraceLoggingProviderWrite(FeatureLogging, "FeatureError", TraceLoggingDescription("Feature error"), TraceLoggingLevel(WINEVENT_LEVEL_ERROR), TraceLoggingKeyword(FeatureLogging::Keyword_Error),
                        // Feature information
                        TraceLoggingValue(featureId, "featureId", "Feature Id associated with this error"),
                        TraceLoggingInt32(traits ? traits->version : -1, "featureVersion", "Feature version"),
                        TraceLoggingInt32(traits ? traits->baseVersion : -1, "featureBaseVersion", "Feature base version"),
                        TraceLoggingInt16(traits ? traits->stage : -1, "featureStage", "Feature stage"),
                        // Actual failure location
                        TraceLoggingHResult(error->hr, "hr", "Error code"),
                        TraceLoggingValue(error->file, "file", "Source file name"),
                        TraceLoggingValue(error->lineNumber, "lineNumber", "Source file line number"),
                        TraceLoggingValue(error->modulePath, "module", "Module where the error occurred"),
                        TraceLoggingValue(error->process, "process", "Process name where the error occurred"),
                        // Origin context where failure was seen
                        TraceLoggingValue(error->originFile, "originFile", "Source file name where the error was observed"),
                        TraceLoggingValue(error->originLineNumber, "originLineNumber", "Source file line number where the error was observed"),
                        TraceLoggingValue(error->originModule, "originModule", "Module name where the error was observed"),
                        // Interesting data that's rarely populated (useful when present)
                        TraceLoggingValue(error->originName, "originName", "Optional origin name"),
                        TraceLoggingValue(error->message, "message", "Message associated with this error"),
                        // Detailed caller information (rarely interesting)
                        TraceLoggingValue(error->callerModule, "callerModule", "Module name of the caller"),
                        TraceLoggingValue(error->callerReturnAddressOffset, "callerReturnAddressOffset", "Return address offset of the caller"),
                        TraceLoggingValue(error->originCallerModule, "originCallerModule", "Module name of the caller where the error was observed"),
                        TraceLoggingValue(error->originCallerReturnAddressOffset, "originCallerReturnAddressOffset", "Return address offset of the caller where the error was observed"));
                }
            }
        }
    }
} // namespace wil

// Automatically set the ResultTelemetry provider as the fallback error-reporting context by including this file
WI_HEADER_INITITALIZATION_FUNCTION(StagingEventsInitialize, []
{
    g_wil_details_pfnFeatureLoggingHook = ::wil::details::FeatureLoggingHook;
    return 1;
});
