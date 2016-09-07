//----------------------------------------------------------------------------------------------------------------------
// <copyright file="VirtualTerminalTests.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>UI Automation tests for the Virtual Terminal feature.</summary>
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
    public class VirtualTerminalTests
    {
        private static string VIRTUAL_TERMINAL_KEY_NAME = "VirtualTerminalLevel";
        private static int VIRTUAL_TERMINAL_ON_VALUE = 100;

        private static string vtAppLocation;

        public const int timeout = Globals.Timeout;


        [ClassInitialize]
        public static void ClassSetup(TestContext context)
        {
            // The test job must place vtapp.exe in the same folder as the test binary.
            vtAppLocation = @"vtapp.exe";
        }


        [TestMethod]
        public void RunVtAppTester()
        {
            using (RegistryHelper reg = new RegistryHelper())
            {
                reg.BackupRegistry(); // we're going to modify the virtual terminal state for this, so back it up first.
                VersionSelector.SetConsoleVersion(reg, ConsoleVersion.V2);
                reg.SetDefaultValue(VIRTUAL_TERMINAL_KEY_NAME, VIRTUAL_TERMINAL_ON_VALUE);
                
                bool haveVtAppPath = !string.IsNullOrEmpty(vtAppLocation);

                Verify.IsTrue(haveVtAppPath, "Ensure that we passed in the location to VtApp.exe");

                if (haveVtAppPath)
                {
                    using (CmdApp app = new CmdApp(vtAppLocation))
                    {
                        using (ViewportArea area = new ViewportArea(app))
                        {
                            // Get keyboard instance to try commands
                            // NOTE: We must wait after every keyboard sequence to give the console time to process before asking it for changes.
                            Keyboard kbd = Keyboard.Instance;

                            // Get console handle.
                            IntPtr hConsole = app.GetStdOutHandle();
                            Verify.IsNotNull(hConsole, "Ensure the STDOUT handle is valid.");

                            Log.Comment("Check that the VT test app loaded up properly with its output line and the cursor down one line.");
                            Rectangle selectRect = new Rectangle(0, 0, 9, 1);
                            IEnumerable<string> scrapedText = area.GetLinesInRectangle(hConsole, selectRect);

                            Verify.AreEqual(scrapedText.Count(), 1, "We should have retrieved one line.");
                            string testerWelcome = scrapedText.Single();

                            Verify.AreEqual(testerWelcome, "VT Tester");

                            WinCon.COORD cursorPos = app.GetCursorPosition(hConsole);
                            WinCon.COORD cursorExpected = new WinCon.COORD();
                            cursorExpected.X = 0;
                            cursorExpected.Y = 1;
                            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved to expected starting position.");

                            TestCursorPositioningCommands(app, kbd, hConsole, cursorExpected);

                            TestCursorVisibilityCommands(app, kbd, hConsole);

                            TestAreaEraseCommands(app, area, kbd, hConsole);

                            TestGraphicsCommands(app, area, kbd, hConsole);

                            TestQueryResponses(app, kbd, hConsole);

                            TestVtToggle(app, kbd, hConsole);

                            TestInsertDelete(app, area, kbd, hConsole);
                        }
                    }
                }
            }
        }

        private static void TestInsertDelete(CmdApp app, ViewportArea area, Keyboard kbd, IntPtr hConsole)
        {
            Log.Comment("--Insert/Delete Commands");
            ScreenFillHelper(app, area, kbd, hConsole);

            Log.Comment("Move cursor to the middle-ish");
            Point cursorExpected = new Point();
            // H is at 5, 1. VT coords are 1-based and buffer is 0-based so adjust.
            cursorExpected.Y = 5 - 1; 
            cursorExpected.X = 1 - 1;
            kbd.SendKeys("H");

            // Move to middle-ish from here. 10 Bs and 10 Cs should about do it.
            for (int i=0; i < 10; i++)
            {
                kbd.SendKeys("BC");
                cursorExpected.Y++;
                cursorExpected.X++;
            }

            WinCon.SMALL_RECT viewport = app.GetViewport(hConsole);

            // The entire buffer should be Zs except for what we're about to insert and delete.
            kbd.SendKeys("O"); // insert
            WinCon.CHAR_INFO ciCursor = area.GetCharInfoAt(hConsole, cursorExpected);
            Verify.AreEqual(' ', ciCursor.UnicodeChar);

            Point endOfCursorLine = new Point(viewport.Right, cursorExpected.Y);

            kbd.SendKeys("P"); // delete
            WinCon.CHAR_INFO ciEndOfLine = area.GetCharInfoAt(hConsole, endOfCursorLine);
            Verify.AreEqual(' ', ciEndOfLine.UnicodeChar);
            ciCursor = area.GetCharInfoAt(hConsole, cursorExpected);
            Verify.AreEqual('Z', ciCursor.UnicodeChar);

            // Move to end of line and check both insert and delete operations
            while (cursorExpected.X < viewport.Right)
            {
                kbd.SendKeys("C");
                cursorExpected.X++;
            }

            // move up a line to get some fresh Z
            kbd.SendKeys("A");
            cursorExpected.Y--;

            kbd.SendKeys("O"); // insert at end of line
            ciCursor = area.GetCharInfoAt(hConsole, cursorExpected);
            Verify.AreEqual(' ', ciCursor.UnicodeChar);

            // move up a line to get some fresh Z
            kbd.SendKeys("A");
            cursorExpected.Y--;

            kbd.SendKeys("P"); // delete at end of line
            ciCursor = area.GetCharInfoAt(hConsole, cursorExpected);
            Verify.AreEqual(' ', ciCursor.UnicodeChar);
        }

        private static void TestVtToggle(CmdApp app, Keyboard kbd, IntPtr hConsole)
        {
            WinCon.COORD cursorPos;
            Log.Comment("--Test VT Toggle--");

            Verify.IsTrue(app.IsVirtualTerminalEnabled(hConsole), "Verify we're starting with VT on.");

            kbd.SendKeys("H-"); // move cursor to top left area H location and then turn off VT.

            cursorPos = app.GetCursorPosition(hConsole);

            Verify.IsFalse(app.IsVirtualTerminalEnabled(hConsole), "Verify VT was turned off.");

            kbd.SendKeys("-");
            Verify.IsTrue(app.IsVirtualTerminalEnabled(hConsole), "Verify VT was turned back on .");
        }

        private static void TestQueryResponses(CmdApp app, Keyboard kbd, IntPtr hConsole)
        {
            WinCon.COORD cursorPos;
            Log.Comment("---Status Request Commands---");
            kbd.SendKeys("c");
            string expectedTitle = string.Format("Response Received: {0}", "\x1b[?1;0c");

            Globals.WaitForTimeout();
            string title = app.GetWindowTitle();
            Verify.AreEqual(expectedTitle, title, "Verify that we received the proper response to the Device Attributes request.");

            kbd.SendKeys("R");
            cursorPos = app.GetCursorPosition(hConsole);
            expectedTitle = string.Format("Response Received: {0}", string.Format("\x1b[{0};{1}R", cursorPos.Y + 1, cursorPos.X + 1));

            Globals.WaitForTimeout();
            title = app.GetWindowTitle();
            Verify.AreEqual(expectedTitle, title, "Verify that we received the proper response to the Cursor Position request.");
        }

        private static void TestGraphicsCommands(CmdApp app, ViewportArea area, Keyboard kbd, IntPtr hConsole)
        {
            Log.Comment("---Graphics Commands---");
            ScreenFillHelper(app, area, kbd, hConsole);

            WinCon.CHAR_INFO ciExpected = new WinCon.CHAR_INFO();
            ciExpected.UnicodeChar = 'z';
            ciExpected.Attributes = app.GetCurrentAttributes(hConsole);

            WinCon.CHAR_INFO ciOriginal = ciExpected;

            WinCon.CHAR_INFO ciActual;
            Point pt = new Point();

            Log.Comment("Set foreground brightness (SGR.1)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("1`");

            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground brightness got set.");

            Log.Comment("Set foreground green (SGR.32)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("2`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_GREEN;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground green got set.");

            Log.Comment("Set foreground yellow (SGR.33)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("3`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_YELLOW;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground yellow got set.");

            Log.Comment("Set foreground blue (SGR.34)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("4`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_BLUE;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground blue got set.");

            Log.Comment("Set foreground magenta (SGR.35)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("5`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_MAGENTA;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground magenta got set.");

            Log.Comment("Set foreground cyan (SGR.36)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("6`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_CYAN;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground cyan got set.");

            Log.Comment("Set background white (SGR.47)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("W`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_COLORS;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background white got set.");

            Log.Comment("Set background black (SGR.40)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("Q`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background black got set.");

            Log.Comment("Set background red (SGR.41)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("q`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_RED;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background red got set.");

            Log.Comment("Set background yellow (SGR.43)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("w`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_YELLOW;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background yellow got set.");

            Log.Comment("Set foreground bright red (SGR.91)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("!`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_RED | WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground bright red got set.");

            Log.Comment("Set foreground bright blue (SGR.94)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("@`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_BLUE | WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground bright blue got set.");

            Log.Comment("Set foreground bright cyan (SGR.96)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("#`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_CYAN | WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that foreground bright cyan got set.");

            Log.Comment("Set background bright red (SGR.101)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("$`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_RED | WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background bright red got set.");

            Log.Comment("Set background bright blue (SGR.104)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("{SHIFT DOWN}5{SHIFT UP}`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_BLUE | WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background bright blue got set.");

            Log.Comment("Set background bright cyan (SGR.106)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("{SHIFT DOWN}6{SHIFT UP}`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_CYAN | WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_INTENSITY;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that background bright cyan  got set.");

            Log.Comment("Set underline (SGR.4)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("e`");

            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.COMMON_LVB_UNDERSCORE;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that underline got set.");

            Log.Comment("Clear underline (SGR.24)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("d`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.COMMON_LVB_UNDERSCORE;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that underline got cleared.");

            Log.Comment("Set negative image video (SGR.7)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("r`");

            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.COMMON_LVB_REVERSE_VIDEO;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that negative video got set.");

            Log.Comment("Set positive image video (SGR.27)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("f`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.COMMON_LVB_REVERSE_VIDEO;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that positive video got set.");

            Log.Comment("Set back to default (SGR.0)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("0`");

            ciExpected = ciOriginal;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that we got set back to the original state.");

            Log.Comment("Set multiple properties in the same message (SGR.1,37,43,4)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("9`");

            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_COLORS;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_YELLOW;
            ciExpected.Attributes |= WinCon.CONSOLE_ATTRIBUTES.COMMON_LVB_UNDERSCORE;

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that we set foreground bright white, background yellow, and underscore in the same SGR command.");

            Log.Comment("Set foreground back to original only (SGR.39)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("{SHIFT DOWN}9{SHIFT UP}`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL; // turn off all foreground flags
            ciExpected.Attributes |= (ciOriginal.Attributes & WinCon.CONSOLE_ATTRIBUTES.FOREGROUND_ALL); // turn on only the foreground part of the original attributes

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that we set the foreground only back to the default.");

            Log.Comment("Set background back to original only (SGR.49)");
            app.FillCursorPosition(hConsole, ref pt);
            kbd.SendKeys("{SHIFT DOWN}0{SHIFT UP}`");

            ciExpected.Attributes &= ~WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL; // turn off all foreground flags
            ciExpected.Attributes |= (ciOriginal.Attributes & WinCon.CONSOLE_ATTRIBUTES.BACKGROUND_ALL); // turn on only the foreground part of the original attributes

            ciActual = area.GetCharInfoAt(hConsole, pt);
            Verify.AreEqual(ciExpected, ciActual, "Verify that we set the background only back to the default.");
        }

        private static void TestCursorPositioningCommands(CmdApp app, Keyboard kbd, IntPtr hConsole, WinCon.COORD cursorExpected)
        {
            WinCon.COORD cursorPos;
            Log.Comment("---Cursor Positioning Commands---");
            // Try cursor commands
            Log.Comment("Press B key (cursor down)");
            kbd.SendKeys("B");
            cursorExpected.Y++;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved down by 1.");

            Log.Comment("Press A key (cursor up)");
            kbd.SendKeys("A");
            cursorExpected.Y--;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved up by 1.");

            Log.Comment("Press C key (cursor right)");
            kbd.SendKeys("C");
            cursorExpected.X++;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved right by 1.");

            Log.Comment("Press D key (cursor left)");
            kbd.SendKeys("D");
            cursorExpected.X--;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved left by 1.");

            Log.Comment("Move to the right (C) then move down a line (E)");
            kbd.SendKeys("CCC");
            cursorExpected.X += 3;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has right by 3.");

            kbd.SendKeys("E");
            cursorExpected.Y++;
            cursorExpected.X = 0;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved down by 1 line and reset X position to 0.");

            Log.Comment("Move to the right (C) then move up a line (F)");
            kbd.SendKeys("CCC");
            cursorExpected.X += 3;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check curs has right by 3.");

            kbd.SendKeys("F");
            cursorExpected.Y--;
            cursorExpected.X = 0;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved up by 1 line.");

            Log.Comment("Check move directly to position 14 horizontally (G)");
            kbd.SendKeys("G");
            cursorExpected.X = 14 - 1; // 14 is the VT position which starts at array offset 1. 13 is the buffer position starting at array offset 0.
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved to horizontal position 14.");

            Log.Comment("Check move directly to position 14 vertically (v key mapped to d)");
            kbd.SendKeys("v");
            cursorExpected.Y = 14 - 1; // 14 is the VT position which starts at array offset 1. 13 is the buffer position starting at array offset 0.
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved to vertical position 14.");

            Log.Comment("Check move directly to row 5, column 1 (H)");
            kbd.SendKeys("H");
            // Again -1s are to convert index base 1 VT to console base 0 arrays
            cursorExpected.Y = 5 - 1;
            cursorExpected.X = 1 - 1;
            cursorPos = app.GetCursorPosition(hConsole);
            Verify.AreEqual(cursorExpected, cursorPos, "Check cursor has moved to row 5, column 1.");
        }

        private static void TestCursorVisibilityCommands(CmdApp app, Keyboard kbd, IntPtr hConsole)
        {
            WinCon.COORD cursorExpected;
            Log.Comment("---Cursor Visibility Commands---");
            Log.Comment("Turn cursor display off. (l)");
            kbd.SendKeys("l");
            Verify.AreEqual(false, app.IsCursorVisible(hConsole), "Check that cursor is invisible.");

            Log.Comment("Turn cursor display on. (h)");
            kbd.SendKeys("h");
            Verify.AreEqual(true, app.IsCursorVisible(hConsole), "Check that cursor is visible.");

            Log.Comment("---Cursor Save/Restore Commands---");
            Log.Comment("Save current cursor position with DEC save.");
            kbd.SendKeys("7");
            cursorExpected = app.GetCursorPosition(hConsole);

            Log.Comment("Move the cursor a bit away from the saved position.");
            kbd.SendKeys("BBBBCCCCC");

            Log.Comment("Restore existing position with DEC restore.");
            kbd.SendKeys("8");
            Verify.AreEqual(cursorExpected, app.GetCursorPosition(hConsole), "Check that cursor restored back to the saved position.");

            Log.Comment("Move the cursor a bit away from the saved position.");
            kbd.SendKeys("BBBBCCCCC");

            Log.Comment("Restore existing position with ANSISYS restore.");
            kbd.SendKeys("u");
            Verify.AreEqual(cursorExpected, app.GetCursorPosition(hConsole), "Check that cursor restored back to the saved position.");

            Log.Comment("Move the cursor a bit away from either position.");
            kbd.SendKeys("BBB");

            Log.Comment("Save current cursor position with ANSISYS save.");
            kbd.SendKeys("y");
            cursorExpected = app.GetCursorPosition(hConsole);

            Log.Comment("Move the cursor a bit away from the saved position.");
            kbd.SendKeys("CCCBB");

            Log.Comment("Restore existing position with DEC restore.");
            kbd.SendKeys("8");
            Verify.AreEqual(cursorExpected, app.GetCursorPosition(hConsole), "Check that cursor restored back to the saved position.");
        }

        private static void TestAreaEraseCommands(CmdApp app, ViewportArea area, Keyboard kbd, IntPtr hConsole)
        {
            WinCon.COORD cursorPos;
            Log.Comment("---Area Erase Commands---");
            ScreenFillHelper(app, area, kbd, hConsole);
            Log.Comment("Clear screen after");
            kbd.SendKeys("J");

            Globals.WaitForTimeout(); // give buffer time to clear.

            cursorPos = app.GetCursorPosition(hConsole);

            GetExpectedChar expectedCharAlgorithm;
            expectedCharAlgorithm = (int rowId, int colId, int height, int width) =>
            {
                if (rowId == (height - 1) && colId == (width - 1))
                {
                    return ' ';
                }
                else if (rowId < cursorPos.Y)
                {
                    return 'Z';
                }
                else if (rowId > cursorPos.Y)
                {
                    return ' ';
                }
                else
                {
                    if (colId < cursorPos.X)
                    {
                        return 'Z';
                    }
                    else
                    {
                        return ' ';
                    }
                }
            };

            BufferVerificationHelper(app, area, hConsole, expectedCharAlgorithm);

            ScreenFillHelper(app, area, kbd, hConsole);
            Log.Comment("Clear screen before");
            kbd.SendKeys("j");

            expectedCharAlgorithm = (int rowId, int colId, int height, int width) =>
            {
                if (rowId == (height - 1) && colId == (width - 1))
                {
                    return ' ';
                }
                else if (rowId < cursorPos.Y)
                {
                    return ' ';
                }
                else if (rowId > cursorPos.Y)
                {
                    return 'Z';
                }
                else
                {
                    if (colId <= cursorPos.X)
                    {
                        return ' ';
                    }
                    else
                    {
                        return 'Z';
                    }
                }
            };

            BufferVerificationHelper(app, area, hConsole, expectedCharAlgorithm);

            ScreenFillHelper(app, area, kbd, hConsole);
            Log.Comment("Clear line after");
            kbd.SendKeys("K");

            expectedCharAlgorithm = (int rowId, int colId, int height, int width) =>
            {
                if (rowId == (height - 1) && colId == (width - 1))
                {
                    return ' ';
                }
                else if (rowId != cursorPos.Y)
                {
                    return 'Z';
                }
                else
                {
                    if (colId < cursorPos.X)
                    {
                        return 'Z';
                    }
                    else
                    {
                        return ' ';
                    }
                }
            };

            BufferVerificationHelper(app, area, hConsole, expectedCharAlgorithm);

            ScreenFillHelper(app, area, kbd, hConsole);
            Log.Comment("Clear line before");
            kbd.SendKeys("k");

            expectedCharAlgorithm = (int rowId, int colId, int height, int width) =>
            {
                if (rowId == (height - 1) && colId == (width - 1))
                {
                    return ' ';
                }
                else if (rowId != cursorPos.Y)
                {
                    return 'Z';
                }
                else
                {
                    if (colId <= cursorPos.X)
                    {
                        return ' ';
                    }
                    else
                    {
                        return 'Z';
                    }
                }
            };

            BufferVerificationHelper(app, area, hConsole, expectedCharAlgorithm);
        }

        delegate char GetExpectedChar(int rowId, int colId, int height, int width);
        
        private static void ScreenFillHelper(CmdApp app, ViewportArea area, Keyboard kbd, IntPtr hConsole)
        {
            Log.Comment("Fill screen with junk");
            kbd.SendKeys("{SHIFT DOWN}`{SHIFT UP}");

            Globals.WaitForTimeout(); // give the buffer time to fill.

            GetExpectedChar expectedCharAlgorithm = (int rowId, int colId, int height, int width) =>
            {
                // For the very last bottom right corner character, it should be space. Every other character is a Z when filled.
                if (rowId == (height - 1) && colId == (width - 1))
                {
                    return ' ';
                }
                else
                {
                    return 'Z';
                }
            };

            BufferVerificationHelper(app, area, hConsole, expectedCharAlgorithm);
        }

        private static void BufferVerificationHelper(CmdApp app, ViewportArea area, IntPtr hConsole, GetExpectedChar expectedCharAlgorithm)
        {
            WinCon.SMALL_RECT viewport = app.GetViewport(hConsole);
            Rectangle selectRect = new Rectangle(viewport.Left, viewport.Top, viewport.Width, viewport.Height);
            IEnumerable<string> scrapedText = area.GetLinesInRectangle(hConsole, selectRect);

            Verify.AreEqual(viewport.Height, scrapedText.Count(), "Verify the rows scraped is equal to the entire viewport height.");

            bool isValidState = true;
            string[] rows = scrapedText.ToArray();
            for (int i = 0; i < rows.Length; i++)
            {
                for (int j = 0; j < viewport.Width; j++)
                {
                    char actual = rows[i][j];
                    char expected = expectedCharAlgorithm(i, j, rows.Length, viewport.Width);

                    isValidState = actual == expected;

                    if (!isValidState)
                    {
                        Verify.Fail(string.Format("Text buffer verification failed at Row: {0} Col: {1} Expected: '{2}' Actual: '{3}'", i, j, expected, actual));
                        break;
                    }
                }

                if (!isValidState)
                {
                    break;
                }
            }
        }
    }
}
