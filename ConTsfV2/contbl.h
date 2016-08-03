/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    contbl.h

Abstract:

    This file defines CConsoleTable class

Author:

Revision History:

Notes:

--*/

#pragma once

const DWORD LayoutNameSize = 256;
const DWORD CandListSize   = 32;

struct KEYBOARD_LAYOUT_INFO
{
    HKL    hKL;
    DWORD  dwProfileType;
    CLSID  clsid;
    LANGID langid;
    GUID   guidProfile;
    DWORD  dwFlags;

    BOOL   fLanguageDefault : 1;
    BOOL   fInComposition;
};



class CConsoleTable
{
public:
    CConsoleTable(KEYBOARD_LAYOUT_INFO* pKLI) : 
      pKeyboardLayoutInfo(pKLI),
      _cRef(1)
    { lpCompStrMem = NULL; }

    ULONG AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    ULONG Release()
    {
        ULONG cr = InterlockedDecrement(&_cRef);
        if (cr == 0)
        {
            delete this;
        }
        return cr;
    }

    virtual ~CConsoleTable()
    {
        if (lpCompStrMem != NULL) {
            delete lpCompStrMem;
        }
    }

public:
    KEYBOARD_LAYOUT_INFO*   pKeyboardLayoutInfo;

    // IMM/IME Composition String Information
    LPCONIME_UICOMPMESSAGE  lpCompStrMem;

    DWORD _cRef;
};
