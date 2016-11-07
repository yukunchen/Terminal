//----------------------------------------------------------------------------------------------------------------------
// <copyright file="SelectionApiTests.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>UI Automation tests for the Selection Information API.</summary>
//----------------------------------------------------------------------------------------------------------------------
namespace Conhost.UIA.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Threading;

    using Microsoft.Win32;

    using WEX.Common.Managed;
    using WEX.Logging.Interop;
    using WEX.TestExecution;
    using WEX.TestExecution.Markup;

    using Conhost.UIA.Tests.Common;
    using Conhost.UIA.Tests.Common.NativeMethods;
    using Conhost.UIA.Tests.Elements;
    using OpenQA.Selenium;

    [TestClass]
    public class SelectionApiTests
    {
        public const int timeout = Globals.Timeout;

        [TestMethod]
        public void TestKeyboardSelection()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry();

                VersionSelector.SetConsoleVersion(reg, ConsoleVersion.V2);

                using (CmdApp app = new CmdApp(CreateType.ProcessOnly))
                {
                    using (ViewportArea area = new ViewportArea(app))
                    {
                        WinCon.CONSOLE_SELECTION_INFO csi;
                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Get initial selection state.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION, "Confirm no selection in progress.");
                        // ignore rectangle and coords. They're undefined when there is no selection.

                        // Get cursor position at the beginning of this operation. The anchor will start at the cursor position for v2 console.
                        // NOTE: It moved to 0,0 for the v1 console.
                        IntPtr hConsole = WinCon.GetStdHandle(WinCon.CONSOLE_STD_HANDLE.STD_OUTPUT_HANDLE);
                        Verify.IsNotNull(hConsole, "Ensure the STDOUT handle is valid.");

                        WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX cbiex = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
                        cbiex.cbSize = (uint)Marshal.SizeOf(cbiex);
                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref cbiex), "Get initial cursor position (from screen buffer info)");

                        // The expected anchor when we're done is this initial cursor position
                        WinCon.COORD expectedAnchor = new WinCon.COORD();
                        expectedAnchor.X = cbiex.dwCursorPosition.X;
                        expectedAnchor.Y = cbiex.dwCursorPosition.Y;

                        // The expected rect is going to start from this cursor position. We'll modify it after we perform some operations.
                        WinCon.SMALL_RECT expectedRect = new WinCon.SMALL_RECT();
                        expectedRect.Top = expectedAnchor.Y;
                        expectedRect.Left = expectedAnchor.X;
                        expectedRect.Right = expectedAnchor.X;
                        expectedRect.Bottom = expectedAnchor.Y;

                        // Now set up the keyboard and enter mark mode.
                        // NOTE: We must wait after every keyboard sequence to give the console time to process before asking it for changes.
                        //IKeyboard app.UIRoot. = Session.Keyboard;

                        area.EnterMode(ViewportArea.ViewportStates.Mark);

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Get state on entering mark mode.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS, "Selection should now be in progress since mark mode is started.");

                        // Select a small region
                        Log.Comment("1. Select a small region");

                        // keys names can be found at %SDXROOT%\sdktools\CommonTest\Mita2.0\Foundation\Keyboard.cs
                        app.UIRoot.SendKeys(Keys.Shift + Keys.Right + Keys.Right + Keys.Right + Keys.Down + Keys.Shift);

                        Globals.WaitForTimeout();

                        // Adjust the expected rectangle for the commands we just entered.
                        expectedRect.Right += 3; // same as the number of {RIGHT}s we put in
                        expectedRect.Bottom += 1; // same as the number of {DOWN}s we put in

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Get state of selected region.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS | WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_NOT_EMPTY, "Selection in progress and is no longer empty now that we've selected a region.");
                        Verify.AreEqual(csi.Selection, expectedRect, "Verify that the selected rectangle matches the keystrokes we entered.");
                        Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Verify anchor didn't go anywhere since we started in the top left.");

                        // End selection by moving
                        Log.Comment("2. End the selection by moving.");

                        app.UIRoot.SendKeys(Keys.Down);

                        Globals.WaitForTimeout();

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Move cursor to attempt to clear selection.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS, "Selection should be still running, but empty.");

                        // Select another region to ensure anchor moved.
                        Log.Comment("3. Select one more region from new position to verify anchor");

                        app.UIRoot.SendKeys(Keys.Shift + Keys.Right + Keys.Shift);

                        Globals.WaitForTimeout();

                        expectedAnchor.X = expectedRect.Right;
                        expectedAnchor.Y = expectedRect.Bottom;
                        expectedAnchor.Y++; // +1 for the {DOWN} in step 2. Not incremented in the line above because C# is unhappy with adding +1 to a short while assigning.

                        Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Verify anchor moved to the new start position.");

                        // Exit mark mode
                        area.EnterMode(ViewportArea.ViewportStates.Normal);

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Move cursor to attempt to clear selection.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION, "Selection should be empty when mode is exited.");
                    }
                }
            }
        }

        [TestMethod]
        public void TestMouseSelection()
        {
            //using (CmdApp app = new CmdApp(CreateType.ProcessOnly))
            //{
            //    using (ViewportArea area = new ViewportArea(app))
            //    {
            //        // Set up the area we're going to attempt to select
            //        Point startPoint = new Point();
            //        Point endPoint = new Point();

            //        startPoint.X = 1;
            //        startPoint.Y = 2;

            //        endPoint.X = 10;
            //        endPoint.Y = 10;

            //        // Save expected anchor
            //        WinCon.COORD expectedAnchor = new WinCon.COORD();
            //        expectedAnchor.X = (short)startPoint.X;
            //        expectedAnchor.Y = (short)startPoint.Y;

            //        // Also save bottom right corner for the end of the selection
            //        WinCon.COORD expectedBottomRight = new WinCon.COORD();
            //        expectedBottomRight.X = (short)endPoint.X;
            //        expectedBottomRight.Y = (short)endPoint.Y;

            //        // Convert the character coordinates into screen pixels.
            //        area.ConvertCharacterOffsetToPixelPosition(ref startPoint);
            //        area.ConvertCharacterOffsetToPixelPosition(ref endPoint);

            //        // Prepare the mouse by moving it into the start position. Prepare the structure
            //        WinCon.CONSOLE_SELECTION_INFO csi;
            //        WinCon.SMALL_RECT expectedRect = new WinCon.SMALL_RECT();

            //        WinCon.CONSOLE_SELECTION_INFO_FLAGS flagsExpected = WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION;


            //        m.MouseMove()
            //        m.MouseMove(startPoint);

            //        // 1. Place mouse button down to start selection and check state
            //        m.Down(MouseButtons.Primary, ModifierKeys.None);

            //        Globals.WaitForTimeout(); // must wait after mouse operation. No good waiters since we have no UI objects

            //        flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS; // a selection is occuring
            //        flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_SELECTION; // it's a "Select" mode not "Mark" mode selection
            //        flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_DOWN; // the mouse is still down
            //        flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_NOT_EMPTY; // mouse selections are never empty. minimum 1x1

            //        expectedRect.Top = expectedAnchor.Y; // rectangle is just at the point itself 1x1 size
            //        expectedRect.Left = expectedAnchor.X;
            //        expectedRect.Bottom = expectedRect.Top;
            //        expectedRect.Right = expectedRect.Left;

            //        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state on mouse button down to start selection.");
            //        Log.Comment("Selection Info: {0}", csi);

            //        Verify.AreEqual(csi.Flags, flagsExpected, "Check initial mouse selection with button still down.");
            //        Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Check that the anchor is equal to the start point.");
            //        Verify.AreEqual(csi.Selection, expectedRect, "Check that entire rectangle is the size of 1x1 and is just at the anchor point.");

            //        // 2. Move to end point and release cursor
            //        m.Move(endPoint);
            //        m.Up(MouseButtons.Primary, ModifierKeys.None);

            //        Globals.WaitForTimeout(); // must wait after mouse operation. No good waiters since we have no UI objects

            //        // on button up, remove mouse down flag
            //        flagsExpected &= ~WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_DOWN;

            //        // anchor remains the same
            //        // bottom right of rectangle now changes to the end point
            //        expectedRect.Bottom = expectedBottomRight.Y;
            //        expectedRect.Right = expectedBottomRight.X;

            //        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state after drag and release mouse.");
            //        Log.Comment("Selection Info: {0}", csi);

            //        Verify.AreEqual(csi.Flags, flagsExpected, "Check selection is still on and valid, but button is up.");
            //        Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Check that the anchor is still equal to the start point.");
            //        Verify.AreEqual(csi.Selection, expectedRect, "Check that entire rectangle reaches from start to end point.");

            //        // 3. Leave mouse selection
            //        area.ExitModes();

            //        flagsExpected = WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION;

            //        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state after exiting mouse selection.");
            //        Log.Comment("Selection Info: {0}", csi);

            //        Verify.AreEqual(csi.Flags, flagsExpected, "Check that selection state is reset.");
            //    }
            //}
        }
    }
}
