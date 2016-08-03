/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    TfConvArea.cpp

Abstract:

    This file implements the CConversionArea Class.

Author:

Revision History:

Notes:

--*/

#include "precomp.h"
#include "ConsoleTSF.h"
#include "TfCtxtComp.h"
#include "TfConvArea.h"

//+---------------------------------------------------------------------------
//
//
// CConversionArea
//
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
// CConversionArea::DrawConversionAreaInfo
//
//----------------------------------------------------------------------------

HRESULT CConversionArea::DrawConversionAreaInfo(CComBSTR CompStr, 
                                                TF_DISPLAYATTRIBUTE* DisplayAttribute, 
                                                ULONG cchDisplayAttribute, 
                                                CComBSTR ResultStr, 
                                                DWORD CompCursorPos)
{
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    DWORD dwSize = sizeof(CONIME_UICOMPMESSAGE) + ((CompStr.Length()+1) * sizeof(WCHAR)) + ((ResultStr.Length()+1) * sizeof(WCHAR)) + (cchDisplayAttribute * sizeof(BYTE));

    lpCompStrMem = (LPCONIME_UICOMPMESSAGE) new BYTE[dwSize];
    if (lpCompStrMem == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lpCompStrMem->dwSize = dwSize;
    lpCompStrMem->CompAttrColor[0] = DEFAULT_COMP_ENTERED;
    lpCompStrMem->CompAttrColor[1] = DEFAULT_COMP_ALREADY_CONVERTED;
    lpCompStrMem->CompAttrColor[2] = DEFAULT_COMP_CONVERSION;
    lpCompStrMem->CompAttrColor[3] = DEFAULT_COMP_YET_CONVERTED;
    lpCompStrMem->CompAttrColor[4] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[5] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[6] = DEFAULT_COMP_INPUT_ERROR;
    lpCompStrMem->CompAttrColor[7] = DEFAULT_COMP_INPUT_ERROR;

    DWORD offset = sizeof(CONIME_UICOMPMESSAGE);

    //
    // Set up composition string
    //
    lpCompStrMem->dwCompStrLen = CompStr.Length() * sizeof(WCHAR);      // byte count.
    if (lpCompStrMem->dwCompStrLen)
    {
        lpCompStrMem->dwCompStrOffset = offset;
        LPWSTR lpCompStr = (LPWSTR)((BYTE*)lpCompStrMem + lpCompStrMem->dwCompStrOffset);
        CopyMemory(lpCompStr, CompStr, lpCompStrMem->dwCompStrLen + sizeof(WCHAR));        // include NULL terminate.
        offset += lpCompStrMem->dwCompStrLen + sizeof(WCHAR);
    }

    //
    // Set up result string
    //
    lpCompStrMem->dwResultStrLen = ResultStr.Length() * sizeof(WCHAR);    // byte count.
    if (lpCompStrMem->dwResultStrLen)
    {
        lpCompStrMem->dwResultStrOffset = offset;
        LPWSTR lpResultStr = (LPWSTR)((BYTE*)lpCompStrMem + lpCompStrMem->dwResultStrOffset);
        CopyMemory(lpResultStr, ResultStr, lpCompStrMem->dwResultStrLen + sizeof(WCHAR));  // include NULL terminate.
        offset += lpCompStrMem->dwResultStrLen + sizeof(WCHAR);
    }

    //
    // Set up composition attribute
    //
    lpCompStrMem->dwCompAttrLen = cchDisplayAttribute;    // byte count
    if (lpCompStrMem->dwCompAttrLen)
    {
        lpCompStrMem->dwCompAttrOffset = offset;
        LPBYTE lpCompAttr = (BYTE*)lpCompStrMem + lpCompStrMem->dwCompAttrOffset;
        for (DWORD i = 0; i < cchDisplayAttribute; i++)
        {
            BYTE bAttr;

            if (DisplayAttribute[i].bAttr == TF_ATTR_OTHER || DisplayAttribute[i].bAttr > TF_ATTR_FIXEDCONVERTED)
            {
                bAttr = ATTR_TARGET_CONVERTED;
            }
            else
            {
                if (DisplayAttribute[i].bAttr == TF_ATTR_INPUT_ERROR)
                {
                    bAttr = ATTR_CONVERTED;
                }
                else
                {
                    bAttr = (BYTE)DisplayAttribute[i].bAttr;
                }
            }

            lpCompAttr[i] = bAttr;
        }

        if (CompCursorPos != -1)
        {
            if (CompCursorPos == 0)
            {
                lpCompAttr[CompCursorPos]   |= (BYTE)0x20;   // special handling for ConSrv... 0x20 = COMMON_LVB_GRID_SINGLEFLAG + COMMON_LVB_GRID_LVERTICAL
            }
            else if (CompCursorPos-1 < cchDisplayAttribute)
            {
                lpCompAttr[CompCursorPos-1] |= (BYTE)0x10;   // special handling for ConSrv... 0x10 = COMMON_LVB_GRID_SINGLEFLAG + COMMON_LVB_GRID_RVERTICAL
            }
        }

        offset += lpCompStrMem->dwCompAttrLen;
    }

    //
    // make composition information packet.
    // Delay packet send to consrv
    // CopyData should be delete on receiver of conime window proc.
    //
    ConsoleImeSendMessage2(CI_CONIMECOMPOSITION, lpCompStrMem->dwSize, lpCompStrMem);

    delete [] lpCompStrMem;

    return S_OK;
}
