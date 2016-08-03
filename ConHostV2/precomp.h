/*++
Copyright (c) Microsoft Corporation

Module Name:
- precomp.h

Abstract:
- Contains external headers to include in the precompile phase of console build process.
- Avoid including internal project headers. Instead include them only in the classes that need them (helps with test project building).
--*/

#pragma once

#define DEFINE_CONSOLEV2_PROPERTIES

// Define and then undefine WIN32_NO_STATUS because windows.h has no guard to prevent it from double defing certain statuses
// when included with ntstatus.h
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

// From ntdef.h, but that can't be included or it'll fight over PROBE_ALIGNMENT and other such arch specific defs
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
/*lint -save -e624 */  // Don't complain about different typedefs.
typedef NTSTATUS *PNTSTATUS;
/*lint -restore */  // Resume checking for different typedefs.
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// Determine if an argument is present by testing the value of the pointer
// to the argument value.

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)((ULONG_PTR)(ArgumentPointer)) != (CHAR *)(NULL) )

// End From ntdef.h

#include <ntstatus.h>

#include <assert.h>

#define SCREEN_BUFFER_POINTER(X,Y,XSIZE,CELLSIZE) (((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)
#include <initguid.h>
#include <shlobj.h>
#include <winuser.h>
#include <shellapi.h>

#include <securityappcontainer.h>

#include "..\PrivateDependencies\Console\winconp.h"

#pragma region wdm.h (public DDK)
//
// Define the base asynchronous I/O argument types
//
extern "C"
{
#include "..\PrivateDependencies\DDK\wdm.h"
#pragma endregion

#pragma region IOCTL codes
    //
    // Define the various device type values.  Note that values used by Microsoft
    // Corporation are in the range 0-32767, and 32768-65535 are reserved for use
    // by customers.
    //

#undef DEVICE_TYPE
#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
#define FILE_DEVICE_DFS_VOLUME          0x00000036
#define FILE_DEVICE_SERENUM             0x00000037
#define FILE_DEVICE_TERMSRV             0x00000038
#define FILE_DEVICE_KSEC                0x00000039
#define FILE_DEVICE_FIPS                0x0000003A
#define FILE_DEVICE_INFINIBAND          0x0000003B
#define FILE_DEVICE_VMBUS               0x0000003E
#define FILE_DEVICE_CRYPT_PROVIDER      0x0000003F
#define FILE_DEVICE_WPD                 0x00000040
#define FILE_DEVICE_BLUETOOTH           0x00000041
#define FILE_DEVICE_MT_COMPOSITE        0x00000042
#define FILE_DEVICE_MT_TRANSPORT        0x00000043
#define FILE_DEVICE_BIOMETRIC           0x00000044
#define FILE_DEVICE_PMI                 0x00000045
#define FILE_DEVICE_EHSTOR              0x00000046
#define FILE_DEVICE_DEVAPI              0x00000047
#define FILE_DEVICE_GPIO                0x00000048
#define FILE_DEVICE_USBEX               0x00000049
#define FILE_DEVICE_CONSOLE             0x00000050
#define FILE_DEVICE_NFP                 0x00000051
#define FILE_DEVICE_SYSENV              0x00000052
#define FILE_DEVICE_VIRTUAL_BLOCK       0x00000053
#define FILE_DEVICE_POINT_OF_SERVICE    0x00000054
#define FILE_DEVICE_STORAGE_REPLICATION 0x00000055
#define FILE_DEVICE_TRUST_ENV           0x00000056
#define FILE_DEVICE_UCM                 0x00000057
#define FILE_DEVICE_UCMTCPCI            0x00000058

    //
    // Macro definition for defining IOCTL and FSCTL function control codes.  Note
    // that function codes 0-2047 are reserved for Microsoft Corporation, and
    // 2048-4095 are reserved for customers.
    //

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

    //
    // Macro to extract device type out of the device io control code
    //
#undef DEVICE_TYPE_FROM_CTL_CODE
#define DEVICE_TYPE_FROM_CTL_CODE(ctrlCode)     (((ULONG)(ctrlCode & 0xffff0000)) >> 16)

    //
    // Macro to extract buffering method out of the device io control code
    //
#undef METHOD_FROM_CTL_CODE
#define METHOD_FROM_CTL_CODE(ctrlCode)          ((ULONG)(ctrlCode & 3))

    //
    // Define the method codes for how buffers are passed for I/O and FS controls
    //

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

    //
    // Define some easier to comprehend aliases:
    //   METHOD_DIRECT_TO_HARDWARE (writes, aka METHOD_IN_DIRECT)
    //   METHOD_DIRECT_FROM_HARDWARE (reads, aka METHOD_OUT_DIRECT)
    //

#define METHOD_DIRECT_TO_HARDWARE       METHOD_IN_DIRECT
#define METHOD_DIRECT_FROM_HARDWARE     METHOD_OUT_DIRECT

    //
    // Define the access check value for any access
    //
    //
    // The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
    // ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
    // constants *MUST* always be in sync.
    //
    //
    // FILE_SPECIAL_ACCESS is checked by the NT I/O system the same as FILE_ANY_ACCESS.
    // The file systems, however, may add additional access checks for I/O and FS controls
    // that use this value.
    //


#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe
#pragma endregion

};
#pragma endregion

#pragma region winternl.h (public no link, DDK)
// Modified from NtDeviceIoControlFile for now.
FORCEINLINE
NTSTATUS WINAPI IoControlFile(
    _In_ HANDLE FileHandle,
    _In_ ULONG IoControlCode,
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DWORD BytesReturned;

    if (FALSE == DeviceIoControl(FileHandle,
                                 IoControlCode,
                                 InputBuffer,
                                 InputBufferLength,
                                 OutputBuffer,
                                 OutputBufferLength,
                                 &BytesReturned,
                                 nullptr))
    {
        Status = NTSTATUS_FROM_WIN32(GetLastError());
    }

    return Status;
}
#pragma endregion

#pragma region ntifs.h (public DDK)

extern "C"
{
    #define RtlOffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))  
};

#pragma endregion

#pragma region WinUserK.h (private internal)

extern "C"
{
    /* WinUserK */
    /*
    * Console window startup optimization.
    */
    typedef struct tagCONSRV_CONNECTION_INFO {
        HANDLE ConsoleHandle;
        HANDLE ServerProcessHandle;
        BOOLEAN ConsoleApp;
        BOOLEAN AttachConsole;
        BOOLEAN AllocConsole;
        BOOLEAN FreeConsole;
        ULONG ProcessGroupId;
        WCHAR DesktopName[MAX_PATH + 1];
    } CONSRV_CONNECTION_INFO, *PCONSRV_CONNECTION_INFO;

    typedef enum _CONSOLECONTROL {
        ConsoleSetVDMCursorBounds,
        ConsoleNotifyConsoleApplication,
        ConsoleFullscreenSwitch,
        ConsoleSetCaretInfo,
        ConsoleSetReserveKeys,
        ConsoleSetForeground,
        ConsoleSetWindowOwner,
        ConsoleEndTask,
    } CONSOLECONTROL;


    //
    // CtrlFlags definitions
    //
#define CONSOLE_CTRL_C_FLAG                     0x00000001
#define CONSOLE_CTRL_BREAK_FLAG                 0x00000002
#define CONSOLE_CTRL_CLOSE_FLAG                 0x00000004
#define CONSOLE_FORCE_SHUTDOWN_FLAG             0x00000008
#define CONSOLE_CTRL_LOGOFF_FLAG                0x00000010
#define CONSOLE_CTRL_SHUTDOWN_FLAG              0x00000020
#define CONSOLE_RESTART_APP_FLAG                0x00000040
#define CONSOLE_QUICK_RESOLVE_FLAG              0x00000080

    typedef struct _CONSOLEENDTASK {
        HANDLE ProcessId;
        HWND hwnd;
        ULONG ConsoleEventCode;
        ULONG ConsoleFlags;
    } CONSOLEENDTASK, *PCONSOLEENDTASK;

    typedef struct _CONSOLEWINDOWOWNER {
        HWND hwnd;
        ULONG ProcessId;
        ULONG ThreadId;
    } CONSOLEWINDOWOWNER, *PCONSOLEWINDOWOWNER;

    typedef struct _CONSOLESETWINDOWOWNER {
        HWND hwnd;
        DWORD dwProcessId;
        DWORD dwThreadId;
    } CONSOLESETWINDOWOWNER, *PCONSOLESETWINDOWOWNER;

    typedef struct _CONSOLESETFOREGROUND {
        HANDLE hProcess;
        BOOL bForeground;
    } CONSOLESETFOREGROUND, *PCONSOLESETFOREGROUND;

    typedef struct _CONSOLERESERVEKEYS {
        HWND hwnd;
        DWORD fsReserveKeys;
    } CONSOLERESERVEKEYS, *PCONSOLERESERVEKEYS;

    typedef struct _CONSOLE_FULLSCREEN_SWITCH {
        IN BOOL      bFullscreenSwitch;
        IN HWND      hwnd;
        IN PDEVMODEW pNewMode;
    } CONSOLE_FULLSCREEN_SWITCH, *PCONSOLE_FULLSCREEN_SWITCH;

    /*
    * Console window startup optimization.
    */
#define CPI_NEWPROCESSWINDOW    0x0001

    typedef struct _CONSOLE_PROCESS_INFO {
        IN DWORD    dwProcessID;
        IN DWORD    dwFlags;
    } CONSOLE_PROCESS_INFO, *PCONSOLE_PROCESS_INFO;

    typedef struct _CONSOLE_CARET_INFO {
        IN HWND hwnd;
        IN RECT rc;
    } CONSOLE_CARET_INFO, *PCONSOLE_CARET_INFO;

    NTSTATUS ConsoleControl(
        __in CONSOLECONTROL Command,
        __in_bcount_opt(ConsoleInformationLength) PVOID ConsoleInformation,
        __in DWORD ConsoleInformationLength);

    /* END WinUserK */
};
#pragma endregion

#include "..\PrivateDependencies\Console\conmsgl1.h"
#include "..\PrivateDependencies\Console\conmsgl2.h"
#include "..\PrivateDependencies\Console\conmsg.h"

#include <winuser.h>
#include <wincon.h>
#include "..\PrivateDependencies\Console\winconp.h"
#include "..\PrivateDependencies\Console\ntcon.h"
#include <windowsx.h>
#include <dde.h>
#include "consrv.h"

#include "conv.h"
#include <imm.h>

// STL
#include <string>
#include <list>

// WIL
// We'll probably want to use this, but there's a conflict in utils.cpp right now.
//#define WIL_SUPPORT_BITOPERATION_PASCAL_NAMES
#include <wil\Common.h>
#include <wil\Result.h>
#include <wil\ResultException.h>

#pragma prefast(push)
#pragma prefast(disable:26071, "Range violation in Intsafe. Not ours.")
#define ENABLE_INTSAFE_SIGNED_FUNCTIONS // Only unsigned intsafe math/casts available without this def
#include <intsafe.h>
#pragma prefast(pop)
#include <strsafe.h>
#include <wchar.h>
#include <mmsystem.h>
#include "utils.hpp"

// Including TraceLogging essentials for the binary
#include <TraceLoggingProvider.h>
#include <winmeta.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);
#include <telemetry\microsofttelemetry.h>
#include <TraceLoggingActivity.h>
#include "telemetry.hpp"
#include "tracing.hpp"

#ifdef BUILDING_INSIDE_WINIDE
#define DbgRaiseAssertionFailure() __int2c()
#endif

extern "C"
{
    BOOLEAN IsPlaySoundPresent();
};

#include <ShellScalingApi.h>
#include "..\ConProps\conpropsp.hpp"

BOOL IsConsoleFullWidth(_In_ HDC hDC, _In_ DWORD CodePage, _In_ WCHAR wch);

// uncomment to build against all public SDK
 #define CON_BUILD_PUBLIC

#ifdef CON_BUILD_PUBLIC
    #define CON_USERPRIVAPI_INDIRECT
    #define CON_DPIAPI_INDIRECT
#endif
