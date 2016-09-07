/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "globals.h"

#include <time.h>

#include "utils.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

class UtilsTests
{
    TEST_CLASS(UtilsTests);

    CommonState* m_state;

    TEST_CLASS_SETUP(ClassSetup)
    {
        m_state = new CommonState();

        m_state->PrepareGlobalFont();
        m_state->PrepareGlobalScreenBuffer();

        UINT const seed = (UINT)time(NULL);
        Log::Comment(String().Format(L"Setting random seed to : %d", seed));
        srand(seed);

        return true;
    }

    TEST_CLASS_CLEANUP(ClassCleanup)
    {
        m_state->CleanupGlobalScreenBuffer();
        m_state->CleanupGlobalFont();

        delete m_state;

        return true;
    }

    TEST_METHOD(TestIncrementScreenCoordinate)
    {
        bool fSuccess = false;

        SMALL_RECT srectEdges;
        srectEdges.Left = 10;
        srectEdges.Right = 19;
        srectEdges.Top = 20;
        srectEdges.Bottom = 29;

        COORD coordOriginal;
        COORD coordScreen;

        // #1 coord inside region
        coordOriginal.X = coordScreen.X = 15;
        coordOriginal.Y = coordScreen.Y = 25;

        fSuccess = Utils::s_DoIncrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_TRUE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, coordOriginal.X + 1);
        VERIFY_ARE_EQUAL(coordScreen.Y, coordOriginal.Y);

        // #2 coord right edge, not bottom
        coordOriginal.X = coordScreen.X = srectEdges.Right;
        coordOriginal.Y = coordScreen.Y = 25;

        fSuccess = Utils::s_DoIncrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_TRUE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, srectEdges.Left);
        VERIFY_ARE_EQUAL(coordScreen.Y, coordOriginal.Y + 1);

        // #3 coord right edge, bottom
        coordOriginal.X = coordScreen.X = srectEdges.Right;
        coordOriginal.Y = coordScreen.Y = srectEdges.Bottom;

        fSuccess = Utils::s_DoIncrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_FALSE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, srectEdges.Right);
        VERIFY_ARE_EQUAL(coordScreen.Y, srectEdges.Bottom);
    }

    TEST_METHOD(TestDecrementScreenCoordinate)
    {
        bool fSuccess = false;

        SMALL_RECT srectEdges;
        srectEdges.Left = 10;
        srectEdges.Right = 19;
        srectEdges.Top = 20;
        srectEdges.Bottom = 29;

        COORD coordOriginal;
        COORD coordScreen;

        // #1 coord inside region
        coordOriginal.X = coordScreen.X = 15;
        coordOriginal.Y = coordScreen.Y = 25;

        fSuccess = Utils::s_DoDecrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_TRUE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, coordOriginal.X - 1);
        VERIFY_ARE_EQUAL(coordScreen.Y, coordOriginal.Y);

        // #2 coord left edge, not top
        coordOriginal.X = coordScreen.X = srectEdges.Left;
        coordOriginal.Y = coordScreen.Y = 25;

        fSuccess = Utils::s_DoDecrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_TRUE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, srectEdges.Right);
        VERIFY_ARE_EQUAL(coordScreen.Y, coordOriginal.Y - 1);

        // #3 coord left edge, top
        coordOriginal.X = coordScreen.X = srectEdges.Left;
        coordOriginal.Y = coordScreen.Y = srectEdges.Top;

        fSuccess = Utils::s_DoDecrementScreenCoordinate(srectEdges, &coordScreen);

        VERIFY_IS_FALSE(fSuccess);
        VERIFY_ARE_EQUAL(coordScreen.X, srectEdges.Left);
        VERIFY_ARE_EQUAL(coordScreen.Y, srectEdges.Top);
    }

    TEST_METHOD(TestAddToPosition)
    {
        const UINT cTestLoopInstances = 100;

        const SHORT sRowWidth = 20;
        ASSERT(sRowWidth > 0);

        // 20x20 box
        SMALL_RECT srectEdges;
        srectEdges.Top = srectEdges.Left = 0;
        srectEdges.Bottom = srectEdges.Right = sRowWidth - 1;

        // repeat test 
        for (UINT i = 0; i < cTestLoopInstances; i++)
        {
            COORD coordPos;
            coordPos.X = RandomShort() % 20;
            coordPos.Y = RandomShort() % 20;

            SHORT sAddAmount = RandomShort() % (sRowWidth * sRowWidth);

            COORD coordFinal;
            coordFinal.X = (coordPos.X + sAddAmount) % sRowWidth;
            coordFinal.Y = coordPos.Y + ((coordPos.X + sAddAmount) / sRowWidth);

            Log::Comment(String().Format(L"Add To Position: (%d, %d)  Amount to add: %d", coordPos.Y, coordPos.X, sAddAmount));

            // Movement result is expected to be true, unless there's an error.
            bool fExpectedResult = true;

            // if we've calculated past the final row, then the function will reset to the original position and the output will be false.
            if (coordFinal.Y >= sRowWidth)
            {
                coordFinal = coordPos;
                fExpectedResult = false;
            }

            bool const fActualResult = Utils::s_AddToPosition(srectEdges, sAddAmount, &coordPos);

            VERIFY_ARE_EQUAL(coordPos.X, coordFinal.X);
            VERIFY_ARE_EQUAL(coordPos.Y, coordFinal.Y);
            VERIFY_ARE_EQUAL(fExpectedResult, fActualResult);

            Log::Comment(String().Format(L"Actual: (%d, %d) Expected: (%d, %d)", coordPos.Y, coordPos.X, coordFinal.Y, coordFinal.X));
        }
    }

    SHORT RandomShort()
    {
        return (SHORT)rand() % SHORT_MAX;
    }

    void FillBothCoordsSameRandom(COORD* pcoordA, COORD* pcoordB)
    {
        pcoordA->X = pcoordB->X = RandomShort();
        pcoordA->Y = pcoordB->Y = RandomShort();
    }

    void LogCoordinates(const COORD coordA, const COORD coordB)
    {
        Log::Comment(String().Format(L"Coordinates - A: (%d, %d) B: (%d, %d)", coordA.X, coordA.Y, coordB.X, coordB.Y));
    }

    void SubtractRandom(short &psValue)
    {
        SHORT const sRand = RandomShort();
        psValue -= max(sRand % psValue, 1);
    }

    TEST_METHOD(TestCompareCoords)
    {
        int result = 5; // not 1, 0, or -1
        COORD coordA;
        COORD coordB;

        // Set the buffer size to be able to accomodate large values.
        COORD coordMaxBuffer;
        coordMaxBuffer.X = SHORT_MAX;
        coordMaxBuffer.Y = SHORT_MAX;
        g_ciConsoleInformation.CurrentScreenBuffer->ScreenBufferSize = coordMaxBuffer;

        Log::Comment(L"#1: 0 case. Coords equal");
        FillBothCoordsSameRandom(&coordA, &coordB);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_ARE_EQUAL(result, 0);

        Log::Comment(L"#2: -1 case. A comes before B");
        Log::Comment(L"A. A left of B, same line");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordA.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_LESS_THAN(result, 0);

        Log::Comment(L"B. A above B, same column");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordA.Y);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_LESS_THAN(result, 0);

        Log::Comment(L"C. A up and to the left of B.");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordA.Y);
        SubtractRandom(coordA.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_LESS_THAN(result, 0);

        Log::Comment(L"D. A up and to the right of B.");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordA.Y);
        SubtractRandom(coordB.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_LESS_THAN(result, 0);

        Log::Comment(L"#3: 1 case. A comes after B");
        Log::Comment(L"A. A right of B, same line");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordB.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_GREATER_THAN(result, 0);

        Log::Comment(L"B. A below B, same column");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordB.Y);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_GREATER_THAN(result, 0);

        Log::Comment(L"C. A down and to the left of B");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordB.Y);
        SubtractRandom(coordA.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_GREATER_THAN(result, 0);

        Log::Comment(L"D. A down and to the right of B");
        FillBothCoordsSameRandom(&coordA, &coordB);
        SubtractRandom(coordB.Y);
        SubtractRandom(coordB.X);
        LogCoordinates(coordA, coordB);
        result = Utils::s_CompareCoords(coordA, coordB);
        VERIFY_IS_GREATER_THAN(result, 0);
    }
};