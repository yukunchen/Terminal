/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

class NoOpTests
{
    TEST_CLASS(NoOpTests);

    TEST_METHOD(TestNoOp);
};

void NoOpTests::TestNoOp()
{
    VERIFY_IS_TRUE(true);
}
