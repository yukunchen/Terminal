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
#include <sal.h>

#include <windows.h>
#include <wincon.h>

#include "..\..\inc\viewport.hpp"

#ifndef _NTSTATUS_DEFINED
#define _NTSTATUS_DEFINED
typedef _Return_type_success_(return >= 0) long NTSTATUS;
#endif

#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)

// WIL
#include <wil\Common.h>
#include <wil\Result.h>
#include <wil\ResultException.h>
