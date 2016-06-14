// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

typedef long NTSTATUS;
#define STATUS_SUCCESS ((DWORD)0x0)
#define STATUS_UNSUCCESSFUL ((DWORD)0xC0000001L)
#define STATUS_ILLEGAL_FUNCTION ((DWORD)0xC00000AFL)
//#include <ntstatus.h>

#include <winioctl.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ header files
#include <stdexcept>
#include <thread>


// TODO: reference additional headers your program requires here


// private dependencies
#include <Console\conapi.h>
#include <Console\condrv.h>
