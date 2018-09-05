/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "..\..\types\inc\Viewport.hpp"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

using Viewport = Microsoft::Console::Types::Viewport;

class ViewportTests
{
    TEST_CLASS(ViewportTests);

    TEST_METHOD(DefaultConstructor)
    {
        Viewport v;
        VERIFY_IS_FALSE(v.IsValid());
        VERIFY_ARE_EQUAL(0, v.Height());
        VERIFY_ARE_EQUAL(0, v.Width());
        VERIFY_ARE_EQUAL(COORD{ 0 }, v.Origin());
    }
};
