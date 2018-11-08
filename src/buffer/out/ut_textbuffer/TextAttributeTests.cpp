/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "../TextAttribute.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class TextAttributeTests
{
    TEST_CLASS(TextAttributeTests);

    TEST_METHOD(TestRoundtripLegacy);
    TEST_METHOD(TestRoundtripExhaustive);
};

void TextAttributeTests::TestRoundtripLegacy()
{
    WORD expectedLegacy = FOREGROUND_BLUE | BACKGROUND_RED;
    WORD bgOnly = expectedLegacy & BG_ATTRS;
    WORD bgShifted = bgOnly >> 4;
    BYTE bgByte = (BYTE)(bgShifted);

    VERIFY_ARE_EQUAL(FOREGROUND_RED, bgByte);

    auto attr = TextAttribute(expectedLegacy);

    VERIFY_IS_TRUE(attr.IsLegacy());
    VERIFY_ARE_EQUAL(expectedLegacy, attr.GetLegacyAttributes());
}

void TextAttributeTests::TestRoundtripExhaustive()
{
    for (WORD wLegacy = 0; wLegacy <= (FG_ATTRS|BG_ATTRS); wLegacy++)
    {
        auto attr = TextAttribute(wLegacy);
        VERIFY_IS_TRUE(attr.IsLegacy());
        VERIFY_ARE_EQUAL(wLegacy, attr.GetLegacyAttributes());
    }
}
