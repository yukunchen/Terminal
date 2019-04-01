/*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* This File was generated using the VisualTAEF C++ Project Wizard.
* Class Name: SelectionTest
*/
#include "stdafx.h"
#include <WexTestClass.h>

#include "../cascadia/TerminalCore/Terminal.hpp"

using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace Microsoft::Terminal::Core;

namespace UnitTests
{
    class SelectionTest
    {
        TEST_CLASS(SelectionTest);

        TEST_METHOD(SelectUnit)
        {
            Terminal term = Terminal();

            // Simulate click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, 10 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_IS_TRUE(selectionRects.size() == 1);
            
            auto selection = term.GetViewport().ConvertToOrigin(selectionRects.at(0)).ToInclusive();
            VerifyRect(selection, 10, 10, 5, 5);
        }

        TEST_METHOD(SelectArea)
        {
            Terminal term = Terminal();
            SHORT rowValue = 10;

            // Simulate click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, rowValue });

            // Simulate move to (x,y) = (15,20)
            term.SetEndSelectionPosition(COORD{ 15, 20 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_IS_TRUE(selectionRects.size() == 10);

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
            SHORT rowValue = 10;

            // Simulate ALT + click at (x,y) = (5,10)
            term.SetSelectionAnchor(COORD{ 5, rowValue });
            term.SetBoxSelection(true);

            // Simulate move to (x,y) = (15,20)
            term.SetEndSelectionPosition(COORD{ 15, 20 });

            // Simulate renderer calling TriggerSelection and acquiring selection area
            auto selectionRects = term.GetSelectionRects();

            // Validate selection area
            VERIFY_IS_TRUE(selectionRects.size() == 10);

            auto viewport = term.GetViewport();
            SHORT rightBoundary = viewport.RightInclusive();
            for (auto selectionRect : selectionRects)
            {
                auto selection = viewport.ConvertToOrigin(selectionRect).ToInclusive();

                // Verify all lines
                VerifyRect(selection, rowValue, rowValue, 5, 15);

                rowValue++;
            }
        }

        //TEST_METHOD(SelectAreaAfterScroll)
        //{
        //    Terminal *term = &Terminal();
        //    SHORT rowValue = 10;

        //    // Simulate click at (x,y) = (5,10)
        //    term->SetSelectionAnchor(COORD{ 5, rowValue });

        //    // Simulate move to (x,y) = (15,20)
        //    term->SetEndSelectionPosition(COORD{ 15, 20 });

        //    // Simulate renderer calling TriggerSelection and acquiring selection area
        //    auto selectionRects = term->GetSelectionRects();

        //    // Validate selection area
        //    VERIFY_IS_TRUE(selectionRects.size() == 10);

        //    auto viewport = term->GetViewport();
        //    SHORT rightBoundary = viewport.RightInclusive();
        //    auto scrollOffset = term->GetScrollOffset();
        //    for (auto selectionRect : selectionRects)
        //    {
        //        auto selection = viewport.ConvertToOrigin(selectionRect).ToInclusive();

        //        if (rowValue == 10)
        //        {
        //            // Verify top line
        //            VerifyRect(selection, 10, 10, 5, rightBoundary);
        //        }
        //        else if (rowValue == 20)
        //        {
        //            // Verify bottom line
        //            VerifyRect(selection, 20, 20, 0, 15);
        //        }
        //        else
        //        {
        //            // Verify other lines (full)
        //            VerifyRect(selection, rowValue, rowValue, 0, rightBoundary);
        //        }

        //        rowValue++;
        //    }
        //}

        void SelectionTest::VerifyRect(SMALL_RECT rect, SHORT top, SHORT bot, SHORT left, SHORT right)
        {
            VERIFY_IS_TRUE(rect.Top = top);
            VERIFY_IS_TRUE(rect.Bottom = bot);
            VERIFY_IS_TRUE(rect.Left = left);
            VERIFY_IS_TRUE(rect.Right = right);
        }
    };
} /* namespace UnitTests */