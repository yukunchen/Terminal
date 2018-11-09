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
    TEST_METHOD(TestRoundtripMetaBits);
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

void TextAttributeTests::TestRoundtripMetaBits()
{
    WORD metaFlags[] =
    {
        COMMON_LVB_LEADING_BYTE,
        COMMON_LVB_TRAILING_BYTE,
        COMMON_LVB_GRID_HORIZONTAL,
        COMMON_LVB_GRID_LVERTICAL,
        COMMON_LVB_GRID_RVERTICAL,
        COMMON_LVB_REVERSE_VIDEO,
        COMMON_LVB_UNDERSCORE
    };

    for (int i = 0; i < 7; ++i)
    {
        WORD flag = metaFlags[i];
        WORD expectedLegacy = FOREGROUND_BLUE | BACKGROUND_RED | flag;
        WORD metaOnly = expectedLegacy & META_ATTRS;
        VERIFY_ARE_EQUAL(flag, metaOnly);

        auto attr = TextAttribute(expectedLegacy);
        VERIFY_IS_TRUE(attr.IsLegacy());
        VERIFY_ARE_EQUAL(expectedLegacy, attr.GetLegacyAttributes());
        VERIFY_ARE_EQUAL(flag, attr._wAttrLegacy);
    }

}

void TextAttributeTests::TestRoundtripExhaustive()
{
    WORD allAttrs = (META_ATTRS | FG_ATTRS | BG_ATTRS);
    // This test covers some 0xdfff test cases, printing out Verify: IsTrue for
    //      each takes a lot longer than checking.
    // Only VERIFY if the comparison actually fails to speed up the test.
    Log::Comment(L"This test will check each possible legacy attribute to make "
        "sure it roundtrips through the creation of a text attribute.");
    Log::Comment(L"It will only log if it fails.");
    for (WORD wLegacy = 0; wLegacy < allAttrs; wLegacy++)
    {
        // 0x2000 is not an actual meta attribute
        if (WI_IsFlagSet(wLegacy, 0x2000)) continue;

        auto attr = TextAttribute(wLegacy);

        bool isLegacy = attr.IsLegacy();
        bool areEqual = (wLegacy == attr.GetLegacyAttributes());
        if (!(isLegacy && areEqual))
        {
            Log::Comment(NoThrowString().Format(
                L"Failed on wLegacy=0x%x", wLegacy
            ));
            VERIFY_IS_TRUE(attr.IsLegacy());
            VERIFY_ARE_EQUAL(wLegacy, attr.GetLegacyAttributes());
        }
    }
}
