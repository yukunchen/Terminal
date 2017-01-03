/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    conime.h

Abstract:

    This module contains the internal structures and definitions used
    by the console IME.

Author:

    v-HirShi Jul.4.1995

Revision History:

--*/

#pragma once

#define CONIME_ATTRCOLOR_SIZE 8

typedef __struct_bcount(dwSize) struct _CONIME_UICOMPMESSAGE {
    DWORD dwSize;
    DWORD dwCompAttrLen;
    DWORD dwCompAttrOffset;
    DWORD dwCompStrLen;
    DWORD dwCompStrOffset;
    DWORD dwResultStrLen;
    DWORD dwResultStrOffset;
    WORD  CompAttrColor[CONIME_ATTRCOLOR_SIZE];
} CONIME_UICOMPMESSAGE, *LPCONIME_UICOMPMESSAGE;

//
// This is PCOPYDATASTRUCT->dwData values for WM_COPYDAT message consrv from conime.
//
#define CI_CONIMECOMPOSITION    0x4B425930
//unused CI_CONIMEMODEINFO       0x4B425931
//unused CI_CONIMESYSINFO        0x4B425932
#define CI_ONSTARTCOMPOSITION   0x4B425933
#define CI_ONENDCOMPOSITION     0x4B425934
//unused CI_CONIMECANDINFO       0x4B425935
//unused CI_CONIMESTATEUPDATED   0x4B425936

//
// This message values for send/post message conime from consrv
//
//unused CONIME_CREATE                   (WM_USER+0)
//unused CONIME_DESTROY                  (WM_USER+1)
//unused CONIME_SETFOCUS                 (WM_USER+2)
//unused CONIME_KILLFOCUS                (WM_USER+3)
//unused CONIME_HOTKEY                   (WM_USER+4)
//unused CONIME_GET_NLSMODE              (WM_USER+5)
//unused CONIME_SET_NLSMODE              (WM_USER+6)
//unused CONIME_NOTIFY_SCREENBUFFERSIZE  (WM_USER+7)
//unused CONIME_NOTIFY_VK_KANA           (WM_USER+8)
//unused CONIME_INPUTLANGCHANGE          (WM_USER+9)
//unused CONIME_NOTIFY_CODEPAGE          (WM_USER+10)
//unused CONIME_INPUTLANGCHANGEREQUEST   (WM_USER+11)
//unused CONIME_INPUTLANGCHANGEREQUESTFORWARD   (WM_USER+12)
//unused CONIME_INPUTLANGCHANGEREQUESTBACKWARD   (WM_USER+13)
//unused CONIME_KEYDATA                  (WM_USER+1024)
//unused CONIME_POSTKEYDATA              (WM_USER+2048)

//
// This message value is for send/post message to consrv
//
//unused CM_CONIME_KL_ACTIVATE           (WM_USER+15)

//
// Default composition color attributes
//
#define DEFAULT_COMP_ENTERED            \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_ALREADY_CONVERTED  \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     BACKGROUND_BLUE )
#define DEFAULT_COMP_CONVERSION         \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_YET_CONVERTED      \
    (FOREGROUND_BLUE |                                                            \
     BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
#define DEFAULT_COMP_INPUT_ERROR        \
    (                                     FOREGROUND_RED |                        \
     COMMON_LVB_UNDERSCORE)
