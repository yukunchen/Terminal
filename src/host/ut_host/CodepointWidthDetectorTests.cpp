/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"
#include "WexTestClass.h"
#include "..\..\inc\consoletaeftemplates.hpp"
#include "CommonState.hpp"

#include "../CodepointWidthDetector.hpp"

using namespace WEX::Logging;

static const std::wstring emoji = L"\xD83E\xDD22"; // U+1F922 nauseated face

class CodepointWidthDetectorTests
{
    TEST_CLASS(CodepointWidthDetectorTests);


    TEST_METHOD(CodepointWidthDetectDefersMapPopulation)
    {
        const CodepointWidthDetector widthDetector;
        VERIFY_IS_TRUE(widthDetector._map.empty());
        widthDetector.IsWide(UNICODE_SPACE);
        VERIFY_IS_TRUE(widthDetector._map.empty());
        // now force checking
        widthDetector.GetWidth(emoji);
        VERIFY_IS_FALSE(widthDetector._map.empty());
    }

    TEST_METHOD(CanLookUpEmoji)
    {
        const CodepointWidthDetector widthDetector;
        VERIFY_IS_TRUE(widthDetector.IsWide(emoji));
    }

    TEST_METHOD(TestUnicodeRangeCompare)
    {
        const CodepointWidthDetector::UnicodeRangeCompare compare;
        // test comparing 2 search terms
        CodepointWidthDetector::UnicodeRange a{ 0x10 };
        CodepointWidthDetector::UnicodeRange b{ 0x15 };
        VERIFY_IS_TRUE(static_cast<bool>(compare(a, b)));
    }
};
