/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
