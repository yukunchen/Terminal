/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"
#include "newdelete.hpp"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

BOOL g_fHeapInitializedTest = EnsureHeap();

class MiscTests
{
    TEST_CLASS(MiscTests);

    TEST_METHOD(TestHeap)
    {
        VERIFY_WIN32_BOOL_SUCCEEDED(g_fHeapInitializedTest);

        const UINT c_cch = 10;
        PWSTR psz = new WCHAR[c_cch];
        VERIFY_IS_NOT_NULL(psz);
        for (UINT i = 0; i < c_cch; i++)
        {
            VERIFY_ARE_EQUAL(psz[i], 0);
        }
        delete[] psz;
    }
};
