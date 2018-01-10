/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#include <windows.h>

#include <sal.h>

#include <cstdio>

#define ENABLE_INTSAFE_SIGNED_FUNCTIONS
#include <intsafe.h>

#include "telemetry.hpp"
#include "tracing.hpp"

// WIL
#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>
#include <wil\Result.h>

#include <string>
#include <list>
#include <memory>
#include <utility>
#include <vector>
#include <memory>
#include <deque>
#include <assert.h>
