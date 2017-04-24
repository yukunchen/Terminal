/*++

Copyright (c) 2003, Microsoft Corporation

Module Name:

    TfCatUtil.h

Abstract:

    This file defines the CicCategoryMgr Class.

Author:

Revision History:

Notes:

--*/

#pragma once

class CicCategoryMgr
{
public:
    CicCategoryMgr();
    virtual ~CicCategoryMgr();

public:
    HRESULT GetGUIDFromGUIDATOM(TfGuidAtom guidatom, GUID *pguid);
    HRESULT InitCategoryInstance();

    inline ITfCategoryMgr* GetCategoryMgr() { return m_pcat; }

private:
    CComQIPtr<ITfCategoryMgr>          m_pcat;
};
