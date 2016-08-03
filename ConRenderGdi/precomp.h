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


#ifndef _NTSTATUS_DEFINED
#define _NTSTATUS_DEFINED
typedef _Return_type_success_(return >= 0) long NTSTATUS;
#endif

#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)

//#include <ntstatus.h>
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)    // ntsubauth
#define FACILITY_NTWIN32                 0x7
#define NTSTATUS_FROM_WIN32(x) ((NTSTATUS)(x) <= 0 ? ((NTSTATUS)(x)) : ((NTSTATUS) (((x) & 0x0000FFFF) | (FACILITY_NTWIN32 << 16) | ERROR_SEVERITY_ERROR)))

#define NT_TESTNULL(var) (((var) == nullptr) ? STATUS_NO_MEMORY : STATUS_SUCCESS)
#define NT_TESTNULL_GLE(var) (((var) == nullptr) ? NTSTATUS_FROM_WIN32(GetLastError()) : STATUS_SUCCESS);
