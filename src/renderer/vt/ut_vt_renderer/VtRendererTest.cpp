/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include <wextestclass.h>
#include "..\..\inc\consoletaeftemplates.hpp"


using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            class VtRendererTest;
        };
    };
};
using namespace Microsoft::Console::VirtualTerminal;

class Microsoft::Console::VirtualTerminal::VtRendererTest
{
    TEST_CLASS(VtRendererTest);

    TEST_CLASS_SETUP(ClassSetup)
    {
        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        return true;
    }
    TEST_METHOD_SETUP(MethodSetup)
    {
        return true;
    }

    TEST_METHOD(Test1);
};

void VtRendererTest::Test1()
{
    VERIFY_SUCCEEDED(E_FAIL);
}