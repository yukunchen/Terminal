/*++
Copyright (c) Microsoft Corporation

Module Name:
- conwinuserrefs.h

Abstract:
- Contains private definitions from WinUserK.h that we'll need to publish.
--*/

#pragma once

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
