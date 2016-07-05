// Windows Internal Libraries (wil)
// ResultException.h:  Windows Error Handling Helpers - Standard error handling mechanisms for handling and throwing exceptions
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

#include "Result.h"

#ifndef WIL_ENABLE_EXCEPTIONS
#error This file is designed for C++ consumers with exceptions turned on.  To enable C++ exceptions set USE_NATIVE_EH=1 in your sources file.
#endif

// NOTE:
// This file used to hold the exception-specific error handling for WIL.  That
// handling is now in Result.h with the non-exception handling as well.  Include
// 'Result.h', rather than this file.
