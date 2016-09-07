/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    imecomp.cpp

Abstract:

    This file implements the composition window functions

Author:

Revision History:

Notes:

--*/

#include "precomp.h"

//+---------------------------------------------------------------------------
//
// GetCompositionStr
//
// Handle WM_IME_COMPOSITION message with GCS_COMPSTR flag on.
//
//----------------------------------------------------------------------------

void GetCompositionStr(HWND hwnd, CConsoleTable* ConTbl, LPARAM CompFlag, WPARAM CompChar)
{
    DebugMsg(TF_FUNC, "CONIME: GetCompositionStr");

    HIMC        hIMC;                   // Input context handle.
    LONG        lBufLen = 0;                // Storage for len. of composition str
    LONG        lBufLenAttr = 0;
    DWORD       SizeToAlloc;
    PWCHAR      TempBuf;
    PUCHAR      TempBufA;
    DWORD       i;
    DWORD       CursorPos;
    COPYDATASTRUCT CopyData;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    //
    // If fail to get input context handle then do nothing.
    // Applications should call ImmGetContext API to get
    // input context handle.
    //
    hIMC = ImmGetContext(hwnd);
    if (hIMC == 0) {
        return;
    }

    if (CompFlag & GCS_COMPSTR) {
        //
        // Determines how much memory space to store the composition string.
        // Applications should call ImmGetCompositionString with
        // GCS_COMPSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString(hIMC, GCS_COMPSTR, (void FAR*)NULL, 0l);
        if (lBufLen < 0) {
            ImmReleaseContext(hwnd, hIMC);
            return;
        }
        if (CompFlag & GCS_COMPATTR)
        {
            DebugMsg(TF_FUNC, "                           GCS_COMPATTR");
            lBufLenAttr = ImmGetCompositionString(hIMC, GCS_COMPATTR, (void FAR *)NULL, 0l);
            if (lBufLenAttr < 0) {
                lBufLenAttr = 0;
            }
        }
        else {
            lBufLenAttr = 0;
        }
    }
    else if (CompFlag & GCS_RESULTSTR) {
        //
        // Determines how much memory space to store the result string.
        // Applications should call ImmGetCompositionString with
        // GCS_RESULTSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString(hIMC, GCS_RESULTSTR, (void FAR *)NULL, 0l);
        if (lBufLen < 0) {
            ImmReleaseContext(hwnd, hIMC);
            return;
        }
        lBufLenAttr = 0;
    }
    else if (CompFlag == 0) {
        lBufLen = 0;
        lBufLenAttr = 0;
    }

    SizeToAlloc = (UINT)(sizeof(CONIME_UICOMPMESSAGE) +
                         lBufLen + sizeof(WCHAR) +
                         lBufLenAttr + sizeof(BYTE));

    if (ConTbl->lpCompStrMem != NULL && SizeToAlloc > ConTbl->lpCompStrMem->dwSize) {
        delete[] ConTbl->lpCompStrMem;
        ConTbl->lpCompStrMem = NULL;
    }

    if (ConTbl->lpCompStrMem == NULL) {
        ConTbl->lpCompStrMem = (LPCONIME_UICOMPMESSAGE) new BYTE[SizeToAlloc];
        if (ConTbl->lpCompStrMem == NULL) {
            ImmReleaseContext(hwnd, hIMC);
            return;
        }
        ConTbl->lpCompStrMem->dwSize = SizeToAlloc;
    }

    lpCompStrMem = ConTbl->lpCompStrMem;
    RtlZeroMemory(&lpCompStrMem->dwCompAttrLen,
                  lpCompStrMem->dwSize - sizeof(lpCompStrMem->dwSize)
    );

    TempBuf = (PWCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE));
    TempBufA = (PUCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE) +
                        lBufLen + sizeof(WCHAR));

    lpCompStrMem->CompAttrColor[0] = DEFAULT_COMP_ENTERED;
    lpCompStrMem->CompAttrColor[1] = DEFAULT_COMP_ALREADY_CONVERTED;
    lpCompStrMem->CompAttrColor[2] = DEFAULT_COMP_CONVERSION;
    lpCompStrMem->CompAttrColor[3] = DEFAULT_COMP_YET_CONVERTED;
    lpCompStrMem->CompAttrColor[4] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[5] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[6] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[7] = DEFAULT_COMP_INPUT_ERROR;

    CopyData.dwData = CI_CONIMECOMPOSITION;
    CopyData.cbData = lpCompStrMem->dwSize;
    CopyData.lpData = lpCompStrMem;

    if (CompFlag & CS_INSERTCHAR) {
        *TempBuf = (WORD)CompChar;
        TempBuf[lBufLen / sizeof(WCHAR)] = TEXT('\0');
        *TempBufA = (BYTE)ATTR_TARGET_CONVERTED;
        TempBufA[lBufLenAttr] = (BYTE)0;
    }
    else if (CompFlag & GCS_COMPSTR) {
        //
        // Reads in the composition string.
        //
        ImmGetCompositionString(hIMC, GCS_COMPSTR, TempBuf, lBufLen);

        //
        // Null terminated.
        //
        TempBuf[lBufLen / sizeof(WCHAR)] = TEXT('\0');

        //
        // If GCS_COMPATTR flag is on, then we need to take care of it.
        //
        if (lBufLenAttr != 0)
        {
            ImmGetCompositionString(hIMC, GCS_COMPATTR, TempBufA, lBufLenAttr);
            TempBufA[lBufLenAttr] = (BYTE)0;
        }

        CursorPos = ImmGetCompositionString(hIMC, GCS_CURSORPOS, NULL, 0);
        if (CursorPos == 0) {
            TempBufA[CursorPos] |= (BYTE)0x20;     // special handling for ConSrv... 0x20 = COMMON_LVB_GRID_SINGLEFLAG + COMMON_LVB_GRID_LVERTICAL
        }
        else {
            TempBufA[CursorPos - 1] |= (BYTE)0x10;     // special handling for ConSrv... 0x10 = COMMON_LVB_GRID_SINGLEFLAG + COMMON_LVB_GRID_RVERTICAL
        }

        lpCompStrMem->dwCompStrLen = lBufLen;
        if (lpCompStrMem->dwCompStrLen) {
            lpCompStrMem->dwCompStrOffset = sizeof(CONIME_UICOMPMESSAGE);
        }

        lpCompStrMem->dwCompAttrLen = lBufLenAttr;
        if (lpCompStrMem->dwCompAttrLen) {
            lpCompStrMem->dwCompAttrOffset = sizeof(CONIME_UICOMPMESSAGE) + lBufLen + sizeof(WCHAR);
        }
    }
    else if (CompFlag & GCS_RESULTSTR) {
        //
        // Reads in the result string.
        //
        ImmGetCompositionString(hIMC, GCS_RESULTSTR, TempBuf, lBufLen);

        //
        // Null terminated.
        //
        TempBuf[lBufLen / sizeof(WCHAR)] = TEXT('\0');

        lpCompStrMem->dwResultStrLen = lBufLen;
        if (lpCompStrMem->dwResultStrLen) {
            lpCompStrMem->dwResultStrOffset = sizeof(CONIME_UICOMPMESSAGE);
        }
    }
    else if (CompFlag == 0) {
        TempBuf[lBufLen / sizeof(WCHAR)] = TEXT('\0');
        TempBufA[lBufLenAttr] = (BYTE)0;
        lpCompStrMem->dwResultStrLen = lBufLen;
        lpCompStrMem->dwCompStrLen = lBufLen;
        lpCompStrMem->dwCompAttrLen = lBufLenAttr;
    }

    //
    // send character to Console
    //
    ConsoleImeSendMessage(&CopyData);

    ImmReleaseContext(hwnd, hIMC);
}
