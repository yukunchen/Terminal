/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#include <wchar.h>
#include <assert.h>

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#include <sal.h>

#include "telemetry.hpp"
#include "tracing.hpp"

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>


// WIL
#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>
#include <wil\Result.h>


#include "..\..\inc\conattrs.hpp"
#include "..\..\host\cursor.h"