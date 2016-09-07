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

    using MS.Internal.Mita.Foundation;
    using MS.Internal.Mita.Foundation.Controls;
    using MS.Internal.Mita.Foundation.Waiters;

    using WEX.Common.Managed;
    using WEX.Logging.Interop;
    using WEX.TestExecution;
    using WEX.TestExecution.Markup;

    using Conhost.UIA.Tests.Common;
    using Conhost.UIA.Tests.Common.NativeMethods;
    using Conhost.UIA.Tests.Elements;


    [TestClass]
    public class SelectionApiTests
    {
        public const int timeout = Globals.Timeout;

        [TestMethod]
        public void TestCtrlHomeEnd()
        {
            using (CmdApp app = new CmdApp(CreateType.ProcessOnly))
            {
                using (ViewportArea area = new ViewportArea(app))
                {
                    // Get keyboard instance to try commands
                    // NOTE: We must wait after every keyboard sequence to give the console time to process before asking it for changes.
                    Keyboard kbd = Keyboard.Instance;

                    // Get console handle.
                    IntPtr hConsole = app.GetStdOutHandle();
                    Verify.IsNotNull(hConsole, "Ensure the STDOUT handle is valid.");

                    // Get us to an expected initial state.
                    kbd.SendKeys("C:{ENTER}");
                    kbd.SendKeys("cd C:\\{ENTER}");
                    kbd.SendKeys("cls{ENTER}");

                    // Get initial screen buffer position
                    WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiexOriginal = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
                    sbiexOriginal.cbSize = (uint)Marshal.SizeOf(sbiexOriginal);
                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexOriginal), "Get initial viewport position.");

                    // Prep comparison structure
                    WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiexCompare = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
                    sbiexCompare.cbSize = (uint)Marshal.SizeOf(sbiexCompare);

                    // Ctrl-End shouldn't move anything yet.
                    Log.Comment("Attempt Ctrl-End. Nothing should move yet.");
                    kbd.SendKeys("{CONTROL DOWN}{END}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after.");

                    // Ctrl-Home shouldn't move anything yet.
                    Log.Comment("Attempt Ctrl-Home. Nothing should move yet.");
                    kbd.SendKeys("{CONTROL DOWN}{HOME}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after.");

                    // Make some text come out
                    Log.Comment("Emit some text into the buffer so we'll have something to scroll and test.");
                    kbd.SendKeys(@"HELP{ENTER}HELP{ENTER}HELP{ENTER}HELP{ENTER}");

                    Globals.WaitForTimeout();

                    // Get the new original position
                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexOriginal), "Get position of viewport once text is emitted.");

                    // Ctrl-Home should move to top of buffer
                    Log.Comment("Attempt Ctrl-Home. Should move to top of buffer.");
                    kbd.SendKeys("{CONTROL DOWN}{HOME}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreNotEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after. Should no longer be the same.");
                    Verify.AreEqual(sbiexCompare.srWindow.Top, 0, "Position of viewport top should be at 0, top of screen buffer.");

                    // Ctrl-End should take us back to the original position at the end line
                    Log.Comment("Attempt Ctrl-End. Should move back to edit line (original position.");
                    kbd.SendKeys("{CONTROL DOWN}{END}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after. Should be back to original position.");


                    Log.Comment("Now test the line with some text in it.");
                    // Retrieve original position (including cursor)
                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexOriginal), "Get position of viewport with nothing on edit line.");

                    // Put some text onto the edit line now
                    Log.Comment("Place some text onto the edit line to ensure behavior will change with edit line full.");
                    const string testText = "SomeTestText";
                    kbd.SendKeys(testText);

                    Globals.WaitForTimeout();

                    // Get the position of the cursor after the text is entered
                    WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX sbiexWithText = new WinCon.CONSOLE_SCREEN_BUFFER_INFO_EX();
                    sbiexWithText.cbSize = (uint)Marshal.SizeOf(sbiexWithText);
                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexWithText), "Get position of viewport with edit line text.");

                    // The cursor can't have moved down a line. We're going to verify the text by reading its "rectangle" out of the screen buffer.
                    // If it moved down a line, the calculation of what to select is more complicated than the simple rectangle assignment below.
                    Verify.AreEqual(sbiexOriginal.dwCursorPosition.Y, sbiexWithText.dwCursorPosition.Y, "There's an assumption here that the cursor stayed on the same line when we added our bit of text.");

                    // Prepare the read rectangle for what we want to get out of the buffer.
                    Rectangle readRectangle = new Rectangle(sbiexOriginal.dwCursorPosition.X,
                                                            sbiexOriginal.dwCursorPosition.Y,
                                                            (sbiexWithText.dwCursorPosition.X - sbiexOriginal.dwCursorPosition.X),
                                                            1);

                    Log.Comment("Verify that the text we keyed matches what's in the buffer.");
                    IEnumerable<string> text = area.GetLinesInRectangle(hConsole, readRectangle);
                    Verify.AreEqual(text.Count(), 1, "We should only have retrieved one line.");
                    Verify.AreEqual(text.First(), testText, "Verify text matches keyed input.");

                    // Move cursor into the middle of the text.
                    Log.Comment("Move cursor into the middle of the string.");

                    const int lefts = 4;
                    for (int i = 0; i < lefts; i++)
                    {
                        kbd.SendKeys("{LEFT}");
                    }

                    Globals.WaitForTimeout();

                    // Get cursor position now that it's moved.
                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexWithText), "Get position of viewport with cursor moved into the middle of the edit line text.");
                    
                    Log.Comment("Ctrl-End should trim the end of the input line from the cursor (and not move the cursor.)");
                    kbd.SendKeys("{CONTROL DOWN}{END}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreEqual<WinCon.SMALL_RECT>(sbiexWithText.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after.");
                    Verify.AreEqual<WinCon.COORD>(sbiexWithText.dwCursorPosition, sbiexCompare.dwCursorPosition, "Compare cursor positions before and after.");

                    Log.Comment("Compare actual text visible on screen.");
                    text = area.GetLinesInRectangle(hConsole, readRectangle);
                    Verify.AreEqual(text.Count(), 1, "We should only have retrieved one line.");

                    // the substring length is the original length of the string minus the number of lefts
                    int substringCtrlEnd = testText.Length - lefts;
                    Verify.AreEqual(text.First().Trim(), testText.Substring(0, substringCtrlEnd), "Verify text matches keyed input without the last characters removed by Ctrl+End.");

                    Log.Comment("Ctrl-Home should trim the remainder of the edit line from the cursor to the beginning (restoring cursor to position before we entered anything.)");
                    kbd.SendKeys("{CONTROL DOWN}{HOME}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");
                    Verify.AreEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after.");
                    Verify.AreEqual<WinCon.COORD>(sbiexOriginal.dwCursorPosition, sbiexCompare.dwCursorPosition, "Compare cursor positions before and after.");

                    Log.Comment("Compare actual text visible on screen.");
                    text = area.GetLinesInRectangle(hConsole, readRectangle);
                    Verify.AreEqual(text.Count(), 1, "We should only have retrieved one line.");

                    Verify.AreEqual(text.First().Trim(), string.Empty, "Verify text is now empty after Ctrl+Home from the end of it.");

                    Log.Comment("Now that all the text is gone, try Ctrl+Home one more time to ensure it moves.");
                    kbd.SendKeys("{CONTROL DOWN}{HOME}{CONTROL UP}");

                    Globals.WaitForTimeout();

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleScreenBufferInfoEx(hConsole, ref sbiexCompare), "Get comparison position.");

                    Verify.AreNotEqual<WinCon.SMALL_RECT>(sbiexOriginal.srWindow, sbiexCompare.srWindow, "Compare viewport positions before and after. Should no longer be the same.");
                    Verify.AreEqual(sbiexCompare.srWindow.Top, 0, "Position of viewport top should be at 0, top of screen buffer.");
                }
            }
        }

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
                        Keyboard kbd = Keyboard.Instance;

                        area.EnterMode(ViewportArea.ViewportStates.Mark);

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Get state on entering mark mode.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS, "Selection should now be in progress since mark mode is started.");

                        // Select a small region
                        Log.Comment("1. Select a small region");

                        // keys names can be found at %SDXROOT%\sdktools\CommonTest\Mita2.0\Foundation\Keyboard.cs
                        kbd.SendKeys("{SHIFT DOWN}{RIGHT}{RIGHT}{RIGHT}{DOWN}{SHIFT UP}");

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

                        kbd.SendKeys("{DOWN}");

                        Globals.WaitForTimeout();

                        NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Move cursor to attempt to clear selection.");
                        Log.Comment("Selection Info: {0}", csi);

                        Verify.AreEqual(csi.Flags, WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS, "Selection should be still running, but empty.");

                        // Select another region to ensure anchor moved.
                        Log.Comment("3. Select one more region from new position to verify anchor");

                        kbd.SendKeys("{SHIFT DOWN}{RIGHT}{SHIFT UP}");

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
            using (CmdApp app = new CmdApp(CreateType.ProcessOnly))
            {
                using (ViewportArea area = new ViewportArea(app))
                {
                    // Set up the area we're going to attempt to select
                    Point startPoint = new Point();
                    Point endPoint = new Point();

                    startPoint.X = 1;
                    startPoint.Y = 2;

                    endPoint.X = 10;
                    endPoint.Y = 10;

                    // Save expected anchor
                    WinCon.COORD expectedAnchor = new WinCon.COORD();
                    expectedAnchor.X = (short)startPoint.X;
                    expectedAnchor.Y = (short)startPoint.Y;

                    // Also save bottom right corner for the end of the selection
                    WinCon.COORD expectedBottomRight = new WinCon.COORD();
                    expectedBottomRight.X = (short)endPoint.X;
                    expectedBottomRight.Y = (short)endPoint.Y;

                    // Convert the character coordinates into screen pixels.
                    area.ConvertCharacterOffsetToPixelPosition(ref startPoint);
                    area.ConvertCharacterOffsetToPixelPosition(ref endPoint);

                    // Prepare the mouse by moving it into the start position. Prepare the structure
                    WinCon.CONSOLE_SELECTION_INFO csi;
                    WinCon.SMALL_RECT expectedRect = new WinCon.SMALL_RECT();
                    
                    WinCon.CONSOLE_SELECTION_INFO_FLAGS flagsExpected = WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION;
                    Mouse m = Mouse.Instance;
                    m.Move(startPoint);

                    // 1. Place mouse button down to start selection and check state
                    m.Down(MouseButtons.Primary, ModifierKeys.None);

                    Globals.WaitForTimeout(); // must wait after mouse operation. No good waiters since we have no UI objects

                    flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_IN_PROGRESS; // a selection is occuring
                    flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_SELECTION; // it's a "Select" mode not "Mark" mode selection
                    flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_DOWN; // the mouse is still down
                    flagsExpected |= WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_SELECTION_NOT_EMPTY; // mouse selections are never empty. minimum 1x1

                    expectedRect.Top = expectedAnchor.Y; // rectangle is just at the point itself 1x1 size
                    expectedRect.Left = expectedAnchor.X;
                    expectedRect.Bottom = expectedRect.Top;
                    expectedRect.Right = expectedRect.Left;

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state on mouse button down to start selection.");
                    Log.Comment("Selection Info: {0}", csi);

                    Verify.AreEqual(csi.Flags, flagsExpected, "Check initial mouse selection with button still down.");
                    Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Check that the anchor is equal to the start point.");
                    Verify.AreEqual(csi.Selection, expectedRect, "Check that entire rectangle is the size of 1x1 and is just at the anchor point.");
                    
                    // 2. Move to end point and release cursor
                    m.Move(endPoint);
                    m.Up(MouseButtons.Primary, ModifierKeys.None);

                    Globals.WaitForTimeout(); // must wait after mouse operation. No good waiters since we have no UI objects

                    // on button up, remove mouse down flag
                    flagsExpected &= ~WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_MOUSE_DOWN;

                    // anchor remains the same
                    // bottom right of rectangle now changes to the end point
                    expectedRect.Bottom = expectedBottomRight.Y;
                    expectedRect.Right = expectedBottomRight.X;

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state after drag and release mouse.");
                    Log.Comment("Selection Info: {0}", csi);

                    Verify.AreEqual(csi.Flags, flagsExpected, "Check selection is still on and valid, but button is up.");
                    Verify.AreEqual(csi.SelectionAnchor, expectedAnchor, "Check that the anchor is still equal to the start point.");
                    Verify.AreEqual(csi.Selection, expectedRect, "Check that entire rectangle reaches from start to end point.");

                    // 3. Leave mouse selection
                    area.ExitModes();

                    flagsExpected = WinCon.CONSOLE_SELECTION_INFO_FLAGS.CONSOLE_NO_SELECTION;

                    NativeMethods.Win32BoolHelper(WinCon.GetConsoleSelectionInfo(out csi), "Check state after exiting mouse selection.");
                    Log.Comment("Selection Info: {0}", csi);

                    Verify.AreEqual(csi.Flags, flagsExpected, "Check that selection state is reset.");
                }
            }
        }
    }
}
