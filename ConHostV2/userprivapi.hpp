/*++
Copyright (c) Microsoft Corporation

Module Name:
- userdpiapi.hpp

Abstract:
- This module is used for abstracting calls to private user32 DLL APIs to break the build system dependency.

Author(s):
- Michael Niksa (MiNiksa) July-2016
--*/
#pragma once


class UserPrivApi sealed
{
public:
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

    

    static NTSTATUS s_ConsoleControl(_In_ UserPrivApi::CONSOLECONTROL ConsoleCommand,
                                     _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
                                     _In_ DWORD ConsoleInformationLength);
   
    static BOOL s_EnterReaderModeHelper(_In_ HWND hwnd);

    #define TM_POSTCHARBREAKS 0x0002
    static BOOL s_TranslateMessageEx(_In_ const MSG *pmsg,
                                     _In_ UINT flags);

#define GWL_CONSOLE_WNDALLOC (3 * sizeof(DWORD))
#define WM_GETDPISCALEDSIZE             0x02E1

#ifdef CON_USERPRIVAPI_INDIRECT
    ~UserPrivApi();
private:
    static UserPrivApi& _Instance();
    HMODULE _hUser32;

    UserPrivApi();
#endif
};