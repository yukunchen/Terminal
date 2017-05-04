//----------------------------------------------------------------------------------------------------------------------
// <copyright file="AccessibilityTests.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>UI Automation tests for the certain key presses.</summary>
//----------------------------------------------------------------------------------------------------------------------
using System;
using System.Windows;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Automation;
using System.Windows.Automation.Text;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Conhost.UIA.Tests.Common;
using Conhost.UIA.Tests.Common.NativeMethods;
using Conhost.UIA.Tests.Elements;
using OpenQA.Selenium;
using System.Drawing;

using System.Runtime.InteropServices;

namespace Conhost.UIA.Tests
{

    class InvalidElementException : Exception
    {

    }

    [TestClass]
    class AccessibilityTests
    {
        public TestContext TestContext { get; set; }

        private AutomationElement GetWindowUiaElement(CmdApp app)
        {
            IntPtr handle = app.GetWindowHandle();
            AutomationElement windowUiaElement = AutomationElement.FromHandle(handle);
            return windowUiaElement;
        }

        private AutomationElement GetTextAreaUiaElement(CmdApp app)
        {
            AutomationElement windowUiaElement = GetWindowUiaElement(app);
            AutomationElementCollection descendants = windowUiaElement.FindAll(TreeScope.Descendants, Condition.TrueCondition);
            for (int i = 0; i < descendants.Count; ++i)
            {
                AutomationElement poss = descendants[i];
                if (poss.Current.AutomationId.Equals("Text Area"))
                {
                    return poss;
                }
            }
            throw new InvalidElementException();
        }

        [TestMethod]
        public void CanAccessAccessibilityTree()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement windowUiaElement = GetWindowUiaElement(app);
                    Verify.IsTrue(windowUiaElement.Current.AutomationId.Equals("Console Window"));
                }
            }
        }

        [TestMethod]
        public void CanAccessTextAreaUiaElement()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    Verify.IsTrue(textAreaUiaElement != null);
                }
            }
        }

        [TestMethod]
        public void CanGetDocumentRangeText()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    // get the text from uia api
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange documentRange = textPattern.DocumentRange;
                    string allText = documentRange.GetText(-1);
                    // get text from console api
                    IntPtr hConsole = app.GetStdOutHandle();
                    using (ViewportArea area = new ViewportArea(app))
                    {
                        WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX screenInfo = app.GetScreenBufferInfo();
                        Rectangle rect = new Rectangle(0, 0, screenInfo.dwSize.X, screenInfo.dwSize.Y);
                        IEnumerable<string> viewportText = area.GetLinesInRectangle(hConsole, rect);

                        // the uia api does not return spaces beyond the last
                        // non -whitespace character so we need to trim those from
                        // the viewportText. The uia api also inserts \r\n to indicate
                        // a new linen so we need to add those back in after trimming.
                        string consoleText = "";
                        for (int i = 0; i < viewportText.Count(); ++i)
                        {
                            consoleText += viewportText.ElementAt(i).Trim() + "\r\n";
                        }
                        consoleText = consoleText.Trim();
                        allText = allText.Trim();
                        // compare
                        Verify.IsTrue(consoleText.Equals(allText));
                    }
                }
            }
        }

        [TestMethod]
        public void CanGetVisibleRange()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    // get the ranges from uia api
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange[] ranges = textPattern.GetVisibleRanges();

                    // get the ranges from the console api
                    WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX screenInfo = app.GetScreenBufferInfo();
                    int viewportHeight = screenInfo.srWindow.Bottom - screenInfo.srWindow.Top + 1;

                    // we should have one range per line in the viewport
                    Verify.AreEqual(ranges.GetLength(0), viewportHeight);

                    // each line should have the same text
                    ViewportArea viewport = new ViewportArea(app);
                    IntPtr hConsole = app.GetStdOutHandle();
                    for (int i = 0; i < viewportHeight; ++i)
                    {
                        Rectangle rect = new Rectangle(0, i, screenInfo.dwSize.X, 1);
                        IEnumerable<string> text = viewport.GetLinesInRectangle(hConsole, rect);
                        Verify.AreEqual(text.ElementAt(0).Trim(), ranges[i].GetText(-1).Trim());
                    }

                }
            }
        }

        /*
        // TODO this is commented out because it will fail. It fails because the c# api of RangeFromPoint
        // throws an exception when passed a point that is outside the bounds of the window, which is
        // allowed in the c++ version and exactly what we want to test. Another way to test this case needs
        // to be found that doesn't go through the c# api.
        [TestMethod]
        public void CanGetRangeFromPoint()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    // RangeFromPoint returns the range closest to the point provided
                    // so we have three cases along the y dimension:
                    // - point above the console window
                    // - point in the console window
                    // - point below the console window

                    // get the window dimensions and pick the point locations
                    IntPtr handle = app.GetWindowHandle();
                    User32.RECT windowRect;
                    User32.GetWindowRect(handle, out windowRect);

                    List<System.Windows.Point> points = new List<System.Windows.Point>();
                    int middleOfWindow = (windowRect.bottom + windowRect.top) / 2;
                    const int windowEdgeOffset = 10;
                    // x doesn't matter until we support more ranges than lines
                    points.Add(new System.Windows.Point(windowRect.left, windowRect.top - windowEdgeOffset));
                    points.Add(new System.Windows.Point(windowRect.left, middleOfWindow));
                    points.Add(new System.Windows.Point(windowRect.left, windowRect.bottom + windowEdgeOffset));


                    // get the ranges from uia api
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    List<TextPatternRange> textPatternRanges = new List<TextPatternRange>();
                    foreach (System.Windows.Point p in points)
                    {
                        textPatternRanges.Add(textPattern.RangeFromPoint(p));
                    }

                    // ranges should be in correct order (starting at top and
                    // going down screen)
                    Rect lastBoundingRect = textPatternRanges.ElementAt(0).GetBoundingRectangles()[0];
                    foreach (TextPatternRange range in textPatternRanges)
                    {
                        Rect[] boundingRects = range.GetBoundingRectangles();
                        // since the ranges returned by RangeFromPoint are supposed to be degenerate,
                        // there should be only one bounding rect per TextPatternRange
                        Verify.AreEqual(boundingRects.GetLength(0), 1);

                        Verify.IsTrue(boundingRects[0].Top >= lastBoundingRect.Top);
                        lastBoundingRect = boundingRects[0];
                    }


                }
            }

        }
        */

        [TestMethod]
        public void CanCloneTextRangeProvider()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange textPatternRange = textPattern.DocumentRange;
                    // clone it
                    TextPatternRange copyRange = textPatternRange.Clone();
                    Verify.IsTrue(copyRange.Compare(textPatternRange));
                    // change the copy and make sure the compare fails
                    copyRange.MoveEndpointByRange(TextPatternRangeEndpoint.End, copyRange, TextPatternRangeEndpoint.Start);
                    Verify.IsFalse(copyRange.Compare(textPatternRange));
                }
            }
        }

        [TestMethod]
        public void CanCompareTextRangeProviderEndpoints()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange textPatternRange = textPattern.DocumentRange;
                    // comparing an endpoint to itself should be the same
                    Verify.AreEqual(0, textPatternRange.CompareEndpoints(TextPatternRangeEndpoint.Start,
                                                                         textPatternRange,
                                                                         TextPatternRangeEndpoint.Start));
                    // comparing an earlier endpoint to a later one should be negative
                    Verify.IsGreaterThan(0, textPatternRange.CompareEndpoints(TextPatternRangeEndpoint.Start,
                                                                              textPatternRange,
                                                                              TextPatternRangeEndpoint.End));
                    // comparing a later endpoint to an earlier one should be positive
                    Verify.IsLessThan(0, textPatternRange.CompareEndpoints(TextPatternRangeEndpoint.End,
                                                                           textPatternRange,
                                                                           TextPatternRangeEndpoint.Start));
                }
            }
        }

        [TestMethod]
        public void CanExpandToEnclosingUnitTextRangeProvider()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange[] visibleRanges = textPattern.GetVisibleRanges();
                    // copy the first range
                    TextPatternRange firstRange = visibleRanges[0].Clone();
                    // change firstRange to a degenerate range and then expand to a line
                    firstRange.MoveEndpointByRange(TextPatternRangeEndpoint.End, firstRange, TextPatternRangeEndpoint.Start);
                    Verify.AreEqual(0, firstRange.CompareEndpoints(TextPatternRangeEndpoint.Start,
                                                                   firstRange,
                                                                   TextPatternRangeEndpoint.End));
                    firstRange.ExpandToEnclosingUnit(TextUnit.Line);
                    Verify.IsTrue(firstRange.Compare(visibleRanges[0]));
                    // expand to document size
                    firstRange.ExpandToEnclosingUnit(TextUnit.Document);
                    Verify.IsTrue(firstRange.Compare(textPattern.DocumentRange));
                    // shrink back to a line
                    firstRange.ExpandToEnclosingUnit(TextUnit.Line);
                    Verify.IsTrue(firstRange.Compare(visibleRanges[0]));
                }
            }
        }

        //TODO this needs more work
        /*
        [TestMethod]
        public void CanGetBoundingRectangles()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();
                using (CmdApp app = new CmdApp(CreateType.ProcessOnly, TestContext))
                {
                    AutomationElement textAreaUiaElement = GetTextAreaUiaElement(app);
                    TextPattern textPattern = textAreaUiaElement.GetCurrentPattern(TextPattern.Pattern) as TextPattern;
                    TextPatternRange[] visibleRanges = textPattern.GetVisibleRanges();
                    // copy the first range
                    TextPatternRange firstRange = visibleRanges[0].Clone();
                    // only one bounding rect should be returned for the one line
                    Rect[] boundingRects = firstRange.GetBoundingRectangles();
                    Verify.AreEqual(1, boundingRects.GetLength(0));
                    // expand to two lines, verify we get a bounding rect per line
                    firstRange.MoveEndpointByRange(TextPatternRangeEndpoint.End, visibleRanges[1], TextPatternRangeEndpoint.End);
                    boundingRects = firstRange.GetBoundingRectangles();
                    Verify.AreEqual(2, boundingRects.GetLength(0));

                    // get the bounds of the screen buffer in screen units
                    WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX screenInfo = app.GetScreenBufferInfo();
                    IntPtr handle = app.GetWindowHandle();
                    User32.POINT topLeft = new User32.POINT() ;
                    User32.POINT bottomRight;
                    topLeft.x = screenInfo.srWindow.Left;
                    topLeft.y = screenInfo.srWindow.Top;
                    bottomRight.x = screenInfo.srWindow.Right;
                    bottomRight.y = screenInfo.srWindow.Bottom;
                    User32.ClientToScreen(handle, ref topLeft);
                    User32.ClientToScreen(handle, ref bottomRight);
                    // verify the bounds
                    Verify.AreEqual(topLeft.x, boundingRects[0].Left);
                    Verify.AreEqual(topLeft.y, boundingRects[0].Top);
                    Verify.AreEqual(bottomRight.x, boundingRects[0].Right);
                    Verify.AreEqual(bottomRight.y, boundingRects[0].Bottom);


                }
            }
        }
        */




    }

}
