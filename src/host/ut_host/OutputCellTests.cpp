/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"
#include "WexTestClass.h"

#include "CommonState.hpp"

#include "../../types/inc/Viewport.hpp"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

using Viewport = Microsoft::Console::Types::Viewport;

class ViewportTests
{
    TEST_CLASS(ViewportTests);

    TEST_METHOD(CreateEmpty)
    {
        const auto v = Viewport::Empty();

        VERIFY_ARE_EQUAL(0, v.Left());
        VERIFY_ARE_EQUAL(-1, v.RightInclusive());
        VERIFY_ARE_EQUAL(0, v.RightExclusive());
        VERIFY_ARE_EQUAL(0, v.Top());
        VERIFY_ARE_EQUAL(-1, v.BottomInclusive());
        VERIFY_ARE_EQUAL(0, v.BottomExclusive());
        VERIFY_ARE_EQUAL(0, v.Height());
        VERIFY_ARE_EQUAL(0, v.Width());
        VERIFY_ARE_EQUAL(COORD({ 0 }), v.Origin());
        VERIFY_ARE_EQUAL(COORD({ 0 }), v.Dimensions());
    }

    TEST_METHOD(CreateFromInclusive)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        COORD origin;
        origin.X = rect.Left;
        origin.Y = rect.Top;

        COORD dimensions;
        dimensions.X = rect.Right - rect.Left + 1;
        dimensions.Y = rect.Bottom - rect.Top + 1;

        const auto v = Viewport::FromInclusive(rect);

        VERIFY_ARE_EQUAL(rect.Left, v.Left());
        VERIFY_ARE_EQUAL(rect.Right, v.RightInclusive());
        VERIFY_ARE_EQUAL(rect.Right + 1, v.RightExclusive());
        VERIFY_ARE_EQUAL(rect.Top, v.Top());
        VERIFY_ARE_EQUAL(rect.Bottom, v.BottomInclusive());
        VERIFY_ARE_EQUAL(rect.Bottom + 1, v.BottomExclusive());
        VERIFY_ARE_EQUAL(dimensions.Y, v.Height());
        VERIFY_ARE_EQUAL(dimensions.X, v.Width());
        VERIFY_ARE_EQUAL(origin, v.Origin());
        VERIFY_ARE_EQUAL(dimensions, v.Dimensions());
    }

    TEST_METHOD(CreateFromExclusive)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        COORD origin;
        origin.X = rect.Left;
        origin.Y = rect.Top;

        COORD dimensions;
        dimensions.X = rect.Right - rect.Left;
        dimensions.Y = rect.Bottom - rect.Top;

        const auto v = Viewport::FromExclusive(rect);

        VERIFY_ARE_EQUAL(rect.Left, v.Left());
        VERIFY_ARE_EQUAL(rect.Right - 1, v.RightInclusive());
        VERIFY_ARE_EQUAL(rect.Right, v.RightExclusive());
        VERIFY_ARE_EQUAL(rect.Top, v.Top());
        VERIFY_ARE_EQUAL(rect.Bottom - 1, v.BottomInclusive());
        VERIFY_ARE_EQUAL(rect.Bottom, v.BottomExclusive());
        VERIFY_ARE_EQUAL(dimensions.Y, v.Height());
        VERIFY_ARE_EQUAL(dimensions.X, v.Width());
        VERIFY_ARE_EQUAL(origin, v.Origin());
        VERIFY_ARE_EQUAL(dimensions, v.Dimensions());
    }

    TEST_METHOD(CreateFromDimensionsWidthHeight)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        COORD origin;
        origin.X = rect.Left;
        origin.Y = rect.Top;

        COORD dimensions;
        dimensions.X = rect.Right - rect.Left + 1;
        dimensions.Y = rect.Bottom - rect.Top + 1;

        const auto v = Viewport::FromDimensions(origin, dimensions.X, dimensions.Y);

        VERIFY_ARE_EQUAL(rect.Left, v.Left());
        VERIFY_ARE_EQUAL(rect.Right, v.RightInclusive());
        VERIFY_ARE_EQUAL(rect.Right + 1, v.RightExclusive());
        VERIFY_ARE_EQUAL(rect.Top, v.Top());
        VERIFY_ARE_EQUAL(rect.Bottom, v.BottomInclusive());
        VERIFY_ARE_EQUAL(rect.Bottom + 1, v.BottomExclusive());
        VERIFY_ARE_EQUAL(dimensions.Y, v.Height());
        VERIFY_ARE_EQUAL(dimensions.X, v.Width());
        VERIFY_ARE_EQUAL(origin, v.Origin());
        VERIFY_ARE_EQUAL(dimensions, v.Dimensions());
    }

    TEST_METHOD(CreateFromDimensions)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        COORD origin;
        origin.X = rect.Left;
        origin.Y = rect.Top;

        COORD dimensions;
        dimensions.X = rect.Right - rect.Left + 1;
        dimensions.Y = rect.Bottom - rect.Top + 1;

        const auto v = Viewport::FromDimensions(origin, dimensions);

        VERIFY_ARE_EQUAL(rect.Left, v.Left());
        VERIFY_ARE_EQUAL(rect.Right, v.RightInclusive());
        VERIFY_ARE_EQUAL(rect.Right + 1, v.RightExclusive());
        VERIFY_ARE_EQUAL(rect.Top, v.Top());
        VERIFY_ARE_EQUAL(rect.Bottom, v.BottomInclusive());
        VERIFY_ARE_EQUAL(rect.Bottom + 1, v.BottomExclusive());
        VERIFY_ARE_EQUAL(dimensions.Y, v.Height());
        VERIFY_ARE_EQUAL(dimensions.X, v.Width());
        VERIFY_ARE_EQUAL(origin, v.Origin());
        VERIFY_ARE_EQUAL(dimensions, v.Dimensions());
    }

    TEST_METHOD(CreateFromCoord)
    {
        COORD origin;
        origin.X = 12;
        origin.Y = 24;

        const auto v = Viewport::FromCoord(origin);

        VERIFY_ARE_EQUAL(origin.X, v.Left());
        VERIFY_ARE_EQUAL(origin.X, v.RightInclusive());
        VERIFY_ARE_EQUAL(origin.X + 1, v.RightExclusive());
        VERIFY_ARE_EQUAL(origin.Y, v.Top());
        VERIFY_ARE_EQUAL(origin.Y, v.BottomInclusive());
        VERIFY_ARE_EQUAL(origin.Y + 1, v.BottomExclusive());
        VERIFY_ARE_EQUAL(1, v.Height());
        VERIFY_ARE_EQUAL(1, v.Width());
        VERIFY_ARE_EQUAL(origin, v.Origin());
        VERIFY_ARE_EQUAL(COORD({ 1, 1, }), v.Dimensions());
    }

    TEST_METHOD(IsInBoundsCoord)
    {
        SMALL_RECT r;
        r.Top = 3;
        r.Bottom = 5;
        r.Left = 10;
        r.Right = 20;

        const auto v = Viewport::FromInclusive(r);

        COORD c;
        c.X = r.Left;
        c.Y = r.Top;
        VERIFY_IS_TRUE(v.IsInBounds(c), L"Top left corner in bounds.");

        c.Y = r.Bottom;
        VERIFY_IS_TRUE(v.IsInBounds(c), L"Bottom left corner in bounds.");

        c.X = r.Right;
        VERIFY_IS_TRUE(v.IsInBounds(c), L"Bottom right corner in bounds.");

        c.Y = r.Top;
        VERIFY_IS_TRUE(v.IsInBounds(c), L"Top right corner in bounds.");

        c.X++;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One right out the top right is out of bounds.");

        c.X--;
        c.Y--;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One up out the top right is out of bounds.");

        c.X = r.Left;
        c.Y = r.Top;
        c.X--;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One left out the top left is out of bounds.");

        c.X++;
        c.Y--;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One up out the top left is out of bounds.");

        c.X = r.Left;
        c.Y = r.Bottom;
        c.X--;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One left out the bottom left is out of bounds.");

        c.X++;
        c.Y++;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One down out the bottom left is out of bounds.");

        c.X = r.Right;
        c.Y = r.Bottom;
        c.X++;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One right out the bottom right is out of bounds.");

        c.X--;
        c.Y++;
        VERIFY_IS_FALSE(v.IsInBounds(c), L"One down out the bottom right is out of bounds.");
    }

    TEST_METHOD(IsInBoundsViewport)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        const auto original = rect;

        const auto view = Viewport::FromInclusive(rect);

        auto test = Viewport::FromInclusive(rect);
        VERIFY_IS_TRUE(view.IsInBounds(test), L"Same size/position viewport is in bounds.");

        rect.Top++;
        rect.Bottom--;
        rect.Left++;
        rect.Right--;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_TRUE(view.IsInBounds(test), L"Viewport inscribed inside viewport is in bounds.");

        rect = original;
        rect.Top--;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_FALSE(view.IsInBounds(test), L"Viewport that is one taller upwards is out of bounds.");

        rect = original;
        rect.Bottom++;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_FALSE(view.IsInBounds(test), L"Viewport that is one taller downwards is out of bounds.");

        rect = original;
        rect.Left--;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_FALSE(view.IsInBounds(test), L"Viewport that is one wider leftwards is out of bounds.");

        rect = original;
        rect.Right++;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_FALSE(view.IsInBounds(test), L"Viewport that is one wider rightwards is out of bounds.");

        rect = original;
        rect.Left++;
        rect.Right++;
        rect.Top++;
        rect.Bottom++;
        test = Viewport::FromInclusive(rect);
        VERIFY_IS_FALSE(view.IsInBounds(test), L"Viewport offset at the origin but same size is out of bounds.");
    }

    TEST_METHOD(ClampCoord)
    {
        SMALL_RECT rect;
        rect.Top = 3;
        rect.Bottom = 5;
        rect.Left = 10;
        rect.Right = 20;

        const auto view = Viewport::FromInclusive(rect);

        COORD pos;
        pos.X = rect.Left;
        pos.Y = rect.Top;

        auto before = pos;
        view.Clamp(pos);
        VERIFY_ARE_EQUAL(before, pos, L"Verify clamp did nothing for position in top left corner.");

        pos.X = rect.Left;
        pos.Y = rect.Bottom;
        before = pos;
        view.Clamp(pos);
        VERIFY_ARE_EQUAL(before, pos, L"Verify clamp did nothing for position in bottom left corner.");

        pos.X = rect.Right;
        pos.Y = rect.Bottom;
        before = pos;
        view.Clamp(pos);
        VERIFY_ARE_EQUAL(before, pos, L"Verify clamp did nothing for position in bottom right corner.");

        pos.X = rect.Right;
        pos.Y = rect.Top;
        before = pos;
        view.Clamp(pos);
        VERIFY_ARE_EQUAL(before, pos, L"Verify clamp did nothing for position in top right corner.");


        COORD expected;
        expected.X = rect.Right;
        expected.Y = rect.Top;

        pos = expected;
        pos.X++;
        pos.Y--;
        before = pos;

        view.Clamp(pos);
        VERIFY_ARE_NOT_EQUAL(before, pos, L"Verify clamp modified position out the top right corner back.");
        VERIFY_ARE_EQUAL(expected, pos, L"Verify position was clamped into the top right corner.");

        expected.X = rect.Left;
        expected.Y = rect.Top;

        pos = expected;
        pos.X--;
        pos.Y--;
        before = pos;

        view.Clamp(pos);
        VERIFY_ARE_NOT_EQUAL(before, pos, L"Verify clamp modified position out the top left corner back.");
        VERIFY_ARE_EQUAL(expected, pos, L"Verify position was clamped into the top left corner.");

        expected.X = rect.Left;
        expected.Y = rect.Bottom;

        pos = expected;
        pos.X--;
        pos.Y++;
        before = pos;

        view.Clamp(pos);
        VERIFY_ARE_NOT_EQUAL(before, pos, L"Verify clamp modified position out the bottom left corner back.");
        VERIFY_ARE_EQUAL(expected, pos, L"Verify position was clamped into the bottom left corner.");

        expected.X = rect.Right;
        expected.Y = rect.Bottom;

        pos = expected;
        pos.X++;
        pos.Y++;
        before = pos;

        view.Clamp(pos);
        VERIFY_ARE_NOT_EQUAL(before, pos, L"Verify clamp modified position out the bottom right corner back.");
        VERIFY_ARE_EQUAL(expected, pos, L"Verify position was clamped into the bottom right corner.");
    }

    TEST_METHOD(IncrementInBounds)
    {
        bool success = false;

        SMALL_RECT edges;
        edges.Left = 10;
        edges.Right = 19;
        edges.Top = 20;
        edges.Bottom = 29;

        const auto v = Viewport::FromInclusive(edges);
        COORD original;
        COORD screen;

        // #1 coord inside region
        original.X = screen.X = 15;
        original.Y = screen.Y = 25;

        success = v.IncrementInBounds(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, original.X + 1);
        VERIFY_ARE_EQUAL(screen.Y, original.Y);

        // #2 coord right edge, not bottom
        original.X = screen.X = edges.Right;
        original.Y = screen.Y = 25;

        success = v.IncrementInBounds(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Left);
        VERIFY_ARE_EQUAL(screen.Y, original.Y + 1);

        // #3 coord right edge, bottom
        original.X = screen.X = edges.Right;
        original.Y = screen.Y = edges.Bottom;

        success = v.IncrementInBounds(screen);

        VERIFY_IS_FALSE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Right);
        VERIFY_ARE_EQUAL(screen.Y, edges.Bottom);
    }

    TEST_METHOD(IncrementInBoundsCircular)
    {
        bool success = false;

        SMALL_RECT edges;
        edges.Left = 10;
        edges.Right = 19;
        edges.Top = 20;
        edges.Bottom = 29;

        const auto v = Viewport::FromInclusive(edges);
        COORD original;
        COORD screen;

        // #1 coord inside region
        original.X = screen.X = 15;
        original.Y = screen.Y = 25;

        success = v.IncrementInBoundsCircular(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, original.X + 1);
        VERIFY_ARE_EQUAL(screen.Y, original.Y);

        // #2 coord right edge, not bottom
        original.X = screen.X = edges.Right;
        original.Y = screen.Y = 25;

        success = v.IncrementInBoundsCircular(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Left);
        VERIFY_ARE_EQUAL(screen.Y, original.Y + 1);

        // #3 coord right edge, bottom
        original.X = screen.X = edges.Right;
        original.Y = screen.Y = edges.Bottom;

        success = v.IncrementInBoundsCircular(screen);

        VERIFY_IS_FALSE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Left);
        VERIFY_ARE_EQUAL(screen.Y, edges.Top);
    }

    TEST_METHOD(DecrementInBounds)
    {
        bool success = false;

        SMALL_RECT edges;
        edges.Left = 10;
        edges.Right = 19;
        edges.Top = 20;
        edges.Bottom = 29;

        const auto v = Viewport::FromInclusive(edges);
        COORD original;
        COORD screen;

        // #1 coord inside region
        original.X = screen.X = 15;
        original.Y = screen.Y = 25;

        success = v.DecrementInBounds(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, original.X - 1);
        VERIFY_ARE_EQUAL(screen.Y, original.Y);

        // #2 coord left edge, not top
        original.X = screen.X = edges.Left;
        original.Y = screen.Y = 25;

        success = v.DecrementInBounds(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Right);
        VERIFY_ARE_EQUAL(screen.Y, original.Y - 1);

        // #3 coord left edge, top
        original.X = screen.X = edges.Left;
        original.Y = screen.Y = edges.Top;

        success = v.DecrementInBounds(screen);

        VERIFY_IS_FALSE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Left);
        VERIFY_ARE_EQUAL(screen.Y, edges.Top);
    }

    TEST_METHOD(DecrementInBoundsCircular)
    {
        bool success = false;

        SMALL_RECT edges;
        edges.Left = 10;
        edges.Right = 19;
        edges.Top = 20;
        edges.Bottom = 29;

        const auto v = Viewport::FromInclusive(edges);
        COORD original;
        COORD screen;

        // #1 coord inside region
        original.X = screen.X = 15;
        original.Y = screen.Y = 25;

        success = v.DecrementInBoundsCircular(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, original.X - 1);
        VERIFY_ARE_EQUAL(screen.Y, original.Y);

        // #2 coord left edge, not top
        original.X = screen.X = edges.Left;
        original.Y = screen.Y = 25;

        success = v.DecrementInBoundsCircular(screen);

        VERIFY_IS_TRUE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Right);
        VERIFY_ARE_EQUAL(screen.Y, original.Y - 1);

        // #3 coord left edge, top
        original.X = screen.X = edges.Left;
        original.Y = screen.Y = edges.Top;

        success = v.DecrementInBoundsCircular(screen);

        VERIFY_IS_FALSE(success);
        VERIFY_ARE_EQUAL(screen.X, edges.Right);
        VERIFY_ARE_EQUAL(screen.Y, edges.Bottom);
    }

    SHORT RandomShort()
    {
        SHORT s;

        do
        {
            s = (SHORT)rand() % SHORT_MAX;
        } while (s == 0i16);

        return s;
    }

    TEST_METHOD(MoveInBounds)
    {
        const UINT cTestLoopInstances = 100;

        const SHORT sRowWidth = 20;
        VERIFY_IS_TRUE(sRowWidth > 0);

        // 20x20 box
        SMALL_RECT srectEdges;
        srectEdges.Top = srectEdges.Left = 0;
        srectEdges.Bottom = srectEdges.Right = sRowWidth - 1;

        const auto v = Viewport::FromInclusive(srectEdges);

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

            bool const fActualResult = v.MoveInBounds(sAddAmount, coordPos);

            VERIFY_ARE_EQUAL(fExpectedResult, fActualResult);
            VERIFY_ARE_EQUAL(coordPos.X, coordFinal.X);
            VERIFY_ARE_EQUAL(coordPos.Y, coordFinal.Y);

            Log::Comment(String().Format(L"Actual: (%d, %d) Expected: (%d, %d)", coordPos.Y, coordPos.X, coordFinal.Y, coordFinal.X));
        }
    }

    TEST_METHOD(CompareInBounds)
    {
        SMALL_RECT edges;
        edges.Left = 10;
        edges.Right = 19;
        edges.Top = 20;
        edges.Bottom = 29;

        const auto v = Viewport::FromInclusive(edges);

        COORD first, second;
        first.X = 12;
        first.Y = 24;
        second = first;
        second.X += 2;

        VERIFY_ARE_EQUAL(-2, v.CompareInBounds(first, second), L"Second and first on same row. Second is right of first.");
        VERIFY_ARE_EQUAL(2, v.CompareInBounds(second, first), L"Reverse params, should get opposite direction, same magnitude.");

        first.X = edges.Left;
        first.Y = 24;

        second.X = edges.Right;
        second.Y = first.Y - 1;
        
        VERIFY_ARE_EQUAL(1, v.CompareInBounds(first, second), L"Second is up a line at the right edge from first at the line below on the left edge.");
        VERIFY_ARE_EQUAL(-1, v.CompareInBounds(second, first), L"Reverse params, should get opposite direction, same magnitude.");
    }
};
