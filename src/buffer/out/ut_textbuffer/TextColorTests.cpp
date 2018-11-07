/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "../TextColor.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class TextColorTests
{
    TEST_CLASS(TextColorTests);

    TEST_METHOD(TestDefaultColor);
    TEST_METHOD(TestIndexColor);
    TEST_METHOD(TestRgbColor);
};

void TextColorTests::TestDefaultColor()
{
    TextColor defaultColor;

    VERIFY_IS_TRUE(defaultColor.IsDefault());
    VERIFY_IS_FALSE(defaultColor.IsLegacy());
    VERIFY_IS_FALSE(defaultColor.IsRgb());
}

void TextColorTests::TestIndexColor()
{
    TextColor indexColor((BYTE)(15));

    VERIFY_IS_FALSE(indexColor.IsDefault());
    VERIFY_IS_TRUE(indexColor.IsLegacy());
    VERIFY_IS_FALSE(indexColor.IsRgb());
}

void TextColorTests::TestRgbColor()
{
    TextColor rgbColor(RGB(1, 2, 3));

    VERIFY_IS_FALSE(rgbColor.IsDefault());
    VERIFY_IS_FALSE(rgbColor.IsLegacy());
    VERIFY_IS_TRUE(rgbColor.IsRgb());
}
