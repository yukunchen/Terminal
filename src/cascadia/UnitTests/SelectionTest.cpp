/*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* This File was generated using the VisualTAEF C++ Project Wizard.
* Class Name: SelectionTest
*/
#include "stdafx.h"
#include <WexTestClass.h>

#include "../cascadia/TerminalCore/Terminal.hpp"
#include "NullRenderTarget.h"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace Microsoft::Terminal::Core;
using namespace Microsoft::Console::Render;

namespace UnitTests
{
    class SelectionTest
    {
        TEST_CLASS(SelectionTest);

        TEST_METHOD(SelectUnit)
        {
            Terminal term = Terminal();
            NullRenderTarget emptyRT;
            term.Create(COORD{ 100,100 }, 0, emptyRT);

            // Simulate click at (x,y) = (5,10)
            auto clickPos = COORD{ 5, 10 };
            term.SetSelectionAnchor(clickPos);
            term.SetEndSelectionPosition(clickPos);

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_ARE_EQUAL(selectionRects.size(), 1);
            
            auto selection = term.GetViewport().ConvertToOrigin(selectionRects.at(0)).ToInclusive();
            VerifyRect(selection, 10, 10, 5, 5);
        }

        TEST_METHOD(SelectArea)
        {
            Terminal term = Terminal();
            NullRenderTarget emptyRT;
            term.Create(COORD{ 100,100 }, 0, emptyRT);

            // Used for two things:
            //    - click y-pos
            //    - keep track of row we're verifying
            SHORT rowValue = 10;

            // Simulate click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, rowValue });

            // Simulate move to (x,y) = (15,20)
            term.SetEndSelectionPosition(COORD{ 15, 20 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_ARE_EQUAL(selectionRects.size(), 11);

            auto viewport = term.GetViewport();
            SHORT rightBoundary = viewport.RightInclusive();
            for (auto selectionRect : selectionRects)
            {
                auto selection = viewport.ConvertToOrigin(selectionRect).ToInclusive();

                if (rowValue == 10) 
                {
                    // Verify top line
                    VerifyRect(selection, 10, 10, 5, rightBoundary);
                }
                else if (rowValue == 20)
                {
                    // Verify bottom line
                    VerifyRect(selection, 20, 20, 0, 15);
                }
                else
                {
                    // Verify other lines (full)
                    VerifyRect(selection, rowValue, rowValue, 0, rightBoundary);
                }

                rowValue++;
            }
        }

        TEST_METHOD(SelectBoxArea)
        {
            Terminal term = Terminal();
            NullRenderTarget emptyRT;
            term.Create(COORD{ 100,100 }, 0, emptyRT);

            // Used for two things:
            //    - click y-pos
            //    - keep track of row we're verifying
            SHORT rowValue = 10;

            // Simulate ALT + click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, rowValue });
            term.SetBoxSelection(true);

            // Simulate move to (x,y) = (15,20)
            term.SetEndSelectionPosition(COORD{ 15, 20 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_ARE_EQUAL(selectionRects.size(), 11);

            auto viewport = term.GetViewport();
            for (auto selectionRect : selectionRects)
            {
                auto selection = viewport.ConvertToOrigin(selectionRect).ToInclusive();

                // Verify all lines
                VerifyRect(selection, rowValue, rowValue, 5, 15);

                rowValue++;
            }
        }

        TEST_METHOD(SelectAreaAfterScroll)
        {
            Terminal term = Terminal();
            NullRenderTarget emptyRT;
            SHORT scrollbackLines = 5;
            term.Create(COORD{ 100,100 }, scrollbackLines, emptyRT);
            
            // Used for two things:
            //    - click y-pos
            //    - keep track of row we're verifying
            SHORT rowValue = 10;

            // Simulate click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, rowValue });

            // Simulate move to (x,y) = (15,20)
            term.SetEndSelectionPosition(COORD{ 15, 20 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_ARE_EQUAL(selectionRects.size(), 11);

            auto viewport = term.GetViewport();
            SHORT rightBoundary = viewport.RightInclusive();
            for (auto selectionRect : selectionRects)
            {
                auto selection = viewport.ConvertToOrigin(selectionRect).ToInclusive();

                if (rowValue == 10)
                {
                    // Verify top line
                    VerifyRect(selection, 10, 10, 5, rightBoundary);
                }
                else if (rowValue == 20)
                {
                    // Verify bottom line
                    VerifyRect(selection, 20, 20, 0, 15);
                }
                else
                {
                    // Verify other lines (full)
                    VerifyRect(selection, rowValue, rowValue, 0, rightBoundary);
                }

                rowValue++;
            }
        }

        void VerifyRect(const SMALL_RECT rect, const SHORT top, const SHORT bot, const SHORT left, const SHORT right) const
        {
            VERIFY_IS_TRUE(rect.Top == top);
            VERIFY_IS_TRUE(rect.Bottom == bot);
            VERIFY_IS_TRUE(rect.Left == left);
            VERIFY_IS_TRUE(rect.Right == right);
        }
    };
} /* namespace UnitTests */