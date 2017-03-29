/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    TfConvArea.h

Abstract:

    This file defines the CConversionAreaJapanese Interface Class.

Author:

Revision History:

Notes:

--*/

#pragma once

//+---------------------------------------------------------------------------
//
// CConversionArea::Pure virtual class
//
//----------------------------------------------------------------------------

class CConversionArea
{
public:
    CConversionArea() { }
    virtual ~CConversionArea() { }

    virtual HRESULT DrawConversionAreaInfo(CComBSTR CompStr, TF_DISPLAYATTRIBUTE* DisplayAttribute, ULONG cchDisplayAttribute, CComBSTR ResultStr, DWORD CompCursorPos = -1);
};
