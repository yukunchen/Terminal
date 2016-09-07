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

// Used by TranslateMessageEx to purposefully return false to certain WM_KEYDOWN/WM_CHAR messages
#define TM_POSTCHARBREAKS 0x0002

// Used by window structures to place our special frozen-console painting data
#define GWL_CONSOLE_WNDALLOC (3 * sizeof(DWORD))

// Used for pre-resize querying of the new scaled size of a window when the DPI is about to change.
#define WM_GETDPISCALEDSIZE             0x02E1

class UserPrivApi sealed
{
public:
    enum CONSOLECONTROL {
        ConsoleSetVDMCursorBounds,
        ConsoleNotifyConsoleApplication,
        ConsoleFullscreenSwitch,
        ConsoleSetCaretInfo,
        ConsoleSetReserveKeys,
        ConsoleSetForeground,
        ConsoleSetWindowOwner,
        ConsoleEndTask,
    };

    static NTSTATUS s_ConsoleControl(_In_ UserPrivApi::CONSOLECONTROL ConsoleCommand,
                                     _In_reads_bytes_(ConsoleInformationLength) PVOID ConsoleInformation,
                                     _In_ DWORD ConsoleInformationLength);
   
    static BOOL s_EnterReaderModeHelper(_In_ HWND hwnd);

    static BOOL s_TranslateMessageEx(_In_ const MSG *pmsg,
                                     _In_ UINT flags);

#ifdef CON_USERPRIVAPI_INDIRECT
    ~UserPrivApi();
private:
    static UserPrivApi& _Instance();
    HMODULE _hUser32;

    UserPrivApi();
#endif
};