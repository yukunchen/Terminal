/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

// This class exists to run a feature test in x86 build mode.
// Our feature tests can only really run in x64 mode because our build/test machines are x64.
// Because the console interfaces with the driver that already exists on those machines, we can't interface
// the x86 application with the existing x64 driver very well. So we want to just run no x86 tests.
// However, if we run no x86 tests, then the VSO Build will throw a warning on x86 builds that no feature tests
// were run... so until VSO 2017 comes out with conditional task execution (allowing us to set to run on x64 only)
// we need to make this class with a test that does nothing so x86 can happily run this one useless test and 
// finish with printing no errors nor warnings.
class NoOpTests
{
    TEST_CLASS(NoOpTests);

    TEST_METHOD(TestNoOp);
};

void NoOpTests::TestNoOp()
{
    VERIFY_IS_TRUE(true);
}
