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
#define NT_SUCCESS(Status)  (((NTSTATUS)(Status)) >= 0)
#define STATUS_SUCCESS ((DWORD)0x0)
#define STATUS_UNSUCCESSFUL ((DWORD)0xC0000001L)
#define STATUS_SHARING_VIOLATION         ((NTSTATUS)0xC0000043L)
#define STATUS_INSUFFICIENT_RESOURCES ((DWORD)0xC000009AL)
#define STATUS_ILLEGAL_FUNCTION ((DWORD)0xC00000AFL)
#define STATUS_PIPE_DISCONNECTED ((DWORD)0xC00000B0L)

//#include <ntstatus.h>

#include <winioctl.h>

// C++ header files
#include <stdexcept>
#include <thread>
#include <vector>
#include <new>
#include <unordered_map>

// private dependencies
#include <Console\conapi.h>
#include <Console\condrv.h>

// in Win32Control.cpp only
//extern "C"
//{
//#include <Win32K\winuserk.h>
//}
