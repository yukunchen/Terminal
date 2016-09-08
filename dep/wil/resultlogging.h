// Windows Internal Libraries (wil)
// ResultLogging.h:  Windows Error Handling Helpers - Fallback telemetry and logging callback for WIL error handling
//
// Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Error%20Handling%20Helpers.docx?web=1
//
// wil Usage Guidelines:
// https://microsoft.sharepoint.com/teams/osg_development/Shared%20Documents/Windows%20Internal%20Libraries%20for%20C++%20Usage%20Guide.docx?web=1
//
// wil Discussion Alias (wildisc):
// http://idwebelements/GroupManagement.aspx?Group=wildisc&Operation=join  (one-click join)

#pragma once

#include "TraceLogging.h"

namespace wil
{
    [uuid(bf4c9654-66d1-5720-7b51-d2ae226735ea)]
    class ErrorHandlingHelpers : public wil::TraceLoggingProvider
    {
    IMPLEMENT_TRACELOGGING_CLASS(ErrorHandlingHelpers, "Microsoft.Windows.ErrorHandling.Fallback", (0xbf4c9654,0x66d1,0x5720,0x7b,0x51,0xd2,0xae,0x22,0x67,0x35,0xea));
    };
} // namespace wil


// Automatically set the ResultTelemetry provider as the fallback error-reporting context by including this file
WI_HEADER_INITITALIZATION_FUNCTION(ResultLoggingInitialize, []
{
    wil::SetResultTelemetryFallback(wil::ErrorHandlingHelpers::FallbackTelemetryCallback);
    return 1;
});
